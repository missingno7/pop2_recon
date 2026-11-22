"""Symbolic fixup proof for whole pinned runtime members (mode runtime-symbolic-v1).

Every FIXUPP field of a pinned library member is computed by the linker equation for
its location, frame and target, from values grounded independently of the field:

* the runtime CODE frame paragraph: original relocated CALL/JMP FAR encodings outside
  the member whose segment word is that frame and whose offset lands exactly on a public
  of another PINNED_RUNTIME member;
* code targets: publics of PINNED_RUNTIME members already in the manifest (their whole
  code is itself proven there), or the member's own publics;
* the DGROUP paragraph: original relocated words storing it (as for placed group data);
* DGROUP data: complete placed segments, whose bytes (with their own fixup fields
  computed the same way) equal the original DGROUP bytes and whose relocated words are
  exactly the generated base fields. The member's own non-empty DGROUP segments must
  all be placed; data publics of other library members come from placed provider
  segments (zero owned bytes);
* absolute publics: the provider member's own absolute PUBDEF (frame 0);
* signature-anchored code providers (zero owned bytes): a whole library CODE segment
  whose ordinary bytes (at least 16) match at exactly one position of the grounded
  CODE frame window, whose own-segment fields satisfy their equations there, and which
  overlaps neither the judged member nor any owner other than the pinned recipe of the
  same member at the same position. Its external fields stay its own open obligations;
* class runs: an evidence file lists an ordered, gap-free run of DGROUP contributions
  of one class (public or common combine, fixup-free, complete), each compared
  literally, ending exactly at the original DGROUP payload end. It grounds placements
  of common-combine segments (PAD/EPAD) that no field references;
* same-library selection: a public declared by several pinned recipes binds only when
  the binding opts in for that symbol and exactly one declaring recipe comes from the
  judged member's own archive (the named one);
* accepted external code providers: a CODE_EXACT/ASM_EXACT owner's claimed entry whose
  public matches the external, with its frame taken from a reviewed relocated CALL FAR
  or far pointer of that target outside the judged member.

Ordinary bytes compare literally; no field is masked, derived from the original operand
or selected from a prefix. The original relocations inside every compared extent must be
exactly the generated segment fields, in FIXUPP order per target. Anything else refuses.
"""
import struct
from pathlib import Path

from binding import fixes
from common import require, sha
from omf import OmfReader

MODE = "runtime-symbolic-v1"
_MEMBERS = {}


def read_member(spec):
    """An independently pinned library member (archive and member hashes)."""
    key = (spec["library_path"], spec["archive_offset"], spec["member_extent"], spec["member_sha256"])
    if key not in _MEMBERS:
        data = Path(spec["library_path"]).read_bytes()
        require(sha(data) == spec["library_sha256"], "Runtime provider library identity changed")
        at, size = spec["archive_offset"], spec["member_extent"]
        require(type(at) is int and type(size) is int and 0 <= at and size > 0 and at+size <= len(data),
                "Runtime provider member escapes archive")
        raw = data[at:at+size]
        require(sha(raw) == spec["member_sha256"], "Runtime provider member identity changed")
        reader = OmfReader(overwrite_ledata=True, library_records=True)
        _MEMBERS[key] = (reader.read(raw), list(reader.overwritten_ranges))
    return _MEMBERS[key]


def _segment(module, name):
    rows = [s for s in module.segment_defs if s["name"] == name]
    require(len(rows) == 1, f"Runtime member lacks segment {name}")
    return rows[0]


def _dgroup_members(module):
    names = set()
    for group in module.groups:
        if group["name"] == "DGROUP":
            names.update(module.segment_defs[i-1]["name"] for i in group["segments"])
    return names


def _addend(raw, at, width):
    return int.from_bytes(raw[at:at+width], "little")


def code_providers(manifest, oracle, exclude=None):
    """symbol -> (recipe name, frame-free linear position) over manifest PINNED_RUNTIME owners."""
    from common import project_path, read_json
    table = {}
    for owner in manifest["owners"]:
        if owner["kind"] != "PINNED_RUNTIME" or owner["target_id"] == exclude:
            continue
        recipe = read_json(project_path(owner["recipe"]))
        module, _ = read_member(recipe)
        start = oracle.spaces[recipe["space"]].position(recipe["segment"], recipe["offset"])
        for public in module.publics:
            if public["segment"] == recipe["object_segment"] and public["group_index"] == 0:
                table.setdefault(public["name"], []).append((recipe["name"], start+public["offset"]))
    return table


