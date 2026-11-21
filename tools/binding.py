"""Narrow external DGROUP offset proof. Never edits objects or oracle bytes.

Every field is checked by a linker equation and every other byte by equality.
This is component proof only: symbol aliases, not original PUBLIC names; no
natural placement, TU identity, DS runtime trace, or complete link is claimed.
"""
import struct

from common import require, sha
from disasm import _capstone


def instructions(raw):
    cs, _ = _capstone()
    engine = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_16)
    engine.detail = True
    rows = list(engine.disasm(raw, 0))
    require(sum(i.size for i in rows) == len(raw), "Binding code contains undecoded bytes")
    return rows


def direct_operand(insn):
    """One unprefixed, direct default-DS memory operand with a 16-bit offset."""
    _, xc = _capstone()
    memory = [o for o in insn.operands if o.type == xc.X86_OP_MEM]
    require(len(memory) == 1 and not any(insn.prefix) and insn.disp_size == 2,
            "Binding needs an unprefixed direct DS operand")
    operand = memory[0]
    require(operand.mem.segment == operand.mem.base == operand.mem.index == 0,
            "Binding memory operand is not direct default-DS")
    return operand


IMMEDIATE_MODE = "external-dgroup-offset16-v3"
# v4: byte-level DGROUP offset fields. Any decoded disp16/imm16 operand field, any
# encoded addend: the linker writes S + A. S (the symbol's DGROUP offset) is grounded
# by an outside original instruction whose disp16/imm16 field equals it exactly.
# Proves code bytes only; data extents, types and the runtime DS association are not claimed.
OPERAND_MODE = "external-dgroup-offset16-v4"


_REACH = {}


def reachable_from_entry(oracle, entry):
    """Instruction starts reached from a root entry that some original relocated CALL FAR targets.

    Recursive descent within the entry's 64K CS window: conditional branches and loops
    follow both edges, JMP follows its target, calls fall through, returns and indirect
    jumps stop. Result is cached per entry."""
    key = (id(oracle), entry)
    if key in _REACH:
        return _REACH[key]
    space_name, seg_text, off_text = entry.split(":")
    require(space_name in oracle.spaces, "Unknown v4 witness entry space")
    root = oracle.spaces[space_name]  # the entry's own space (resident or one overlay)
    segment, offset = int(seg_text, 16), int(off_text, 16)
    start = root.position(segment, offset)
    def reviewed_entry():
        from common import ROOT, read_json
        for t in read_json(ROOT / "evidence/targets.json")["functions"]:
            if (t["space"] == space_name and t.get("boundary_status") == "REVIEWED" and
                    t.get("entry_kind", "direct-code") == "direct-code" and
                    0 <= (t["segment"] - root.link_segment) * 16 + t["offset"] == start):
                return True
        return False

    far_entry = any(at >= 0 and root.data[at] == 0x9a and
                    root.data[at+1:at+5] == struct.pack("<HH", offset, segment)
                    for at in (r["image_offset"] - 3 for r in root.relocations))

    def vector_destination():
        # An overlay entry named by an original RTLink vector record.
        if space_name == "root":
            return False
        from tu_census import overlay_vectors
        overlays = {n: s for n, s in oracle.spaces.items() if s.descriptor and s.descriptor["flags"] == 4}
        return any(v["target_space"] == space_name and (v["target_segment"], v["target_offset"]) == (segment, offset)
                   for v in overlay_vectors(oracle, overlays).values())

    require(far_entry or reviewed_entry() or vector_destination(),
            "v4 witness entry is not a relocated CALL FAR destination, reviewed entry or vector target")
    cs, _ = _capstone()
    engine = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_16)
    engine.detail = True
    base = (segment - root.link_segment) * 16  # space-relative position of the CS window
    seen, work = set(), [start]
    while work:
        pos = work.pop()
        while base <= pos < min(base + 0x10000, root.size) and pos not in seen:
            insn = next(engine.disasm(root.data[pos:pos + 16], pos - base), None)
            if insn is None:
                break
            seen.add(pos)
            groups = set(insn.groups)
            # Prefixes (CS:, REP, ...) may decorate the mnemonic, so test the last word.
            op_name = insn.mnemonic.split()[-1]
            if groups.intersection((cs.CS_GRP_RET, cs.CS_GRP_IRET)) or op_name in ("ljmp", "hlt", "iret"):
                break
            if cs.CS_GRP_CALL in groups and op_name == "call" and insn.operands and insn.operands[0].type == 2:
                # A direct near call's target is code in the same CS window; the call returns.
                work.append(base + (insn.operands[0].imm & 0xFFFF))
            branch = cs.CS_GRP_JUMP in groups or op_name.startswith("loop") or op_name in ("jcxz", "jecxz")
            if branch:
                op = insn.operands[0] if insn.operands else None
                if op is not None and op.type == 2:  # immediate near target, both edges followed
                    work.append(base + (op.imm & 0xFFFF))
                if op_name == "jmp":
                    break  # unconditional: direct target queued above, indirect stops
            pos += insn.size
    _REACH[key] = seen
    return seen


