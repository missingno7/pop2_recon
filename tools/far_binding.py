"""Finite root external CALL FAR equations and grouped MZ obligations.

The callee entry and an original relocated CALL outside the owner establish
its alias independently. Raw OMF and oracle bytes are never rewritten.
"""
import struct

from binding import (declarations, fixes, instructions, ground as ground_data,
                     data_context, data_equations)
from common import require, sha
from disasm import _capstone


FAR_KINDS = ("far-code-entry", "rtlink-vector", "overlay-code-entry", "segment-base", "far-data")


def _vector(oracle, segment, offset):
    """The original RTLink vector record at root SEGMENT:OFFSET, decoded by the census rules."""
    from tu_census import overlay_vectors
    overlays = {n: s for n, s in oracle.spaces.items() if s.code_overlay}
    vectors = overlay_vectors(oracle, overlays)
    require((segment, offset) in vectors, "Far callee alias is not an original RTLink vector")
    return vectors[(segment, offset)]


def _ground_segment(symbol, target, oracle, claimed=None):
    """A far data segment's paragraph N, grounded by original relocated words outside the
    candidate whose stored (link-time) value is N."""
    require(type(symbol.get("segment")) is int and 0 <= symbol["segment"] <= 65535 and symbol.get("witnesses"),
            "Segment-base symbol needs a segment and independent relocated witnesses")
    space = oracle.spaces[target["space"]]
    start = space.position(target["segment"], target["offset"])
    end = start + target["size"]
    receipts = []
    for spec in symbol["witnesses"]:
        wspace = oracle.spaces[spec["space"]]
        at = spec["image_offset"]
        relocs = [r for r in wspace.relocations if r["image_offset"] == at]
        require(len(relocs) == 1 and relocs[0]["index"] == spec["relocation_index"] and
                struct.unpack_from("<H", wspace.data, at)[0] == symbol["segment"],
                "Segment-base witness is not an original relocated word with that value")
        # Groups: an original relocated word in an unclaimed draft region also counts.
        spans = claimed if claimed is not None else [(start, end)]
        require(wspace.name != space.name or not any(lo - 1 <= at < hi for lo, hi in spans),
                "Segment-base witness overlaps candidate")
        receipts.append({"space": wspace.name, "image_offset": at, "relocation_index": relocs[0]["index"]})
    return {"kind": "segment-base", "segment": symbol["segment"], "witnesses": receipts}


def _carried_pointer(rows, load, value):
    """True when the first instruction forms a near pointer with offset `value` and straight-line register/stack-slot
    copies carry it to the base register of the last instruction's plain ES:[r] access (see _far_window)."""
    from binding import registers_written
    from disasm import _capstone
    _, xc = _capstone()
    frame = lambda o: (o.type == xc.X86_OP_MEM and o.mem.base == xc.X86_REG_BP and o.mem.index == 0 and
                       o.mem.segment in (0, xc.X86_REG_SS) and o.size == 2)
    place = lambda i, o: ("slot", o.mem.disp) if frame(o) else ("reg", i.reg_name(o.reg)) if o.type == xc.X86_OP_REG else None
    first, held, loaded = rows[0], set(), False
    ops = first.operands
    if not (len(ops) == 2 and ops[1].type == xc.X86_OP_IMM and first.imm_size == 2 and ops[1].imm & 0xFFFF == value and
            ops[0].size == 2 and (first.mnemonic == "mov" and place(first, ops[0]) or
                                  first.mnemonic == "add" and ops[0].type == xc.X86_OP_REG)):
        return False
    held.add(place(first, ops[0]))
    for insn in rows[1:]:
        written, ops = registers_written(insn), insn.operands
        if set(insn.groups).intersection((1, 2, 3, 4, 5, 7)) or "ds" in written or "bp" in written or \
                ("es" in written and insn is not load):
            return False
        loaded = loaded or insn is load
        if insn is rows[-1]:
            mem = [o for o in ops if o.type == xc.X86_OP_MEM]
            return bool(loaded and len(mem) == 1 and mem[0].mem.segment == xc.X86_REG_ES and mem[0].mem.index == 0 and
                        mem[0].mem.disp == 0 and ("reg", insn.reg_name(mem[0].mem.base)) in held)
        if insn.mnemonic == "mov" and len(ops) == 2 and ops[0].size == 2 and place(insn, ops[0]):
            carries = place(insn, ops[1]) in held if ops[1].type in (xc.X86_OP_REG, xc.X86_OP_MEM) else False
            held.discard(place(insn, ops[0]))
            if carries:
                held.add(place(insn, ops[0]))
            continue
        if any(o.type == xc.X86_OP_MEM and o.access & 2 for o in ops) or insn.mnemonic.startswith(("push", "rep", "movs", "stos")):
            return False  # some other store: a carried slot may have changed
        held -= {("reg", n) for n in written}
        held -= {("reg", n[0] + "x") for n in written if len(n) == 2 and n[1] in "lh"}
    return False


def _selector_word(spec, segment, oracle):
    """The DGROUP word at `selector_offset` is an original relocated word storing `segment`."""
    from module_gate import _dgroup_home
    home, dgroup = _dgroup_home({"dgroup_segment": spec["dgroup_segment"]}, oracle.spaces["root"], oracle)
    word = dgroup + spec["selector_offset"]
    relocs = [r for r in home.relocations if r["image_offset"] == word]
    require(len(relocs) == 1 and relocs[0]["index"] == spec["relocation_index"] and
            struct.unpack_from("<H", home.data, word)[0] == segment,
            "Selector word is not an original relocated DGROUP word storing the segment")


