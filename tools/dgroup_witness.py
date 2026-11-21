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
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("offsets", nargs="+", help="DGROUP offsets, hex")
    parser.add_argument("--exclude", action="append", default=[], help="START:END root positions, hex")
    parser.add_argument("--output")
    args = parser.parse_args()
    exclude = [tuple(int(x, 16) for x in e.split(":")) for e in args.exclude]
    found = witnesses(Oracle.load(), [int(o, 16) for o in args.offsets], exclude)
    for offset, rows in found.items():
        print(f"{offset:04x}: {len(rows)} witnesses " + ", ".join(r["context"] for r in rows[:4]))
    if args.output:
        write_json(project_path(args.output), {f"{k:04x}": v for k, v in found.items()})


if __name__ == "__main__":
    main()
