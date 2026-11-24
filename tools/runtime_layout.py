"""Linker-made values of the Microsoft runtime (docs/runtime-fixups.md "Linker-made values", 2026-11-24).

Some runtime fields name values no single library member stores: the linker defines `_end`/`_edata`, allocates the near
communal `__bufin`, lays out the common and empty DGROUP segments of crt0/crt0dat, and fills segment bases of empty
segments and the program start. Each is computed here from pinned library declarations plus original bytes outside the
field being judged; nothing is read back from that field. No function here owns bytes.
"""
import struct

from binding import fixes, fit_bounds, fits_once, reachable_from_entry
from common import require, sha

RUN_MODE = "dgroup-tail-run-v1"


def communal(spec, module, oracle):
    """Offset of a near communal: its declared length has one even place between two original direct accesses."""
    rows = [c for c in module.communals if c["name"] == spec["name"]]
    require(set(spec) == {"name", "offset", "member", "fit"} and set(spec["fit"]) == {"below", "above"} and
            len(rows) == 1 and rows[0]["kind"] == "near", "Unsupported runtime communal")
    raws = []
    for side in ("below", "above"):
        row = spec["fit"][side]
        require(set(row) == {"space", "image_offset", "bytes_hex", "entry"} and
                row["entry"].split(":")[0] == row["space"], "Unsupported communal neighbour")
        raw, at = bytes.fromhex(row["bytes_hex"]), row["image_offset"]
        require(type(at) is int and raw and oracle.spaces[row["space"]].data[at:at + len(raw)] == raw and
                at in reachable_from_entry(oracle, row["entry"]), "Communal neighbour is not reached original code")
        raws.append(raw)
    low, high = fit_bounds(*raws)
    require(type(spec["offset"]) is int and fits_once(spec["offset"], rows[0]["length"], low, high, True),
            "The communal fits in another place between its neighbours")
    return spec["offset"]


def linker_symbol(name, spec, ctx, manifest, judged_sha):
    """Value of `_end`/`_edata`, which the linker itself defines.

    `code`: another pinned member's complete code, located by its unique signature, whose every field naming the symbol
    stores value + A + D. `data`: the same for a data segment whose placement an already pinned member proves. `bss-start`
    (`_edata` only): the linker's start of the BSS class, the first word at or after the initialized DGROUP payload."""
    from runtime_fixups import _field_free, anchor_provider, read_member
    value, witness = spec["offset"], spec["witness"]
    require(name in ("_end", "_edata") and type(value) is int, "Unsupported linker symbol")
    if witness == {"kind": "bss-start"}:
        paragraph, home, base = ctx.dgroup
        require(name == "_edata" and value == (home.size - base + 1) & ~1, "_edata is not the start of the BSS class")
        return value
    if witness["kind"] == "code":
        member = witness["member"]
        module, overwritten = read_member(member)
        where = anchor_provider(member, ctx.frame, ctx.space, ctx.oracle, manifest, (0, 0))
        segment, original = member["object_segment"], ctx.space.data[where["position"]:]
    else:
        require(set(witness) == {"kind", "recipe", "segment"} and witness["kind"] == "data", "Unsupported linker witness")
        member, binding = _pinned(witness["recipe"], manifest)
        module, overwritten = read_member(member)
        segment = witness["segment"]
        paragraph, home, base = ctx.dgroup
        original = home.data[base + binding["placements"][segment]:]
    require(member["member_sha256"] != judged_sha, "A linker symbol is never proven by the judged member itself")
    values = set()
    for fix in fixes(module):
        if fix["segment"] != segment or fix["target"].get("name") != name:
            continue
        _field_free(fix, overwritten)
        require(fix["loc_type"] == 1 and not fix["self_relative"], "Unsupported linker symbol field")
        at = fix["segment_offset"]
        stored = struct.unpack_from("<H", module.segments[segment], at)[0] + (fix["displacement"] or 0)
        values.add((struct.unpack_from("<H", original, at)[0] - stored) & 0xFFFF)
    require(values == {value}, f"{name} is not the value the other member stores")
    return value


def _pinned(name, manifest):
    """(recipe, binding) of one PINNED_RUNTIME owner."""
    import json
    from common import project_path, read_json_shared
    found = [read_json_shared(project_path(o["recipe"])) for o in manifest["owners"] if o["kind"] == "PINNED_RUNTIME"]
    found = [r for r in found if r["name"] == name]
    require(len(found) == 1 and "binding" in found[0], f"Runtime recipe {name} is not pinned with a binding")
    raw = project_path(found[0]["binding"]).read_bytes()
    require(sha(raw) == found[0]["binding_sha256"], "Pinned runtime binding changed")
    return found[0], json.loads(raw)