def _ground_frame(spec, providers, space, owner):
    """The CODE frame paragraph of the runtime _TEXT segment."""
    frame = spec["segment"]
    require(type(frame) is int and 0 <= frame <= 0xffff and spec.get("witnesses"),
            "Runtime CODE frame needs a paragraph and independent far-call witnesses")
    receipts = []
    for w in spec["witnesses"]:
        at = w["image_offset"]
        require(type(at) is int and 0 <= at <= space.size-5 and space.data[at] in (0x9a, 0xea),
                "Runtime frame witness is not an original CALL/JMP FAR")
        relocs = [r for r in space.relocations if at-1 <= r["image_offset"] < at+5]
        require([(r["image_offset"], r["index"]) for r in relocs] == [(at+3, w["relocation_index"])],
                "Runtime frame witness lacks its exact segment relocation")
        offset, segment = struct.unpack_from("<HH", space.data, at+1)
        require(not (owner[0]-5 < at < owner[1]), "Runtime frame witness lies inside the judged member")
        hits = [p for name, p in providers.get(w["symbol"], []) if name == w["provider"]]
        require(segment == frame and len(providers.get(w["symbol"], [])) == 1 and hits and
                hits[0] == frame*16+offset,
                "Runtime frame witness does not reach a unique pinned provider public in that frame")
        receipts.append({"image_offset": at, "relocation_index": w["relocation_index"],
                         "provider": w["provider"], "symbol": w["symbol"]})
    return frame, receipts


def _ground_dgroup(spec, oracle):
    from far_binding import _ground_segment
    _ground_segment({"segment": spec["segment"], "witnesses": spec["witnesses"]},
                    {"space": "root", "segment": 0, "offset": 0, "size": 0}, oracle)
    homes = [s for s in oracle.spaces.values() if 0 <= (spec["segment"]-s.link_segment)*16 < s.size]
    require(len(homes) == 1, "DGROUP paragraph does not lie in exactly one space")
    return spec["segment"], homes[0], (spec["segment"]-homes[0].link_segment)*16


ANCHOR_MIN_ORDINARY = 16
RUN_MODE = "dgroup-class-run-v1"
_ALIGN = {1: 1, 2: 2, 3: 16, 4: 256}


def _code_frame(frame, module, target_name, segment_name=None):
    """F0/F1 naming a CODE segment of the member (or exactly segment_name), F5, or F2 = target."""
    kind = frame.get("kind")
    if kind == "segment":
        rows = [s for s in module.segment_defs if s["name"] == frame.get("name")]
        return len(rows) == 1 and rows[0]["class"] == "CODE" and \
            (segment_name is None or frame.get("name") == segment_name)
    return kind == "target_frame" or (kind == "external" and frame.get("name") == target_name)


def _signature_hits(data, lo, hi, code, mask):
    """Every position p in [lo, hi-len(code)] whose bytes equal code at every unmasked offset."""
    best, run = (0, 0), None
    for i, keep in enumerate(mask + [False]):
        if keep and run is None:
            run = i
        elif not keep and run is not None:
            if i-run > best[1]-best[0]:
                best = (run, i)
            run = None
    s, e = best
    require(e > s, "Signature has no ordinary bytes")
    hits, at = [], data.find(code[s:e], lo+s, hi)
    while at >= 0:
        p = at-s
        if lo <= p and p+len(code) <= hi and all(not k or data[p+i] == code[i] for i, k in enumerate(mask)):
            hits.append(p)
        at = data.find(code[s:e], at+1, hi)
    return hits


def _owner_overlaps(manifest, oracle, position, size, spec):
    """Owners touching [position, position+size); only the same member pinned at the same place is allowed."""
    from common import project_path, read_json
    for owner in manifest["owners"]:
        if owner["space"] != "root":
            continue
        lo = oracle.spaces["root"].position(owner["segment"], owner["offset"])
        if not (lo < position+size and position < lo+owner["size"]):
            continue
        same = False
        if owner["kind"] == "PINNED_RUNTIME":
            recipe = read_json(project_path(owner["recipe"]))
            same = (recipe["member_sha256"] == spec["member_sha256"] and lo == position and
                    recipe["object_segment"] == spec["object_segment"] and owner["size"] == size)
        require(same, f"Anchored provider {spec['name']} overlaps owner {owner['target_id']}")


