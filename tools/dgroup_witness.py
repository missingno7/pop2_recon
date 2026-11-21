"""List v4 DGROUP witnesses: original instructions whose disp16/imm16 equals an offset.

Scans code reached by control flow from every original relocated CALL FAR entry and
prints witness specs usable in an `external-dgroup-offset16-v4` binding. Pass
`--exclude START:END` (root image positions) to skip the candidate's own extent.
Research helper; it grants nothing and writes only to --output.
"""
import argparse
import json

from binding import operand_field
from common import ROOT, project_path, read_json, write_json
from disasm import _capstone
from oracle import Oracle


def witnesses(oracle, offsets, exclude=()):
    """Scan code reached from every original far-call entry (see binding.reachable_from_entry)."""
    from binding import reachable_from_entry
    from tu_census import far_call_sites
    cs, _ = _capstone()
    engine = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_16)
    engine.detail = True
    root = oracle.spaces["root"]
    found = {o: [] for o in offsets}
    entries = {(c["segment"], c["offset"]) for c in far_call_sites(root)
               if 0 <= (c["segment"] * 16 + c["offset"]) < root.size}
    entries |= {(t["segment"], t["offset"]) for t in read_json(ROOT / "evidence/targets.json")["functions"]
                if t["space"] == "root" and t.get("boundary_status") == "REVIEWED"
                and t.get("entry_kind", "direct-code") == "direct-code"}
    entries = sorted(entries)
    seen = set()
    for segment, offset in entries:
        entry = f"root:{segment:04x}:{offset:04x}"
        try:
            reach = reachable_from_entry(oracle, entry)
        except ValueError:
            continue
        for at in sorted(reach - seen):
            seen.add(at)
            if any(lo <= at < hi for lo, hi in exclude):
                continue
            insn = next(engine.disasm(root.data[at:at + 16], 0), None)
            for kind, size, rel in (("displacement", insn.disp_size, insn.disp_offset),
                                    ("immediate", insn.imm_size, insn.imm_offset)):
                if size != 2 or operand_field(insn, rel) != kind:
                    continue
                value = int.from_bytes(root.data[at + rel:at + rel + 2], "little")
                if value in found:
                    found[value].append({"space": "root", "image_offset": at, "bytes_hex": bytes(insn.bytes).hex(),
                                         "operand_offset": rel, "entry": entry,
                                         "context": f"{insn.mnemonic} {insn.op_str} reached from {entry}"})
    # Overlay code reached from RTLink vector destinations and each overlay's own far entries.
    from tu_census import overlay_vectors
    overlays = {n: s for n, s in oracle.spaces.items() if s.descriptor and s.descriptor["flags"] == 4}
    starts = {(v["target_space"], v["target_segment"], v["target_offset"])
              for v in overlay_vectors(oracle, overlays).values()}
    for name, space in overlays.items():
        starts |= {(name, c["segment"], c["offset"]) for c in far_call_sites(space)
                   if 0 <= (c["segment"] - space.link_segment) * 16 + c["offset"] < space.size}
    for name, segment, offset in sorted(starts):
        space = oracle.spaces[name]
        entry = f"{name}:{segment:04x}:{offset:04x}"
        try:
            reach = reachable_from_entry(oracle, entry)
        except ValueError:
            continue
        for at in sorted(reach):
            if (name, at) in seen:
                continue
            seen.add((name, at))
            insn = next(engine.disasm(space.data[at:at + 16], 0), None)
            for kind, size, rel in (("displacement", insn.disp_size, insn.disp_offset),
                                    ("immediate", insn.imm_size, insn.imm_offset)):
                if size != 2 or operand_field(insn, rel) != kind:
                    continue
                value = int.from_bytes(space.data[at + rel:at + rel + 2], "little")
                if value in found:
                    found[value].append({"space": name, "image_offset": at, "bytes_hex": bytes(insn.bytes).hex(),
                                         "operand_offset": rel, "entry": entry,
                                         "context": f"{insn.mnemonic} {insn.op_str} in {name} from {entry}"})
    return found


def add_members(oracle, binding, source, flags, exclude):
    """Add witnessed members for every non-zero v4 addend the compiled source uses."""
    import struct
    from binding import fixes
    from compiler import compile_c
    result = compile_c(source, "msc600", flags=flags, workdir=ROOT / "build/workers/dgroup_witness/compile")
    module = result.parsed
    code = next(s for s in module.segment_defs if s["class"] == "CODE")
    raw = module.segments[code["name"]]
    wanted, missing = {}, []
    for fix in fixes(module):
        name = fix["target"].get("name")
        symbol = binding["symbols"].get(name)
        if not symbol or symbol.get("kind") != "dgroup-offset" or fix["loc_type"] != 1 or fix["self_relative"]:
            continue
        addend = struct.unpack_from("<H", raw, fix["segment_offset"])[0]
        if addend:
            wanted.setdefault(name, set()).add(addend)
    for name, addends in wanted.items():
        symbol = binding["symbols"][name]
        have = {m["offset"] for m in symbol.get("members", [])}
        need = sorted(addends - have)
        found = witnesses(oracle, [(symbol["offset"] + k) & 0xFFFF for k in need], exclude)
        for k in need:
            rows = found[(symbol["offset"] + k) & 0xFFFF]
            if rows:
                symbol.setdefault("members", []).append(
                    {"offset": k, "witnesses": [{x: v for x, v in r.items() if x != "context"} for r in rows[:2]]})
            else:
                missing.append(f"{name}+{k:#x} (DS:{(symbol['offset'] + k) & 0xFFFF:04x})")
        symbol.get("members", []).sort(key=lambda m: m["offset"])
    return binding, missing


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("offsets", nargs="*", help="DGROUP offsets, hex")
    parser.add_argument("--members", metavar="BINDING", help="add witnessed members for SOURCE's non-zero addends")
    parser.add_argument("--source", help="candidate C source for --members")
    parser.add_argument("--flags", nargs="+", default=["/c", "/AM", "/Oe", "/Gs", "/Zl", "/Gc"],
                        help="compiler flags for --members (default the project flags)")
    parser.add_argument("--exclude", action="append", default=[], help="START:END root positions, hex")
    parser.add_argument("--output")
    args = parser.parse_args()
    exclude = [tuple(int(x, 16) for x in e.split(":")) for e in args.exclude]
    if args.members:
        binding, missing = add_members(Oracle.load(), read_json(project_path(args.members)),
                                       project_path(args.source).read_text(encoding="ascii"),
                                       args.flags, exclude)
        write_json(project_path(args.output or args.members), binding)
        print("members added; unwitnessed:", ", ".join(missing) or "none")
        return
    found = witnesses(Oracle.load(), [int(o, 16) for o in args.offsets], exclude)
    for offset, rows in found.items():
        print(f"{offset:04x}: {len(rows)} witnesses " + ", ".join(r["context"] for r in rows[:4]))
    if args.output:
        write_json(project_path(args.output), {f"{k:04x}": v for k, v in found.items()})


if __name__ == "__main__":
    main()