# Immediates that may carry a data address: MOV r16/m16, imm16 and PUSH imm16.
# Branch displacements, RET/RETF n, ENTER and arithmetic constants never qualify.
_ADDRESS_IMMEDIATE = ("mov", "push")


def operand_field(insn, at, *, candidate=False):
    """The 16-bit address field (memory disp16 or address-carrying imm16) starting at `at`.

    For a candidate field the compiler's FIXUPP already marks it as an address, so any
    non-branch imm16 (ADD/SUB/CMP ... offset X) qualifies; an original witness has no
    FIXUPP, so only MOV/PUSH immediates count there."""
    cs, xc = _capstone()
    if set(insn.groups).intersection((cs.CS_GRP_JUMP, cs.CS_GRP_CALL, cs.CS_GRP_RET,
                                      cs.CS_GRP_IRET, cs.CS_GRP_INT)):
        # An indirect near/far CALL or JMP through memory (FF /2../5) reads a pointer stored at
        # its disp16: that field is a data address like any other memory operand. Direct
        # branch targets, returns and interrupts never are.
        indirect = (insn.opcode[0] == 0xff and (insn.modrm >> 3) & 7 in (2, 3, 4, 5) and
                    any(o.type == xc.X86_OP_MEM for o in insn.operands))
        if indirect and insn.disp_size == 2 and insn.address + insn.disp_offset == at:
            return "displacement"
        return None
    if insn.disp_size == 2 and insn.address + insn.disp_offset == at:
        return "displacement"
    if (insn.imm_size == 2 and insn.address + insn.imm_offset == at and
            (candidate or insn.mnemonic.split()[-1] in _ADDRESS_IMMEDIATE)):
        return "immediate"
    return None


def immediate_base(insn):
    """The freshly observed 16-bit MOV SI/DI,imm16 address fields only."""
    _, xc = _capstone()
    require(len(insn.bytes) == 3 and insn.bytes[0] in (0xbe, 0xbf) and
            not any(insn.prefix) and insn.imm_offset == 1 and insn.imm_size == 2 and
            len(insn.operands) == 2 and insn.operands[0].type == xc.X86_OP_REG and
            insn.operands[0].reg in (xc.X86_REG_SI, xc.X86_REG_DI) and
            insn.operands[1].type == xc.X86_OP_IMM,
            "Binding needs MOV SI/DI,imm16 as a string address base")
    return insn.operands[0].reg, insn.operands[1].imm & 0xffff


def preserve_segments(rows, allowed_calls=(), *, es_ds=False):
    cs, xc = _capstone()
    for n, insn in enumerate(rows):
        pair = (es_ds and n > 0 and bytes(insn.bytes) == b"\x07" and
                bytes(rows[n-1].bytes) == b"\x1e")
        forbidden = set(insn.groups).intersection((cs.CS_GRP_CALL, cs.CS_GRP_INT, cs.CS_GRP_IRET))
        segments = (xc.X86_REG_DS, xc.X86_REG_ES, xc.X86_REG_SS, xc.X86_REG_CS)
        if es_ds:
            # The 16-bit decoder also recognizes later FS/GS instructions.
            # v3 permits only ES:=DS; leave the legacy modes unchanged.
            segments += (xc.X86_REG_FS, xc.X86_REG_GS)
        writes = set(insn.regs_access()[1]).intersection(segments)
        # Capstone omits implicit segment writes for LFS/LGS/LSS and marks
        # even POP ES privileged. Check the actual context writers explicitly.
        modern_context = es_ds and insn.mnemonic in (
            "lfs", "lgs", "lss", "sysenter", "sysexit", "syscall", "sysret", "sysretq", "rsm")
        require((insn.address in allowed_calls or not forbidden) and not modern_context and
                insn.mnemonic not in ("int", "into", "iret", "iretd", "lds", "les") and
                (not writes or (pair and writes == {xc.X86_REG_ES})),
                "Near-data binding helper changes segment context or calls unknown code")


