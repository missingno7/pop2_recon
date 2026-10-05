"""Strict runtime near-code equations, with independently anchored public context.

No linked buffer is constructed. Raw objects remain intact; original bytes are
verification only. Provider signatures locate publics but grant no ownership.
"""
import struct
from pathlib import Path

from binding import declarations, fixes, instructions
from common import require, sha
from disasm import _capstone
from omf import OmfReader


def member(spec):
    data = Path(spec["library_path"]).read_bytes()
    require(sha(data) == spec["library_sha256"], "Near provider library identity changed")
    start, size = spec["archive_offset"], spec["member_extent"]
    require(type(start) is int and type(size) is int and 0 <= start and size > 0 and
            start+size <= len(data), "Near provider member escapes archive")
    raw = data[start:start+size]
    require(sha(raw) == spec["member_sha256"], "Near provider member identity changed")
    return OmfReader().read(raw)


def original_far_entry(space, spec):
    at = spec["image_offset"]
    raw = bytes.fromhex(spec["bytes_hex"])
    require(len(raw) == 5 and raw[0] == 0x9a and 0 <= at <= space.size-5 and
            space.data[at:at+5] == raw, "Original near-code entry anchor is not a far CALL")
    require([r["image_offset"] for r in space.relocations if at-1 <= r["image_offset"] < at+5] == [at+3],
            "Far-entry anchor relocation correspondence differs")
    offset, segment = struct.unpack_from("<HH", raw, 1)
    return segment, offset


def no_relocations(space, start, length):
    require(not any(start-1 <= r["image_offset"] < start+length for r in space.relocations),
            "Near offset16 proof cannot generate original MZ relocation obligations")


