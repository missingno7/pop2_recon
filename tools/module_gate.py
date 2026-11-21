"""Same-source group gate: several reviewed functions from one compiled object.

One historical compilation emits one CODE segment holding several publics in
source order. Each *claim* names a reviewed target and the public that starts
it. Every claim must sit at one shared placement (object offset 0 maps to one
original position), and each is judged over its complete extent: from its
public to the next public or the segment end. Publics that are not claimed are
in-place drafts; they own nothing and are reported as debt.

Supported fixups inside a claim: a same-object near call (`PUSH CS; CALL near`
to another public of this object, equation `S - (P + 2)`), and the existing
external DGROUP offset16 modes with witnesses outside the whole object.
Everything else, including any original MZ relocation in the claim, refuses
that claim. No bytes are masked. This proves CODE_EXACT for each claim and
the relative placement of the group; it does not prove the original file
boundary, declarations outside the group, or link placement.

Object-level rules (refusals, not claim failures): every public lies inside
the initialized CODE segment and one public starts at offset 0; fixup fields
are disjoint; a near call is proven only inside the original CS window named by
an independently relocated CALL FAR into a claim, so 16-bit IP arithmetic and
linear placement agree.
"""
import struct

from binding import operand_field as operand_field_of
from disasm import _capstone
from binding import (IMMEDIATE_MODE, OPERAND_MODE, data_context, data_equations, declarations, fixes, ground,
                     instructions, preserve_segments)
from far_binding import FAR_KINDS, ground_symbol as far_ground, relocation_order

DATA_MODES = ("external-dgroup-offset16-v1", "external-dgroup-word-pair-offset16-v2",
              "external-overlay-dgroup-offset16-v1", IMMEDIATE_MODE, OPERAND_MODE)
from common import require, sha
from match import code_segment, get_target
from omf import OmfReader

GROUP_PROOF_SCOPE = ("Complete code of each claimed function at one shared placement from one compilation; "
                     "same-object near calls inside the witnessed original CS window and external DGROUP "
                     "offset16 equations proven; original file boundary, unclaimed drafts, data placement and "
                     "natural link placement unproved")


def _near_equation(fix, publics, actual, expected, at, rows):
    """Same-object self-relative near call; returns (value, observed) or raises.

    The field must be the rel16 operand of a decoded `CALL near` instruction that
    immediately follows a decoded `PUSH CS`, not merely sit after matching bytes.
    """
    target = fix["target"]
    require(fix["self_relative"] and fix["loc_type"] == 1 and fix["field_width"] == 2 and
            target["kind"] == "external" and target["method"] == 6 and fix["displacement"] is None and
            fix["frame"] == {"kind": "target_frame", "target": target},
            "Unsupported self-relative fixup shape")
    require(target["name"] in publics, "Near call target is not a public of this object")
    by_address = {insn.address: bytes(insn.bytes) for insn in rows}
    require(at >= 2 and by_address.get(at - 1, b"")[:1] == b"\xe8" and len(by_address[at - 1]) == 3 and
            by_address.get(at - 2) == b"\x0e",
            "Near fixup is not the operand of a decoded PUSH CS; CALL near pair")
    require(actual[at:at+2] == b"\x00\x00", "Only zero encoded near-call addends are proven")
    value = (publics[target["name"]] - (at + 2)) & 0xffff
    return value, struct.unpack_from("<H", expected, at)[0]


def _contained(rows, length):
    """Control stays inside the claim: direct in-range jumps only, ending in a return or JMP."""
    from disasm import _capstone
    cs, xc = _capstone()
    last = None
    for insn in rows:
        if set(insn.groups).intersection((cs.CS_GRP_JUMP,)):
            require(len(insn.operands) == 1 and insn.operands[0].type == xc.X86_OP_IMM and
                    0 <= insn.operands[0].imm < length,
                    "Jump leaves the claim or is indirect")
        if bytes(insn.bytes) != b"\x90":
            last = insn
    require(last is not None and (set(last.groups).intersection((cs.CS_GRP_RET, cs.CS_GRP_IRET)) or
                                  last.mnemonic == "jmp"),
            "Control can fall through out of the claim")


