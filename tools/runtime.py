"""Verify pinned independent historical library members, never source EXE bytes."""
from pathlib import Path

from common import require, sha, read_json, project_path
from omf import OmfReader


def verify_member(recipe, oracle):
    library = Path(recipe["library_path"]).read_bytes()
    require(sha(library) == recipe["library_sha256"], "Pinned runtime library identity changed")
    at, extent = recipe["archive_offset"], recipe["member_extent"]
    require(0 <= at and at+extent <= len(library), "Runtime member outside pinned archive")
    raw = library[at:at+extent]
    require(sha(raw) == recipe["member_sha256"], "Pinned runtime member identity changed")
    obj = OmfReader().read(raw, recipe["name"])
    code_defs = [s for s in obj.segment_defs if s["class"] == "CODE"]
    require(len(code_defs) == 1 and code_defs[0]["name"] == recipe["object_segment"],
            "Runtime pin must own a complete single CODE segment")
    segment = code_defs[0]
    require(not segment["use32"] and not segment["big"] and segment["alignment"] != 0,
            "Unsupported runtime segment declaration")
    require(all(s["length"] == 0 for s in obj.segment_defs if s is not segment),
            "Runtime member carries unowned secondary storage")
    actual = obj.segments.get(segment["name"], b"")
    require(len(actual) == segment["length"] == recipe["size"] and
            obj.initialized_ranges.get(segment["name"]) == [(0, len(actual))],
            "Runtime member extent is incomplete or trimmed")
    actual_fixes = [e for r in obj.fixups for e in r["decoded"] if e["kind"] == "fixup"]
    require(not actual_fixes or "binding" in recipe,
            "Runtime member requires unsupported symbolic fixup proof")
    require(actual_fixes or "binding" not in recipe, "Unused runtime binding proof")
    space = oracle.spaces[recipe["space"]]
    start = space.position(recipe["segment"], recipe["offset"])
    require(not any(start-1 <= r["image_offset"] < start+len(actual) for r in space.relocations),
            "Original runtime extent carries ungenerated relocation obligations")
    expected = space.extent(recipe["segment"], recipe["offset"], len(actual))
    require(sha(expected) == recipe["target_sha256"], "Complete pinned runtime code differs from immutable original")
    bound = None
    if "binding" in recipe:
        path = project_path(recipe["binding"])
        require(path.is_relative_to(project_path("evidence/bindings")) and
                sha(path.read_bytes()) == recipe["binding_sha256"], "Runtime binding evidence changed or escapes evidence cone")
        from near_binding import compare
        bound = compare(obj, segment["name"], actual, expected, read_json(path), recipe, oracle)
        require(bound["exact"], "Complete pinned runtime bytes/near equations differ from immutable original")
        require(len(obj.publics) == 1 and obj.publics[0]["segment"] == segment["name"] and
                obj.publics[0]["offset"] == 0 and obj.publics[0]["group_index"] == 0,
                "Bound runtime public must own the whole code segment at zero")
    else:
        require(actual == expected, "Complete pinned runtime code differs from immutable original")
    require([p["name"] for p in obj.publics] == recipe["publics"], "Runtime public declarations changed")
    report = {"kind": "PINNED_RUNTIME", "state": "PINNED_RUNTIME", "name": recipe["name"],
            "size": len(actual), "sha256": sha(expected), "raw_code_sha256": sha(actual), "member_sha256": sha(raw),
            "fixups": len(actual_fixes), "relocations": 0, "publics": obj.publics,
            "proof_scope": "Complete independent library member CODE with empty relocation obligations; natural RTLink placement unproved"}
    if bound:
        report.update(bound)
    return report
