"""Find whole relocation-free library CODE matches for independent review.

This diagnostic never publishes ownership or supplies acceptance fixtures. It
reads every member through the strict OMF reader, excludes secondary storage,
holes and actual fixups, and compares entire CODE segments literally.
"""
import argparse
import struct
from pathlib import Path

from binding import declarations, fixes
from common import ROOT, project_path, read_json, require, sha, write_json
from omf import OmfReader
from oracle import Oracle


def archive_members(data):
    """Read page-aligned THEADR..MODEND members before the LIB dictionary."""
    require(len(data) >= 10 and data[0] == 0xf0, "Expected a 16-bit OMF LIB header")
    page_size = struct.unpack_from("<H", data, 1)[0] + 3
    dictionary = struct.unpack_from("<I", data, 3)[0]
    require(page_size >= 16 and page_size & (page_size-1) == 0 and
            page_size <= dictionary <= len(data), "Invalid LIB page/dictionary bounds")
    cursor = page_size
    while cursor < dictionary:
        if data[cursor] == 0xf1:
            break
        if data[cursor] != 0x80:
            cursor += page_size
            continue
        end = cursor
        complete = False
        while end+3 <= dictionary:
            kind = data[end]
            length = struct.unpack_from("<H", data, end+1)[0]
            if length < 1 or end+3+length > dictionary:
                break
            end += 3+length
            if kind in (0x8a, 0x8b):
                complete = True
                break
        if complete:
            yield cursor, data[cursor:end]
            cursor = ((end+page_size-1)//page_size)*page_size
        else:
            cursor += page_size


def whole_code(module, minimum_size):
    code = [s for s in module.segment_defs if s["class"] == "CODE"]
    if len(code) != 1:
        return None
    segment = code[0]
    raw = module.segments.get(segment["name"], b"")
    if (segment["use32"] or segment["big"] or segment["alignment"] == 0 or
        len(raw) < minimum_size or len(raw) != segment["length"] or
        module.initialized_ranges.get(segment["name"]) != [(0, len(raw))] or
        any(s["length"] for s in module.segment_defs if s is not segment) or fixes(module) or
        len(module.publics) != 1 or module.publics[0]["segment"] != segment["name"] or
        module.publics[0]["offset"] != 0 or module.publics[0]["group_index"] != 0):
        return None
    return segment, raw


def scan_archive(path, oracle, owners=(), *, minimum_size=8):
    require(minimum_size > 0, "Minimum CODE size must be positive")
    path = Path(path)
    data = path.read_bytes()
    spans = []
    for owner in owners:
        space = oracle.spaces[owner["space"]]
        start = space.position(owner["segment"], owner["offset"])
        spans.append((owner["space"], start, start+owner["size"], owner["target_id"]))
    matches, rejected_omf, examined, shapes = [], 0, 0, 0
    for offset, member in archive_members(data):
        examined += 1
        try:
            module = OmfReader().read(member)
        except ValueError:
            rejected_omf += 1
            continue
        shape = whole_code(module, minimum_size)
        if shape is None:
            continue
        shapes += 1
        segment, code = shape
        hits = []
        for space in oracle.spaces.values():
            at = -1
            while True:
                at = space.data.find(code, at+1)
                if at < 0:
                    break
                hits.append({"space": space.name, "image_offset": at,
                    "file_offset": space.file_offset+at, "size": len(code),
                    "original_sha256": sha(space.data[at:at+len(code)]),
                    "original_relocations": [r for r in space.relocations
                                             if at-1 <= r["image_offset"] < at+len(code)],
                    "overlap_owners": [identifier for name, left, right, identifier in spans
                                       if name == space.name and at < right and left < at+len(code)]})
        if hits:
            matches.append({"module": module.name, "archive_offset": offset,
                "member_extent": len(member), "member_sha256": sha(member),
                "object_segment": segment["name"], "code_size": len(code),
                "raw_code_sha256": sha(code), "declarations": declarations(module), "hits": hits})
    return {"library_path": str(path.resolve()), "library_size": len(data),
        "library_sha256": sha(data), "members_examined": examined,
        "strict_omf_refusals": rejected_omf, "whole_zero_fixup_shapes": shapes,
        "review_candidates": matches}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("libraries", nargs="+", type=Path)
    parser.add_argument("--output", required=True)
    parser.add_argument("--minimum-code-size", type=int, default=8)
    args = parser.parse_args()
    output = project_path(args.output)
    require(output.is_relative_to(ROOT/"build/workers"), "Diagnostic output must be worker-local")
    oracle = Oracle.load()
    owners = read_json(ROOT/"layout/manifest.json")["owners"]
    libraries = [scan_archive(path, oracle, owners, minimum_size=args.minimum_code_size)
                 for path in args.libraries]
    write_json(output, {"schema": 1, "target_sha256": sha(oracle.data),
        "scope": "Diagnostic whole literal CODE matches only; boundary/ABI/caller review and fresh pin_runtime verification required. No ownership or acceptance cache.",
        "libraries": libraries})
    print(f"Wrote {sum(len(lib['review_candidates']) for lib in libraries)} whole-member research matches to {args.output}")


if __name__ == "__main__":
    main()