def anchor_provider(spec, frame, space, oracle, manifest, owner_span):
    """A zero-owned code provider located by its complete, unique ordinary-byte signature."""
    module, overwritten = read_member(spec)
    name, offset = spec["object_segment"], spec["offset"]
    seg = _segment(module, name)
    code = module.segments.get(name, b"")
    require(seg["class"] == "CODE" and not seg["use32"] and len(code) == seg["length"] > 0 and
            module.initialized_ranges.get(name) == [(0, len(code))],
            f"Anchored provider {spec['name']} needs one complete initialized CODE segment")
    base = frame*16
    hi = min(base+0x10000, space.size)
    require(type(offset) is int and 0 <= offset and base+offset+len(code) <= hi,
            f"Anchored provider {spec['name']} lies outside the CODE frame window")
    position = base+offset
    require(not (position < owner_span[1] and owner_span[0] < position+len(code)),
            f"Anchored provider {spec['name']} overlaps the judged member")
    _owner_overlaps(manifest, oracle, position, len(code), spec)
    rows = [f for f in fixes(module) if f["segment"] == name]
    mask = [True]*len(code)
    for fix in rows:
        _field_free(fix, overwritten)
        at, width = fix["segment_offset"], fix["field_width"]
        require(at+width <= len(code) and all(mask[at:at+width]),
                f"Anchored provider {spec['name']} fixup fields overlap or escape")
        mask[at:at+width] = [False]*width
    require(sum(mask) >= ANCHOR_MIN_ORDINARY,
            f"Anchored provider {spec['name']} has fewer than {ANCHOR_MIN_ORDINARY} ordinary bytes")
    expected = space.data[position:position+len(code)]
    require(all(not k or expected[i] == code[i] for i, k in enumerate(mask)),
            f"Anchored provider {spec['name']} ordinary bytes differ from the original")
    hits = _signature_hits(space.data, base, hi, code, mask)
    require(hits == [position], f"Anchored provider {spec['name']} signature is not unique in the frame window")
    publics = {p["name"]: p["offset"] for p in module.publics if p["segment"] == name and p["group_index"] == 0}
    equations, obligations = 0, 0
    for fix in rows:
        at, width, target, fr = fix["segment_offset"], fix["field_width"], fix["target"], fix["frame"]
        addend, disp = _addend(code, at, width), fix["displacement"] or 0
        tname = target.get("name")
        own = target["kind"] == "segment" and tname == name
        own_public = target["kind"] == "external" and tname in publics
        codeframe = _code_frame(fr, module, tname, name)
        if fix["self_relative"] and fix["loc_type"] == 1 and width == 2 and codeframe and own:
            destination = offset+disp+addend
            require(offset <= destination < offset+len(code), "Anchored own-segment branch leaves the member")
            value = destination-(offset+at+2)
        elif fix["self_relative"] and fix["loc_type"] == 1 and width == 2 and codeframe and own_public and \
                addend == 0 and disp == 0:
            value = publics[tname]-(at+2)
        elif not fix["self_relative"] and fix["loc_type"] == 1 and width == 2 and codeframe and own:
            value = offset+disp+addend
        elif not fix["self_relative"] and fix["loc_type"] == 3 and width == 4 and codeframe and own_public and \
                addend == 0 and disp == 0:
            value = (offset+publics[tname]) | (frame << 16)
        else:
            obligations += 1   # external/DGROUP field: the anchored member's own open obligation
            continue
        require(value & ((1 << (8*width))-1) == _addend(expected, at, width),
                f"Anchored provider {spec['name']} own field at +{at:#x} differs")
        equations += 1
    return {"name": spec["name"], "position": position, "offset": offset, "size": len(code),
            "member_sha256": spec["member_sha256"], "ordinary_bytes": sum(mask),
            "own_equations": equations, "open_obligations": obligations,
            "publics": {n: position+o for n, o in publics.items()}}


