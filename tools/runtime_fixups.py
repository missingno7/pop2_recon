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
* absolute publics: the provider member's own absolute PUBDEF (frame 0).

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
        self.code = {}
        for symbol, name in binding.get("code_providers", {}).items():
            rows = self.providers.get(symbol, [])
            require(len(rows) == 1 and rows[0][0] == name,
                    f"Code provider {symbol} is missing, ambiguous or not the named pinned member")
            self.code[symbol] = rows[0][1]
        self.absolute = {}
        for spec in binding.get("absolute_providers", []):
            module, _ = read_member(spec)
            for symbol in spec["symbols"]:
                rows = [p for p in module.publics if p["name"] == symbol]
                require(len(rows) == 1 and rows[0]["segment_index"] == 0 and rows[0]["group_index"] == 0 and
                        rows[0]["frame"] == 0, f"Absolute provider does not define {symbol} at frame 0")
                require(symbol not in self.absolute, "Duplicate absolute provider")
                self.absolute[symbol] = rows[0]["offset"]
        self.data, self.placed = {}, []
        for spec in binding.get("data_providers", []):
            module, overwritten = read_member(spec)
            report = place_segment(module, overwritten, spec["segment"], spec["placement"], self,
                                   {spec["segment"]: spec["placement"]}, provider=True)
            groups = {i+1: g["name"] for i, g in enumerate(module.groups)}
            for symbol in spec["symbols"]:
                rows = [p for p in module.publics if p["name"] == symbol]
                require(len(rows) == 1 and rows[0]["segment"] == spec["segment"] and
                        groups.get(rows[0]["group_index"]) == "DGROUP",
                        f"Data provider does not define {symbol} as a DGROUP public of its placed segment")
                require(symbol not in self.data, "Duplicate data provider symbol")
                self.data[symbol] = spec["placement"]+rows[0]["offset"]
            self.placed.append(dict(report, member_sha256=spec["member_sha256"], owned=False))
        self.used = set()

    def window(self, position, size):
        base = self.frame*16
        require(self.space.name == "root" and base <= position and position+size <= base+0x10000,
                "Runtime extent lies outside the grounded CODE frame window")
        return position-base

    def code_target(self, name, own_publics, origin):
        if name in own_publics:
            return origin+own_publics[name]
        require(name in self.code, f"Missing pinned code provider for {name}")
        self.used.add(("code", name))
        return self.window(self.code[name], 1)


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


def place_segment(module, overwritten, name, placement, ctx, placements, provider=False, code_uses=None):
    """Compare one complete placed DGROUP segment with the original DGROUP bytes."""
    require(ctx.dgroup is not None, "Placed DGROUP data needs the grounded DGROUP paragraph")
    paragraph, home, base = ctx.dgroup
    seg = _segment(module, name)
    require(name in _dgroup_members(module) and seg["class"] not in ("STACK", "CODE") and
            seg["combine"] != 6 and not seg["use32"] and seg["alignment"] != 0,
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
        else:
            raise ValueError(f"Unsupported fixup in data {name}: loc {fix['loc_type']} {frame} {target}")
        observed = _addend(expected, at, width)
        equations.append({"segment": name, "offset": at, "kind": kind, "linked": value & 0xffff,
                          "original": observed, "equal": value & 0xffff == observed})
        if value & 0xffff != observed:
            diffs.append(at)
        occupied.update(range(at, at+width))
    diffs += [i for i in range(seg["length"]) if i not in occupied and content[i] != expected[i]]
    require(not diffs, f"Placed segment {name} differs from the original DGROUP bytes at +{min(diffs, default=0):#x}")
    indices = _relocation_check(home, start, seg["length"], generated, keys, f"Placed segment {name}")
    return {"segment": name, "placement": placement, "length": seg["length"], "space": home.name,
            "position": start, "sha256": sha(expected), "equations": equations,
            "relocation_indices": indices,
            "basis": f"complete bytes equal original {home.name}+{start:#x}"}


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
            offset = ctx.code_target(tname, own_publics, origin)
            value, kind = offset | (ctx.frame << 16), "far pointer"
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
                                   code_uses=code_uses.get(name, set()))
            placed.append(dict(report, member_sha256=recipe["member_sha256"], owned=False))
    unused = ({("code", s) for s in ctx.code} | {("data", s) for s in ctx.data} |
              {("absolute", s) for s in ctx.absolute}) - ctx.used
    require(not unused, f"Unused runtime provider bindings: {sorted(s for _, s in unused)}")
    indices = _relocation_check(space, start, len(actual), generated, keys, "Runtime member") if not diffs else []
    proven = len(equations)+sum(len(row.get("equations", [])) for row in placed)
    require(diffs or proven == len(rows), "Some runtime member fixups were not proven")
    return {"exact": not diffs, "first_difference": min(diffs, default=None), "fixups": proven,
            "relocations": len(generated), "relocation_indices": indices, "equations": equations,
            "code_frame": {"segment": ctx.frame, "witnesses": ctx.frame_receipts},
            "data_placements": ctx.placed+placed,
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
            span = (row["placement"], row["placement"]+row["length"], key)
            if span not in ranges:
                ranges.append(span)
    ranges.sort()
    for (lo, hi, a), (lo2, hi2, b) in zip(ranges, ranges[1:]):
        require(a == b or hi <= lo2, "Placed runtime DGROUP contributions overlap")