def _record_window(spec, segment, value, oracle):
    """Stride of one reached original window that steps through records at segment:value: its only ES load is
    `mov es,[D]` (D a selector word of `segment`) or a relocated `mov r16,N` directly followed by `mov es,r16`, and
    `binding.record_stride` follows the index of its last instruction, an ES access with displacement `value` or a string
    store whose DI is `lea di,[r+value]`. A window with `construct` builds a far pointer to a record instead: it ends in
    `add r16,value` and a relocated `mov r16,N` directly behind it, and loads no segment register (2026-11-24)."""
    from binding import instructions, reachable_from_entry, record_stride, registers_written
    wspace = oracle.spaces[spec["space"]]
    at, raw = spec["image_offset"], bytes.fromhex(spec["bytes_hex"])
    require(wspace.data[at:at + len(raw)] == raw and spec["entry"].split(":")[0] == spec["space"] and
            at in reachable_from_entry(oracle, spec["entry"]), "Record window is not reached original code")
    rows = instructions(raw)
    loads = [k for k, i in enumerate(rows) if {"es", "ds"} & registers_written(i)]
    if "construct" in spec:
        last = rows[-1]
        require(spec["construct"] is True and not loads and len(rows) >= 3 and
                last.bytes[0] & 0xF8 == 0xB8 and struct.unpack_from("<H", last.bytes, 1)[0] == segment and
                any(r["image_offset"] == at + last.address + 1 for r in wspace.relocations),
                "Record window does not build a far pointer into the segment")
        return record_stride(raw[:last.address], value, construct=True)
    require(len(loads) == 1, "Record window needs exactly one ES load and no DS load")
    load = rows[loads[0]]
    if bytes(load.bytes[:2]) == b"\x8e\x06":
        require(struct.unpack_from("<H", load.bytes, 2)[0] == spec.get("selector_offset"),
                "Record window loads ES from another word")
        _selector_word(spec, segment, oracle)
    else:
        before = rows[loads[0] - 1] if loads[0] else None
        require(before is not None and before.bytes[0] & 0xF8 == 0xB8 and
                bytes(load.bytes) == bytes((0x8E, 0xC0 | before.bytes[0] & 7)) and
                struct.unpack_from("<H", before.bytes, 1)[0] == segment and
                any(r["image_offset"] == at + before.address + 1 for r in wspace.relocations),
                "Record window's ES is not loaded with the relocated segment")
    return record_stride(raw, value, es=True)


def _far_window(spec, raw, at, wspace, symbol, value, oracle):
    """Explicit original far-address forms pairing offset `value` with segment N (2026-11-23):
    - `selector-access`: `mov es,[D]` first, straight-line code with no write to ES or DS, and a
      last instruction whose ES: memory operand has displacement `value`. The DGROUP word at D is
      one original relocated word storing N (an MSC far-data selector in CONST).
      A `push es ... pop es` pair with no other stack operation between restores ES (2026-11-23).
    - `selector-copy`: the same start, then `lea di,[r+B]` and, as the window's end, a run of plain MOVSW/MOVSB
      through ES:DI of L bytes: the original copies one unit to [B, B+L), and `value` lies in it (2026-11-23).
    - `selector-pointer`: the window starts with `mov word [bp+d],value`, `mov r16,value` or `add r16,value`; straight
      through register/stack-slot copies that near pointer reaches r, `mov es,[D]` is the window's only segment
      load, and its last instruction accesses ES:[r] without a displacement (2026-11-23).
    - `address-construction`: `add r16,value` directly followed by `mov r16,N`, N relocated.
    - `stored-pointer`: `mov word [bp+d],value` directly followed by `mov word [bp+d+2],N`, N
      relocated (the two halves of one far pointer local)."""
    from binding import instructions, registers_written
    from disasm import _capstone
    _, xc = _capstone()
    rows = instructions(raw)
    segment = symbol["segment"]
    relocated = {r["image_offset"] for r in wspace.relocations}
    if spec["kind"] in ("selector-access", "selector-copy", "selector-pointer"):
        from module_gate import _dgroup_home
        load = [i for i in rows if bytes(i.bytes[:2]) == b"\x8e\x06"]
        require(len(rows) >= 2 and len(load) == 1 and (spec["kind"] == "selector-pointer" or rows[0] is load[0]) and
                struct.unpack_from("<H", load[0].bytes, 2)[0] == spec["selector_offset"],
                "Selector window must start with mov es,[D]" if spec["kind"] != "selector-pointer" else
                "Pointer window must hold exactly one mov es,[D]")
        _selector_word(spec, segment, oracle)
        if spec["kind"] == "selector-pointer":
            require(_carried_pointer(rows, load[0], value), "Selector window does not carry that offset to an ES access")
            return
        saved = False
        for insn in rows[1:]:
            written = registers_written(insn)
            if bytes(insn.bytes) == b"\x06" and not saved:      # push es
                saved = True
                continue
            if bytes(insn.bytes) == b"\x07" and saved:          # pop es, the value just pushed
                saved = False
                continue
            require(not set(insn.groups).intersection((1, 2, 3, 4, 5, 7)) and not written.intersection(("es", "ds")) and
                    not (saved and written.intersection(("sp", "ss"))),
                    "Selector window must be straight-line without another ES or DS load")
        if spec["kind"] == "selector-copy":
            moves = []
            for insn in reversed(rows):
                if bytes(insn.bytes) not in (b"\xa4", b"\xa5"):
                    break
                moves.append(1 if bytes(insn.bytes) == b"\xa4" else 2)
            head = rows[:len(rows) - len(moves)]
            loads = [i for i in head if i.mnemonic == "lea" and i.op_str.startswith("di,")]
            require(moves and len(loads) == 1 and loads[0].disp_size == 2 and
                    loads[0].operands[1].mem.index == loads[0].operands[1].mem.segment == 0 and
                    loads[0].operands[1].mem.base in (xc.X86_REG_BX, xc.X86_REG_SI, xc.X86_REG_DI) and
                    not any("di" in registers_written(i) or i.mnemonic in ("std", "popf")
                            for i in head[head.index(loads[0]) + 1:]),
                    "Selector copy must be one `lea di,[r+B]` followed by a run of MOVS")
            base = loads[0].operands[1].mem.disp & 0xFFFF
            require(base <= value < base + sum(moves), "Address lies outside the unit the selector window copies")
            return
        last = rows[-1]
        mem = [o for o in last.operands if o.type == xc.X86_OP_MEM]
        require(len(mem) == 1 and mem[0].mem.segment == xc.X86_REG_ES and last.disp_size == 2 and
                mem[0].mem.disp & 0xFFFF == value,
                "Selector window does not end in an ES access with that displacement")
        return
    require(len(rows) == 2, "Far address window must be two adjacent instructions")
    first, second = rows
    if spec["kind"] == "address-construction":
        ok = (first.mnemonic == "add" and second.mnemonic == "mov" and
              all(len(r.operands) == 2 and r.operands[0].type == xc.X86_OP_REG and r.operands[0].size == 2 and
                  r.operands[1].type == xc.X86_OP_IMM for r in rows) and
              first.operands[0].reg != second.operands[0].reg and first.imm_size == 2)
    else:
        require(spec["kind"] == "stored-pointer", "Unsupported far data offset witness kind")
        ok = (all(r.mnemonic == "mov" and len(r.operands) == 2 and r.operands[0].type == xc.X86_OP_MEM and
                  r.operands[0].size == 2 and r.operands[1].type == xc.X86_OP_IMM and
                  r.operands[0].mem.base == xc.X86_REG_BP and r.operands[0].mem.index == 0 and
                  r.operands[0].mem.segment == 0 for r in rows) and
              second.operands[0].mem.disp == first.operands[0].mem.disp + 2)
    require(ok and first.operands[1].imm & 0xFFFF == value and second.operands[1].imm & 0xFFFF == segment and
            at + second.address + second.imm_offset in relocated,
            "Far address window does not pair the offset with a relocated segment")