def ground_symbol(symbol, space, owner_start, owner_size):
    """Anchor CS with an unrelated far-called wrapper, then check a public prefix.

The prefix is a reviewed context signature, not an accepted/trimmed component.
All fields in it must be local self-relative fixups and satisfy their equations;
everything else compares literally. Unreviewed provider tails/data stay unowned.
"""
    frame = symbol["segment"]
    wrapper = symbol["incoming_wrapper"]
    call_at = wrapper["entry_call"]["image_offset"]
    require(not (call_at < owner_start+owner_size and owner_start < call_at+5),
            "Independent wrapper anchor overlaps accepted owner")
    caller_seg, caller_off = original_far_entry(space, wrapper["entry_call"])
    start = space.position(caller_seg, caller_off)
    raw = bytes.fromhex(wrapper["bytes_hex"])
    require(caller_seg == frame and raw and start+len(raw) <= space.size and
            not (start < owner_start+owner_size and owner_start < start+len(raw)) and
            space.data[start:start+len(raw)] == raw,
            "Independent near wrapper differs, overlaps owner, or uses another CS")
    rows = instructions(raw)
    cs, xc = _capstone()
    path = wrapper["path"]
    by_offset = {i.address: i for i in rows}
    require(rows[-1].size == 3 and rows[-1].bytes[0] == 0xe9 and path and path[0] == 0 and
            path[-1] == rows[-1].address and len(path) == len(set(path)) and
            all(at in by_offset for at in path), "Independent wrapper lacks a bounded entry-to-tail path")
    for at, next_at in zip(path, path[1:]):
        insn = by_offset[at]
        require(not set(insn.groups).intersection((cs.CS_GRP_CALL, cs.CS_GRP_RET, cs.CS_GRP_IRET)) and
                not any(o.type == xc.X86_OP_REG and o.reg == xc.X86_REG_CS for o in insn.operands),
                "Independent near wrapper path changes CS or leaves via a call/return")
        following = at+insn.size
        if cs.CS_GRP_JUMP in insn.groups:
            require(len(insn.operands) == 1 and insn.operands[0].type == xc.X86_OP_IMM,
                    "Independent wrapper path contains non-relative/indirect control flow")
            dest = insn.operands[0].imm
            choices = {dest} if insn.mnemonic == "jmp" else {following, dest}
        else:
            choices = {following}
        require(next_at in choices, "Independent wrapper path has an invalid control-flow edge")
    jump_at = rows[-1].address
    delta = struct.unpack_from("<h", raw, jump_at+1)[0]
    # Wrapping near transfers are outside this first proof subset.
    dest = caller_off+jump_at+3+delta
    require(0 <= caller_off and caller_off+len(raw) <= 65536 and 0 <= dest <= 65535 and
            dest == symbol["offset"], "Independent wrapper does not target the grounded near entry")
    no_relocations(space, start, len(raw))

    provider = symbol["provider"]
    module = member(provider)
    require(declarations(module) == provider["declarations"] and fixes(module) == provider["fixups"],
            "Near provider object context changed")
    name = provider["object_segment"]
    segs = [s for s in module.segment_defs if s["name"] == name and s["class"] == "CODE"]
    require(len(segs) == 1 and not segs[0]["use32"] and not segs[0]["big"] and
            segs[0]["alignment"] != 0, "Unsupported near provider CODE declaration")
    code = module.segments.get(name, b"")
    require(module.initialized_ranges.get(name) == [(0, len(code))] and
            len(code) == segs[0]["length"], "Provider context CODE is sparse or incomplete")
    publics = [p for p in module.publics if p["name"] == symbol["name"]]
    require(len(publics) == 1 and publics[0]["segment"] == name and publics[0]["group_index"] == 0,
            "Near provider lacks a unique ungrouped code public")
    base = provider["module_offset"]
    prefix_size = provider["prefix_size"]
    require(type(base) is int and type(prefix_size) is int and 0 <= base and
            0 < prefix_size <= len(code) and base+len(code) <= 65536 and
            symbol["offset"] == base+publics[0]["offset"] and publics[0]["offset"] < prefix_size,
            "Provider public offset/frame or context extent differs")
    pos = space.position(frame, base)
    require(pos+prefix_size <= space.size and
            not (pos < owner_start+owner_size and owner_start < pos+prefix_size),
            "Provider context overlaps owner or escapes original")
    expected = space.extent(frame, base, prefix_size)
    require(sha(expected) == provider["prefix_sha256"], "Original provider context changed")
    no_relocations(space, pos, prefix_size)
    actual = code[:prefix_size]
    occupied, field_values = set(), {}
    code_rows = instructions(actual)
    boundaries = {i.address for i in instructions(code)}
    for fix in fixes(module):
        at = fix["segment_offset"]
        if fix["segment"] != name or at >= prefix_size:
            continue
        require(fix["loc_type"] == 1 and fix["field_width"] == 2 and fix["self_relative"] and
                fix["frame"] == {"kind": "segment", "index": fix["segment_index"], "name": name} and
                fix["target"] == {"kind": "segment", "index": fix["segment_index"], "name": name, "method": 0} and
                type(fix["displacement"]) is int and fix["displacement"] in boundaries and
                at+2 <= prefix_size and not occupied.intersection((at, at+1)),
                "Unsupported/overlapping provider context fixup")
        require(any(i.address+1 == at and i.size == 3 and i.bytes[0] in (0xe8, 0xe9)
                    for i in code_rows) and actual[at:at+2] == b"\0\0",
                "Provider context fixup is not a zero-addend near transfer")
        delta = fix["displacement"]-(at+2)
        require(-32768 <= delta <= 32767 and struct.unpack_from("<h", expected, at)[0] == delta,
                "Provider internal near equation differs")
        field_values[at] = delta
        occupied.update((at, at+1))
    require(all(a == b for i, (a, b) in enumerate(zip(actual, expected)) if i not in occupied),
            "Provider ordinary context bytes differ")
    # Prove this original signature is unique using independent object bytes
    # and separately computed field equations. No masked/relocated buffer is
    # created and the accepted member's displacement is never a search input.
    runs, begin = [], None
    for i in range(prefix_size+1):
        if i < prefix_size and i not in occupied:
            if begin is None:
                begin = i
        elif begin is not None:
            runs.append((begin, i))
            begin = None
    require(runs, "Provider signature has no ordinary-byte anchor")
    lo, hi = max(runs, key=lambda span: span[1]-span[0])
    seed, search_at, matches = actual[lo:hi], 0, []
    while True:
        found = space.data.find(seed, search_at)
        if found < 0:
            break
        search_at = found+1
        candidate = found-lo
        if candidate < 0 or candidate+prefix_size > space.size:
            continue
        if (all(actual[i] == space.data[candidate+i] for i in range(prefix_size) if i not in occupied) and
            all(struct.unpack_from("<h", space.data, candidate+at)[0] == value
                for at, value in field_values.items())):
            matches.append(candidate)
    require(matches == [pos], "Provider public context signature is ambiguous in the original")
    original_rows = instructions(expected)
    boundary = next((i for i in original_rows if i.address == publics[0]["offset"]), None)
    require(boundary is not None and
            any(i.address+i.size == publics[0]["offset"] and i.mnemonic == "retf" for i in original_rows),
            "Grounded near public is not at a reviewed return/entry boundary")
    return {"symbol": symbol["name"], "segment": frame, "offset": symbol["offset"],
            "provider_context_bytes": prefix_size, "provider_owned_bytes": 0,
            "unique_original_context_position": pos,
            "provider_member_sha256": provider["member_sha256"]}


