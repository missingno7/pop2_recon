"""Assemble a compact evidence packet for one discovered function candidate."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from common import ROOT
from disasm import decode_one

DEFAULT_INPUT = ROOT / "build/workers/inventory/discovery.json"
DEFAULT_OUTPUT = ROOT / "build/workers/inventory/context.json"
DEFAULT_TARGETS = ROOT / "evidence/targets.json"
DEFAULT_MANIFEST = ROOT / "layout/manifest.json"


def _resolve_function(discovery, function_id, oracle):
    functions = discovery.get("functions", [])
    by_id = {row["id"]: row for row in functions}
    try:
        space_name, seg_text, off_text = function_id.rsplit(":", 2)
        segment, offset = int(seg_text, 16), int(off_text, 16)
        space = oracle.spaces[space_name]
        position = space.position(segment, offset)
    except (ValueError, KeyError):
        row = by_id.get(function_id)
        return (row, [function_id]) if row else (None, [])
    aliases = []
    for row in functions:
        if row.get("space") != space_name:
            continue
        try:
            if space.position(row["segment"], row["offset"]) == position:
                aliases.append(row)
        except (KeyError, ValueError):
            continue
    if not aliases:
        return None, []
    aliases.sort(key=lambda row: (-(len(row.get("callers", [])) + len(row.get("callees", []))),
                                  -len(row.get("evidence", [])), row["id"] != function_id, row["id"]))
    return aliases[0], [row["id"] for row in aliases]


def build_context(discovery, function_id, oracle, *, asm=False):
    target_sha = hashlib.sha256(oracle.data).hexdigest()
    if discovery.get("target_sha256") != target_sha:
        raise ValueError("Discovery target SHA-256 does not match the current immutable Oracle")
    functions = discovery.get("functions", [])
    by_id = {row["id"]: row for row in functions}
    row, aliases = _resolve_function(discovery, function_id, oracle)
    if row is None:
        raise KeyError(f"Unknown function candidate: {function_id}")
    alias_rows = [by_id[key] for key in aliases if key in by_id]
    caller_ids = sorted({key for item in alias_rows for key in item.get("callers", [])})
    callee_ids = sorted({key for item in alias_rows for key in item.get("callees", [])})
    related = sorted(set(caller_ids + callee_ids))
    packet = {
        "schema": 1,
        "function": row,
        "callers": [by_id[key] for key in caller_ids if key in by_id],
        "callees": [by_id[key] for key in callee_ids if key in by_id],
        "related_candidates": related,
        "space": next((s for s in discovery.get("spaces", []) if s["name"] == row["space"]), None),
        "interpretation": "Discovery hypotheses with tentative extents; verify against immutable bytes before naming or matching.",
        "target_sha256": target_sha,
    }
    if row["id"] != function_id:
        packet["requested_id"] = function_id
    if len(aliases) > 1:
        packet["same_space_alias_ids"] = aliases
    if asm:
        space = oracle.spaces[row["space"]]
        start = space.position(row["segment"], row["offset"])
        # Keep the caller's requested CS:IP alias in the display even when the
        # richer same-position candidate uses a different alias.
        display_segment, display_offset = row["segment"], row["offset"]
        try:
            req_space, req_seg, req_off = function_id.rsplit(":", 2)
            req_segment, req_offset = int(req_seg, 16), int(req_off, 16)
            if req_space == row["space"] and space.position(req_segment, req_offset) == start:
                display_segment, display_offset = req_segment, req_offset
        except (ValueError, KeyError):
            pass
        extent = row.get("extent", {})
        end = extent.get("end_exclusive")
        if end is None or end <= start:
            end = min(space.size, start + 64)
        else:
            end = min(space.size, end)
        decoded = []
        pos = start
        while pos < end and len(decoded) < 128:
            ins = decode_one(space.data, pos, origin=display_offset - start)
            if ins is None or not ins.size:
                break
            canonical_address = space.address(pos)
            decoded.append({"address": {"space": row["space"], "segment": display_segment,
                                        "offset": (display_offset + pos - start) & 0xffff},
                            "canonical_address": canonical_address,
                            "position": pos, "file_offset": space.file_offset + pos,
                            "bytes_hex": ins.bytes_hex, "size": ins.size, "mnemonic": ins.mnemonic,
                            "operands": ins.operands, "flow": ins.flow,
                            "near_target": ins.near_target, "far_target": ins.far_target})
            pos += ins.size
        packet["asm"] = decoded
        packet["asm_bytes_sha256"] = hashlib.sha256(space.data[start:pos]).hexdigest()
        packet["asm_extent_status"] = extent.get("status", "unclassified")
    return packet


def _fallback_discovery(target_index, function_id, oracle):
    target_sha = target_index.get("target_sha256")
    if target_sha != hashlib.sha256(oracle.data).hexdigest():
        raise ValueError("Reviewed-target index SHA-256 does not match the current immutable Oracle")
    try:
        space_name, segment_text, offset_text = function_id.rsplit(":", 2)
        segment, offset = int(segment_text, 16), int(offset_text, 16)
        space = oracle.spaces[space_name]
        position = space.position(segment, offset)
    except (ValueError, KeyError) as exc:
        raise ValueError(f"Invalid or out-of-range reviewed fallback target: {function_id}") from exc
    target = next((item for item in target_index.get("functions", [])
                   if item.get("id") == function_id), None)
    if target is None:
        for item in target_index.get("functions", []):
            if item.get("space") != space_name:
                continue
            try:
                if space.position(item["segment"], item["offset"]) == position:
                    target = item
                    break
            except (KeyError, ValueError):
                continue
    if target is None:
        raise KeyError(f"No discovery file and no reviewed fallback target for {function_id}")
    if target.get("space") != space_name:
        raise ValueError(f"Reviewed fallback target belongs to another space: {function_id}")
    target_position = space.position(target["segment"], target["offset"])
    if target_position != position:
        raise ValueError(f"Reviewed fallback target address does not match its ID: {function_id}")
    size = int(target.get("size", 0))
    if size <= 0 or position + size > space.size:
        raise ValueError(f"Reviewed fallback extent is outside Oracle space: {function_id}")
    target_segment, target_offset = target["segment"], target["offset"]
    actual_sha = hashlib.sha256(space.extent(target_segment, target_offset, size)).hexdigest()
    if target.get("sha256") != actual_sha:
        raise ValueError(f"Reviewed fallback extent SHA-256 mismatch: {function_id}")
    row = dict(target)
    row.update({"id": function_id, "space": space_name, "segment": segment, "offset": offset,
                "extent": {"start": position, "end_exclusive": position + size,
                           "status": "reviewed_extent", "size": size, "sha256": actual_sha},
                "callers": row.get("callers", []), "callees": row.get("callees", []),
                "relocations": row.get("relocations", [])})
    return {"schema": 1, "target_sha256": target_sha, "functions": [row],
            "spaces": [{"name": space_name, "link_segment": space.link_segment,
                        "file_offset": space.file_offset, "size": space.size}]}


def _owner_recipe(manifest, function_id, *, root=ROOT, oracle=None):
    owner = next((item for item in manifest.get("owners", [])
                  if item.get("target_id") == function_id), None)
    if owner is None and oracle is not None:
        try:
            space_name, seg_text, off_text = function_id.rsplit(":", 2)
            space = oracle.spaces[space_name]
            position = space.position(int(seg_text, 16), int(off_text, 16))
            for candidate in manifest.get("owners", []):
                try:
                    own_space, own_seg, own_off = candidate["target_id"].rsplit(":", 2)
                    if own_space == space_name and space.position(int(own_seg, 16), int(own_off, 16)) == position:
                        owner = candidate
                        break
                except (KeyError, ValueError):
                    continue
        except (KeyError, ValueError):
            pass
    if owner is None:
        return None
    recipe_path = (root / owner["recipe"]).resolve()
    if not recipe_path.is_relative_to(root.resolve()):
        raise ValueError("Owner recipe path escapes the project")
    recipe = json.loads(recipe_path.read_text(encoding="utf-8"))
    if recipe.get("target_id") != owner["target_id"]:
        raise ValueError(f"Owner recipe target mismatch for {owner['target_id']}")
    return {"owner": owner, "recipe": recipe}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("function_id")
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--targets", type=Path, default=DEFAULT_TARGETS,
                        help="reviewed Oracle-validated target rows for missing discoveries")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST,
                        help="canonical owner manifest; attached separately from discovery")
    parser.add_argument("--asm", action="store_true", help="include a bounded 16-bit disassembly from current Oracle bytes")
    args = parser.parse_args(argv)
    from oracle import Oracle
    oracle = Oracle.load()
    target_index = json.loads(args.targets.read_text(encoding="utf-8")) if args.targets.is_file() else None
    if args.input.is_file():
        discovery = json.loads(args.input.read_text(encoding="utf-8"))
    else:
        if target_index is None:
            parser.error(f"Discovery is absent and reviewed target index is unavailable: {args.targets}")
        discovery = _fallback_discovery(target_index, args.function_id, oracle)
    if _resolve_function(discovery, args.function_id, oracle)[0] is None:
        if target_index is None:
            parser.error(f"Candidate is absent from discovery and reviewed target index is unavailable: {args.targets}")
        discovery = _fallback_discovery(target_index, args.function_id, oracle)
    context = build_context(discovery, args.function_id, oracle, asm=args.asm)
    if args.manifest.is_file():
        owner_review = _owner_recipe(json.loads(args.manifest.read_text(encoding="utf-8")),
                                     args.function_id, oracle=oracle)
        if owner_review is not None:
            # Canonical owner and recipe are sidecars; never overwrite discovery state.
            context["canonical_owner_review"] = owner_review
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(context, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"Wrote context for {args.function_id}: {len(context['callers'])} callers, {len(context['callees'])} callees")


if __name__ == "__main__":
    main()