def segment_base(spec, ctx):
    """Paragraph of an empty segment: original relocated words elsewhere store it (as for a far segment base)."""
    from far_binding import _ground_segment
    require(set(spec) == {"segment", "witnesses"}, "Unsupported empty segment base")
    for w in spec["witnesses"]:
        require(not any(space == w["space"] and lo - 1 <= w["image_offset"] < hi for space, lo, hi in ctx.judged),
                "Empty segment base witness lies inside the judged member")
    _ground_segment(spec, {"space": "root", "segment": 0, "offset": 0, "size": 0}, ctx.oracle)
    return spec["segment"]


def tail_run(path, digest, ctx, manifest):
    """The DATA-class segments behind `_DATA` that crt0 and crt0dat declare: XIQC, DBDATA, CDATA, XIFB..XIE, XPB..XCFE.

    The linker lays segments out in the order of their first SEGDEF and packs each at its word alignment; a common
    segment is as long as its longest contribution, a public one the sum of them. The listed members in link order give
    the order and the lengths, so one start fixes every placement, also of empty segments. Proven here: the linked bytes
    of the whole run (fields computed by the linker equations) equal the original DGROUP bytes, with exactly the
    generated relocations; they occur once in the payload; an accepted group's placed segment ends at the start and
    another begins at the end, so nothing else lies inside."""
    from common import ROOT, project_path, read_json_shared
    from runtime_fixups import _addend, _code_frame, _dgroup_members, _field_free, anchor_provider, read_member
    source = project_path(path)
    raw = source.read_bytes()
    require(source.is_relative_to(ROOT / "evidence/bindings") and sha(raw) == digest and b"\r" not in raw,
            "Tail run evidence changed, escapes evidence cone or is not LF")
    run = read_json_shared(source)
    require(run.get("schema") == 1 and run.get("mode") == RUN_MODE and run.get("target_sha256") == sha(ctx.oracle.data) and
            ctx.dgroup is not None and run.get("dgroup_paragraph") == ctx.dgroup[0], "Unsupported/foreign tail run evidence")
    paragraph, home, base = ctx.dgroup
    members = [read_member(spec) for spec in run["members"]]
    names, declared = [], {}
    for module, _ in members:
        for seg in module.segment_defs:
            if seg["class"] != "DATA" or seg["name"] == "_DATA":
                continue
            require(seg["name"] in _dgroup_members(module) and seg["combine"] in (2, 6) and seg["alignment"] == 2 and
                    not seg["use32"], f"Tail run segment {seg['name']} is not a word-aligned DGROUP segment")
            if seg["name"] not in declared:
                names.append(seg["name"])
            require(declared.setdefault(seg["name"], seg["combine"]) == seg["combine"], "Tail run combine types disagree")
    require(names == run["segments"], "Tail run order is not the SEGDEF order of its members")
    anchors = {}
    for spec in run["code_anchors"]:
        anchors[spec["member_sha256"]] = (spec, anchor_provider(spec, ctx.frame, ctx.space, ctx.oracle, manifest, (0, 0)))
    publics = {n: p for _, a in anchors.values() for n, p in a["publics"].items()}
    bases = {}
    tiles, equations, cursor, image = {}, {}, run["start"], bytearray()
    generated, used = [], set()
    require(type(cursor) is int and cursor % 2 == 0, "Tail run start is not word-aligned")
    for name in names:
        cursor += cursor % 2
        image += bytes(cursor - run["start"] - len(image))
        common, start, end = declared[name] == 6, cursor, cursor
        for spec, (module, overwritten) in zip(run["members"], members):
            for seg in (s for s in module.segment_defs if s["name"] == name):
                at = start if common else end
                require(common or at % 2 == 0, "Public tail run contribution is not word-aligned")
                content = bytearray(module.segments.get(name) or bytes(seg["length"]))
                require(len(content) == seg["length"], "Incomplete tail run contribution")
                rows = []
                for fix in (f for f in fixes(module) if f["segment"] == name):
                    _field_free(fix, overwritten)
                    q, width, target, frame = fix["segment_offset"], fix["field_width"], fix["target"], fix["frame"]
                    addend, disp = _addend(content, q, width), fix["displacement"] or 0
                    require(not fix["self_relative"], "Unsupported self-relative tail run field")
                    if fix["loc_type"] == 1 and target["kind"] == "segment" and \
                            target["name"] == "_TEXT" and _code_frame(frame, module, "_TEXT", "_TEXT"):
                        require(spec["member_sha256"] in anchors, "Tail run code offset needs its member's code anchor")
                        value = anchors[spec["member_sha256"]][1]["offset"] + disp + addend
                        used.add(spec["member_sha256"])
                    elif fix["loc_type"] == 3 and target["kind"] == "external" and addend == disp == 0 and \
                            target["name"] in publics and _code_frame(frame, module, target["name"]):
                        value = (ctx.window(publics[target["name"]], 1)) | ctx.frame << 16
                        used.update(k for k, (_, a) in anchors.items() if target["name"] in a["publics"])
                        generated.append(base + at + q + 2)
                    elif fix["loc_type"] == 2 and target["kind"] == "segment" and addend == 0 and \
                            fix["displacement"] is None and {k: frame.get(k) for k in ("kind", "name")} == \
                            {"kind": "segment", "name": target["name"]} and target["name"] in run["segment_bases"]:
                        empty = next(s for s in module.segment_defs if s["name"] == target["name"])
                        require(empty["length"] == 0 and empty["class"] == "ENDCODE" and
                                target["name"] not in _dgroup_members(module), "Tail run base is not an empty ENDCODE segment")
                        spec_base = run["segment_bases"][target["name"]]
                        # The end of code: the linker's first paragraph behind every code area, where the
                        # preloaded data overlay begins.
                        value = segment_base(spec_base, ctx)
                        require(value == home.link_segment and not any(
                            w["space"] == home.name and base + run["start"] - 1 <= w["image_offset"] < base + run["end"]
                            for w in spec_base["witnesses"]), "ENDCODE is not the paragraph where the data overlay begins")
                        bases[target["name"]] = value
                        generated.append(base + at + q)
                    else:
                        raise ValueError(f"Unsupported tail run field in {name}: {target}")
                    require(0 <= value < 1 << 8 * width, "Tail run value leaves its field")
                    content[q:q + width] = value.to_bytes(width, "little")
                    observed = _addend(home.data, base + at + q, width)
                    rows.append({"segment": name, "offset": q, "kind": "tail run", "linked": value, "original": observed,
                                 "equal": value == observed})
                tiles[(spec["member_sha256"], name)] = at
                equations[(spec["member_sha256"], name)] = rows
                lo = at - run["start"]
                image += bytes(max(0, lo + len(content) - len(image)))
                for i, b in enumerate(content):
                    require(not (common and image[lo + i] and b and image[lo + i] != b), "Common contributions disagree")
                    image[lo + i] = image[lo + i] or b
                end = max(end, at + seg["length"]) if common else at + seg["length"]
        cursor = end
    require(cursor == run["end"] and home.data[base + run["start"]:base + run["end"]] == bytes(image),
            "Tail run differs from the original DGROUP bytes")
    require(home.data.count(bytes(image), base) == 1, "Tail run content occurs more than once")
    require(sorted(generated) == sorted(r["image_offset"] for r in home.relocations
                                        if base + run["start"] - 1 <= r["image_offset"] < base + run["end"]),
            "Tail run relocations differ from its generated words")
    require(used == set(anchors) and set(bases) == set(run["segment_bases"]), "Unused tail run evidence")
    below, above = (_neighbour(run[side], manifest) for side in ("below", "above"))
    require(below[1] == run["start"] and above[0] == run["end"], "Tail run is not bracketed by placed segments")
    receipt = {"path": path, "class": "DATA tail", "start": run["start"], "end": run["end"], "entries":
               [{"segment": n, "placement": p, "member_sha256": m} for (m, n), p in tiles.items()]}
    return tiles, equations, receipt