def _ground_far_data(symbol, target, oracle, claimed=None):
    """A far data object at N:O. N is grounded like a segment base; O by reached original
    instructions whose address field equals O and that sit within 16 bytes of an original
    relocated word storing N (the same far-pointer construction in other code), or by one of the
    explicit forms of `_far_window` (docs/far-call-binding.md "Far data offset witnesses"). In a
    group an offset or member may instead carry `"consensus": true`: module_gate then requires
    equal fields at two or more distinct claimed instruction starts (as for v4)."""
    from binding import instructions, operand_field, reachable_from_entry
    segment = _ground_segment({"segment": symbol["segment"], "witnesses": symbol["segment_witnesses"]},
                              target, oracle, claimed)
    offset = symbol.get("offset")
    # Offset 0 (an object starting its own far segment) is not a free choice: it needs no
    # offset witness, and any mismatch is caught against the original field directly.
    def consensus(row):
        require("consensus" not in row or (row["consensus"] is True and claimed is not None and
                                           not row.get("offset_witnesses")),
                "Far data consensus is a group rule for an address without offset witnesses")
        return row.get("consensus", False)
    require(type(offset) is int and 0 <= offset <= 65535 and
            (offset == 0 or symbol.get("offset_witnesses") or consensus(symbol)),
            "Far data symbol needs an offset and independent offset witnesses")
    space = oracle.spaces[target["space"]]
    start = space.position(target["segment"], target["offset"])
    end = start + target["size"]
    def witnessed(spec, value):
        wspace = oracle.spaces[spec["space"]]
        if spec.get("kind") == "relocated-far-pointer":
            # An original `dd N:O` far pointer (offset word, then its relocated segment word),
            # e.g. in preloaded DGROUP data: the linker wrote exactly N:O.
            at = spec["image_offset"]  # the segment word
            relocs = [r for r in wspace.relocations if r["image_offset"] == at]
            spans = claimed if claimed is not None else [(start, end)]
            require(len(relocs) == 1 and relocs[0]["index"] == spec["relocation_index"] and at >= 2 and
                    struct.unpack_from("<H", wspace.data, at)[0] == symbol["segment"] and
                    struct.unpack_from("<H", wspace.data, at - 2)[0] == value and
                    (wspace.name != space.name or not any(lo <= at and at - 2 < hi for lo, hi in spans)),
                    "Far data pointer witness is not an original relocated N:O pair outside the claims")
            return
        at, raw = spec["image_offset"], bytes.fromhex(spec["bytes_hex"])
        spans = claimed if claimed is not None else [(start, end)]
        require(wspace.data[at:at+len(raw)] == raw and
                (wspace.name != space.name or not any(lo <= at+len(raw) and at < hi for lo, hi in spans)),
                "Far data offset witness changed or overlaps candidate")
        if "kind" in spec:
            require(spec["entry"].split(":")[0] == spec["space"] and at in reachable_from_entry(oracle, spec["entry"]),
                    "Far data offset witness is not reached code")
            _far_window(spec, raw, at, wspace, symbol, value, oracle)
            return
        rows = instructions(raw)
        require(len(rows) == 1 and operand_field(rows[0], spec["operand_offset"]) is not None and
                struct.unpack_from("<H", raw, spec["operand_offset"])[0] == value,
                "Far data offset witness field does not equal the offset")
        require(spec["entry"].split(":")[0] == spec["space"] and at in reachable_from_entry(oracle, spec["entry"]),
                "Far data offset witness is not reached code")
        require(any(abs(r["image_offset"] - at) <= 16 and struct.unpack_from("<H", wspace.data, r["image_offset"])[0]
                    == symbol["segment"] for r in wspace.relocations),
                "Far data offset witness is not paired with a relocated word storing the segment")

    for spec in symbol.get("offset_witnesses", []):
        witnessed(spec, offset)
    # A member O+k (k > 0, e.g. ES:[BX+1] into a far table) is proven only by its own
    # independent offset witnesses, as v4 members are: a free addend would re-fit O.
    # An `unproven` member (a reason, no witness) makes every claim that uses it a CODE_MATCH, as for v4 members.
    # A member with `record` windows: the program steps through records of d bytes at O and, in the other window, at
    # O+k with |k| < d. Two arrays of two or more d-byte records starting less than d apart would overlap, so both
    # windows address one array (2026-11-24). O itself keeps its own witnesses.
    members = symbol.get("members", [])
    for m in (m for m in members if "record" in m):
        near = m["offset"] if m["offset"] < 0x8000 else m["offset"] - 0x10000
        strides = [_record_window(m["record"][side], symbol["segment"], value, oracle)
                   for side, value in (("base", offset), ("member", (offset + m["offset"]) & 0xFFFF))]
        require(set(m["record"]) == {"base", "member"} and strides[0] is not None and strides[0] == strides[1] and
                abs(near) < strides[0] and symbol.get("offset_witnesses") and claimed is not None,
                "Far member's record windows do not step through one array of records")
    require(all(type(m.get("offset")) is int and 0 < m["offset"] < 65536 and
                (m.get("offset_witnesses") or consensus(m) or "record" in m or
                 claimed is not None and type(m.get("unproven")) is str and m["unproven"].strip())
                for m in members) and len({m["offset"] for m in members}) == len(members),
            "Every far data member needs a positive offset and its own offset witnesses")
    for member in members:
        for spec in member.get("offset_witnesses", []):
            witnessed(spec, (offset + member["offset"]) & 0xFFFF)
    return {"kind": "far-data", "segment": symbol["segment"], "offset": offset,
            "segment_witnesses": segment["witnesses"], "offset_witnesses": len(symbol.get("offset_witnesses", []))}


