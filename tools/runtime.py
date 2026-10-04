"""Verify pinned independent historical library members, never source EXE bytes."""
from pathlib import Path

from common import require, sha
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
    require(not segment["use32"] and not segment["big"], "Unsupported runtime segment declaration")
    require(all(s["length"] == 0 for s in obj.segment_defs if s is not segment),
            "Runtime member carries unowned secondary storage")
    actual = obj.segments.get(segment["name"], b"")
    require(len(actual) == segment["length"] == recipe["size"] and
            obj.initialized_ranges.get(segment["name"]) == [(0, len(actual))],
            "Runtime member extent is incomplete or trimmed")
    require(not any(e["kind"] == "fixup" for r in obj.fixups for e in r["decoded"]),
            "Runtime member requires unsupported symbolic fixup proof")
    space = oracle.spaces[recipe["space"]]
    start = space.position(recipe["segment"], recipe["offset"])
    require(not any(start <= r["image_offset"] < start+len(actual) for r in space.relocations),
            "Original runtime extent carries ungenerated relocation obligations")
    expected = space.extent(recipe["segment"], recipe["offset"], len(actual))
    require(actual == expected and sha(expected) == recipe["target_sha256"],
            "Complete pinned runtime code differs from immutable original")
    require([p["name"] for p in obj.publics] == recipe["publics"], "Runtime public declarations changed")
    return {"kind": "PINNED_RUNTIME", "state": "PINNED_RUNTIME", "name": recipe["name"],
            "size": len(actual), "sha256": sha(actual), "member_sha256": sha(raw),
            "fixups": 0, "relocations": 0, "publics": obj.publics,
            "proof_scope": "Complete independent library member CODE with empty relocation obligations; natural RTLink placement unproved"}