def _neighbour(spec, manifest):
    """[lo, hi) of a placed segment of an accepted group."""
    import json
    from common import project_path, read_json_shared
    require(set(spec) == {"group", "segment"}, "Unsupported tail run neighbour")
    found = {o["recipe"] for o in manifest["owners"] if read_json_shared(project_path(o["recipe"])).get("group_id") == spec["group"]}
    require(len(found) == 1, "Tail run neighbour is not one accepted group")
    recipe = read_json_shared(project_path(found.pop()))
    raw = project_path(recipe["binding"]).read_bytes()
    require(sha(raw) == recipe["binding_sha256"], "Tail run neighbour binding changed")
    binding = json.loads(raw)
    lo = binding["placements"][spec["segment"]]
    return lo, lo + next(s["length"] for s in binding["declarations"]["segments"] if s["name"] == spec["segment"])


def empty_consensus(spec, module, dgroup_names, ctx):
    """Empty BSS marker segments (XOB, XO, XOE) that the original places at one address.

    Only empty own segments lie between them, so with no contribution elsewhere they share one address; the two fields
    that read it agree (`prove` requires a field for each). A contribution of another member would separate them."""
    rows = [s for s in module.segment_defs if s["name"] in spec]
    indices = [i for i, s in enumerate(module.segment_defs) if s["name"] in spec]
    paragraph, home, base = ctx.dgroup
    require(len(spec) >= 2 and len(rows) == len(spec) and len(set(spec.values())) == 1 and
            all(s["length"] == 0 and s["class"] == "BSS" and s["combine"] == 2 and s["name"] in dgroup_names for s in rows) and
            all(s["length"] == 0 and s["class"] == "BSS" for s in module.segment_defs[min(indices):max(indices) + 1]),
            "Empty segment consensus needs adjacent empty BSS segments at one address")
    value = next(iter(spec.values()))
    require(type(value) is int and value % 2 == 0 and value >= home.size - base, "Empty BSS segments lie in initialized data")
    return dict(spec)


