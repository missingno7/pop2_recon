"""Worker-only RTLink 6.10 relocation contrasts. The executables are never run."""
from collections import Counter
from dataclasses import asdict
from itertools import groupby, product
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys

WORKER = Path(__file__).resolve().parent
ROOT = WORKER.parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from assembler import verify_assembler
from compiler import load_lock, sha256, verify_profile
from mz import MZ
from omf import OmfReader
from oracle import Oracle
from probe_rtlink import DESC_FMT, DESC_SIZE, parse_overlays

PROFILE = "msc600a_bound"
FLAGS = ["/c", "/AM", "/Oe", "/Gs", "/Zl", "/Gc", "/Zi"]
LINKER = Path("C:/tools/rtlink-plus-6.10/installed/RTLINK.EXE")
LIBRARY = LINKER.with_name("RTLUTILS.LIB")
PINNED = {
    LINKER: "ea6f57a470a26801086593d54aa99f6bf983dd024fc19b5e13361a07f43e17fb",
    LIBRARY: "c718bac09b0b1204a259494446acfa8bfffb129144cc7d36799855621f4fdccb",
}
START = """EXTRN PROBE_ENTRY:FAR
ENTRY_TEXT SEGMENT WORD PUBLIC 'CODE'
ASSUME CS:ENTRY_TEXT
PUBLIC ENTRY_POINT
ENTRY_POINT PROC FAR
    mov ax,SEG EMPTY_SINK
    push ax
    mov ax,OFFSET EMPTY_SINK
    push ax
    call PROBE_ENTRY
    mov ax,4c00h
    int 21h
ENTRY_POINT ENDP
EMPTY_SINK PROC FAR
    retf 4
EMPTY_SINK ENDP
ENTRY_TEXT ENDS
STACK SEGMENT PARA STACK 'STACK'
    dw 32 dup (?)
STACK ENDS
END ENTRY_POINT
"""
PROVIDER = """PROVIDE SEGMENT PARA PUBLIC 'CODE'
ASSUME CS:PROVIDE
PUBLIC X,Y,XADDR
    nop
XADDR LABEL FAR
X PROC FAR
    retf
X ENDP
Y PROC FAR
    retf
Y ENDP
ALIGN 16
PROVIDE ENDS
END
"""
PUBLIC = re.compile(
    r"^[ \t]*([0-9A-Fa-f]+):([0-9A-Fa-f]+)[ \t]+(?:(Res|Ovl|Abs)[ \t]+)?(\S+)"
    r"(?:[ \t]+\(([0-9A-Fa-f]+)H\))?[ \t]*$", re.MULTILINE
)


def digest(path):
    return sha256(path.read_bytes())


def relative(path):
    return path.relative_to(ROOT).as_posix()


def write_dos(path, source):
    path.write_bytes(source.replace("\n", "\r\n").encode("ascii"))


