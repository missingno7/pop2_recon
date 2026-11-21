"""Symbolic fixup proof for whole RTLink/Plus overlay-manager members (mode rtlink-manager-v1).

The RTLink 6.10 manager library (`RTLUTILS.LIB`) links its members into the `$$`-named
public segments `$$OVLMGR` and `$$OVLMGR_UTILS` instead of `_TEXT`/DGROUP. A member owns
its single segment contribution; every FIXUPP field is computed by the linker equation
from values grounded independently of the field:

* the frame of each output segment the member names (its own, or a provider's):
  - `far_call`: an original relocated CALL/JMP FAR outside the member whose segment word
    is the frame and whose offset lands exactly on a group-free public of another pinned
    member in a segment of that name (an ungrouped public's far pointer stores its
    output segment's frame);
  - `site_record`: an original MZ relocation record outside the member with that frame.
    RTLink 6.10 stores the paragraph floor of the start of the output segment holding
    the site (evidence/rtlink-probe/site-frames, also for grouped segments). When that
    site lies after the start of a contribution (the member itself, or a named pinned
    member of that segment) which reaches past `frame*16+15`, the segment holding the
    site overlaps that contribution, so it is that contribution's output segment;
* code/data providers: publics of PINNED_RUNTIME members in the manifest;
* interior uninitialized LEDATA gaps (MASM `?`) are linked as zero bytes.

Offsets are measured from the frame of the target's output segment, which must be the
FIXUPP frame (F0 own segment, F5 target, F2 the target). Groups, foreign frames,
communals and unbound or unused providers refuse. Nothing is masked and no field value
is read back from the original operand. Original relocations inside the member must be
exactly the generated segment words, in FIXUPP order per target.
"""
import struct

from binding import fixes
from common import project_path, read_json, require, sha
from runtime_fixups import _field_free, _relocation_check, read_member

MODE = "rtlink-manager-v1"
# Library identities whose `$$`-class manager segments may be pinned.
LIBRARIES = {"c718bac09b0b1204a259494446acfa8bfffb129144cc7d36799855621f4fdccb": "RTLink/Plus 6.10 RTLUTILS.LIB"}
CLASSES = ("$$OVLMGR", "$$OVLMGR_UTILS")
_ALIGN = {1: 1, 2: 2, 3: 16, 4: 256}


def check_segment(recipe, segment, initialized):
    """A manager-class member: pinned RTLink library, one public segment at its alignment,
    initialized at both ends (interior `?` gaps link as zero)."""
    require(recipe["library_sha256"] in LIBRARIES and segment["class"] in CLASSES and
            segment["name"] == segment["class"],
            "RTLink manager class requires the pinned RTLink library and a $$ manager segment")
    require(segment["combine"] == 2 and segment["alignment"] in _ALIGN,
            "RTLink manager segment must be public-combine with a known alignment")
    require(initialized and initialized[0][0] == 0 and initialized[-1][1] == segment["length"],
            "RTLink manager member extent is incomplete or trimmed")


def check_alignment(segment, position):
    require(position % _ALIGN[segment["alignment"]] == 0,
            "RTLink manager member position violates its segment alignment")


def providers(manifest, oracle, exclude=None):
    """symbol -> [(recipe name, linear position, output segment name)] over pinned members,
    and recipe name -> (start, size, output segment name)."""
    table, members = {}, {}
    for owner in manifest["owners"]:
        if owner["kind"] != "PINNED_RUNTIME" or owner["target_id"] == exclude:
            continue
        recipe = read_json(project_path(owner["recipe"]))
        module, _ = read_member(recipe)
        start = oracle.spaces[recipe["space"]].position(recipe["segment"], recipe["offset"])
        if recipe["space"] == "root":
            members[recipe["name"]] = (start, recipe["size"], recipe["object_segment"])
        for public in module.publics:
            if public["segment"] == recipe["object_segment"] and public["group_index"] == 0:
                table.setdefault(public["name"], []).append(
                    (recipe["name"], start+public["offset"], recipe["object_segment"]))
    return table, members