def _accepted_entry(target_id, oracle):
    """The target's entry is the start of a canonical CODE_EXACT/ASM_EXACT owner or a pinned
    runtime member (same space and position, whatever segment alias names it). The caller
    has already refused any entry inside the candidate, or inside a group's whole object."""
    from common import ROOT, read_json_shared

    def position(identifier):
        try:
            space, segment, offset = identifier.split(":")
            return space, oracle.spaces[space].position(int(segment, 16), int(offset, 16))
        except (KeyError, ValueError):
            return None  # not addressable in this oracle, so it cannot be the same entry

    wanted = position(target_id)
    if wanted is None:
        return False
    # Pinned runtime members are exact original library code matched to their archive,
    # so their entries are as well established as reconstructed ones.
    owners = read_json_shared(ROOT / "layout/manifest.json")["owners"]
    if any(owner["state"] in ("CODE_EXACT", "ASM_EXACT", "PINNED_RUNTIME") and
           position(owner["target_id"]) == wanted for owner in owners):
        return True
    # A pinned member's other publics (e.g. `_dos_creat` at +7 of `_dos_creatnew`) are original
    # entries too: their offsets come from the hash-checked archive member's own PUBDEF records.
    return any(wanted in _runtime_public_positions(owner, position) for owner in owners
               if owner["state"] == "PINNED_RUNTIME")


def _matched_entry(target_id):
    """The target is a recorded CODE_MATCH of an accepted group: its code compiles exactly (every
    byte and field equation under declared values) and only listed bindings remain unproven."""
    from module_gate import _accepted_recipes
    return any(m["target_id"] == target_id for _, r in _accepted_recipes() for m in r.get("code_matches", []))


def _runtime_public_positions(owner, position):
    from pathlib import Path
    from common import project_path, read_json_shared, sha
    from omf import OmfReader
    try:
        recipe = read_json_shared(project_path(owner["recipe"]))
        library = Path(recipe["library_path"]).read_bytes()
        raw = library[recipe["archive_offset"]:recipe["archive_offset"] + recipe["member_extent"]]
        if sha(library) != recipe["library_sha256"] or sha(raw) != recipe["member_sha256"]:
            return set()
        obj = OmfReader(overwrite_ledata=True, library_records=True).read(raw, recipe["name"])
    except (OSError, KeyError, ValueError):
        return set()
    start = position(owner["target_id"])
    if start is None:
        return set()
    return {(start[0], start[1] + p["offset"]) for p in obj.publics
            if p.get("segment") == recipe["object_segment"] and not p.get("local")}

STRUCTURAL_RETURNS = {0xc3: 1, 0xcb: 1, 0xc2: 3, 0xca: 3}  # RET, RETF, RET iw, RETF iw
STRUCTURAL_PROLOGUE = bytes((0x55, 0x8b, 0xec))  # push bp; mov bp,sp


def _extents(oracle, members=False):
    """(space, start, end, id, accepted) for every reviewed target, accepted marked by the canonical
    CODE_EXACT/ASM_EXACT/PINNED_RUNTIME owners naming it. With `members`, also every such owner without a
    target row: the pinned runtime members, whose publics may lie inside their extent."""
    from common import ROOT, read_json_shared
    from match import EXTRA_TARGETS
    owners = {o["target_id"]: o for o in read_json_shared(ROOT / "layout/manifest.json")["owners"]
              if o["state"] in ("CODE_EXACT", "ASM_EXACT", "PINNED_RUNTIME")}
    rows, named = [], set()
    for f in read_json_shared(ROOT / "evidence/targets.json")["functions"] + EXTRA_TARGETS:
        try:
            lo = oracle.spaces[f["space"]].position(f["segment"], f["offset"])
        except (KeyError, ValueError):
            continue
        rows.append((f["space"], lo, lo + f["size"], f["id"], f["id"] in owners))
        named.add(f["id"])
    for ident, owner in owners.items() if members else ():
        if ident not in named and owner["space"] in oracle.spaces:
            try:
                lo = oracle.spaces[owner["space"]].position(owner["segment"], owner["offset"])
            except ValueError:
                continue
            rows.append((owner["space"], lo, lo + owner["size"], ident, True))
    return rows


def _inside_accepted(oracle, space, entry, size):
    """Refuse a callee entry strictly inside, or a callee extent overlapping, accepted code
    that does not itself start at the entry."""
    for name, lo, hi, ident, accepted in _extents(oracle):
        require(not (accepted and name == space and lo != entry and lo < entry + size and entry < hi),
                f"Far callee entry lies inside or overlaps accepted owner {ident}")