def _own_segment(fix, code_name):
    """An offset16 field addressing this object's own CODE segment (switch tables, CS: operands)."""
    target, frame = fix["target"], fix["frame"]
    own = {"kind": "segment", "name": code_name}
    return (not fix["self_relative"] and fix["loc_type"] == 1 and fix["field_width"] == 2 and
            fix["displacement"] is None and
            {k: target.get(k) for k in ("kind", "name")} == own and
            (frame.get("kind") == "target_frame" or {k: frame.get(k) for k in ("kind", "name")} == own))


def _tables(fields, piece, start):
    """Jump tables: a run of consecutive own-segment words that another own-segment field
    (the `jmp cs:[bx+T]` displacement) points at. `fields` are claim-relative offsets;
    addends are object offsets, so T - start locates the table inside the claim."""
    present = set(fields)
    starts = sorted({struct.unpack_from("<H", piece, at)[0] - start for at in fields} & present)
    runs = []
    for first in starts:
        last = first
        while last + 2 in present:
            last += 2
        runs.append((first, last + 2))
    merged = []
    for lo, hi in sorted(runs):
        if merged and lo < merged[-1][1]:
            merged[-1] = (merged[-1][0], max(hi, merged[-1][1]))
        else:
            merged.append((lo, hi))
    return merged


def _decode(piece, tables):
    """Decode every byte outside the jump tables, each code run from its own start."""
    cs, _ = _capstone()
    engine = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_16)
    engine.detail = True
    rows, cursor = [], 0
    for lo, hi in list(tables) + [(len(piece), len(piece))]:
        chunk = piece[cursor:lo]
        part = list(engine.disasm(chunk, cursor))
        require(sum(i.size for i in part) == len(chunk), "Claim code does not decode completely")
        rows += part
        cursor = hi
    return rows


def _check_placements(module, placements, uses, binding, space, oracle):
    """Each placed DGROUP segment must be over-determined: initialized bytes equal the original
    DGROUP bytes at its placement, or (uninitialized) at least two distinct addends agree."""
    if not placements:
        return {}
    require(space.name == "root" and binding and "frame" in binding, "Placed data needs a resident DGROUP frame")
    # The runtime DGROUP paragraph (the C runtime reloads DS with it before main) is
    # grounded by an original relocated word storing it; initialized DGROUP bytes live in
    # whichever space contains that paragraph (the preloaded overlay 17 in this image).
    if "dgroup_segment" in binding:
        from far_binding import _ground_segment
        seg_spec = binding["dgroup_segment"]
        _ground_segment({"segment": seg_spec["segment"], "witnesses": seg_spec["witnesses"]},
                        {"space": space.name, "segment": 0, "offset": 0, "size": 0}, oracle)
        homes = [sp for sp in oracle.spaces.values()
                 if 0 <= (seg_spec["segment"] - sp.link_segment) * 16 < sp.size]
        require(len(homes) == 1, "DGROUP paragraph does not lie in exactly one space")
        home = homes[0]
        dgroup = (seg_spec["segment"] - home.link_segment) * 16
    else:
        home = space
        dgroup = (binding["frame"]["segment"] - space.link_segment) * 16
    report = {}
    for seg in module.segment_defs:
        if seg["name"] not in placements:
            continue
        p = placements[seg["name"]]
        require(type(p) is int and 0 <= p and p + seg["length"] <= 0x10000, "Placement outside DGROUP")
        content = module.segments.get(seg["name"])
        initialized = module.initialized_ranges.get(seg["name"], [])
        if initialized:
            require("dgroup_segment" in binding,
                    "Initialized placed data needs the grounded runtime DGROUP paragraph (dgroup_segment)")
            require(initialized == [(0, seg["length"])] and seg["length"] >= 2,
                    f"Placed segment {seg['name']} must be completely initialized")
            start = dgroup + p
            require(start + seg["length"] <= home.size and home.data[start:start + seg["length"]] == content,
                    f"Placed segment {seg['name']} content differs from the original DGROUP bytes")
            require(not any(start - 1 <= r["image_offset"] < start + seg["length"] for r in home.relocations),
                    f"Placed segment {seg['name']} overlaps original relocated words")
            basis = f"initialized content equals original {home.name}+{start:#x}"
        else:
            require(len(uses.get(seg["name"], set())) >= 2,
                    f"Uninitialized placed segment {seg['name']} needs two distinct proven addends")
            basis = "two or more distinct addends agree"
        report[seg["name"]] = {"placement": p, "length": seg["length"], "basis": basis}
    require(set(report) == set(placements), "Placement for a segment the object does not declare")
    return report