def second_code(spec, name, module, overwritten, ctx, manifest, judged_end):
    """A small paragraph-aligned second CODE segment of the member (crt0dat's EMULATOR_TEXT), placed and not owned.

    It starts at the first paragraph after the last owned code, behind zero padding; its complete linked bytes equal the
    original, with exactly its relocations, and no other root relocation stores such a word."""
    from runtime_fixups import _addend, _segment
    seg = _segment(module, name)
    content = bytearray(module.segments.get(name, b""))
    at = spec["image_offset"]
    require(set(spec) == {"image_offset"} and type(at) is int and seg["class"] == "CODE" and seg["alignment"] == 3 and
            len(content) == seg["length"] > 0 and module.initialized_ranges.get(name) == [(0, len(content))] and
            at >= judged_end, "Second code segment is not complete and paragraph-aligned")
    root = ctx.space
    spans = [(root.position(o["segment"], o["offset"]), o["size"]) for o in manifest["owners"] if o["space"] == "root"]
    last = max(lo + size for lo, size in spans if lo + size <= at)
    require(at == -(-last // 16) * 16 and not any(root.data[last:at]) and
            not any(lo < at + len(content) and at < lo + size for lo, size in spans),
            "Second code segment does not follow the last owned code")
    equations, generated = [], []
    for fix in (f for f in fixes(module) if f["segment"] == name):
        q, target, frame = fix["segment_offset"], fix["target"], fix["frame"]
        require(fix["loc_type"] == 2 and fix["displacement"] is None and target["kind"] == "segment" and
                target["name"] in ctx.bases and frame.get("kind") == "segment" and frame.get("name") == target["name"] and
                _addend(content, q, 2) == 0, "Unsupported field in the second code segment")
        value = ctx.bases[target["name"]]
        ctx.used.add(("base", target["name"]))
        require(sum(_addend(root.data, r["image_offset"], 2) == value for r in root.relocations) == 1,
                "Another root relocation stores that word")
        content[q:q + 2] = value.to_bytes(2, "little")
        observed = _addend(root.data, at + q, 2)
        equations.append({"segment": name, "offset": q, "kind": "empty segment base", "linked": value, "original": observed,
                          "equal": value == observed})
        generated.append(at + q)
    require(root.data[at:at + len(content)] == bytes(content) and equations and
            sorted(generated) == sorted(r["image_offset"] for r in root.relocations if at - 1 <= r["image_offset"] < at + len(content)),
            "Second code segment differs from the original")
    return {"segment": name, "position": at, "length": len(content), "equations": equations, "space": "root"}


def start_entry(module, code_name, spec, origin, span, ctx):
    """The program start of a main module: an original relocated far pointer outside the member names exactly it."""
    body = bytes.fromhex(module.start_address)
    index = next(i for i, s in enumerate(module.segment_defs, 1) if s["name"] == code_name)
    require(len(body) == 5 and body[0] == 0 and body[1] == body[2] == index and set(spec) == {"image_offset", "relocation_index"},
            "Unsupported program start address")
    entry, at = origin + struct.unpack_from("<H", body, 3)[0], spec["image_offset"]
    relocs = [(r["image_offset"], r["index"]) for r in ctx.space.relocations if at <= r["image_offset"] < at + 4]
    require(type(at) is int and not (span[0] - 4 < at < span[1]) and relocs == [(at + 2, spec["relocation_index"])] and
            struct.unpack_from("<HH", ctx.space.data, at) == (entry, ctx.frame),
            "Program start has no original far pointer")
    return {"entry": entry, "witness": at}