def _structural_entry(oracle, entry, segment, offset, size, space, start, end):
    """Candidate-independent original-byte proof that a resident routine begins at ENTRY,
    for a reviewed callee whose only relocated caller lies in the candidate. Weaker than an
    outside CALL FAR or an accepted owner, so it is recorded as structural-entry debt.

    All of: the entry precedes the RTLink manager/vector region; it starts with a BP-frame
    prologue; it lies strictly inside no reviewed or accepted extent; a reviewed or accepted
    extent ends at entry-pad (pad <= 1 byte of 90/00 without relocation) and its linear
    decode ends exactly there with RET/RETF[ iw], or with an unconditional JMP back into
    itself when the extent is accepted code (a runtime member's shared tail, 2026-11-24); and
    another relocated CALL FAR outside the candidate object uses the same segment word for a
    different reviewed entry."""
    root = oracle.spaces["root"]
    data = root.data
    require(entry < root.position(oracle.mz.header["initial_cs"], 0), "Structural entry refuses the RTLink manager and vector region")
    require(data[entry:entry + 3] == STRUCTURAL_PROLOGUE, "Structural entry needs a BP-frame prologue")
    extents = [e for e in _extents(oracle, members=True) if e[0] == "root"]
    require(not any(lo < entry < hi for _, lo, hi, _, _ in extents),
            "Structural entry lies inside a reviewed or accepted extent")
    require(not any(accepted and lo != entry and lo < entry + size and entry < hi
                    for _, lo, hi, _, accepted in extents), "Structural entry extent overlaps accepted code")
    anchors = []
    for pad in (0, 1):
        stop = entry - pad
        if pad and (data[stop] not in (0x90, 0x00) or
                    any(stop - 1 <= r["image_offset"] <= stop for r in root.relocations)):
            continue
        for _, lo, hi, ident, accepted in extents:
            if hi != stop or lo >= hi:
                continue
            try:
                rows_ = instructions(data[lo:hi])
            except ValueError:
                continue  # an extent that does not decode linearly cannot anchor
            last = rows_[-1]
            if STRUCTURAL_RETURNS.get(last.bytes[0]) == last.size:
                anchors.append({"anchor": ident, "pad": pad})
            elif (accepted and last.bytes[0] in (0xE9, 0xEB) and
                  any(r.address == last.operands[0].imm & 0xFFFF for r in rows_[:-1])):
                anchors.append({"anchor": ident, "pad": pad, "closing_jump": True})
            elif (pad == 0 and len(rows_) >= 2 and list(last.bytes) == [0x90] and
                  STRUCTURAL_RETURNS.get(rows_[-2].bytes[0]) == rows_[-2].size):
                # The reviewed extent already includes its one alignment NOP after the return:
                # the same single pad byte, counted inside the anchor.
                anchors.append({"anchor": ident, "pad": 1, "pad_inside_anchor": True})
    require(anchors, "Structural entry is not preceded by a decoded return ending a reviewed extent")
    starts = {(lo, ident) for _, lo, _, ident, _ in extents}
    frames = []
    for r in root.relocations:
        at = r["image_offset"] - 3
        if at < 0 or data[at] != 0x9a or (space == "root" and at < end and start < at + 5):
            continue
        woff, wseg = struct.unpack_from("<HH", data, at + 1)
        if wseg != segment or woff == offset:
            continue
        try:
            wpos = root.position(wseg, woff)
        except ValueError:
            continue
        names = [ident for lo, ident in starts if lo == wpos]
        if names:
            frames.append({"image_offset": at, "relocation_index": r["index"], "entry": names[0]})
    require(frames, "Structural entry segment word has no independent relocated CALL FAR to another reviewed entry")
    return {"anchor": anchors[0], "frame_witness": frames[0], "frame_witnesses": len(frames)}