def string_context(rows, address_fields):
    """Straight-line ES:=DS idiom with symbolic SI/DI bases and literal counts.

    The count is a transfer footprint, never an allocation/forward-DF proof.
    No generic segment dataflow, branches, pointer arithmetic or prefixes.
    """
    cs, xc = _capstone()
    bases, transferred = {}, {at: 0 for at in address_fields}
    es_ds, pairs, count, returned = False, 0, None, False
    string_names = {stem + suffix for stem in ("movs", "stos", "lods", "cmps", "scas", "ins", "outs")
                    for suffix in ("b", "w", "d", "q")}
    for n, insn in enumerate(rows):
        raw = bytes(insn.bytes)
        require(not set(insn.groups).intersection((cs.CS_GRP_JUMP, cs.CS_GRP_CALL,
                                                   cs.CS_GRP_INT, cs.CS_GRP_IRET)) and
                not insn.mnemonic.startswith("loop"), "Immediate binding needs straight-line code")
        require(not returned or raw == b"\x90", "Only alignment NOPs may follow the return")
        if raw == b"\x1e":
            require(n+1 < len(rows) and bytes(rows[n+1].bytes) == b"\x07",
                    "PUSH DS must be immediately followed by POP ES")
        if raw == b"\x07":
            require(n > 0 and bytes(rows[n-1].bytes) == b"\x1e",
                    "POP ES lacks the exact PUSH DS pair")
            es_ds, pairs = True, pairs+1
        is_string = insn.mnemonic.split()[-1] in string_names
        if is_string:
            require(raw in (b"\xa4", b"\xa5", b"\xf3\xa5", b"\xf3\xaa"),
                    "Only MOVSB/MOVSW, F3 MOVSW and F3 STOSB strings are supported")
            require(es_ds and pairs == 1, "String destination lacks the reached ES=DS pair")
            repeat = raw[0] == 0xf3
            require(not repeat or (count is not None and count > 0),
                    "REP string needs a positive literal CX count")
            amount = (count if repeat else 1) * (2 if raw[-1] in (0xa5, 0xab) else 1)
            regs = [xc.X86_REG_DI] + ([xc.X86_REG_SI] if raw[-1] in (0xa4, 0xa5) else [])
            for reg in regs:
                require(reg in bases, "String pointer lacks a symbolic immediate address base")
                transferred[bases[reg]] += amount
            if repeat:
                count = 0
            continue
        require(not any(insn.prefix), "Immediate binding refuses non-string prefixes")
        writes = set(insn.regs_access()[1])
        for reg in (xc.X86_REG_SI, xc.X86_REG_DI):
            if reg in writes:
                bases.pop(reg, None)
        if xc.X86_REG_CX in writes or xc.X86_REG_CL in writes or xc.X86_REG_CH in writes:
            count = None
        if len(raw) == 3 and raw[0] in (0xbe, 0xbf):
            reg, _ = immediate_base(insn)
            field = insn.address+insn.imm_offset
            require(field in address_fields, "Immediate address lacks a symbolic candidate fixup")
            bases[reg] = field
        if len(raw) == 3 and raw[0] == 0xb9:
            count = struct.unpack_from("<H", raw, 1)[0]
        if cs.CS_GRP_RET in insn.groups:
            returned = True
    require(pairs == 1 and transferred and all(0 < n <= 65536 for n in transferred.values()),
            "Immediate address fields must feed the single ES=DS string idiom")
    return transferred


def declarations(module):
    return {"segments": module.segment_defs, "groups": module.groups,
            "publics": module.publics, "externals": module.externals}


def fixes(module):
    return [e for r in module.fixups for e in r["resolved"] if e["kind"] == "fixup"]