def compare(module, code_name, actual, expected, binding, recipe, oracle):
    require(binding["schema"] == 1 and binding["mode"] == "external-near-offset16-v1" and
            binding["target_sha256"] == sha(oracle.data), "Unsupported/foreign near-code binding")
    require(recipe["space"] == "root" and binding["space"] == "root" and
            len(actual) == len(expected) == recipe["size"], "Near binding needs a complete resident extent")
    space = oracle.spaces["root"]
    start = space.position(recipe["segment"], recipe["offset"])
    frame, entry = original_far_entry(space, binding["entry_call"])
    call_at = binding["entry_call"]["image_offset"]
    require(not (call_at < start+len(actual) and start < call_at+5) and
            space.position(frame, entry) == start and entry+len(actual) <= 65536,
            "Far-entry anchor does not establish the owned near frame")
    require(declarations(module) == binding["declarations"] and fixes(module) == binding["fixups"],
            "Near candidate declarations/ordered fixups changed")
    obligations = fixes(module)
    symbols = binding["symbols"]
    require(obligations and set(symbols) == {f["target"].get("name") for f in obligations},
            "Missing or unused near symbol binding")
    grounding = []
    for name, symbol in symbols.items():
        require(symbol["name"] == name and symbol["segment"] == frame,
                "Near external target is not in the caller-established CS frame")
        grounding.append(ground_symbol(symbol, space, start, len(actual)))
    no_relocations(space, start, len(actual))
    rows, occupied, equations, differences = instructions(actual), set(), [], []
    for fix in obligations:
        at, ref = fix["segment_offset"], fix["target"]
        require(fix["segment"] == code_name and fix["loc_type"] == 1 and fix["field_width"] == 2 and
                fix["self_relative"] and fix["frame"] == {"kind": "segment", "index": fix["segment_index"], "name": code_name} and
                ref["kind"] == "external" and ref["method"] == 6 and fix["displacement"] is None and
                0 <= at <= len(actual)-2 and not occupied.intersection((at, at+1)),
                "Unsupported/overlapping near candidate fixup")
        require(any(i.address+1 == at and i.size == 3 and i.bytes[0] in (0xe8, 0xe9) for i in rows) and
                actual[at:at+2] == b"\0\0", "Candidate near fixup is not a zero-addend CALL/JMP operand")
        symbol = symbols[ref["name"]]
        next_ip = entry+at+2
        delta = symbol["offset"]-next_ip
        require(-32768 <= delta <= 32767, "Near displacement needs unsupported signed wrapping")
        observed = struct.unpack_from("<h", expected, at)[0]
        if observed != delta:
            differences.append(at)
        equations.append({"offset": at, "symbol": ref["name"], "frame": frame,
                          "target_offset": symbol["offset"], "next_ip": next_ip,
                          "encoded_addend": 0, "explicit_displacement": None,
                          "linked_displacement": delta, "original_displacement": observed,
                          "equal": observed == delta})
        occupied.update((at, at+1))
    differences += [i for i, (a, b) in enumerate(zip(actual, expected)) if i not in occupied and a != b]
    return {"exact": not differences, "first_difference": min(differences, default=None),
            "fixups": len(obligations), "relocations": 0, "equations": equations,
            "ordinary_bytes_compared": len(actual)-len(occupied), "symbol_grounding": grounding,
            "proof_scope": "Whole pinned member CODE with external near equations to independently anchored library public context; provider context owns zero bytes, final provider/TU/link placement and behavior unproved"}
