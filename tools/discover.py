"""Conservative function-entry and call-graph discovery for the frozen DOS image.

Every row is a hypothesis with its originating evidence. Prologue scans and linear
instruction sweeps never silently promote bytes into canonical functions.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict, deque
from dataclasses import asdict
from pathlib import Path

from common import ROOT, sha, write_json
from disasm import decode_one

REFERENCE = ROOT / "build/references/prince-of-persia-2-windows/recomp"
DEFAULT_OUTPUT = ROOT / "build/workers/inventory/discovery.json"
PROLOGUE = re.compile(rb"(?:\x45)?\x55\x8b\xec")


def _field(obj, *names, default=None):
    for name in names:
        if isinstance(obj, dict) and name in obj:
            return obj[name]
        if hasattr(obj, name):
            return getattr(obj, name)
    return default


def _byte_image(oracle):
    data = _field(oracle, "data", "raw_data", "file_bytes", "bytes")
    if callable(data):
        data = data()
    if not isinstance(data, (bytes, bytearray, memoryview)):
        raise TypeError("Oracle must expose full executable bytes as data or raw_data")
    return bytes(data)


def _space_rows(oracle, full_data):
    rows = []
    source_spaces = _field(oracle, "spaces", default=())
    if isinstance(source_spaces, dict):
        source_spaces = source_spaces.values()
    for space in source_spaces:
        name = str(_field(space, "name"))
        file_offset = int(_field(space, "file_offset", default=0))
        size = int(_field(space, "size", default=0))
        link_segment = int(_field(space, "link_segment", default=0)) & 0xffff
        image_offset = _field(space, "image_offset")
        space_data = _field(space, "data")
        if space_data is None:
            if file_offset < 0 or size < 0 or file_offset + size > len(full_data):
                raise ValueError(f"Oracle space {name} exceeds source image")
            space_data = full_data[file_offset:file_offset + size]
        space_data = bytes(space_data)
        if size < 0 or len(space_data) < size:
            raise ValueError(f"Oracle space {name} exceeds source image")
        row = {"name": name, "link_segment": link_segment, "file_offset": file_offset,
               "size": min(size, len(space_data)), "image_offset": image_offset, "bytes": space_data,
               "relocations": list(_field(space, "relocations", default=()) or ()), "object": space}
        rows.append(row)
    return rows


def _space_for_name(spaces, name):
    return next((s for s in spaces if s["name"].casefold() == name.casefold()), None)


def _segment_offset_to_space(spaces, segment, offset, preferred=None):
    segment &= 0xffff
    offset &= 0xffff
    hits = []
    candidates = [preferred] if preferred is not None else spaces
    for space in candidates:
        obj = space["object"]
        position = getattr(obj, "position", None)
        try:
            rel = position(segment, offset) if position else ((segment - space["link_segment"]) & 0xffff) * 16 + offset
        except (KeyError, ValueError, IndexError):
            rel = None
        if rel is not None and 0 <= rel < space["size"]:
            hits.append((space, rel))
    return hits[0] if len(hits) == 1 else None


def _candidate(rows, seen, space, segment, offset, provenance, confidence, note=None):
    obj = space["object"]
    position = getattr(obj, "position", None)
    try:
        pos = position(segment, offset) if position else ((segment - space["link_segment"]) & 0xffff) * 16 + offset
    except (KeyError, ValueError, IndexError):
        return None
    if pos is None or not (0 <= pos < space["size"]):
        return None
    key = (space["name"], segment & 0xffff, offset & 0xffff)
    row = seen.get(key)
    evidence = {"source": provenance, "confidence": confidence}
    if note:
        evidence["note"] = note
    if row is None:
        row = {"id": f"{key[0]}:{key[1]:04x}:{key[2]:04x}", "space": key[0],
               "segment": key[1], "offset": key[2], "evidence": [],
               "state": "UNKNOWN", "probable_abi": None, "class_hypotheses": [],
               "extent": {"start": pos, "end_exclusive": None, "status": "unclassified"},
               "callers": [], "callees": [], "relocations": [], "_position": pos}
        seen[key] = row
        rows.append(row)
    if evidence not in row["evidence"]:
        row["evidence"].append(evidence)
    return row


def _read_seed_file(path, expected_space=None):
    if not path.is_file():
        return []
    out = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) == 1 and expected_space:
            space_name, address = expected_space, parts[0]
        elif len(parts) >= 2:
            space_name, address = parts[0], parts[1]
        else:
            continue
        try:
            segment, offset = (int(v, 16) for v in address.split(":", 1))
        except (ValueError, TypeError):
            continue
        out.append((space_name, segment, offset))
    return out


def _reloc_location(reloc):
    if isinstance(reloc, dict):
        pos = _field(reloc, "image_offset", "offset", "position", "relocation_offset")
    else:
        pos = _field(reloc, "image_offset", "offset", "position", "relocation_offset")
    return int(pos) if pos is not None else None


def discover(oracle, *, reference=REFERENCE, prologue_scan=True):
    full_data = _byte_image(oracle)
    spaces = _space_rows(oracle, full_data)
    rows, seen = [], {}

    # The PE/MZ initial CS:IP is strong entry evidence when Oracle exposes it.
    entry = _field(oracle, "entry", "initial_entry")
    if entry is None:
        mz = _field(oracle, "mz")
        header = _field(mz, "header", default={}) if mz is not None else {}
        if header:
            entry = {"segment": header.get("initial_cs"), "offset": header.get("initial_ip")}
    if entry is not None:
        seg = _field(entry, "segment", "cs")
        off = _field(entry, "offset", "ip")
        if seg is not None and off is not None:
            root_space = _space_for_name(spaces, "root")
            if root_space:
                _candidate(rows, seen, root_space, int(seg), int(off), "mz_entry_point", "high")

    # Existing public reference lists are separate evidence, never ground truth.
    for name, segment, offset in _read_seed_file(reference / "prologues_root.txt", "root"):
        space = _space_for_name(spaces, name)
        if space:
            _candidate(rows, seen, space, segment, offset, "reference_root_prologue", "medium",
                       f"reference address {segment:04x}:{offset:04x}")
    for name, segment, offset in _read_seed_file(reference / "extra_entries.txt"):
        space = _space_for_name(spaces, name)
        if space:
            _candidate(rows, seen, space, segment, offset, "reference_extra_entry", "medium",
                       f"reference address {segment:04x}:{offset:04x}")

    # Far pointers have an offset word followed by a relocated segment word.
    # The location alone is evidence of a pointer. It is a function candidate
    # only when its value resolves uniquely and begins with a recognized prologue.
    pointer_count = 0
    far_call_sites = []
    for source in spaces:
        for reloc in source["relocations"]:
            pos = _reloc_location(reloc)
            if pos is None or pos < 2 or pos + 2 > source["size"]:
                continue
            pointer_count += 1
            target_off = int.from_bytes(source["bytes"][pos - 2:pos], "little")
            target_seg = int.from_bytes(source["bytes"][pos:pos + 2], "little")
            mapped = _segment_offset_to_space(spaces, target_seg, target_off)
            if not mapped:
                continue
            target, target_pos = mapped
            opcode_pos = pos - 3
            is_far_call = opcode_pos >= 0 and source["bytes"][opcode_pos] == 0x9a
            has_prologue = bool(PROLOGUE.match(target["bytes"][target_pos:target_pos + 4]))
            if is_far_call or has_prologue:
                target_row = _candidate(rows, seen, target, target_seg, target_off,
                                        "direct_far_call_relocation" if is_far_call else
                                        ("overlay_entry_pointer" if target["name"].casefold() != "root"
                                         else "relocated_far_pointer_to_prologue"),
                                        "medium", f"pointer at {source['name']}+0x{pos:x}")
                if target_row:
                    target_row["relocations"].append({
                        "direction": "incoming", "kind": "far_call" if is_far_call else "far_pointer",
                        "source_space": source["name"], "source_position": pos,
                        "source_file_offset": source["file_offset"] + pos,
                        "relocation_index": _field(reloc, "index"),
                    })
                if is_far_call and target_row:
                    far_call_sites.append((source["name"], opcode_pos, target_row["id"]))

    # Build only direct near-call edges. A call target is an entry hypothesis;
    # decoding continues with explicit tentative boundaries and conservative caps.
    queue = deque((r["space"], r["segment"], r["offset"]) for r in rows)
    decoded = set()
    call_edges = set()
    while queue:
        space_name, segment, start = queue.popleft()
        source = _space_for_name(spaces, space_name)
        source_row = seen.get((space_name, segment, start))
        if source is None or source_row is None or source_row["id"] in decoded:
            continue
        decoded.add(source_row["id"])
        method = getattr(source["object"], "position", None)
        try:
            start_pos = method(segment, start) if method else ((segment - source["link_segment"]) & 0xffff) * 16 + start
        except (KeyError, ValueError, IndexError):
            continue
        if start_pos is None:
            continue
        pos, end, insn_count, stop = start_pos, start_pos, 0, "decode_gap"
        while pos < source["size"] and insn_count < 512 and pos - start_pos < 0x400:
            ins = decode_one(source["bytes"], pos,
                             origin=start - start_pos)
            if ins is None or not ins.size:
                stop = "undecodable_byte"
                break
            insn_count += 1
            pos += ins.size
            end = pos
            if ins.flow == "call" and ins.near_target is not None:
                target_off = ins.near_target & 0xffff
                mapped = _segment_offset_to_space(spaces, segment, target_off, preferred=source)
                if mapped:
                    target, _target_pos = mapped
                    target_row = _candidate(rows, seen, target, segment, target_off,
                                            "direct_near_call", "medium",
                                            f"call from {source_row['id']}+0x{ins.offset:x}")
                    if target_row:
                        edge = (source_row["id"], target_row["id"])
                        call_edges.add(edge)
                        target_row["callers"].append(source_row["id"])
                        source_row["callees"].append(target_row["id"])
                        if target_row["id"] not in decoded:
                            queue.append((target["name"], segment, target_off))
            elif ins.flow == "call" and ins.far_target is not None:
                far_segment, far_offset = ins.far_target
                mapped = _segment_offset_to_space(spaces, far_segment, far_offset)
                if mapped:
                    target, _target_pos = mapped
                    target_row = _candidate(rows, seen, target, far_segment, far_offset,
                                            "direct_far_call", "medium",
                                            f"far call from {source_row['id']}+0x{ins.offset:x}")
                    if target_row:
                        call_edges.add((source_row["id"], target_row["id"]))
                        target_row["callers"].append(source_row["id"])
                        source_row["callees"].append(target_row["id"])
                        if target_row["id"] not in decoded:
                            queue.append((target["name"], far_segment, far_offset))
            if ins.flow == "return":
                stop = "return"
                break
            if ins.flow == "jump" and ins.mnemonic in {"jmp", "ljmp"}:
                stop = "unfollowed_jump"
                break
        if insn_count >= 512:
            stop = "instruction_limit"
        elif pos - start_pos >= 0x400:
            stop = "byte_limit"
        source_row["extent"] = {"start": start_pos, "end_exclusive": end,
                                "status": "tentative_linear_sweep", "stop_reason": stop,
                                "decoded_instructions": insn_count}

    # Attribute relocated far-call sites to any traced tentative extent that covers them.
    by_name = defaultdict(list)
    for row in rows:
        by_name[row["space"]].append(row)
    for space_name, opcode_pos, target_id in far_call_sites:
        caller = next((r for r in by_name[space_name]
                       if r["extent"].get("end_exclusive") is not None
                       and r["extent"]["start"] <= opcode_pos < r["extent"]["end_exclusive"]), None)
        if caller:
            caller["callees"].append(target_id)
            caller["relocations"].append({"direction": "outgoing", "kind": "far_call",
                                          "target_id": target_id, "source_space": space_name,
                                          "source_position": opcode_pos})
            target = next((r for r in rows if r["id"] == target_id), None)
            if target:
                target["callers"].append(caller["id"])

    # Prologue-only scan creates low-confidence hypotheses; no extent is assigned.
    prologue_candidates = 0
    if prologue_scan:
        for space in spaces:
            for match in PROLOGUE.finditer(space["bytes"]):
                pos = match.start()
                # Oracle's map owns all segment wrapping and alias decisions.
                address = getattr(space["object"], "address", None)
                if not address:
                    continue
                target_addr = address(pos)
                segment, offset = target_addr["segment"], target_addr["offset"]
                if (space["name"], segment, offset) in seen:
                    continue
                row = _candidate(rows, seen, space, segment, offset,
                                 "prologue_scan_tentative", "low",
                                 "raw-byte prologue pattern; not a verified instruction boundary")
                if row is None:
                    continue
                row["extent"] = {"start": pos, "end_exclusive": None, "status": "unclassified"}
                prologue_candidates += 1

    for row in rows:
        row["callers"] = sorted(set(row["callers"]))
        row["callees"] = sorted(set(row["callees"]))
        unique_relocations = {json.dumps(item, sort_keys=True): item for item in row["relocations"]}
        row["relocations"] = [unique_relocations[key] for key in sorted(unique_relocations)]
        row["evidence"].sort(key=lambda e: (e["source"], e["confidence"], e.get("note", "")))
        row.pop("_position", None)
    rows.sort(key=lambda r: (r["space"], r["segment"], r["offset"]))
    return {"schema": 1, "method": "conservative-16bit-static-discovery",
            "target_sha256": sha(full_data),
            "source_authority": "immutable Oracle bytes and separately attributed reference hints",
            "confidence_guide": {"high": "format-defined initial entry", "medium": "direct flow or labeled reference/pointer evidence",
                                 "low": "byte-pattern hint only"},
            "spaces": [{k: s[k] for k in ("name", "link_segment", "file_offset", "size", "image_offset")}
                       for s in spaces],
            "statistics": {"candidate_count": len(rows), "decoded_entry_count": len(decoded),
                           "call_edge_count": len(call_edges), "relocated_pointer_sites": pointer_count,
                           "prologue_only_candidates": prologue_candidates},
            "functions": rows}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--no-prologue-scan", action="store_true")
    args = parser.parse_args(argv)
    try:
        from oracle import Oracle
        result = discover(Oracle.load(), prologue_scan=not args.no_prologue_scan)
        write_json(args.output, result)
    except Exception as exc:
        parser.exit(2, f"discovery failed: {exc}\n")
    print(json.dumps(result["statistics"], sort_keys=True))


if __name__ == "__main__":
    main()