def ground(binding, target, oracle, claimed=None):
    """Recheck frozen original witnesses outside the contribution being judged.

    claimed (same-source groups only): positions (start, end) of the claimed extents. A v4
    symbol witness may then also be an original instruction in an unclaimed draft region of
    the object, and a v4 address may have no witness at all when the group proves it by
    internal consensus (module_gate checks the second claimed site)."""
    require(binding["schema"] == 1 and binding["mode"] in ("external-dgroup-offset16-v1", "external-dgroup-word-pair-offset16-v2",
                                                        "external-overlay-dgroup-offset16-v1", IMMEDIATE_MODE,
                                                        OPERAND_MODE),
            "Unsupported binding proof mode")
    require(binding["target_id"] == target["id"] and
            binding["target_sha256"] == sha(oracle.data), "Binding belongs to another target/oracle")
    overlay_mode = binding["mode"] == "external-overlay-dgroup-offset16-v1"
    # v4 proves offset bytes only (S + k with S grounded by original fields), so the owner may
    # be resident or overlay code: the overlay runs with the same startup DGROUP in DS.
    require(target["space"] in oracle.spaces and
            (target["space"].startswith("overlay-") if overlay_mode else
             target["space"] == "root" or (binding["mode"] == OPERAND_MODE and
                                           target["space"].startswith("overlay-"))),
            "Unsupported owner space for DGROUP binding mode")
    space = oracle.spaces["root"]
    start = oracle.spaces[target["space"]].position(target["segment"], target["offset"])
    end = start + target["size"]

    def witness(spec, draft_ok=False):
        # v4 witnesses may sit in overlay code (reached from that overlay's own entries);
        # every other mode and the DGROUP frame witness stay resident.
        overlay_ok = binding["mode"] == OPERAND_MODE and "entry" in spec and spec["space"] in oracle.spaces
        require(spec["space"] == "root" or overlay_ok, "Unsupported cross-space data witness")
        wspace = oracle.spaces[spec["space"]]
        at, raw = spec["image_offset"], bytes.fromhex(spec["bytes_hex"])
        same = spec["space"] == target["space"]
        inside = ([(lo, hi) for lo, hi in claimed] if draft_ok and claimed is not None else [(start, end)])
        require(raw and 0 <= at and at+len(raw) <= wspace.size and
                not (same and any(at < hi and lo < at+len(raw) for lo, hi in inside)),
                "Binding witness overlaps candidate or escapes its space")
        require(wspace.data[at:at+len(raw)] == raw, "Original binding witness changed")
        return at, raw

    frame = binding["frame"]
    at, raw = witness(frame["witness"])
    # Only MOV r16,paragraph / MOV DS,r16 is supported.
    require(len(raw) == 5 and 0xb8 <= raw[0] <= 0xbf and
            raw[3:] == bytes((0x8e, 0xd8+raw[0]-0xb8)) and
            struct.unpack_from("<H", raw, 1)[0] == frame["segment"] and
            sum(r["image_offset"] == at+1 for r in space.relocations) == 1,
            "DGROUP frame lacks a relocated startup DS load")
    require(0 <= frame["segment"] <= 0xffff and
            frame["segment"]*16 < space.size, "DGROUP frame outside root")
    entry_at, entry_raw = witness(frame["entry_path"])
    entry = space.position(oracle.mz.header["initial_cs"], oracle.mz.header["initial_ip"])
    entry_rows = instructions(entry_raw)
    require(entry_at == entry and entry_at+len(entry_raw) == at+len(raw) and
            entry_raw.endswith(raw) and
            len(entry_rows) >= 2 and entry_rows[-2].address == at-entry_at and
            bytes(entry_rows[-2].bytes) == raw[:3] and bytes(entry_rows[-1].bytes) == raw[3:] and
            all(not set(i.groups).intersection((1, 2, 3, 4, 5, 7)) for i in entry_rows),
            "DS load is not reached by the frozen straight-line MZ entry prefix")
    symbols = binding["symbols"]
    spans = set()
    for name, symbol in symbols.items():
        if binding["mode"] == OPERAND_MODE:
            offset = symbol["offset"]
            require(symbol["kind"] == "dgroup-offset" and symbol["group"] == "DGROUP" and
                    type(offset) is int and 0 <= offset < 65536, "Unsupported v4 DGROUP symbol")
            members = symbol.get("members", [])
            internal = claimed is not None  # a group may prove an address by internal consensus
            require(type(symbol.get("witnesses")) is list and (symbol["witnesses"] or internal),
                    "DGROUP symbol has no independent witness")
            require(all(type(m.get("offset")) is int and 0 < m["offset"] < 65536 and
                        type(m.get("witnesses")) is list and (m["witnesses"] or internal)
                        for m in members) and len({m["offset"] for m in members}) == len(members),
                    "Every v4 member needs a positive offset and its own witnesses")
            # No linked value S+k of one symbol may equal T+j of another (aliasing).
            values = {offset} | {(offset + m["offset"]) & 0xFFFF for m in members}
            require(not spans.intersection(values), "Two v4 symbols share one DGROUP offset")
            spans.update(values)
            specs = [(spec, offset) for spec in symbol["witnesses"]] +                     [(spec, (offset + m["offset"]) & 0xFFFF) for m in members for spec in m["witnesses"]]
            for spec, expected_value in specs:
                at, raw = witness(spec, draft_ok=True)
                rows = instructions(raw)
                require(len(rows) == 1, "Witness must be one complete instruction")
                if "entry" in spec:
                    # Immediates and indexed operands must be real code: reached by control flow
                    # from a relocated CALL FAR destination or a reviewed entry.
                    require(spec["entry"].split(":")[0] == spec["space"] and
                            at in reachable_from_entry(oracle, spec["entry"]),
                            "v4 witness is not reached from its entry")
                else:
                    # Without an entry, only the v1 witness shape: one unprefixed direct
                    # default-DS memory operand (the project's long-standing witness standard).
                    direct_operand(rows[0])
                    require(rows[0].disp_offset == spec["operand_offset"],
                            "Unreached v4 witness must be a direct DS memory operand")
                field = operand_field(rows[0], spec["operand_offset"])
                require(field is not None and
                        struct.unpack_from("<H", raw, spec["operand_offset"])[0] == expected_value,
                        "Witness operand field does not equal this DGROUP address")
            continue
        if binding["mode"] == IMMEDIATE_MODE and symbol["kind"] == "near-data-address-alias":
            offset = symbol["offset"]
            require(symbol["group"] == "DGROUP" and type(offset) is int and 0 <= offset < 65536 and
                    "width" not in symbol and "members" not in symbol,
                    "Unsupported grounded address alias (no width is inferred)")
            require(offset not in spans, "Grounded data aliases overlap")
            spans.add(offset)
            require(symbol.get("witnesses"), "Address alias has no independent witness")
            for spec in symbol["witnesses"]:
                at, raw = witness(spec)
                rows = instructions(raw)
                require(len(rows) == 1, "Address witness must be one complete instruction")
                if spec["operand_kind"] == "direct":
                    operand = direct_operand(rows[0])
                    require(rows[0].disp_offset == spec["operand_offset"] and
                            (operand.mem.disp & 0xffff) == offset,
                            "Data witness does not ground this address")
                else:
                    require(spec["operand_kind"] == "immediate", "Unsupported address witness operand")
                    _, value = immediate_base(rows[0])
                    require(spec["operand_offset"] == rows[0].imm_offset and value == offset,
                            "Immediate witness does not ground this address")
                    use_at, use_raw = witness(spec["base_use"])
                    use_rows = instructions(use_raw)
                    relative = at-use_at
                    require(any(i.address == relative and bytes(i.bytes) == raw for i in use_rows),
                            "Immediate witness is not an instruction in its original base use")
                    require(not any(use_at-1 <= r["image_offset"] < use_at+len(use_raw)
                                    for r in space.relocations), "Address base use touches an MZ relocation")
                    preserve_segments(use_rows, es_ds=True)
                    fields = {i.address+i.imm_offset for i in use_rows
                              if len(i.bytes) == 3 and i.bytes[0] in (0xbe, 0xbf)}
                    transfers = string_context(use_rows, fields)
                    require(type(spec["transfer_bytes"]) is int and
                            transfers.get(relative+rows[0].imm_offset) == spec["transfer_bytes"],
                            "Independent string transfer footprint changed")
            continue
        offset, width = symbol["offset"], symbol.get("width")
        require(symbol["kind"] == "near-data-alias" and symbol["group"] == "DGROUP" and
                type(offset) is int and type(width) is int and
                (width in (1, 2) or (width == 4 and binding["mode"] == "external-dgroup-word-pair-offset16-v2")) and
                0 <= offset and offset+width <= 65536, "Unsupported grounded data object")
        span = set(range(offset, offset+width))
        require(not spans.intersection(span), "Grounded data aliases overlap")
        spans.update(span)
        if width == 4:
            require(symbol.get("members") and
                    [m.get("offset") for m in symbol["members"]] == [0, 2] and
                    all(type(m.get("offset")) is int and type(m.get("width")) is int and
                        m["width"] == 2 for m in symbol["members"]),
                    "Word pair must cover exactly two reviewed word members")
            reviewed = symbol["members"]
        else:
            reviewed = [{"offset": 0, "width": width, "witnesses": symbol.get("witnesses")}]
        for member in reviewed:
            require(member["witnesses"], "Data member has no independent witness")
            for spec in member["witnesses"]:
                _, raw = witness(spec)
                rows = instructions(raw)
                require(len(rows) == 1, "Data witness must be one complete instruction")
                operand = direct_operand(rows[0])
                require(rows[0].disp_offset == spec["operand_offset"] and
                        operand.size == member["width"] and
                        (operand.mem.disp & 0xffff) == offset+member["offset"],
                        "Data witness does not ground this member address/width")
            # A witness grounds DS-relative offset/width only. Its annotation
            # cannot prove that runtime DS still equals the startup paragraph.
    return symbols