def _witnessed_frames(target, oracle):
    """Segment words of the reviewed, relocated CALL FARs that enter this target."""
    frames = set()
    for call in target.get("call_sites", []):
        if call["kind"] == "far_call":
            caller = oracle.spaces[call["space"]]
            frames.add(struct.unpack_from("<H", caller.data, call["image_offset"] + 3)[0])
    return frames


def check_group(obj_data, claims, oracle, *, binding=None, group_id=None):
    """Judge every claim; never raises for a claim-level mismatch, only for object-level refusal."""
    module = OmfReader().read(obj_data, "candidate")
    placements = (binding or {}).get("placements", {})
    segment, name, actual = code_segment(module, placed=tuple(placements))
    publics = {}
    for public in module.publics:
        require(public["segment"] == name, "Public outside the CODE segment")
        require(public["name"] not in publics, "Duplicate public")
        require(0 <= public["offset"] < len(actual), "Public outside the initialized CODE segment")
        publics[public["name"]] = public["offset"]
    offsets = sorted(publics.values())
    require(len(set(offsets)) == len(offsets), "Two publics share one offset")
    require(offsets and offsets[0] == 0, "Code before the first public would be unaccounted for")
    require(claims and len({c["target_id"] for c in claims}) == len(claims) and
            len({c["public"] for c in claims}) == len(claims), "Claims must be unique")
    obligations = fixes(module)
    fields = set()
    for fix in obligations:
        span = set(range(fix["segment_offset"], fix["segment_offset"] + fix["field_width"]))
        require(not fields.intersection(span) and max(span) < len(actual),
                "Fixup fields overlap or leave the CODE segment")
        fields.update(span)
    resolved, origin, space, frames = [], None, None, set()
    for claim in claims:
        require(claim["public"] in publics, f"Claimed public {claim['public']} is not emitted")
        target, target_space, expected = get_target(claim["target_id"], oracle)
        require(space is None or target_space.name == space.name, "Claims span several spaces")
        space = target_space
        position = space.position(target["segment"], target["offset"])
        start = publics[claim["public"]]
        require(origin is None or position - start == origin,
                "Claims do not share one object placement")
        origin = position - start
        frames |= _witnessed_frames(target, oracle)
        resolved.append((claim, target, expected, start))
    require(0 <= origin and origin + len(actual) <= space.size, "Object placement escapes the space")
    require(len(frames) <= 1, "Claims are entered through different original frames")
    window = None
    if frames:
        base = (next(iter(frames)) - space.link_segment) * 16
        window = (base, base + 0x10000)
    symbols, mode, far_symbols = None, None, {}
    if binding is not None:
        require(binding["fixups"] == obligations, "Ordered symbolic fixup obligations changed")
        require(binding["declarations"] == declarations(module), "Bound object declarations changed")
        require(group_id is not None and binding["target_id"] == group_id, "Binding belongs to another group")
        hull = dict(space.address(origin), id=group_id, size=len(actual))
        # Far callees (other files) ground exactly as in the single-function far gate:
        # a reviewed direct entry plus independent relocated CALL FAR witnesses, all
        # outside the whole object. Data symbols use the DGROUP modes.
        far_symbols = {n: s for n, s in binding["symbols"].items() if s.get("kind") in FAR_KINDS}
        data_only = {n: s for n, s in binding["symbols"].items() if n not in far_symbols}
        for symbol in far_symbols.values():
            require(space.name == "root", "Group far calls are supported in root only")
            far_ground(symbol, hull, oracle)
        if data_only:
            symbols = ground({**binding, "symbols": data_only}, hull, oracle)
            mode = binding["mode"]
        elif "frame" in binding:
            # Verify the DGROUP frame witness even without data symbols (DGROUP base fields).
            ground({**binding, "mode": OPERAND_MODE, "symbols": {}}, hull, oracle)
        else:
            require(binding["mode"] in ("external-far-call-v1",) + DATA_MODES, "Unsupported group binding mode")
            symbols, mode = {}, binding["mode"]

    # Pass 1: extent, decoding, near equations. Pass 2: segment safety. Pass 3: data.
    states, reports, placed_uses = {}, [], {}
    for claim, target, expected, start in resolved:
        later = [o for o in offsets if o > start]
        end = later[0] if later else len(actual)
        report = {"target_id": claim["target_id"], "public": claim["public"], "object_offset": start,
                  "expected_size": len(expected), "emitted_size": end - start,
                  "expected_sha256": sha(expected), "emitted_sha256": sha(actual[start:end])}
        reports.append(report)
        try:
            piece = actual[start:end]
            require(len(piece) == len(expected), "Claim extent differs from the reviewed target; trimming forbidden")
            position = origin + start
            mine = [f for f in obligations if start <= f["segment_offset"] < end]
            require(all(f["segment_offset"] + f["field_width"] <= end for f in mine) and
                    not any(f["segment_offset"] < start < f["segment_offset"] + f["field_width"]
                            for f in obligations), "Fixup field crosses the claim boundary")
            own_fields = [f["segment_offset"] - start for f in mine if _own_segment(f, name)]
            tables = _tables(own_fields, piece, start)
            try:
                rows = _decode(piece, tables) if tables else instructions(piece)
            except ValueError:
                require(not mine, "Fixup-bearing claim does not decode completely")
                rows = None
            state = {"start": start, "piece": piece, "expected": expected, "rows": rows,
                     "occupied": set(), "differences": [], "equations": [], "calls": {}, "data": [],
                     "near": [], "far": [], "own": [], "tables": tables, "group_bases": [],
                     "fixups": len(mine), "frames": _witnessed_frames(target, oracle),
                     "report": report, "position": position,
                     "relocations": [r for r in space.relocations
                                     if position-1 <= r["image_offset"] < position+len(expected)]}
            starts = {i.address for i in rows} if rows else set()
            for fix in mine:
                at = fix["segment_offset"] - start
                if _own_segment(fix, name):
                    in_table = any(lo <= at < hi for lo, hi in tables)
                    # Any decoded disp16/imm16 field qualifies here, including the indirect
                    # `jmp cs:[bx+T]` displacement that selects the jump table.
                    require(in_table or any((i.disp_size == 2 and i.address + i.disp_offset == at) or
                                            (i.imm_size == 2 and i.address + i.imm_offset == at)
                                            for i in rows),
                            "Own-segment field is neither a jump-table word nor an operand field")
                    addend = struct.unpack_from("<H", piece, at)[0]
                    if in_table:
                        # A table entry must point at an instruction start inside this claim.
                        require(start <= addend < end and (addend - start) in starts,
                                "Jump-table entry does not target an instruction of the claim")
                    state["own"].append({"offset": at, "addend": addend})
                    state["occupied"].update((at, at+1))
                elif fix["self_relative"]:
                    callee = fix["target"].get("name")
                    # Claim-relative coordinates: S and P both shift by the claim start.
                    value, observed = _near_equation(fix, {k: v - start for k, v in publics.items()},
                                                     piece, expected, at, rows)
                    state["equations"].append({"offset": at, "kind": "same-object near16", "symbol": callee,
                                               "linked_value": value, "original_value": observed,
                                               "equal": value == observed})
                    state["near"].append({"pair_start": origin + fix["segment_offset"] - 2,
                                          "field_end": origin + fix["segment_offset"] + 2,
                                          "callee": callee, "callee_at": origin + publics[callee],
                                          "equal": value == observed})
                    if value != observed:
                        state["differences"].append(at)
                    state["occupied"].update((at, at+1))
                    state["calls"][at - 1] = callee
                elif fix["target"].get("name") in far_symbols:
                    require(fix["target"]["name"] not in publics,
                            "Far fixup target resolves to a CODE public of this object")
                    state["far"].append(dict(fix, segment_offset=at))
                    if far_symbols[fix["target"]["name"]].get("kind") not in ("segment-base", "far-data"):
                        state["calls"][at - 1] = None  # a far callee, not a claim
                elif (fix["target"].get("kind") == "segment" and fix["target"].get("name") in placements and
                      fix["loc_type"] == 1 and not fix["self_relative"] and fix["field_width"] == 2 and
                      fix["displacement"] is None):
                    # A field addressing this object's own placed DGROUP segment: P + A.
                    require(any(operand_field_of(i, at, candidate=True) for i in rows),
                            "Placed-data fixup is not the address field of one decoded instruction")
                    seg_name = fix["target"]["name"]
                    addend = struct.unpack_from("<H", piece, at)[0]
                    value = (placements[seg_name] + addend) & 0xFFFF
                    observed = struct.unpack_from("<H", expected, at)[0]
                    state["equations"].append({"offset": at, "kind": "placed data", "segment": seg_name,
                                               "encoded_addend": addend, "placement": placements[seg_name],
                                               "linked_value": value, "original_value": observed,
                                               "equal": value == observed})
                    if value != observed:
                        state["differences"].append(at)
                    state["occupied"].update((at, at+1))
                    if value == observed:  # only proven fields over-determine a placement
                        placed_uses.setdefault(seg_name, set()).add(addend)
                elif (fix["loc_type"] == 2 and not fix["self_relative"] and fix["field_width"] == 2 and
                      fix["target"].get("kind") == "group" and fix["target"].get("name") == "DGROUP"):
                    # `mov ax, DGROUP`: the DGROUP paragraph, grounded by the binding's startup
                    # DS-load frame witness; relocated in MZ at the field itself.
                    require(binding is not None and "frame" in binding and fix["displacement"] is None,
                            "DGROUP base needs a binding with a grounded DGROUP frame")
                    require(piece[at:at+2] == bytes(2), "Only zero encoded DGROUP base addends are proven")
                    value = binding["frame"]["segment"]
                    observed = struct.unpack_from("<H", expected, at)[0]
                    state["equations"].append({"offset": at, "kind": "DGROUP base", "linked_segment": value,
                                               "original_segment": observed, "equal": value == observed})
                    if value != observed:
                        state["differences"].append(at)
                    state["occupied"].update((at, at+1))
                    state["group_bases"].append(at)
                else:
                    require(fix["target"].get("name") not in publics,
                            "Data fixup target resolves to a CODE public of this object")
                    state["data"].append(dict(fix, segment_offset=at))
            require(not state["relocations"] or state["far"] or state["group_bases"],
                    "Original MZ relocations need far CALL fixups in the claim")
            states[claim["public"]] = state
        except ValueError as error:
            report.update(exact=False, state="REFUSED", reason=str(error))

    # Original CS: a claim runs in the witnessed frame if a relocated CALL FAR names
    # it directly, or a claim already proven so reaches it by an equal near call whose
    # whole PUSH CS; CALL pair and callee lie inside that frame's 64K window.
    def in_window(near):
        return (window is not None and window[0] <= near["pair_start"] and near["field_end"] <= window[1]
                and window[0] <= near["callee_at"] < window[1])

    cs_proven = {p for p, s in states.items() if s["frames"]}
    changed = True
    while changed:
        changed = False
        for public in list(cs_proven):
            for near in states[public]["near"]:
                if near["equal"] and in_window(near) and near["callee"] in states and \
                        near["callee"] not in cs_proven:
                    cs_proven.add(near["callee"])
                    changed = True
    for public, state in list(states.items()):
        if state["own"] and public in cs_proven and window is not None:
            # Own-segment word = segment-relative offset in the witnessed frame: origin - base + A.
            for own in state["own"]:
                value = (origin - window[0] + own["addend"]) & 0xFFFF
                observed = struct.unpack_from("<H", state["expected"], own["offset"])[0]
                state["equations"].append({"offset": own["offset"], "kind": "own-segment offset16",
                                           "encoded_addend": own["addend"], "linked_value": value,
                                           "original_value": observed, "equal": value == observed})
                if value != observed:
                    state["differences"].append(own["offset"])
        if not state["near"] and not state["own"]:
            continue
        reason = ("Near call or own-segment word lacks an independently witnessed original frame"
                  if public not in cs_proven else
                  "Near call leaves the witnessed original CS window"
                  if not all(in_window(n) for n in state["near"]) else None)
        if reason:
            state["report"].update(exact=False, state="REFUSED", reason=reason)
            del states[public]

    # A claim preserves DS/ES/SS only if its own code does, control cannot leave it
    # except by return or an allowed call, and every call goes to a claim that does too.
    # Unclaimed drafts are never assumed safe.
    safe, changed = set(), True
    while changed:
        changed = False
        for public, state in states.items():
            if public in safe or state["rows"] is None:
                continue
            # Far callees in other files keep DS/SS per the MSC far-call ABI, as the
            # single-function far gate also assumes; drafts and unproved claims do not.
            allowed = {at for at, callee in state["calls"].items() if callee is None or callee in safe}
            try:
                preserve_segments(state["rows"], allowed, es_ds=mode == IMMEDIATE_MODE)
                _contained(state["rows"], len(state["piece"]))
            except ValueError:
                continue
            safe.add(public)
            changed = True

    used_symbols, used_far = set(), set()
    for report in reports:
        state = states.get(report["public"])
        if state is None:
            continue
        try:
            relocation_proof = {}
            if state["far"]:
                sites = []
                by_address = {i.address: i for i in state["rows"]}
                for fix in state["far"]:
                    at, ref = fix["segment_offset"], fix["target"]
                    symbol = far_symbols[ref["name"]]
                    # LOC5 (loader-resolved offset16) writes the same value as LOC1 here.
                    if symbol.get("kind") == "far-data" and fix["loc_type"] in (1, 5) and not fix["self_relative"]:
                        # Offset half of a far data pointer: O within the object's own segment.
                        require(fix["field_width"] == 2 and ref["kind"] == "external" and fix["displacement"] is None and
                                fix["frame"] == {"kind": "target_frame", "target": ref},
                                "Unsupported far data offset fixup shape")
                        require(state["piece"][at:at+2] == bytes(2), "Only zero encoded far data addends are proven")
                        observed = struct.unpack_from("<H", state["expected"], at)[0]
                        state["equations"].append({"offset": at, "kind": "far-data offset", "symbol": ref["name"],
                                                   "linked_offset": symbol["offset"], "original_offset": observed,
                                                   "equal": symbol["offset"] == observed})
                        if symbol["offset"] != observed:
                            state["differences"].append(at)
                        state["occupied"].update(range(at, at+2))
                        used_far.add(ref["name"])
                        continue
                    if symbol.get("kind") in ("segment-base", "far-data") and fix["loc_type"] == 2:
                        # LOC2 base16 of an external far segment (`mov ax, seg Table`): the
                        # linker writes that segment's paragraph, relocated in MZ at the field.
                        require(fix["loc_type"] == 2 and fix["field_width"] == 2 and not fix["self_relative"] and
                                ref["kind"] == "external" and fix["displacement"] is None and
                                fix["frame"] == {"kind": "target_frame", "target": ref},
                                "Unsupported segment-base fixup shape")
                        require(state["piece"][at:at+2] == bytes(2), "Only zero encoded segment addends are proven")
                        observed = struct.unpack_from("<H", state["expected"], at)[0]
                        state["equations"].append({"offset": at, "kind": "segment-base", "symbol": ref["name"],
                                                   "linked_segment": symbol["segment"], "original_segment": observed,
                                                   "equal": symbol["segment"] == observed})
                        if symbol["segment"] != observed:
                            state["differences"].append(at)
                        state["occupied"].update(range(at, at+2))
                        sites.append({"site": state["position"] + at, "offset": at, "symbol": ref["name"]})
                        used_far.add(ref["name"])
                        continue
                    require(fix["loc_type"] == 3 and fix["field_width"] == 4 and not fix["self_relative"] and
                            ref["kind"] == "external" and ref["method"] == 6 and fix["displacement"] is None and
                            fix["frame"] == {"kind": "target_frame", "target": ref},
                            "Unsupported far CALL fixup equation/frame/displacement")
                    insn = by_address.get(at - 1)
                    require(insn is not None and insn.size == 5 and bytes(insn.bytes)[:1] == b"\x9a",
                            "Far fixup is not a complete unprefixed 9A CALL operand")
                    require(state["piece"][at:at+4] == bytes(4), "Only zero encoded far addends are proven")
                    value = (symbol["offset"], symbol["segment"])
                    observed = struct.unpack_from("<HH", state["expected"], at)
                    state["equations"].append({"offset": at, "kind": "far-call", "symbol": ref["name"],
                                               "linked_offset": value[0], "linked_segment": value[1],
                                               "original_offset": observed[0], "original_segment": observed[1],
                                               "equal": value == observed})
                    if value != observed:
                        state["differences"].append(at)
                    state["occupied"].update(range(at, at+4))
                    sites.append({"site": state["position"] + at + 2, "offset": at, "symbol": ref["name"]})
                    used_far.add(ref["name"])
                sites += [{"site": state["position"] + g, "offset": g, "symbol": "DGROUP"} for g in state["group_bases"]]
                relocation_proof = relocation_order(state["relocations"], sites,
                                                    [r["index"] for r in state["relocations"]])
            if state["group_bases"] and not state["far"]:
                relocation_proof = relocation_order(
                    state["relocations"],
                    [{"site": state["position"] + g, "offset": g, "symbol": "DGROUP"} for g in state["group_bases"]],
                    [r["index"] for r in state["relocations"]])
            if state["data"]:
                require(symbols is not None, "DGROUP fixups require a grounded group binding")
                missing = sorted({f["target"].get("name") for f in state["data"]} - set(symbols))
                require(not missing, "DGROUP fixup targets lack grounded binding symbols: " + ", ".join(map(str, missing)))
                # v4 proves offset bytes only and claims no DS association, so it needs no
                # segment-context proof; the older modes keep theirs.
                require(mode == OPERAND_MODE or report["public"] in safe,
                        "DGROUP access after a call into code not proven to preserve segment context")
                allowed = {at for at, callee in state["calls"].items() if callee is None or callee in safe}
                data_context(module, state["rows"], allowed, mode=mode, placed=tuple(placements))
                found, unequal = data_equations(state["data"], symbols, name, state["piece"], state["expected"],
                                                state["rows"], mode, state["occupied"])
                state["equations"] += [dict(e, kind="DGROUP offset16") for e in found]
                state["differences"] += unequal
                used_symbols.update(f["target"]["name"] for f in state["data"])
            piece, expected, occupied = state["piece"], state["expected"], state["occupied"]
            differences = state["differences"] + [i for i in range(len(piece))
                                                  if i not in occupied and piece[i] != expected[i]]
            report.update(exact=not differences, state="CODE_EXACT" if not differences else "CANDIDATE_C",
                          first_difference=min(differences, default=None),
                          mismatch_count=len(differences), fixups=state["fixups"],
                          relocations=len(state["relocations"]),
                          covered_relocation_indices=[r["index"] for r in state["relocations"]],
                          equations=state["equations"], ordinary_bytes_compared=len(piece) - len(occupied),
                          **{k: v for k, v in relocation_proof.items()
                             if k in ("relocation_order", "relocation_order_debt", "relocation_groups")})
        except ValueError as error:
            report.update(exact=False, state="REFUSED", reason=str(error))
    # A refused claim never evaluated its data fields; report it instead of masking the reason.
    if symbols is not None and not any(r["state"] == "REFUSED" for r in reports):
        require(set(symbols) == used_symbols, "Missing or unused grounded data symbol")
        require(set(far_symbols) == used_far, "Missing or unused grounded far symbol")
    placement_report = _check_placements(module, placements, placed_uses, binding, space, oracle)
    claimed = {c["public"] for c in claims}
    return {"exact": all(r["exact"] for r in reports), "object_sha256": sha(obj_data),
            "placements": placement_report,
            "space": space.name, "origin": origin, "emitted_size": len(actual),
            "frame": next(iter(frames)) if frames else None,
            "segment": segment, "claims": reports,
            "drafts": sorted(({"public": p, "object_offset": o} for p, o in publics.items() if p not in claimed),
                             key=lambda d: d["object_offset"]),
            "unreferenced_external_declarations": [e for e in module.externals
                                                   if e not in {f["target"].get("name") for f in obligations}],
            "fixup_thread_declarations": [entry for record in module.fixups for entry in record["decoded"]],
            "record_types": [r["type"] for r in module.records], "proof_scope": GROUP_PROOF_SCOPE}


def compile_group(source_text, profile, flags, claims, workdir, *, binding=None, group_id=None):
    """Compile exactly the given source text (the bytes the caller hashed) and judge it."""
    from pathlib import Path
    from compiler import compile_c
    from oracle import Oracle
    oracle = Oracle.load()
    result = compile_c(source_text, profile, flags=flags, workdir=Path(workdir))
    require(result.ok, "Historical compilation failed: " + result.log)
    return result, check_group(result.obj_bytes, claims, oracle, binding=binding, group_id=group_id)