def _ground_frames(spec, table, members, space, span, own):
    frames, receipts = {}, {}
    lo, hi = span
    require(isinstance(spec, dict) and own in spec, "RTLink binding must ground the member's own segment frame")
    for name, row in spec.items():
        frame = row.get("segment")
        require(type(frame) is int and 0 <= frame <= 0xffff and row.get("witnesses"),
                f"Frame of {name} needs a paragraph and independent witnesses")
        rows = []
        for w in row["witnesses"]:
            if w.get("kind") == "far_call":
                at = w["image_offset"]
                require(type(at) is int and 0 <= at <= space.size-5 and space.data[at] in (0x9a, 0xea),
                        "Frame witness is not an original CALL/JMP FAR")
                relocs = [r for r in space.relocations if at-1 <= r["image_offset"] < at+5]
                require([(r["image_offset"], r["index"]) for r in relocs] == [(at+3, w["relocation_index"])],
                        "Frame witness lacks its exact segment relocation")
                require(not (lo-5 < at < hi), "Frame witness lies inside the judged member")
                offset, segment = struct.unpack_from("<HH", space.data, at+1)
                hits = table.get(w["symbol"], [])
                require(segment == frame and len(hits) == 1 and hits[0][0] == w["provider"] and
                        hits[0][2] == name and hits[0][1] == frame*16+offset,
                        f"Frame witness for {name} does not reach a unique pinned public of that segment")
                rows.append({"kind": "far_call", "image_offset": at, "relocation_index": w["relocation_index"],
                             "provider": w["provider"], "symbol": w["symbol"]})
            elif w.get("kind") == "site_record":
                record = [r for r in space.relocations if r["index"] == w["relocation_index"]]
                require(len(record) == 1 and record[0]["segment"] == frame,
                        f"Site-record witness for {name} does not name that frame")
                site = record[0]["image_offset"]
                require(not (lo-1 <= site < hi), "Site-record witness lies inside the judged member")
                # The segment holding the site starts in [frame*16, frame*16+15] and reaches the
                # site; overlapping a contribution of segment `name` makes it that segment.
                if "member" in w:
                    require(w["member"] in members and members[w["member"]][2] == name,
                            f"Site-record witness member is not a pinned contribution of {name}")
                    base, size, _ = members[w["member"]]
                else:
                    require(name == own, f"Site-record witness for {name} needs a pinned member")
                    base, size = lo, hi-lo
                require(frame*16 <= base and max(frame*16+15, base) <= min(site, base+size-1),
                        f"Site-record witness for {name} does not overlap that segment contribution")
                rows.append({"kind": "site_record", "relocation_index": w["relocation_index"],
                             "image_offset": site, "member": w.get("member", "self")})
            else:
                raise ValueError(f"Unsupported RTLink frame witness {w.get('kind')}")
        frames[name], receipts[name] = frame, {"segment": frame, "witnesses": rows}
    return frames, receipts