def ground_symbol(symbol, target, oracle, claimed=None):
    """Ground one far callee alias S = SEG:OFF independently of the candidate.

    far-code-entry: a reviewed resident direct-code entry. overlay-code-entry: a
    reviewed direct-code entry in a named overlay (the call relies on that overlay
    being loaded, as the original's direct call does). rtlink-vector: an original
    RTLink vector record in root, whose decoded destination is recorded. In every
    case independent relocated CALL FAR witnesses outside the candidate name S.
    """
    from match import get_target
    kind = symbol.get("kind")
    require(kind in FAR_KINDS, "Unsupported far callee kind")
    if kind == "segment-base":
        return _ground_segment(symbol, target, oracle, claimed)
    if kind == "far-data":
        return _ground_far_data(symbol, target, oracle, claimed)
    require(type(symbol["segment"]) is int and type(symbol["offset"]) is int and
            0 <= symbol["segment"] <= 65535 and 0 <= symbol["offset"] <= 65535,
            "Far code alias must be an exact 16:16 pair")
    space = oracle.spaces[target["space"]]
    start = space.position(target["segment"], target["offset"])
    end = start+target["size"]
    destination = None
    if kind == "rtlink-vector":
        require(symbol.get("space") == "root", "RTLink vectors are resident")
        vector = _vector(oracle, symbol["segment"], symbol["offset"])
        entry, size = vector["position"], 10
        require(target["space"] != "root" or not (entry < end and start < entry+size),
                "Far callee vector overlaps candidate")
        destination = {k: vector[k] for k in ("target_space", "target_segment", "target_offset", "descriptor_index")}
        callee = {"id": f"root:{symbol['segment']:04x}:{symbol['offset']:04x}"}
    else:
        require(kind != "far-code-entry" or symbol.get("space") == "root",
                "Resident far-code-entry refuses overlay/vector aliases (use overlay-code-entry or rtlink-vector)")
        callee, cspace, raw = get_target(symbol["entry_target_id"], oracle)
        expected_space = "root" if kind == "far-code-entry" else symbol.get("space")
        # An accepted exact owner's entry is direct code by its own reconstruction.
        require((callee.get("entry_kind") == "direct-code" or _accepted_entry(callee["id"], oracle)) and
                callee["space"] == expected_space and
                (kind == "far-code-entry") == (expected_space == "root") and
                (callee["segment"], callee["offset"]) == (symbol["segment"], symbol["offset"]) and
                callee["sha256"] == symbol["entry_sha256"],
                "Far callee is not the frozen reviewed direct-code entry")
        entry = cspace.position(callee["segment"], callee["offset"])
        require(cspace.name != space.name or not (entry < end and start < entry+len(raw)),
                "Far callee entry overlaps candidate")
        if space.name != "root" and cspace.name not in ("root", space.name):
            # A direct call from one overlay into another relies on both being resident; the
            # manager evicts any section overlapping a new allocation, so they must not overlap.
            from tu_census import can_coexist
            require(can_coexist(space, cspace), "Direct far call into an overlay that cannot be "
                    "resident together with the caller's overlay")
        _inside_accepted(oracle, cspace.name, entry, len(raw))
    require("consensus" not in symbol or kind != "rtlink-vector" and not symbol.get("witnesses"),
            "Far callee consensus is for a direct entry without CALL witnesses")
    if not symbol.get("witnesses"):
        # Without an outside caller, the entry may instead be the start of accepted exact
        # code elsewhere: its reconstruction independently proves a function starts there.
        if kind == "rtlink-vector":
            # The vector record is itself original evidence of an entry: the census decoder
            # checks its manager-intercept call, relocated far jump and overlay descriptor.
            # Its decoded destination must start a reviewed target (or accepted owner).
            starts = {(name, lo) for name, lo, _, _, _ in _extents(oracle)}
            require((destination["target_space"], vector["target_position"]) in starts,
                    "RTLink vector destination does not start a reviewed target")
            return {"kind": kind, "entry_target_id": callee["id"], "segment": symbol["segment"],
                    "offset": symbol["offset"], "witnesses": [], "grounded_by": "vector-record",
                    "callee_owned_bytes": 0, "vector_destination": destination}
        if _accepted_entry(callee["id"], oracle):
            return {"kind": kind, "entry_target_id": callee["id"], "segment": symbol["segment"],
                    "offset": symbol["offset"], "witnesses": [], "grounded_by": "accepted exact entry",
                    "callee_owned_bytes": 0}
        if _matched_entry(callee["id"]):
            # Entry identity only: the callee's code is exact (a recorded CODE_MATCH of an accepted
            # group); its own unproven data bindings do not bear on where it starts.
            return {"kind": kind, "entry_target_id": callee["id"], "segment": symbol["segment"],
                    "offset": symbol["offset"], "witnesses": [], "grounded_by": "accepted code match entry",
                    "callee_owned_bytes": 0}
        if symbol.get("consensus"):
            # Group rule (2026-11-23): every caller is a claim of this group, so no outside CALL exists. The entry
            # is a frozen reviewed direct-code target; module_gate requires equal relocated CALL FAR fields at two
            # or more distinct instruction starts in CODE_EXACT claims, as for far data and v4 addresses.
            require(symbol["consensus"] is True and claimed is not None and callee.get("entry_kind") == "direct-code" and
                    callee.get("boundary_status") == "REVIEWED",
                    "Far callee consensus is a group rule for a reviewed direct-code entry")
            return {"kind": kind, "entry_target_id": callee["id"], "segment": symbol["segment"],
                    "offset": symbol["offset"], "witnesses": [], "grounded_by": "internal consensus",
                    "callee_owned_bytes": 0}
        # Last resort for a resident callee whose only caller is the candidate: structural
        # original-byte proof that a routine starts there (weaker; recorded as such).
        require(kind == "far-code-entry", "Far callee has no independent original CALL witness or accepted exact entry")
        try:
            receipt = _structural_entry(oracle, entry, symbol["segment"], symbol["offset"], len(raw),
                                        space.name, start, end)
        except ValueError as exc:
            raise ValueError("Far callee has no independent original CALL witness, accepted exact "
                             f"entry or structural entry ({exc})") from exc
        return {"kind": kind, "entry_target_id": callee["id"], "segment": symbol["segment"],
                "offset": symbol["offset"], "witnesses": [], "grounded_by": "structural-entry",
                "structural": receipt, "callee_owned_bytes": 0}
    receipts = []
    for spec in symbol["witnesses"]:
        # A witness may be resident or in an overlay: either way it is an original
        # CALL FAR whose segment word is relocated by that space's own table.
        require(spec["space"] in oracle.spaces, "Unknown far entry witness space")
        wspace = oracle.spaces[spec["space"]]
        at, witness = spec["image_offset"], bytes.fromhex(spec["bytes_hex"])
        require(type(at) is int and len(witness) == 5 and witness[0] == 0x9a and
                0 <= at <= wspace.size-5 and wspace.data[at:at+5] == witness,
                "Far entry witness is not an original CALL FAR")
        # In a same-source group (claimed = positions of the claimed extents) an original
        # relocated CALL FAR in an unclaimed draft region is an original fact the claims are
        # not judged on, as for v4 data witnesses; claiming that region later refuses it.
        spans = claimed if claimed is not None else [(start, end)]
        require(wspace.name != space.name or not any(at < hi and lo < at+5 for lo, hi in spans),
                "Far entry witness overlaps candidate")
        offset, segment = struct.unpack_from("<HH", witness, 1)
        require((segment, offset) == (symbol["segment"], symbol["offset"]),
                "Independent far CALL does not ground the exact callee alias")
        relocs = [r for r in wspace.relocations if at-1 <= r["image_offset"] < at+5]
        require(len(relocs) == 1 and relocs[0]["image_offset"] == at+3 and
                relocs[0]["index"] == spec["relocation_index"] and
                spec["file_offset"] == wspace.file_offset+at,
                "Independent far CALL relocation/file provenance differs")
        receipts.append({"space": wspace.name, "image_offset": at, "relocation_index": relocs[0]["index"]})
    return {"kind": kind, "entry_target_id": callee["id"], "segment": symbol["segment"],
            "offset": symbol["offset"], "witnesses": receipts, "callee_owned_bytes": 0,
            **({"vector_destination": destination} if destination else {})}


NEAR_KIND = "near-code-entry"