def data_context(module, rows, allowed_calls=(), *, mode=None, placed=()):
    """Preserve DS; only explicit v3 permits the adjacent ES:=DS pair (v4: hygiene only)."""
    if mode == OPERAND_MODE:
        # Byte-level mode: the same DGROUP hygiene as v1, but no DS association is claimed.
        groups = [g for g in module.groups if g["name"] == "DGROUP"]
        require(len(groups) == 1 and groups[0]["segments"] and
                all(1 <= i <= len(module.segment_defs) and
                    (module.segment_defs[i-1]["length"] == 0 or module.segment_defs[i-1]["name"] in placed)
                    for i in groups[0]["segments"]),
                "External data binding needs an empty declared DGROUP (except placed segments)")
        return
    groups = [g for g in module.groups if g["name"] == "DGROUP"]
    require(len(groups) == 1 and groups[0]["segments"] and
            all(1 <= i <= len(module.segment_defs) and
                module.segment_defs[i-1]["class"] in ("DATA", "BSS", "CONST") and
                module.segment_defs[i-1]["length"] == 0 for i in groups[0]["segments"]),
            "External data binding needs an empty declared DGROUP")
    preserve_segments(rows, allowed_calls, es_ds=mode == IMMEDIATE_MODE)



def data_equations(obligations, symbols, code_name, actual, expected, rows, mode, occupied):
    """Check data fields without constructing or modifying a linked byte buffer."""
    _, xc = _capstone()
    equations, differences, address_fields = [], [], set()
    for fix in obligations:
        at = fix["segment_offset"]
        target_ref = fix["target"]
        # v4 also accepts MASM's explicit DGROUP frame (F1 naming the GRPDEF DGROUP, from
        # `ASSUME DS:DGROUP`): S - DGROUP + A is the same grounded DGROUP offset equation.
        dgroup_frame = (mode == OPERAND_MODE and fix["frame"].get("kind") == "group" and
                        fix["frame"].get("name") == "DGROUP")
        require(fix["segment"] == code_name and fix["loc_type"] == 1 and
                fix["field_width"] == 2 and not fix["self_relative"] and
                target_ref["kind"] == "external" and target_ref["method"] == 6 and
                fix["displacement"] is None and
                (fix["frame"] == {"kind": "target_frame", "target": target_ref} or dgroup_frame),
                "Unsupported DGROUP fixup equation")
        require(0 <= at <= len(actual)-2 and not occupied.intersection((at, at+1)),
                "Overlapping/out-of-range bound fields")
        symbol = symbols[target_ref["name"]]
        if mode == OPERAND_MODE:
            fields = [operand_field(i, at, candidate=True) for i in rows if operand_field(i, at, candidate=True)]
            require(len(fields) == 1, "v4 FIXUPP is not the disp16/imm16 field of one decoded instruction")
            addend = struct.unpack_from("<H", actual, at)[0]
            # A non-zero addend must be a member whose own address S+k is independently
            # witnessed; a free addend would let the binding re-fit S (2026-11-21 review).
            require(addend == 0 or addend in {m["offset"] for m in symbol.get("members", [])},
                    "v4 proves zero-addend fields or independently witnessed members only")
            value = (symbol["offset"] + addend) & 0xFFFF
            observed = struct.unpack_from("<H", expected, at)[0]
            if value != observed:
                differences.append(at)
            equations.append({"offset": at, "symbol": target_ref["name"], "operand": fields[0],
                              "encoded_addend": addend, "grounded_dgroup_offset": symbol["offset"],
                              "linked_value": value, "original_value": observed, "equal": value == observed})
            occupied.update((at, at+1))
            continue
        address_alias = mode == IMMEDIATE_MODE and symbol["kind"] == "near-data-address-alias"
        insns = ([i for i in rows if i.address+i.imm_offset == at and i.imm_size == 2]
                 if address_alias else
                 [i for i in rows if i.address+i.disp_offset == at and i.disp_size == 2])
        require(len(insns) == 1, "FIXUPP has no unique direct DS memory operand or immediate base")
        addend = struct.unpack_from("<H", actual, at)[0]
        if address_alias:
            immediate_base(insns[0])
            require(addend == 0, "Only zero encoded data addends are proven")
            address_fields.add(at)
        elif symbol["width"] == 4:
            member = next((m for m in symbol["members"] if m["offset"] == addend), None)
            require(mode == "external-dgroup-word-pair-offset16-v2" and
                    member is not None and direct_operand(insns[0]).size == member["width"] and
                    addend+member["width"] <= symbol["width"],
                    "Word-pair addend is not an independently reviewed word member")
        else:
            require(direct_operand(insns[0]).size == symbol["width"],
                    "FIXUPP is not the grounded DS memory operand")
            require(addend == 0, "Only zero encoded data addends are proven")
        # F5 selects the external's group frame. For the independently reviewed
        # relative alias S=F+offset, S-F+A+D = offset+A (D omitted).
        # v1 has A=0; the v2 word pair permits only independently witnessed A=0 or A=2.
        # No absolute runtime address or natural symbol placement is inferred.
        value = symbol["offset"]+addend
        observed = struct.unpack_from("<H", expected, at)[0]
        if value != observed:
            differences.append(at)
        equations.append({"offset": at, "symbol": target_ref["name"], "encoded_addend": addend,
                          "grounded_ds_relative_offset": symbol["offset"],
                          "linked_value": value, "original_value": observed, "equal": value == observed})
        occupied.update((at, at+1))
    # A literal address may not stand in for a missing external FIXUPP.
    for insn in rows:
        memory = [o for o in insn.operands if o.type == xc.X86_OP_MEM and
                  o.mem.base == o.mem.index == 0 and o.mem.segment == 0]
        if memory:
            direct_operand(insn)
            require({insn.address+insn.disp_offset, insn.address+insn.disp_offset+1} <= occupied,
                    "Absolute DS operand lacks a symbolic candidate fixup")
    if mode == IMMEDIATE_MODE:
        string_context(rows, address_fields)
    return equations, differences