def prove(module, overwritten, seg_name, actual, expected, binding, recipe, oracle, manifest):
    """Every FIXUPP field of one manager member, or a refusal."""
    require(binding.get("schema") == 1 and binding.get("mode") == MODE and
            binding.get("target_sha256") == sha(oracle.data), "Unsupported/foreign RTLink fixup binding")
    require(recipe["space"] == "root", "RTLink manager proof supports resident members only")
    require(not getattr(module, "communals", []) and not getattr(module, "start_address", None) and
            not module.groups, "RTLink manager member with groups, communals or a start address is unsupported")
    require(all(s["name"] == seg_name or not s["length"] for s in module.segment_defs),
            "RTLink manager member carries secondary storage")
    space = oracle.spaces["root"]
    start = space.position(recipe["segment"], recipe["offset"])
    identifier = f"root:{recipe['segment']:04x}:{recipe['offset']:04x}"
    table, members = providers(manifest, oracle, exclude=identifier)
    frames, receipts = _ground_frames(binding.get("segment_frames"), table, members, space,
                                      (start, start+len(actual)), seg_name)
    origin = start-frames[seg_name]*16
    require(0 <= origin and origin+len(actual) <= 0x10000, "Member lies outside its grounded frame window")
    rows = [f for f in fixes(module) if f["segment"] == seg_name]
    externals = sorted({f["target"]["name"] for f in rows if f["target"]["kind"] == "external"})
    bound = binding.get("code_providers", {})
    missing = [s for s in externals if s not in bound]
    require(not missing, f"Missing pinned providers for {missing}")
    code = {}
    for symbol, name in bound.items():
        hits = table.get(symbol, [])
        require(len(hits) == 1 and hits[0][0] == name,
                f"Code provider {symbol} is missing, ambiguous or not the named pinned member")
        require(hits[0][2] in frames, f"Frame of provider segment {hits[0][2]} is not grounded")
        code[symbol] = hits[0]
    require(set(code) == set(externals), f"Unused RTLink provider bindings: {sorted(set(code)-set(externals))}")

    def target(fix):
        t = fix["target"]
        if t["kind"] == "segment" and t["name"] == seg_name:
            return start, seg_name, f"segment:{seg_name}"
        require(t["kind"] == "external", f"Unsupported RTLink target {t}")
        _, position, segment = code[t["name"]]
        return position, segment, f"external:{t['name']}"

    def frame_of(fix, tsegment):
        f = fix["frame"]
        if f.get("kind") == "target_frame":
            return tsegment
        if f.get("kind") == "segment" and f.get("name") == seg_name:
            return seg_name
        if f.get("kind") == "external" and f.get("name") == fix["target"].get("name"):
            return tsegment
        raise ValueError(f"Unsupported RTLink frame {f}")

    occupied, generated, keys, equations, diffs = set(), [], {}, [], []
    for fix in rows:
        _field_free(fix, overwritten)
        at, width = fix["segment_offset"], fix["field_width"]
        require(at+width <= len(actual) and not occupied.intersection(range(at, at+width)),
                "RTLink fixup fields overlap or escape the member")
        addend = int.from_bytes(actual[at:at+width], "little")
        disp = fix["displacement"] or 0
        position, tsegment, key = target(fix)
        fsegment = frame_of(fix, tsegment)
        require(fsegment == tsegment, "RTLink offset measured from a foreign segment frame")
        base = frames[fsegment]*16
        site = start+at
        if fix["self_relative"]:
            require(fix["loc_type"] == 1 and width == 2 and fsegment == seg_name,
                    "Unsupported self-relative RTLink fixup")
            require((at >= 1 and actual[at-1] in (0xe8, 0xe9)) or
                    (at >= 2 and actual[at-2] == 0x0f and 0x80 <= actual[at-1] <= 0x8f),
                    "Self-relative RTLink field is not a rel16 CALL/JMP/Jcc operand")
            destination = position+disp+addend-base
            if key == f"segment:{seg_name}":
                require(origin <= destination < origin+len(actual), "Own-segment branch leaves the member")
            value, kind = destination-(origin+at+2), "near rel16"
        elif fix["loc_type"] == 1 and width == 2:
            value, kind = position+disp+addend-base, "segment frame offset"
            require(0 <= value <= 0xffff, "RTLink offset leaves its frame window")
        elif fix["loc_type"] == 2 and width == 2 and fix["displacement"] is None and addend == 0:
            value, kind = frames[fsegment], "segment base"
            generated.append(site)
            keys[site] = key
        elif fix["loc_type"] == 3 and width == 4 and addend >> 16 == 0:
            offset = position+disp+(addend & 0xffff)-base
            require(0 <= offset <= 0xffff, "RTLink far pointer leaves its frame window")
            value, kind = offset | (frames[fsegment] << 16), "far pointer"
            generated.append(site+2)
            keys[site+2] = key
        else:
            raise ValueError(f"Unsupported RTLink fixup loc {fix['loc_type']} frame {fix['frame']} "
                             f"target {fix['target']}")
        mask = (1 << (8*width))-1
        observed = int.from_bytes(expected[at:at+width], "little")
        equations.append({"offset": at, "kind": kind, "target": key, "frame_segment": fsegment,
                          "linked": value & mask, "original": observed, "equal": value & mask == observed})
        if value & mask != observed:
            diffs.append(at)
        occupied.update(range(at, at+width))
    diffs += [i for i in range(len(actual)) if i not in occupied and actual[i] != expected[i]]
    indices = _relocation_check(space, start, len(actual), generated, keys, "RTLink member") if not diffs else []
    require(diffs or len(equations) == len(rows), "Some RTLink member fixups were not proven")
    return {"exact": not diffs, "first_difference": min(diffs, default=None), "fixups": len(equations),
            "relocations": len(generated), "relocation_indices": indices, "equations": equations,
            "segment_frames": receipts,
            "proof_scope": "Whole pinned RTLink manager member segment with every fixup computed from "
                           "independently grounded output-segment frames and pinned providers; interior "
                           "uninitialized bytes linked as zero; natural RTLink placement and behavior unproved"}
