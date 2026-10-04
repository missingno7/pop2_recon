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


def build_context(discovery, function_id, oracle, *, asm=False):
    target_sha = hashlib.sha256(oracle.data).hexdigest()
    if discovery.get("target_sha256") != target_sha:
        raise ValueError("Discovery target SHA-256 does not match the current immutable Oracle")
    functions = discovery.get("functions", [])
    by_id = {row["id"]: row for row in functions}
    if function_id not in by_id:
        raise KeyError(f"Unknown function candidate: {function_id}")
    row = by_id[function_id]
    related = sorted(set(row.get("callers", []) + row.get("callees", [])))
    packet = {
        "schema": 1,
        "function": row,
        "callers": [by_id[key] for key in row.get("callers", []) if key in by_id],
        "callees": [by_id[key] for key in row.get("callees", []) if key in by_id],
        "related_candidates": related,
        "space": next((s for s in discovery.get("spaces", []) if s["name"] == row["space"]), None),
        "interpretation": "Discovery hypotheses with tentative extents; verify against immutable bytes before naming or matching.",
        "target_sha256": target_sha,
    }
    if asm:
        space = oracle.spaces[row["space"]]
        start = space.position(row["segment"], row["offset"])
        extent = row.get("extent", {})
        end = extent.get("end_exclusive")
        if end is None or end <= start:
            end = min(space.size, start + 64)
        else:
            end = min(space.size, end)
        decoded = []
        pos = start
        while pos < end and len(decoded) < 128:
            ins = decode_one(space.data, pos, origin=row["offset"] - start)
            if ins is None or not ins.size:
                break
            canonical_address = space.address(pos)
            decoded.append({"address": {"space": row["space"], "segment": row["segment"],
                                        "offset": (row["offset"] + pos - start) & 0xffff},
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
    target = next((item for item in target_index.get("functions", [])
                   if item.get("id") == function_id), None)
    if target is None:
        raise KeyError(f"No discovery file and no reviewed fallback target for {function_id}")
    try:
        space_name, segment_text, offset_text = function_id.rsplit(":", 2)
        segment, offset = int(segment_text, 16), int(offset_text, 16)
        space = oracle.spaces[space_name]
        position = space.position(segment, offset)
    except (ValueError, KeyError) as exc:
        raise ValueError(f"Invalid or out-of-range reviewed fallback target: {function_id}") from exc
    if (target.get("space"), target.get("segment"), target.get("offset")) != (space_name, segment, offset):
        raise ValueError(f"Reviewed fallback target address does not match its ID: {function_id}")
    size = int(target.get("size", 0))
    if size <= 0 or position + size > space.size:
        raise ValueError(f"Reviewed fallback extent is outside Oracle space: {function_id}")
    actual_sha = hashlib.sha256(space.extent(segment, offset, size)).hexdigest()
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


def _owner_recipe(manifest, function_id, *, root=ROOT):
    owner = next((item for item in manifest.get("owners", [])
                  if item.get("target_id") == function_id), None)
    if owner is None:
        return None
    recipe_path = (root / owner["recipe"]).resolve()
    if not recipe_path.is_relative_to(root.resolve()):
        raise ValueError("Owner recipe path escapes the project")
    recipe = json.loads(recipe_path.read_text(encoding="utf-8"))
    if recipe.get("target_id") != function_id:
        raise ValueError(f"Owner recipe target mismatch for {function_id}")
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
    if not any(item.get("id") == args.function_id for item in discovery.get("functions", [])):
        if target_index is None:
            parser.error(f"Candidate is absent from discovery and reviewed target index is unavailable: {args.targets}")
        discovery = _fallback_discovery(target_index, args.function_id, oracle)
    context = build_context(discovery, args.function_id, oracle, asm=args.asm)
    if args.manifest.is_file():
        owner_review = _owner_recipe(json.loads(args.manifest.read_text(encoding="utf-8")),
                                     args.function_id)
        if owner_review is not None:
            # Canonical owner and recipe are sidecars; never overwrite discovery state.
            context["canonical_owner_review"] = owner_review
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(context, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"Wrote context for {args.function_id}: {len(context['callers'])} callers, {len(context['callees'])} callees")


if __name__ == "__main__":
    main()