def ground_near_entry(symbol, hull, oracle):
    """Ground the position of an external near callee independently of the candidate.

    The symbol names a reviewed direct-code entry (`entry_target_id`, frozen extent hash)
    in the object's own space, outside the whole object and outside accepted code other
    than an owner starting there. Its position is the start of an accepted CODE_EXACT,
    ASM_EXACT or PINNED_RUNTIME owner, or is reached by original near CALL witnesses
    outside the whole object: `E8 rel16` (optionally after its PUSH CS) whose bytes,
    image offset and instruction starts are rechecked, reached by recursive descent from
    an independent entry (`reachable_from_entry`), and whose rel16 lands on the entry in
    that entry's CS window. The candidate's own call can never ground its callee. Whether
    the candidate's call reaches the entry at runtime is a separate CS-window check."""
    from binding import reachable_from_entry
    from match import get_target
    require(symbol.get("kind") == NEAR_KIND and type(symbol.get("segment")) is int and
            type(symbol.get("offset")) is int and 0 <= symbol["segment"] <= 65535 and
            0 <= symbol["offset"] <= 65535, "Near code alias must be an exact 16:16 pair")
    space = oracle.spaces[hull["space"]]
    start = space.position(hull["segment"], hull["offset"])
    end = start + hull["size"]
    require(symbol.get("space") == space.name, "Near callee must lie in the object's own space")
    callee, cspace, raw = get_target(symbol["entry_target_id"], oracle)
    accepted = _accepted_entry(callee["id"], oracle)
    require((callee.get("entry_kind") == "direct-code" or accepted) and cspace.name == space.name and
            (callee["segment"], callee["offset"]) == (symbol["segment"], symbol["offset"]) and
            callee["sha256"] == symbol["entry_sha256"],
            "Near callee is not the frozen reviewed direct-code entry")
    entry = space.position(callee["segment"], callee["offset"])
    require(not (entry < end and start < entry + len(raw)), "Near callee entry overlaps the object")
    _inside_accepted(oracle, space.name, entry, len(raw))
    receipts = []
    for spec in symbol.get("witnesses", []):
        require(spec.get("space") == space.name, "Near callee witness must lie in the callee's space")
        at, witness = spec["image_offset"], bytes.fromhex(spec["bytes_hex"])
        width = len(witness)
        require(type(at) is int and width in (3, 4) and witness[-3] == 0xe8 and
                (width == 3 or witness[0] == 0x0e) and 0 <= at <= space.size - width and
                space.data[at:at + width] == witness,
                "Near callee witness is not an original near CALL")
        require(not (at < end and start < at + width), "Near callee witness lies inside the object")
        call_at = at + width - 3
        entry_space, entry_segment, _ = spec["entry"].split(":")
        reached = reachable_from_entry(oracle, spec["entry"])
        require(entry_space == space.name and call_at in reached and at in reached,
                "Near callee witness is not reached code")
        base = (int(entry_segment, 16) - space.link_segment) * 16
        destination = base + ((call_at - base + 3 + struct.unpack_from("<H", witness, width - 2)[0]) & 0xFFFF)
        require(destination == entry, "Independent near CALL does not reach the callee entry")
        receipts.append({"space": space.name, "image_offset": at, "entry": spec["entry"]})
    require(receipts or accepted,
            "Near callee has no independent original near CALL witness or accepted exact entry")
    return {"kind": NEAR_KIND, "entry_target_id": callee["id"], "position": entry,
            "grounded_by": "original near CALL" if receipts else "accepted exact entry",
            "witnesses": receipts, "callee_owned_bytes": 0}


ORDER_DEBT = ["Between-target-symbol relocation group order and combined output-segment placement require structural linker proof"]


def relocation_order(relocs, fields, frozen_indices):
    """Project RTLink's output-segment/symbol buckets onto one whole CODE owner.

    `fields` retains OMF FIXUPP traversal, never site-sorted. Distinct external
    symbols remain distinct even when their grounded 16:16 addresses coincide.
    No between-symbol bucket permutation is inferred from the local object.
    """
    original_sites = [r["image_offset"] for r in relocs]
    sites = [f["site"] for f in fields]
    require(len(original_sites) == len(sites) == len(set(original_sites)) == len(set(sites)) and
            set(original_sites) == set(sites),
            "Original MZ relocation site set differs from far segment fields")
    indices = [r["index"] for r in relocs]
    require(all(type(i) is int and i >= 0 for i in indices) and len(indices) == len(set(indices)),
            "Duplicate/invalid original relocation index")
    require(indices == frozen_indices, "Frozen original relocation indices changed")
    by_site = {f["site"]: f for f in fields}
    groups = []
    # RTLink keys a relocation group by its field kind and target symbol: base16 (LOC2, `mov ax,seg X`)
    # and pointer32 (LOC3, `call far X`) references to one symbol are separate groups (o9_linktools
    # probes: flags 048Bh vs 0C8Bh in the group key, 1,709 predicted relocations exact).
    key = lambda f: (f["symbol"], f.get("field", "pointer32"))
    names = list(dict.fromkeys(key(f) for f in fields))
    original_names = [key(by_site[r["image_offset"]]) for r in relocs]
    runs = [n for i, n in enumerate(original_names) if i == 0 or original_names[i-1] != n]
    require(len(runs) == len(set(runs)), "Original MZ target-symbol groups are interleaved")
    for name in names:
        generated = [f for f in fields if key(f) == name]
        observed = [r for r in relocs if key(by_site[r["image_offset"]]) == name]
        require([f["site"] for f in generated] == [r["image_offset"] for r in observed],
                "Original MZ within-symbol FIXUPP order differs: " + name[0])
        groups.append({"symbol": name[0], "field": name[1],
                       "fixup_operand_offsets": [f["offset"] for f in generated],
                       "segment_word_sites": [f["site"] for f in generated],
                       "original_relocation_indices": [r["index"] for r in observed],
                       "within_group_order": "EXACT"})
    qualification = "EXACT" if original_sites == sites else "GROUPED"
    index_by_site = {r["image_offset"]: r["index"] for r in relocs}
    return {"relocation_order": qualification, "relocation_groups": groups,
            "generated_relocation_indices": [index_by_site[f["site"]] for f in fields],
            "relocation_order_debt": list(ORDER_DEBT) if qualification == "GROUPED" else [],
            "relocation_order_scope": "Exact site bijection and contiguous target-symbol groups within one CODE contribution; exact FIXUPP order inside each group; no natural between-group/output-segment placement proof"}