def execute(argv, directory, paths, log, timeout=60):
    env = {key: "." for key in ("TEMP", "TMP", "MSDOS_TEMP")}
    env.update(PATH=";".join(map(str, paths)), MSDOS_PATH=";".join(map(str, paths)),
               SYSTEMROOT=os.environ.get("SYSTEMROOT", r"C:\Windows"))
    result = subprocess.run(argv, cwd=directory, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=timeout,
                            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    (directory / log).write_bytes(result.stdout)
    output = result.stdout.decode("latin1")
    if result.returncode or re.search(r"\b(?:warning|error)\s+[A-Z]?\d+", output, re.I):
        raise RuntimeError(f"{relative(directory)}: {output}")
    return {"argv": argv, "cwd": relative(directory), "env": env, "log": log,
            "exit_code": result.returncode}


def build(name, source, runner, spec, asm=False):
    directory = WORKER / "runs" / "objects" / name
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / ("PROBE.ASM" if asm else "PROBE.C")
    obj = directory / "PROBE.OBJ"
    obj.unlink(missing_ok=True)
    write_dos(path, source)
    args = ["PROBE.ASM,PROBE.OBJ,NUL,NUL;"] if asm else [*FLAGS, "PROBE.C"]
    argv = [runner["path"], *runner["options"], spec["executable"], *args]
    receipt = execute(argv, directory, [spec["directory"], *spec.get("path_extra", [])], "build.log")
    receipt.update(source=relative(path), source_sha256=digest(path), object=relative(obj),
                   object_sha256=digest(obj))
    return receipt


def c_source(two_targets, across_chunks, split_first):
    names = ["X", "Y"] if two_targets else ["X"]
    headers = (
        "typedef void (far pascal *Function)(void);\n"
        "typedef void (far pascal *Sink)(Function);\n"
        + "".join(f"extern void far pascal {name}(void);\n" for name in names)
    )
    first = " ".join(f"use({name});" if split_first else f"{name}();" for name in names)
    second = " ".join(f"{name}();" if split_first else f"use({name});" for name in names)
    if across_chunks:
        return (headers + "void far pascal PROBE_ENTRY(Sink use) {\n"
                + f"    {first}\n    {second}\n"
                + "}\n"
                + "void far pascal REPEAT(Sink use) {\n"
                + f"    {first}\n    {second}\n"
                + "}\n")
    return (headers + "void far pascal PROBE_ENTRY(Sink use) {\n"
            + f"    {first} {second} {first} {second}\n" + "}\n")


def shape_source(statements, following=""):
    return (
        "typedef void (far pascal *Function)(void);\n"
        "typedef void (far pascal *Sink)(Function);\n"
        "extern void far pascal X(void), Y(void), XADDR(void);\n"
        "void far pascal PROBE_ENTRY(Sink use) {\n"
        f"    {statements}\n"
        "}\n" + (f"void far pascal REPEAT(Sink use) {{\n    {following}\n}}\n" if following else "")
    )


def read_c(artifact):
    data = (ROOT / artifact["object"]).read_bytes()
    module = OmfReader().read(data, artifact["object"])
    code = module.segment_bytes("PROBE_TEXT")
    code_index = next(i for i, d in enumerate(module.segment_defs, 1) if d["name"] == "PROBE_TEXT")
    chunks = []
    for kind, raw, file_offset in OmfReader.records(data):
        if kind != 0xA0:
            continue
        si, at = OmfReader._index(raw, 0)
        if si == code_index:
            chunks.append({"object_file_offset": file_offset,
                           "start": struct.unpack_from("<H", raw, at)[0],
                           "length": len(raw) - at - 2})
    fixups = []
    for record in module.fixups:
        if record["segment_index"] != code_index:
            continue
        for item in record["resolved"]:
            if item["kind"] != "fixup":
                continue
            field = item["segment_offset"]
            loc = item["loc_type"]
            assert item["target"]["name"] in ("X", "Y", "XADDR"), item
            assert loc in (2, 3, 5) and not item["self_relative"], item
            assert code[field:field + item["field_width"]] == bytes(item["field_width"]), item
            if loc == 3:
                assert code[field - 1] == 0x9A
            fixups.append({
                "index": len(fixups), "target": item["target"]["name"], "loc": loc,
                "field": field, "reloc_site": field + 2 if loc == 3 else field if loc == 2 else None,
                "chunk": record["data_offset"], "chunk_length": record["data_length"],
                "frame": item["frame"], "resolved_target": item["target"],
                "unlinked_instruction_bytes": code[max(0, field - 2):field + item["field_width"] + 2].hex(),
            })
    segment_fixes = [f for f in fixups if f["loc"] in (2, 3)]
    offset_fixes = {f["field"]: f for f in fixups if f["loc"] == 5}
    for fix in segment_fixes:
        assert fix["frame"]["kind"] == "target_frame" and fix["resolved_target"]["method"] == 6
        if fix["loc"] == 2:
            offset = offset_fixes.pop(fix["field"] - 3)
            assert offset["target"] == fix["target"] and offset["chunk"] == fix["chunk"]
            assert code[offset["field"] - 1] == 0xB8 and code[fix["field"] - 1] == 0xBA
            assert code[fix["field"] + 2:fix["field"] + 4] == b"\x52\x50"
            fix["paired_offset_field"] = offset["field"]
    assert not offset_fixes
    artifact.update(code_size=len(code), code_sha256=sha256(code), code_ledata=chunks,
                    fixups=fixups, publics=module.publics)


def map_publics(map_text):
    publics = {}
    for m in PUBLIC.finditer(map_text):
        segment, offset, scope, name, section = m.groups()
        publics[name] = (int(segment, 16), int(offset, 16), scope or "Res",
                         int(section, 16) if section else None)
    return publics


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def read_overlay(data, mz, publics):
    seg, off, _, _ = publics["$$OVLPBLOCK"]
    pblock = mz.header_size + seg * 16 + off
    count = u16(data, mz.header_size + seg * 16 + u16(data, pblock))
    table = mz.header_size + seg * 16 + u16(data, pblock + 2)
    assert data[pblock + 4] == DESC_SIZE
    # An uninitialized-only preload section is listed in the manager but
    # has no file payload. Pass the physical sections to the payload parser.
    last = struct.unpack_from(DESC_FMT, data, table + (count - 1) * DESC_SIZE)
    empty_preload = None
    if last[-1] == 0:
        _, _, position, high, flags, _, relocations, _, section, _ = last
        assert flags == 1 and relocations == 0 and section == count + 1
        assert (position + (high << 16)) * 16 == len(data)
        empty_preload = {"descriptor_offset": table + (count - 1) * DESC_SIZE,
                         "fields": last}
        count -= 1
    descriptors = parse_overlays(data, table, count)
    result = {}
    for d in descriptors:
        rows = []
        for index in range(d.relocation_count):
            offset, segment = struct.unpack_from("<HH", data, d.relocation_offset + 4 * index)
            site = (segment - d.link_segment) * 16 + offset
            rows.append({"index": index, "segment": segment, "offset": offset,
                         "image_offset": site, "stored_word": u16(data, d.code_offset + site)})
        result[d.overlay_id] = (d, rows)
    return result, empty_preload


def link(name, mode, runner, inputs):
    directory = WORKER / "runs" / "links" / name / mode
    directory.mkdir(parents=True, exist_ok=True)
    files = {"START": inputs["start"], "PROBE": inputs[name], "DEFS": inputs["provider"]}
    for dos_name, artifact in files.items():
        obj = directory / (dos_name + ".OBJ")
        shutil.copyfile(ROOT / artifact["object"], obj)
        assert digest(obj) == artifact["object_sha256"]
    header = "SYNTAX FREEFORMAT\nVERBOSE\nOUTPUT ORDER\nMAP\nFILE START\n"
    if mode == "root":
        response = header + "FILE DEFS\nFILE PROBE\n"
    else:
        shutil.copyfile(LIBRARY, directory / LIBRARY.name)
        section_body = 'BEGINAREA\n  SECTION = "CALLER"\n    FILE PROBE\nENDAREA\n'
        if mode == "overlay_late":
            response = header + section_body + "FILE DEFS\n"
        else:
            response = header + "FILE DEFS\n" + section_body
    arf = directory / "ORDER.ARF"
    write_dos(arf, response)
    exe = directory / "ORDER.EXE"
    exe.unlink(missing_ok=True)
    argv = [runner["path"], *runner["options"], str(LINKER), "@ORDER.ARF"]
    receipt = execute(argv, directory, [directory, LINKER.parent], "link.log")
    full_log = (directory / "link.log").read_bytes().decode("latin1")
    assert not re.search(r"warning|unresolved|duplicate start", full_log, re.I), full_log
    data = exe.read_bytes()
    mz = MZ.parse(data)
    publics = map_publics((directory / "ORDER.MAP").read_text(encoding="latin1"))
    caller_seg, caller_off, scope, section = publics["PROBE_ENTRY"]
    obj_caller = next(p for p in inputs[name]["publics"] if p["name"] == "PROBE_ENTRY")
    base = caller_seg * 16 + caller_off - obj_caller["offset"]
    overlays = {}
    empty_preload = None
    if mode == "root":
        assert scope == "Res" and len(data) == mz.file_image_size
        rows = mz.relocations
        payload = mz.load_image(data)
    else:
        assert scope == "Ovl" and section == 2
        overlays, empty_preload = read_overlay(data, mz, publics)
        descriptor, rows = overlays[section]
        base -= descriptor.link_segment * 16
        payload = data[descriptor.code_offset:descriptor.declared_code_end]
    assert publics["X"][2] == publics["Y"][2] == publics["XADDR"][2] == "Res"
    assert publics["XADDR"] == publics["X"] and publics["X"] != publics["Y"]
    for fix in inputs[name]["fixups"]:
        if fix["loc"] == 5:
            assert u16(payload, base + fix["field"]) == publics[fix["target"]][1]
    sites = {base + f["reloc_site"]: f for f in inputs[name]["fixups"] if f["reloc_site"] is not None}
    observed = []
    for row in rows:
        if row["image_offset"] not in sites:
            continue
        fix = sites[row["image_offset"]]
        provider_seg, provider_off, _, _ = publics[fix["target"]]
        assert row["stored_word"] == provider_seg, (row, fix, publics)
        if fix["loc"] == 3:
            assert u16(payload, row["image_offset"] - 2) == provider_off
            assert payload[row["image_offset"] - 3] == 0x9A
        observed.append({**row, "fixup_index": fix["index"], "target": fix["target"],
                         "loc": fix["loc"], "code_site": fix["reloc_site"], "chunk": fix["chunk"]})
    assert Counter(r["image_offset"] for r in observed) == Counter(sites.keys())
    if overlays:
        assert len(rows) == len(observed)
    mz_rows = list(mz.relocations)
    receipt.update(response=response, response_sha256=digest(arf), exe_sha256=digest(exe),
                   map_sha256=digest(directory / "ORDER.MAP"), log_sha256=digest(directory / "link.log"),
                   object_sha256={n: a["object_sha256"] for n, a in files.items()},
                   publics={n: publics[n] for n in ("X", "Y", "XADDR", "PROBE_ENTRY")},
                   mz_header=mz.header, mz_rows=mz_rows, caller_image_offset=base,
                   overlays={i: {"descriptor": asdict(d), "rows": records}
                             for i, (d, records) in overlays.items()},
                   empty_preload=empty_preload, observed=observed)
    return receipt


def original_windows():
    oracle = Oracle.load()
    cases = [
        ("DRAWCAVE", 6, 0x2a93, 0x0ebc, 264, "w40_drawcave", "DRAWCAVEREVPLATE"),
        ("DRAWTEMP", 9, 0x2b4b, 0x0b6a, 284, "w41_drawtemp", "DRAW_TEMPLE_POSTS"),
    ]
    report = {"target_sha256": sha256(oracle.data), "windows": {}}
    for name, overlay, frame, entry, size, worker, public_name in cases:
        space = oracle.spaces[f"overlay-{overlay}"]
        base = space.position(frame, 0)
        binding_path = ROOT / "build" / "workers" / worker / "binding.json"
        binding = json.loads(binding_path.read_bytes())
        public = next(p for p in binding["declarations"]["publics"] if p["name"] == public_name)
        bias = entry - public["offset"]
        raw_projection = []
        for i, fix in enumerate(binding["fixups"]):
            loc = fix["loc_type"]
            if fix["segment_index"] != public["segment_index"] or loc not in (2, 3):
                continue
            field = bias + fix["segment_offset"]
            site = field + (2 if loc == 3 else 0)
            raw_projection.append({"binding_fixup_index": i, "cs_field": field, "cs_site": site,
                                   "loc": loc, "target": fix["target"]["name"]})
        candidates = {f["cs_site"]: f for f in raw_projection}
        assert len(candidates) == len(raw_projection)
        rows = []
        for r in space.relocations:
            if r["segment"] != frame:
                continue
            pos = r["image_offset"]
            cs_offset = pos - base
            row = {**r, "cs_offset": cs_offset, "in_function": entry <= cs_offset < entry + size,
                   "bytes_at_site_minus_5_through_plus_7": space.data[pos-5:pos+8].hex()}
            candidate = candidates[cs_offset]
            candidate["oracle_index"] = r["index"]
            if candidate["loc"] == 3:
                assert space.data[pos - 3] == 0x9A
            symbol = binding["symbols"].get(candidate["target"])
            if candidate["loc"] == 3:
                row["direct_call_offset"] = u16(space.data, pos - 2)
            if symbol:
                assert r["stored_word"] == symbol["segment"]
                if candidate["loc"] == 3:
                    assert row["direct_call_offset"] == symbol["offset"]
            candidate["binding_address_available"] = symbol is not None
            if (name, r["index"]) == ("DRAWCAVE", 137):
                assert space.data[pos - 4] == 0xB8 and space.data[pos - 1] == 0xBA
                row["split_address_offset"] = u16(space.data, pos - 3)
                assert row["split_address_offset"] == symbol["offset"]
            if name == "DRAWTEMP" and r["index"] in (129, 130, 142):
                assert space.data[pos - 3:pos - 1] == b"\xc7\x46"
                assert space.data[pos - 8:pos - 6] == b"\xc7\x46"
                row["split_address_offset"] = u16(space.data, pos - 5)
                assert row["split_address_offset"] == symbol["offset"]
            row["candidate"] = candidate
            rows.append(row)
        assert len(rows) == len(raw_projection) and all("oracle_index" in f for f in raw_projection)
        kind_runs = []
        for (target, loc), run in groupby(rows, lambda r: (r["candidate"]["target"], r["candidate"]["loc"])):
            indexes = [r["index"] for r in run]
            raw_indexes = [f["oracle_index"] for f in raw_projection if (f["target"], f["loc"]) == (target, loc)]
            kind_runs.append({"target": target, "loc": loc, "original_indices": indexes,
                              "candidate_raw_indices": raw_indexes, "raw_order_agrees": indexes == raw_indexes})
        assert len({(run["target"], run["loc"]) for run in kind_runs}) == len(kind_runs)
        report["windows"][name] = {
            "descriptor": space.descriptor, "cs": frame, "entry": entry, "size": size,
            "rows": rows, "archived_candidate_bias": bias, "archived_candidate_raw_projection": raw_projection,
            "kind_runs": kind_runs, "binding_path": relative(binding_path), "binding_sha256": digest(binding_path),
            "notes_sha256": digest(binding_path.with_name("notes.md")),
        }
    return report


def check_order(artifact, result):
    raw = [f for f in artifact["fixups"] if f["loc"] in (2, 3)]
    target_runs = [target for target, _ in groupby(result["observed"], lambda r: r["target"])]
    assert len(target_runs) == len(set(target_runs)), target_runs
    predicted = [f["index"] for target in target_runs for loc in (2, 3)
                 for f in raw if (f["target"], f["loc"]) == (target, loc)]
    assert predicted == [r["fixup_index"] for r in result["observed"]]
    result["model_check"] = {"target_run_order": target_runs,
                             "predicted_fixup_indices_with_observed_bucket_permutation": predicted}


def main():
    runner = load_lock()["runner"]
    compiler = verify_profile(PROFILE)
    assembler = verify_assembler("masm510")
    for path, expected in PINNED.items():
        assert digest(path) == expected
    report = {
        "schema": 1, "script_sha256": digest(Path(__file__).resolve()),
        "original": original_windows(), "tools": {
            "runner": runner, "compiler_profile": PROFILE, "compiler_flags": FLAGS,
            "compiler_files": compiler["files"], "assembler_files": assembler["files"],
            "linker_files": {str(p): h for p, h in PINNED.items()},
        },
        "inputs": {}, "links": {},
    }
    inputs = report["inputs"]
    inputs["start"] = build("start", START, runner, assembler, asm=True)
    inputs["provider"] = build("provider", PROVIDER, runner, assembler, asm=True)
    cases = {}
    for two_targets, across_chunks, split_first in product((False, True), repeat=3):
        name = f"{'xy' if two_targets else 'x'}_{'across' if across_chunks else 'same'}_{'addr' if split_first else 'call'}"
        cases[name] = c_source(two_targets, across_chunks, split_first)
    cases.update({
        "x_kind_addr": shape_source("use(X); use(X);", "X(); X();"),
        "x_kind_call": shape_source("X(); X();", "use(X); use(X);"),
        "xy_kind_addr": shape_source("use(X); use(Y); use(X); use(Y);", "X(); Y(); X(); Y();"),
        "xy_kind_call": shape_source("X(); Y(); X(); Y();", "use(X); use(Y); use(X); use(Y);"),
        "cave_same": shape_source("Y(); X(); use(X);"),
        "cave_across": shape_source("Y(); X();", "use(X);"),
        "temple_same": shape_source("use(X); use(X); use(Y); X();"),
        "temple_across": shape_source("use(X);", "use(X); use(Y); X();"),
        "cave_alias": shape_source("Y(); X(); use(XADDR);"),
        "temple_alias": shape_source("use(XADDR); use(XADDR); use(Y); X();"),
    })
    for name, source in cases.items():
        artifact = build(name, source, runner, compiler)
        read_c(artifact)
        codes = [f for f in artifact["fixups"] if f["loc"] in (2, 3)]
        chunks = {f["chunk"] for f in codes}
        if "_same" in name:
            assert len(chunks) == 1
        if "_across" in name or "_kind" in name:
            assert len(chunks) == 2
        if "_kind" in name:
            assert len({(f["chunk"], f["loc"]) for f in codes}) == 2
        if name == "temple_across":
            assert len({f["chunk"] for f in codes if f["target"] == "X" and f["loc"] == 2}) == 2
        inputs[name] = artifact
        print(name, [(f["target"], f["loc"], f["field"], f["chunk"]) for f in artifact["fixups"]],
              flush=True)
        report["links"][name] = {}
        for mode in ("root", "overlay", "overlay_late"):
            result = link(name, mode, runner, inputs)
            check_order(artifact, result)
            report["links"][name][mode] = result
            print(mode, [(r["target"], r["loc"], r["code_site"]) for r in result["observed"]], flush=True)
        assert len({tuple(r["fixup_index"] for r in link["observed"])
                    for link in report["links"][name].values()}) == 1
    links = [r for modes in report["links"].values() for r in modes.values()]
    report["summary"] = {"caller_variants": len(cases), "links": len(links),
                         "overlay_links": len(cases) * 2,
                         "caller_relocations_checked": sum(len(r["observed"]) for r in links),
                         "resident_and_overlay_caller_orders_agree": True}
    temp = WORKER / "receipt.json.tmp"
    temp.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    temp.replace(WORKER / "receipt.json")


if __name__ == "__main__":
    main()