def verify_class_run(path, digest, ctx):
    """An ordered gap-free run of fixup-free DGROUP contributions ending at the payload end."""
    from common import ROOT, project_path, read_json
    source = project_path(path)
    raw = source.read_bytes()
    require(source.is_relative_to(ROOT/"evidence/bindings") and sha(raw) == digest and b"\r" not in raw,
            "Class run evidence changed, escapes evidence cone or is not LF")
    run = read_json(source)
    require(run.get("schema") == 1 and run.get("mode") == RUN_MODE and
            run.get("target_sha256") == sha(ctx.oracle.data), "Unsupported/foreign class run evidence")
    require(ctx.dgroup is not None and run.get("dgroup_paragraph") == ctx.dgroup[0],
            "Class run needs the grounded DGROUP paragraph")
    paragraph, home, base = ctx.dgroup
    start = cursor = run["start"]
    require(type(start) is int and 0 <= start and run["contributions"], "Class run needs a start and contributions")
    tiles, receipts = {}, []
    for entry in run["contributions"]:
        name, specs = entry["segment"], entry["members"]
        require(specs, "Class run entry without members")
        decls = []
        for spec in specs:
            module, _ = read_member(spec)
            seg = _segment(module, name)
            content = module.segments.get(name, b"")
            require(name in _dgroup_members(module) and seg["class"] == run["class"] and not seg["use32"] and
                    seg["combine"] in (2, 6) and seg["alignment"] in _ALIGN and seg["length"] > 0 and
                    module.initialized_ranges.get(name) == [(0, seg["length"])] and
                    not [f for f in fixes(module) if f["segment"] == name],
                    f"Class run entry {name} is not a complete fixup-free {run['class']} DGROUP contribution")
            decls.append((seg["combine"], seg["alignment"], content, spec["member_sha256"]))
        require(len({d[0] for d in decls}) == 1 and len({d[1] for d in decls}) == 1,
                f"Class run entry {name} contributors disagree on combine/alignment")
        combine, align = decls[0][0], _ALIGN[decls[0][1]]
        require(combine == 6 or len(decls) == 1, f"Public class run entry {name} must have one contributor")
        placement = -(-cursor // align)*align
        require(cursor != start or placement == start, "Class run start is not aligned for its first entry")
        require(not any(home.data[base+cursor:base+placement]), f"Class run alignment gap before {name} is not zero")
        length = max(len(d[2]) for d in decls)
        require(base+placement+length <= home.size, f"Class run entry {name} escapes the DGROUP payload")
        for _, _, content, member in decls:
            require(home.data[base+placement:base+placement+len(content)] == content,
                    f"Class run entry {name} differs from the original DGROUP bytes")
            require((member, name) not in tiles, "Duplicate class run contribution")
            tiles[(member, name)] = placement
        receipts.append({"segment": name, "placement": placement, "length": length,
                         "common": combine == 6, "contributors": len(decls)})
        cursor = placement+length
    require(cursor == home.size-base, "Class run does not end exactly at the original DGROUP payload end")
    return tiles, {"path": path, "class": run["class"], "start": start, "end": cursor, "entries": receipts}


def _provider_libraries(manifest):
    """Pinned recipe name -> its archive identity (library_sha256)."""
    from common import project_path, read_json
    return {read_json(project_path(o["recipe"]))["name"]: read_json(project_path(o["recipe"]))["library_sha256"]
            for o in manifest["owners"] if o["kind"] == "PINNED_RUNTIME"}


ACCEPTED_CODE = (("MATCHING_C", "CODE_EXACT"), ("MATCHING_ASM", "ASM_EXACT"))


def external_provider(row, oracle, manifest, owner_span):
    """An accepted exact owner's entry as a code provider: (linear position, frame, receipt)."""
    from common import project_path, read_json
    from match import get_target
    symbol, target_id, at = row["symbol"], row["target_id"], row["frame_witness"]
    owners = [o for o in manifest["owners"] if o["target_id"] == target_id]
    require(len(owners) == 1 and (owners[0]["kind"], owners[0]["state"]) in ACCEPTED_CODE and
            owners[0]["space"] == "root", f"External provider {target_id} is not one accepted exact resident owner")
    owner = owners[0]
    root = oracle.spaces["root"]
    position = root.position(owner["segment"], owner["offset"])
    require(not (position < owner_span[1] and owner_span[0] < position+owner["size"]),
            "External provider overlaps the judged member")
    recipe = read_json(project_path(owner["recipe"]))
    if "claims" in recipe:
        publics = [c["public"] for c in recipe["claims"] if c["target_id"] == target_id]
    else:
        publics = [recipe["public"]] if recipe.get("target_id") == target_id else []
    require(len(publics) == 1, f"External provider {target_id} has no single claimed public")
    public = publics[0]
    # MASM without /Ml uppercases publics; only that exact folding is accepted.
    folded = public != symbol and recipe.get("language") == "asm" and public == symbol.upper()
    require(public == symbol or folded, f"External provider public {public} does not name {symbol}")
    target, space, _ = get_target(target_id, oracle)
    require(space is root and root.position(target["segment"], target["offset"]) == position,
            "External provider target row disagrees with its owner")
    sites = [c for c in target.get("call_sites", []) if c["space"] == "root" and c["image_offset"] == at and
             c["kind"] in ("far_call", "far_pointer")]
    require(len(sites) == 1, "External provider frame witness is not a reviewed relocated far call/pointer of its target")
    field, size = (at+1, 5) if sites[0]["kind"] == "far_call" else (at, 4)
    require(not (at < owner_span[1] and owner_span[0] < at+size), "External provider frame witness lies inside the judged member")
    offset, frame = struct.unpack_from("<HH", root.data, field)
    require(frame*16+offset == position, "External provider frame witness does not land on its entry")
    return position, frame, {"symbol": symbol, "target_id": target_id, "public": public, "case_folded": folded,
                             "frame": frame, "witness": at, "witness_kind": sites[0]["kind"]}


class Context:
    """Grounded values shared by one member's code and data equations."""

    def __init__(self, binding, recipe, oracle, manifest, owner_span):
        require(binding.get("schema") == 1 and binding.get("mode") == MODE and
                binding.get("target_sha256") == sha(oracle.data), "Unsupported/foreign runtime fixup binding")
        self.oracle = oracle
        self.space = oracle.spaces[recipe["space"]]
        require(recipe["space"] == "root", "Runtime symbolic proof supports resident members only")
        identifier = f"{recipe['space']}:{recipe['segment']:04x}:{recipe['offset']:04x}"
        self.providers = code_providers(manifest, oracle, exclude=identifier)
        self.frame, self.frame_receipts = _ground_frame(binding["code_frame"], self.providers,
                                                        self.space, owner_span)
        self.dgroup = None
        if "dgroup" in binding:
            self.dgroup = _ground_dgroup(binding["dgroup"], oracle)
        self.used = set()
        recipe_names = {name for rows in self.providers.values() for name, _ in rows}
        self.anchors = {}
        for spec in binding.get("code_anchors", []):
            require(type(spec.get("name")) is str and spec["name"] not in recipe_names and
                    spec["name"] not in self.anchors, "Anchored provider name is missing, duplicate or a recipe name")
            self.anchors[spec["name"]] = anchor_provider(spec, self.frame, self.space, oracle, manifest, owner_span)
        self.code, self.code_frames, self.external, self.selected = {}, {}, [], []
        selection = binding.get("same_library_selection", [])
        require(type(selection) is list and len(set(selection)) == len(selection) and
                set(selection) <= set(binding.get("code_providers", {})), "Invalid same-library selection")
        libraries = _provider_libraries(manifest) if selection else {}
        for symbol, name in binding.get("code_providers", {}).items():
            rows = self.providers.get(symbol, [])
            if name in self.anchors:
                position = self.anchors[name]["publics"].get(symbol)
                require(position is not None and all(p == position for _, p in rows),
                        f"Anchored provider {name} does not declare {symbol} or conflicts with a pinned public")
                self.code[symbol] = position
                continue
            if symbol in selection:
                # Several pinned declarations: only the single one from the judged member's archive binds.
                same = [r for r in rows if libraries.get(r[0]) == recipe["library_sha256"]]
                require(len(rows) > 1 and len(same) == 1 and same[0][0] == name,
                        f"Same-library selection of {symbol} is not unambiguous for the named recipe")
                self.selected.append({"symbol": symbol, "recipe": name, "declarations": len(rows)})
                self.code[symbol] = same[0][1]
                continue
            require(len(rows) == 1 and rows[0][0] == name,
                    f"Code provider {symbol} is missing, ambiguous or not the named pinned member")
            self.code[symbol] = rows[0][1]
        for row in binding.get("external_providers", []):
            symbol = row["symbol"]
            require(symbol not in self.code and not self.providers.get(symbol),
                    f"External provider {symbol} duplicates a pinned or bound provider")
            position, frame, receipt = external_provider(row, oracle, manifest, owner_span)
            self.code[symbol], self.code_frames[symbol] = position, frame
            self.external.append(receipt)
        require(set(self.anchors) <= set(binding.get("code_providers", {}).values()),
                "Unused anchored code provider")
        self.absolute = {}
        for spec in binding.get("absolute_providers", []):
            module, _ = read_member(spec)
            for symbol in spec["symbols"]:
                rows = [p for p in module.publics if p["name"] == symbol]
                require(len(rows) == 1 and rows[0]["segment_index"] == 0 and rows[0]["group_index"] == 0 and
                        rows[0]["frame"] == 0, f"Absolute provider does not define {symbol} at frame 0")
                require(symbol not in self.absolute, "Duplicate absolute provider")
                self.absolute[symbol] = rows[0]["offset"]
        self.tiles, self.runs, self.used_runs = {}, [], set()
        for row in binding.get("class_runs", []):
            tiles, receipt = verify_class_run(row["path"], row["sha256"], self)
            require(not set(tiles) & set(self.tiles), "Class runs overlap")
            self.tiles.update({key: (placement, row["path"]) for key, placement in tiles.items()})
            self.runs.append(receipt)
        self.data, self.placed = {}, []
        for spec in binding.get("data_providers", []):
            module, overwritten = read_member(spec)
            report = place_segment(module, overwritten, spec["segment"], spec["placement"], self,
                                   {spec["segment"]: spec["placement"]}, provider=True,
                                   member_sha256=spec["member_sha256"])
            groups = {i+1: g["name"] for i, g in enumerate(module.groups)}
            for symbol in spec["symbols"]:
                rows = [p for p in module.publics if p["name"] == symbol]
                require(len(rows) == 1 and rows[0]["segment"] == spec["segment"] and
                        groups.get(rows[0]["group_index"]) == "DGROUP",
                        f"Data provider does not define {symbol} as a DGROUP public of its placed segment")
                require(symbol not in self.data, "Duplicate data provider symbol")
                self.data[symbol] = spec["placement"]+rows[0]["offset"]
            self.placed.append(dict(report, member_sha256=spec["member_sha256"], owned=False))

    def window(self, position, size):
        base = self.frame*16
        require(self.space.name == "root" and base <= position and position+size <= base+0x10000,
                "Runtime extent lies outside the grounded CODE frame window")
        return position-base

    def code_target(self, name, own_publics, origin):
        """Frame offset of a near-reachable code target (same CODE frame only)."""
        if name in own_publics:
            return origin+own_publics[name]
        require(name in self.code, f"Missing pinned code provider for {name}")
        require(self.code_frames.get(name, self.frame) == self.frame,
                f"Code provider {name} lies in another frame; only a far pointer can reach it")
        self.used.add(("code", name))
        return self.window(self.code[name], 1)

    def far_target(self, name, own_publics, origin):
        """(offset, frame) of a far code target; an external provider keeps its witnessed frame."""
        if name in own_publics or name not in self.code_frames:
            return self.code_target(name, own_publics, origin), self.frame
        self.used.add(("code", name))
        frame = self.code_frames[name]
        offset = self.code[name]-frame*16
        require(0 <= offset <= 0xffff, "External provider lies outside its witnessed frame")
        return offset, frame


def _relocation_check(space, start, length, generated, keys, label):
    """Original relocated words inside [start-1, start+length) are exactly the generated
    segment fields, and per target they appear in FIXUPP order."""
    present = [r for r in space.relocations if start-1 <= r["image_offset"] < start+length]
    require(sorted(r["image_offset"] for r in present) == sorted(generated),
            f"{label}: original relocations differ from the generated segment fields")
    order = {r["image_offset"]: r["index"] for r in present}
    for key in set(keys.values()):
        sites = [at for at in generated if keys[at] == key]
        indices = [order[at] for at in sites]
        require(indices == sorted(indices), f"{label}: relocation order inside target {key} differs from FIXUPP")
    return sorted(order[at] for at in generated)


def _field_free(fix, overwritten):
    at, width = fix["segment_offset"], fix["field_width"]
    require(not any(seg == fix["segment"] and lo < at+width and at < hi for seg, lo, hi in overwritten),
            "A FIXUPP field lies in back-patched (overwritten) LEDATA bytes")


def place_segment(module, overwritten, name, placement, ctx, placements, provider=False, code_uses=None,
                  member_sha256=None):
    """Compare one complete placed DGROUP segment with the original DGROUP bytes."""
    require(ctx.dgroup is not None, "Placed DGROUP data needs the grounded DGROUP paragraph")
    paragraph, home, base = ctx.dgroup
    seg = _segment(module, name)
    tile = ctx.tiles.get((member_sha256, name))
    if tile is not None:
        require(tile[0] == placement, f"Placement of {name} differs from its class run")
        ctx.used_runs.add(tile[1])
    # A common-combine segment is placeable only where a class run grounds its start.
    require(name in _dgroup_members(module) and seg["class"] not in ("STACK", "CODE") and
            (seg["combine"] != 6 or tile is not None) and not seg["use32"] and seg["alignment"] != 0,
            f"Segment {name} is not a placeable public DGROUP data segment")
    require(type(placement) is int and 0 <= placement and placement+seg["length"] <= 0x10000 and
            seg["length"] > 0, f"Placement of {name} outside DGROUP")
    content = module.segments.get(name)
    if not module.initialized_ranges.get(name):
        require(not provider, f"Provider segment {name} has no bytes to compare")
        require(len(code_uses or set()) >= 2, f"Uninitialized placed segment {name} needs two distinct proven addends")
        return {"segment": name, "placement": placement, "length": seg["length"],
                "basis": "two or more distinct proven code addends agree"}
    require(module.initialized_ranges[name] == [(0, seg["length"])],
            f"Placed segment {name} must be completely initialized")
    start = base+placement
    require(start+seg["length"] <= home.size, f"Placed segment {name} escapes the DGROUP space")
    expected = home.data[start:start+seg["length"]]
    occupied, generated, keys, equations, diffs = set(), [], {}, [], []
    for fix in fixes(module):
        if fix["segment"] != name:
            continue
        _field_free(fix, overwritten)
        at, width, target, frame = fix["segment_offset"], fix["field_width"], fix["target"], fix["frame"]
        require(not fix["self_relative"] and not occupied.intersection(range(at, at+width)),
                f"Unsupported self-relative/overlapping fixup in data {name}")
        addend = _addend(content, at, width)
        disp = fix["displacement"] or 0
        if fix["loc_type"] == 1 and frame.get("kind") == "group" and frame.get("name") == "DGROUP":
            if target["kind"] == "segment" and target["name"] in placements:
                value = placements[target["name"]]+disp+addend
            elif target["kind"] == "external" and target["name"] in ctx.data and not provider:
                ctx.used.add(("data", target["name"]))
                value = ctx.data[target["name"]]+disp+addend
            else:
                raise ValueError(f"Unbound DGROUP offset in data {name}: {target}")
            kind = "DGROUP offset"
        elif (fix["loc_type"] == 2 and target["kind"] == "group" and target["name"] == "DGROUP" and
              fix["displacement"] is None and addend == 0):
            value, kind = paragraph, "DGROUP base"
            generated.append(start+at)
            keys[start+at] = "group:DGROUP"
        elif (target["kind"] == "external" and target["name"] not in ctx.data and
              target["name"] not in ctx.absolute and _code_frame(frame, module, target["name"]) and
              ((fix["loc_type"] == 1 and width == 2) or
               (fix["loc_type"] == 3 and width == 4 and addend == 0 and disp == 0))):
            # A code address stored in data: CS-relative `dw Proc` (S+A+D) or far `dd Proc` (S:F).
            if fix["loc_type"] == 1:
                value, kind = ctx.code_target(target["name"], {}, 0)+disp+addend, "CODE frame offset"
            else:
                offset, segment = ctx.far_target(target["name"], {}, 0)
                value, kind = offset | (segment << 16), "far code pointer"
                generated.append(start+at+2)
                keys[start+at+2] = f"external:{target['name']}"
        else:
            raise ValueError(f"Unsupported fixup in data {name}: loc {fix['loc_type']} {frame} {target}")
        observed = _addend(expected, at, width)
        mask = (1 << (8*width))-1
        equations.append({"segment": name, "offset": at, "kind": kind, "linked": value & mask,
                          "original": observed, "equal": value & mask == observed})
        if value & mask != observed:
            diffs.append(at)
        occupied.update(range(at, at+width))
    diffs += [i for i in range(seg["length"]) if i not in occupied and content[i] != expected[i]]
    require(not diffs, f"Placed segment {name} differs from the original DGROUP bytes at +{min(diffs, default=0):#x}")
    indices = _relocation_check(home, start, seg["length"], generated, keys, f"Placed segment {name}")
    return {"segment": name, "placement": placement, "length": seg["length"], "space": home.name,
            "position": start, "sha256": sha(expected), "equations": equations,
            "relocation_indices": indices, "common": seg["combine"] == 6,
            "basis": f"complete bytes equal original {home.name}+{start:#x}" +
                     (f"; placement from class run {tile[1]}" if tile is not None else "")}


def prove(module, overwritten, code_name, actual, expected, binding, recipe, oracle, manifest):
    """Every field of the member's CODE and placed DGROUP segments, or a refusal."""
    space = oracle.spaces[recipe["space"]]
    start = space.position(recipe["segment"], recipe["offset"])
    ctx = Context(binding, recipe, oracle, manifest, (start, start+len(actual)))
    origin = ctx.window(start, len(actual))
    own_publics = {p["name"]: p["offset"] for p in module.publics
                   if p["segment"] == code_name and p["group_index"] == 0}
    placements = binding.get("placements", {})
    dgroup_names = _dgroup_members(module)
    # Secondary storage: every other non-empty segment must be a placed DGROUP segment.
    for seg in module.segment_defs:
        if seg["name"] == code_name or not seg["length"]:
            continue
        require(seg["name"] in placements, f"Unplaced secondary storage {seg['name']} ({seg['class']})")
    require(set(placements) <= {s["name"] for s in module.segment_defs if s["length"]} and
            set(placements) <= dgroup_names, "Placement for a segment the member does not contribute")
    require(not getattr(module, "communals", []) and not getattr(module, "start_address", None),
            "Runtime member with communals or a program start address is unsupported")
    rows = fixes(module)
    occupied, generated, keys, equations, diffs = set(), [], {}, [], []
    code_uses = {}
    for fix in rows:
        if fix["segment"] != code_name:
            continue
        _field_free(fix, overwritten)
        at, width, target, frame = fix["segment_offset"], fix["field_width"], fix["target"], fix["frame"]
        require(at+width <= len(actual) and not occupied.intersection(range(at, at+width)),
                "Runtime fixup fields overlap or escape the member")
        addend = _addend(actual, at, width)
        disp = fix["displacement"] or 0
        tkind, tname = target["kind"], target.get("name")
        own_segment = tkind == "segment" and tname == code_name
        frame_code = (frame.get("kind") == "segment" and frame.get("name") == code_name) or \
            frame.get("kind") == "target_frame" or (frame.get("kind") == "external" and frame.get("name") == tname)
        # F5, or F2 naming the target itself: the frame is the target's own (a DGROUP
        # public's group frame); an explicit CODE segment frame never addresses data.
        target_derived = frame.get("kind") == "target_frame" or             (frame.get("kind") == "external" and frame.get("name") == tname)
        site = start+at
        if fix["self_relative"]:
            require(fix["loc_type"] == 1 and width == 2 and frame_code, "Unsupported self-relative runtime fixup")
            require((at >= 1 and actual[at-1] in (0xe8, 0xe9)) or
                    (at >= 2 and actual[at-2] == 0x0f and 0x80 <= actual[at-1] <= 0x8f),
                    "Self-relative runtime field is not a rel16 CALL/JMP/Jcc operand")
            if own_segment:
                destination = origin+disp+addend
                require(origin <= destination < origin+len(actual), "Own-segment branch leaves the member")
            else:
                require(tkind == "external" and addend == 0 and disp == 0,
                        "Self-relative runtime fixup to an unsupported target or with an addend")
                destination = ctx.code_target(tname, own_publics, origin)
            value, kind = destination-(origin+at+2), "near rel16"
        elif fix["loc_type"] == 1 and width == 2 and tkind == "external" and tname in ctx.absolute and \
                frame.get("kind") in ("external", "target_frame") and frame.get("name", tname) == tname:
            ctx.used.add(("absolute", tname))
            value, kind = ctx.absolute[tname]+disp+addend, "absolute public"
        elif fix["loc_type"] == 1 and width == 2 and frame_code and (own_segment or
                                                                    (tkind == "external" and tname not in ctx.data)):
            # CS-relative offset (`dw label`, `mov ax, offset Proc`): needs the grounded frame.
            value = (origin+disp+addend if own_segment else ctx.code_target(tname, own_publics, origin)+disp+addend)
            kind = "CODE frame offset"
            if own_segment:
                require(origin <= value <= origin+len(actual), "Own-segment offset leaves the member")
        elif fix["loc_type"] == 1 and width == 2 and ((frame.get("kind") == "group" and frame.get("name") == "DGROUP")
                                                      or (tkind == "external" and target_derived)):
            if tkind == "segment" and tname in placements and frame.get("kind") == "group":
                value = placements[tname]+disp+addend
                code_uses.setdefault(tname, set()).add(disp+addend)
            elif tkind == "external" and tname in ctx.data:
                ctx.used.add(("data", tname))
                value = ctx.data[tname]+disp+addend
            else:
                raise ValueError(f"Unbound DGROUP offset to {tkind}:{tname}")
            kind = "DGROUP offset"
        elif fix["loc_type"] == 2 and width == 2 and addend == 0 and fix["displacement"] is None and \
                tkind == "group" and tname == "DGROUP":
            require(ctx.dgroup is not None, "DGROUP base needs the grounded DGROUP paragraph")
            value, kind = ctx.dgroup[0], "DGROUP base"
            generated.append(site)
            keys[site] = "group:DGROUP"
        elif fix["loc_type"] == 3 and width == 4 and addend == 0 and disp == 0 and tkind == "external" and \
                frame_code and tname not in ctx.data and tname not in ctx.absolute:
            offset, segment = ctx.far_target(tname, own_publics, origin)
            value, kind = offset | (segment << 16), "far pointer"
            generated.append(site+2)
            keys[site+2] = f"external:{tname}"
        else:
            raise ValueError(f"Unsupported runtime fixup loc {fix['loc_type']} frame {frame} target {target}")
        mask = (1 << (8*width))-1
        observed = _addend(expected, at, width)
        equations.append({"offset": at, "kind": kind, "target": tname, "linked": value & mask,
                          "original": observed, "equal": value & mask == observed})
        if value & mask != observed:
            diffs.append(at)
        occupied.update(range(at, at+width))
    diffs += [i for i in range(len(actual)) if i not in occupied and actual[i] != expected[i]]
    placed = []
    if not diffs:
        for name, placement in placements.items():
            report = place_segment(module, overwritten, name, placement, ctx, placements,
                                   code_uses=code_uses.get(name, set()),
                                   member_sha256=recipe["member_sha256"])
            placed.append(dict(report, member_sha256=recipe["member_sha256"], owned=False))
    unused = ({("code", s) for s in ctx.code} | {("data", s) for s in ctx.data} |
              {("absolute", s) for s in ctx.absolute}) - ctx.used
    require(not unused, f"Unused runtime provider bindings: {sorted(s for _, s in unused)}")
    require(diffs or {r["path"] for r in ctx.runs} <= ctx.used_runs, "Unused class run evidence")
    indices = _relocation_check(space, start, len(actual), generated, keys, "Runtime member") if not diffs else []
    proven = len(equations)+sum(len(row.get("equations", [])) for row in placed)
    require(diffs or proven == len(rows), "Some runtime member fixups were not proven")
    return {"exact": not diffs, "first_difference": min(diffs, default=None), "fixups": proven,
            "relocations": len(generated), "relocation_indices": indices, "equations": equations,
            "code_frame": {"segment": ctx.frame, "witnesses": ctx.frame_receipts},
            "data_placements": ctx.placed+placed,
            "code_anchors": [{k: v for k, v in a.items() if k != "publics"} for a in ctx.anchors.values()],
            "class_runs": ctx.runs,
            "external_providers": ctx.external,
            "same_library_selection": ctx.selected,
            "proof_scope": "Whole pinned member CODE with every fixup computed from independently grounded "
                           "providers, CODE frame and DGROUP placements; secondary DGROUP storage placed and "
                           "compared, data bytes not owned; natural RTLink placement and behavior unproved"}


def check_layout(reports):
    """Across all runtime proofs: one placement per contribution, no overlapping contributions."""
    seen, ranges = {}, []
    for report in reports:
        for row in report.get("data_placements", []):
            key = (row["member_sha256"], row["segment"])
            require(seen.setdefault(key, row["placement"]) == row["placement"],
                    "One runtime data contribution is placed at two DGROUP offsets")
            span = (row["placement"], row["placement"]+row["length"], key, bool(row.get("common")))
            if span not in ranges:
                ranges.append(span)
    ranges.sort()
    # Common-combine contributions of one segment name share their start; others never overlap.
    for i, (lo, hi, a, common) in enumerate(ranges):
        for lo2, hi2, b, common2 in ranges[i+1:]:
            if lo2 >= hi:
                break
            require(a == b or (common and common2 and a[1] == b[1] and lo == lo2),
                    "Placed runtime DGROUP contributions overlap")