def compare(module, code_name, actual, expected, binding, target, space, oracle):
    require(binding["schema"] == 1 and binding["mode"] == "external-far-call-v1" and
            binding["target_id"] == target["id"] and binding["target_sha256"] == sha(oracle.data),
            "Unsupported/foreign far-call binding")
    require(target["space"] == space.name == "root", "Far-call v1 requires a root owner; overlays refused")
    require(len(actual) == len(expected) == target["size"], "Far component extent differs; trimming forbidden")
    obligations = fixes(module)
    require(obligations and obligations == binding["fixups"], "Ordered far symbolic fixup obligations changed")
    require(declarations(module) == binding["declarations"], "Far object declarations changed")
    symbols = binding["symbols"]
    require(set(symbols) == {f["target"].get("name") for f in obligations},
            "Missing or unused grounded far/data symbol")
    far_symbols = {n: s for n, s in symbols.items() if s.get("kind") in FAR_KINDS}
    data_symbols = {n: s for n, s in symbols.items() if s.get("kind") == "near-data-alias"}
    require(far_symbols and len(far_symbols)+len(data_symbols) == len(symbols),
            "Far binding refuses overlay/vector/unknown code aliases")
    grounding = {n: ground_symbol(s, target, oracle) for n, s in far_symbols.items()}
    if data_symbols:
        data = binding.get("data", {})
        require(data.get("mode") in ("external-dgroup-offset16-v1", "external-dgroup-word-pair-offset16-v2"),
                "Composed far binding needs an explicit resident DGROUP mode")
        ground_data({**binding, **data, "symbols": data_symbols}, target, oracle)
    else:
        require("data" not in binding, "Unused DGROUP proof in far binding")
    rows = instructions(actual)
    cs, _ = _capstone()
    occupied, equations, differences, sites, allowed_calls = set(), [], [], [], set()
    for fix in obligations:
        at, ref = fix["segment_offset"], fix["target"]
        if ref.get("name") in data_symbols:
            continue
        require(fix["segment"] == code_name and fix["loc_type"] == 3 and fix["field_width"] == 4 and
                not fix["self_relative"] and ref["kind"] == "external" and ref["method"] == 6 and
                fix["displacement"] is None and fix["frame"] == {"kind": "target_frame", "target": ref},
                "Unsupported far CALL fixup equation/frame/displacement")
        require(type(at) is int and 1 <= at <= len(actual)-4 and
                not occupied.intersection(range(at, at+4)), "Overlapping/out-of-range far fields")
        require(any(i.address == at-1 and i.size == 5 and bytes(i.bytes) == b"\x9a"+actual[at:at+4]
                    for i in rows), "Far fixup is not a complete unprefixed 9A CALL operand")
        require(actual[at:at+4] == bytes(4), "Only zero encoded far addends are proven")
        symbol = far_symbols[ref["name"]]
        value = (symbol["offset"], symbol["segment"])
        observed = struct.unpack_from("<HH", expected, at)
        if value != observed:
            differences.append(at)
        equations.append({"kind": "far-call", "offset": at, "symbol": ref["name"],
                          "encoded_addend": 0, "explicit_displacement": None,
                          "linked_offset": value[0], "linked_segment": value[1],
                          "original_offset": observed[0], "original_segment": observed[1],
                          "equal": value == observed})
        occupied.update(range(at, at+4))
        allowed_calls.add(at-1)
        sites.append({"field": "pointer32", "site": space.position(target["segment"], target["offset"])+at+2,
                      "offset": at, "symbol": ref["name"]})
    # No literal or indirect call may bypass the supported symbolic obligations.
    for insn in rows:
        require(not set(insn.groups).intersection((cs.CS_GRP_CALL, cs.CS_GRP_INT, cs.CS_GRP_IRET)) or
                insn.address in allowed_calls, "Unproved call/interrupt in far-bound code")
        require(insn.mnemonic not in ("ljmp", "jmpf"), "Unproved far jump/vector transfer")
    if not data_symbols:
        # Reuse the symbolic-address audit even when no DS fixups were emitted.
        data_equations([], {}, code_name, actual, expected, rows,
                       "external-dgroup-offset16-v1", occupied)
    if data_symbols:
        data_context(module, rows, allowed_calls)
        data_fixes = [f for f in obligations if f["target"].get("name") in data_symbols]
        data_rows, data_diff = data_equations(data_fixes, data_symbols, code_name, actual, expected,
                                            rows, binding["data"]["mode"], occupied)
        equations.extend({"kind": "dgroup-offset16", **e} for e in data_rows)
        differences.extend(data_diff)
    start = space.position(target["segment"], target["offset"])
    relocs = [r for r in space.relocations if start-1 <= r["image_offset"] < start+len(expected)]
    order_proof = relocation_order(relocs, sites, binding["relocation_indices"])
    indices = [r["index"] for r in relocs]
    # Retain complete FIXUPP order even when data/far equations were evaluated separately.
    order = {f["segment_offset"]: i for i, f in enumerate(obligations)}
    equations.sort(key=lambda e: order[e["offset"]])
    ordinary = [i for i, (a, b) in enumerate(zip(actual, expected)) if i not in occupied and a != b]
    field_count = len(differences)
    differences.extend(ordinary)
    return {**order_proof, "exact": not differences, "first_difference": min(differences, default=None),
            "mismatch_count": len(differences), "mismatch_basis": "ordinary bytes plus unequal field equations",
            "ordinary_mismatch_count": len(ordinary), "field_mismatch_count": field_count,
            "fixups": len(obligations), "relocations": len(relocs), "equations": equations,
            "covered_relocation_indices": indices if not differences else [],
            "original_relocations": relocs,
            "ordinary_bytes_compared": len(actual)-len(occupied), "symbol_grounding": grounding,
            "proof_scope": "Complete root component bytes, external zero-addend CALL FAR equations to independently witnessed reviewed direct entries, exact original MZ segment-word coverage with within-target-symbol FIXUPP order (between-group order is structural debt); optional resident DGROUP equations hold for every DS; raw OMF preserved, original symbols/TU/runtime DS/natural placement/overlay vectors/whole link unproved"}
