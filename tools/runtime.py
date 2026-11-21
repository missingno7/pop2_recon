"""Verify pinned independent historical library members, never source EXE bytes."""
from pathlib import Path

from common import ROOT, require, sha, read_json, project_path
from omf import OmfReader


def verify_member(recipe, oracle, manifest=None):
    library = Path(recipe["library_path"]).read_bytes()
    require(sha(library) == recipe["library_sha256"], "Pinned runtime library identity changed")
    at, extent = recipe["archive_offset"], recipe["member_extent"]
    require(0 <= at and at+extent <= len(library), "Runtime member outside pinned archive")
    raw = library[at:at+extent]
    require(sha(raw) == recipe["member_sha256"], "Pinned runtime member identity changed")
    reader = OmfReader(overwrite_ledata=True, library_records=True)
    obj = reader.read(raw, recipe["name"])
    # A member owns one CODE segment, or (RTLink manager library) one `$$` manager segment.
    segment_class = recipe.get("segment_class", "CODE")
    code_defs = [s for s in obj.segment_defs if s["class"] == segment_class]
    require(len(code_defs) == 1 and code_defs[0]["name"] == recipe["object_segment"],
            "Runtime pin must own a complete single CODE segment")
    segment = code_defs[0]
    manager = segment_class != "CODE"
    if manager:
        from rtlink_fixups import check_segment
        check_segment(recipe, segment, obj.initialized_ranges.get(segment["name"]))
    require(not segment["use32"] and not segment["big"] and segment["alignment"] != 0,
            "Unsupported runtime segment declaration")
    binding = None
    if "binding" in recipe:
        path = project_path(recipe["binding"])
        require(path.is_relative_to(project_path("evidence/bindings")) and
                sha(path.read_bytes()) == recipe["binding_sha256"], "Runtime binding evidence changed or escapes evidence cone")
        binding = read_json(path)
    from runtime_fixups import MODE
    from rtlink_fixups import MODE as RTLINK_MODE
    mode = binding.get("mode") if binding is not None else None
    require(mode != RTLINK_MODE or manager, "RTLink manager binding for a non-manager member")
    require(not manager or binding is None or mode == RTLINK_MODE, "RTLink manager member needs its own binding mode")
    symbolic = mode in (MODE, RTLINK_MODE)
    # Secondary storage is either absent or (symbolic mode) placed and compared there.
    require(symbolic or all(s["length"] == 0 for s in obj.segment_defs if s is not segment),
            "Runtime member carries unowned secondary storage")
    require(symbolic or not (getattr(obj, "communals", []) or getattr(obj, "start_address", None) or
                             any(p.get("local") for p in obj.publics)),
            "Runtime member carries unsupported library records")
    actual = obj.segments.get(segment["name"], b"")
    require(len(actual) == segment["length"] == recipe["size"] and
            (manager or obj.initialized_ranges.get(segment["name"]) == [(0, len(actual))]),
            "Runtime member extent is incomplete or trimmed")
    actual_fixes = [e for r in obj.fixups for e in r["decoded"] if e["kind"] == "fixup"]
    # MASM back-patching (a later LEDATA overwriting bytes) is accepted for fixup-free
    # members, where LINK's last-write-wins leaves no ambiguity; the symbolic mode also
    # accepts it when no FIXUPP field lies in an overwritten range.
    require(not reader.overwritten_bytes or not actual_fixes or symbolic,
            "Overwritten LEDATA in a fixup-bearing runtime member is not supported")
    require(not actual_fixes or binding is not None,
            "Runtime member requires unsupported symbolic fixup proof")
    require(actual_fixes or binding is None, "Unused runtime binding proof")
    space = oracle.spaces[recipe["space"]]
    start = space.position(recipe["segment"], recipe["offset"])
    expected = space.extent(recipe["segment"], recipe["offset"], len(actual))
    require(sha(expected) == recipe["target_sha256"], "Complete pinned runtime code differs from immutable original")
    bound = None
    if manager:
        from rtlink_fixups import check_alignment
        check_alignment(segment, start)
    if symbolic:
        from runtime_fixups import prove
        if mode == RTLINK_MODE:
            from rtlink_fixups import prove
        if manifest is None:
            manifest = read_json(ROOT/"layout/manifest.json")
        bound = prove(obj, reader.overwritten_ranges, segment["name"], actual, expected, binding,
                      recipe, oracle, manifest)
        require(bound["exact"], "Complete pinned runtime bytes/fixup equations differ from immutable original")
        require(recipe.get("relocation_indices", []) == bound["relocation_indices"],
                "Runtime recipe relocation indices differ from the fresh proof")
    else:
        require(not any(start-1 <= r["image_offset"] < start+len(actual) for r in space.relocations),
                "Original runtime extent carries ungenerated relocation obligations")
    if binding is not None and not symbolic:
        from near_binding import compare
        bound = compare(obj, segment["name"], actual, expected, binding, recipe, oracle)
        require(bound["exact"], "Complete pinned runtime bytes/near equations differ from immutable original")
        require(len(obj.publics) == 1 and obj.publics[0]["segment"] == segment["name"] and
                obj.publics[0]["offset"] == 0 and obj.publics[0]["group_index"] == 0,
                "Bound runtime public must own the whole code segment at zero")
    elif binding is None:
        require(actual == expected, "Complete pinned runtime code differs from immutable original")
    require([p["name"] for p in obj.publics] == recipe["publics"], "Runtime public declarations changed")
    report = {"kind": "PINNED_RUNTIME", "state": "PINNED_RUNTIME", "name": recipe["name"],
            "size": len(actual), "sha256": sha(expected), "raw_code_sha256": sha(actual), "member_sha256": sha(raw),
            "fixups": len(actual_fixes), "relocations": 0, "publics": obj.publics,
            "proof_scope": "Complete independent library member CODE with empty relocation obligations; natural RTLink placement unproved"}
    if manager:
        report["proof_scope"] = ("Complete independent RTLink manager member segment "
                                 f"{segment['name']}; natural RTLink placement and behavior unproved")
    if bound:
        report.update(bound)
    return report