def compare(module, code_name, actual, expected, binding, target, space, oracle):
    symbols = ground(binding, target, oracle)
    obligations = fixes(module)
    require(obligations and obligations == binding["fixups"], "Ordered symbolic fixup obligations changed")
    require(declarations(module) == binding["declarations"], "Bound object declarations changed")
    require(len(actual) == len(expected), "Bound component extent differs; trimming forbidden")
    require(set(symbols) == {e["target"].get("name") for e in obligations},
            "Missing or unused grounded data symbol")
    rows = instructions(actual)
    data_context(module, rows, mode=binding["mode"])
    occupied = set()
    equations, differences = data_equations(obligations, symbols, code_name, actual, expected,
                                           rows, binding["mode"], occupied)
    start = space.position(target["segment"], target["offset"])
    require(not any(start-1 <= r["image_offset"] < start+len(expected) for r in space.relocations),
            "Original relocation obligations are not generated by offset16 fixups")
    ordinary_differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if i not in occupied and a != b]
    field_mismatches = len(differences)
    differences += ordinary_differences
    return {"exact": not differences, "first_difference": min(differences, default=None),
            "mismatch_count": len(differences),
            "mismatch_basis": "ordinary bytes plus unequal field equations",
            "ordinary_mismatch_count": len(ordinary_differences), "field_mismatch_count": field_mismatches,
            "fixups": len(obligations), "relocations": 0, "equations": equations,
            "ordinary_bytes_compared": len(actual)-len(occupied),
            "proof_scope": "Complete component bytes and external DGROUP offset16 equations to independent DS-relative aliases; raw object preserved, absolute data placement/original symbols/TU/runtime DS association/natural link placement unproved"}
