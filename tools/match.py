"""Strict bootstrap gate: complete single-function CODE segment, no fixup debt.

Fixup-bearing components deliberately fail until independently grounded symbolic
binding is implemented. Diagnostics never mask bytes or grant ownership.
"""
from pathlib import Path

from common import ROOT, read_json, require, sha
from oracle import Oracle
from omf import OmfReader


def get_target(identifier, oracle=None):
    oracle = oracle or Oracle.load()
    inventory = read_json(ROOT / "evidence/targets.json")
    require(inventory["target_sha256"] == sha(oracle.data), "Targets belong to another oracle")
    matches = [f for f in inventory["functions"] if identifier == f["id"]]
    require(len(matches) == 1, "Unknown/ambiguous reviewed target")
    target = matches[0]
    require(target["boundary_status"] == "REVIEWED" and target["boundary_evidence"],
            "Acceptance requires reviewed independent boundary evidence")
    space = oracle.spaces[target["space"]]
    raw = space.extent(target["segment"], target["offset"], target["size"])
    require(sha(raw) == target["sha256"], "Target extent differs from frozen original evidence")
    for call in target.get("call_sites", []):
        require(call["kind"] == "far_call", "Unsupported reviewed callsite proof")
        caller = oracle.spaces[call["space"]]
        at = call["image_offset"]
        import struct
        require(0 <= at and at+5 <= caller.size and caller.data[at] == 0x9a,
                "Reviewed far-call opcode does not exist")
        offset, segment = struct.unpack_from("<HH", caller.data, at+1)
        require(space.position(segment, offset) == space.position(target["segment"], target["offset"]),
                "Reviewed caller does not target this function")
        require(call["file_offset"] == caller.file_offset+at and
                call["relocated_segment_site"] == at+3 and
                any(r["image_offset"] == at+3 for r in caller.relocations),
                "Reviewed callsite relocation/file provenance mismatch")
    return target, space, raw


def check_object(obj_data, target, space, expected, public_name):
    """Compare the WHOLE emitted code extent, including any trailing bytes."""
    module = OmfReader().read(obj_data, "candidate")
    code = [s for s in module.segment_defs if s["class"] == "CODE"]
    require(len(code) == 1, "Bootstrap gate requires exactly one CODE segment")
    segment = code[0]
    name = segment["name"]
    require(not segment["use32"] and not segment["big"] and segment["alignment"] != 0,
            "Unsupported code segment declaration")
    fixups = [entry for record in module.fixups for entry in record["decoded"] if entry["kind"] == "fixup"]
    require(not fixups, "Fixup-bearing components require a future symbolic binding proof")
    require(all(s["length"] == 0 for s in module.segment_defs if s["name"] != name),
            "Unowned DATA/BSS/secondary declarations are not accepted")
    require(len(module.publics) == 1 and module.publics[0]["name"] == public_name and
            module.publics[0]["segment"] == name and module.publics[0]["offset"] == 0,
            "The public must own the complete CODE segment from offset zero")
    # Independently establish LEDATA coverage; OmfReader payload gaps must never become authority.
    initialized = set()
    supported = {0x80, 0x82, 0x88, 0x8a, 0x8c, 0x90, 0x96, 0x98, 0x9a, 0x9c, 0xa0}
    for record in module.records:
        require(record["type"] in supported, "Unsupported OMF record at strict acceptance")
        if record["type"] != 0xa0:
            continue
        body = bytes.fromhex(record["body_hex"])
        index, at = OmfReader._index(body, 0)
        import struct
        offset = struct.unpack_from("<H", body, at)[0]
        segment_name = module.segment_defs[index-1]["name"]
        require(segment_name == name, "Initialized secondary segment is unowned")
        span = set(range(offset, offset + len(body) - at - 2))
        require(not span.intersection(initialized), "Overlapping initialized object data")
        initialized.update(span)
    require(initialized == set(range(segment["length"])), "CODE segment contains uninitialized holes")
    actual = module.segments.get(name, b"")
    require(len(actual) == segment["length"], "Incomplete emitted CODE segment")
    start = space.position(target["segment"], target["offset"])
    relocations = [r for r in space.relocations if start <= r["image_offset"] < start+len(expected)]
    require(not relocations, "Original relocation obligations require symbolic candidate fixups")
    exact = actual == expected
    return {"exact": exact, "state": "CODE_EXACT" if exact else "CANDIDATE_C",
            "target_id": target["id"], "expected_size": len(expected), "emitted_size": len(actual),
            "expected_sha256": sha(expected), "emitted_sha256": sha(actual),
            "object_sha256": sha(obj_data), "fixups": 0, "relocations": 0,
            "unreferenced_external_declarations": module.externals,
            "fixup_thread_declarations": [entry for record in module.fixups for entry in record["decoded"]],
            "first_difference": next((i for i in range(max(len(actual), len(expected)))
                                      if actual[i:i+1] != expected[i:i+1]), None),
            "segment": segment, "public": module.publics[0],
            "record_types": [r["type"] for r in module.records],
            "proof_scope": "Complete component code and empty fixup/relocation obligations; object declarations recorded, original TU identity and structural link unproved"}


def compile_and_check(source, profile, public_name, identifier, workdir, flags=None, *, language="c"):
    from compiler import compile_c
    require(language in ("c", "asm"), "Unknown source language")
    target, space, expected = get_target(identifier)
    # Compiler source API is supplied by tools/compiler.py; source is independent of expected bytes.
    if language == "asm":
        from assembler import assemble_asm
        result = assemble_asm(Path(source), profile, flags=flags, workdir=Path(workdir))
    else:
        result = compile_c(Path(source), profile, flags=flags, workdir=Path(workdir))
    require(result.ok, "Historical compilation failed: " + result.log)
    report = check_object(result.obj.read_bytes(), target, space, expected, public_name)
    if language == "asm":
        report["state"] = "ASM_EXACT" if report["exact"] else "CANDIDATE_ASM"
    report["language"] = language
    return result, report
