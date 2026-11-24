"""Narrow external DGROUP offset proof. Never edits objects or oracle bytes.

Every field is checked by a linker equation and every other byte by equality.
This is component proof only: symbol aliases, not original PUBLIC names; no
natural placement, TU identity, DS runtime trace, or complete link is claimed.
"""
import re
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


def registers_written(insn):
    """Names of the registers an instruction writes. Capstone omits the segment register LES/LDS load and calls the
    16-bit stack pointer `esp` for PUSH/POP (2026-11-23)."""
    names = {insn.reg_name(r) for r in insn.regs_access()[1]}
    if "esp" in names or insn.mnemonic.startswith(("push", "pop", "enter", "leave", "call", "lcall", "ret")):
        names.add("sp")
    if insn.mnemonic in ("les", "lds", "lss"):
        names.add(insn.mnemonic[1:])
    return names


def _not_dgroup(operand):
    """A memory operand that does not address DGROUP: an ES/CS/FS/GS override, or BP-based without a DS override."""
    _, xc = _capstone()
    mem = operand.mem
    return (mem.segment in (xc.X86_REG_ES, xc.X86_REG_CS, xc.X86_REG_FS, xc.X86_REG_GS) or
            mem.segment != xc.X86_REG_DS and xc.X86_REG_BP in (mem.base, mem.index))


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


# MSC switch dispatch, immediately before `jmp cs:[bx+T]`: an unsigned bound check on the index, then its
# scaling by two (or an index already scaled, checked against an even bound).
_SWITCH = re.compile(rb"(?:(?P<ax>\x3d(?P<axn>..))|(?P<bx>\x83\xfb(?P<b8>[\x00-\x7f])|\x81\xfb(?P<b16>..)))"
                     rb"(?:(?P<le>\x76\x03\xe9..|\x77.)|(?P<lt>\x72\x03\xe9..|\x73.))"
                     rb"(?P<scale>\x03\xc0\x93|\xd1\xe0\x93|\x93|\xd1\xe3|\x03\xdb)$", re.S)


def _switch_targets(data, pos, seen):
    """Offsets stored in the jump table of a bounded MSC switch whose `jmp cs:[bx+T]` is at `pos`, else ().

    The bound check must be reached code directly before the jump; the table then has exactly
    bound+1 (`ja`/`jbe`) or bound (`jae`/`jb`) words, and every one is a CS offset of code."""
    if data[pos:pos + 3] != b"\x2e\xff\xa7":
        return ()
    match = _SWITCH.search(data[max(0, pos - 16):pos])
    if match is None or pos - len(match.group(0)) not in seen:
        return ()
    scale, on_ax = match.group("scale"), match.group("ax") is not None
    if on_ax != (scale in (b"\x03\xc0\x93", b"\xd1\xe0\x93", b"\x93")):
        return ()
    bound = int.from_bytes(match.group("axn") or match.group("b16") or match.group("b8"), "little")
    count = bound + (1 if match.group("le") else 0)
    if scale == b"\x93":
        if match.group("lt") or bound % 2:
            return ()
        count = bound // 2 + 1
    table = struct.unpack_from("<H", data, pos + 3)[0]
    return table, count


def reachable_from_entry(oracle, entry):
    """Instruction starts reached from a root entry that some original relocated CALL FAR targets.

    Recursive descent within the entry's 64K CS window: conditional branches and loops
    follow both edges, JMP follows its target, calls fall through, returns and indirect
    jumps stop, except a bounded MSC switch table (`_switch_targets`), whose words are
    followed. Result is cached per entry."""
    # Keyed by id, holding the oracle itself: a collected test oracle's id can be reused.
    key = (id(oracle), entry)
    if key in _REACH and _REACH[key][0] is oracle:
        return _REACH[key][1]
    space_name, seg_text, off_text = entry.split(":")
    require(space_name in oracle.spaces, "Unknown v4 witness entry space")
    root = oracle.spaces[space_name]  # the entry's own space (resident or one overlay)
    segment, offset = int(seg_text, 16), int(off_text, 16)
    start = root.position(segment, offset)
    def reviewed_entry():
        from common import ROOT, read_json_shared
        for t in read_json_shared(ROOT / "evidence/targets.json")["functions"]:
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
        overlays = {n: s for n, s in oracle.spaces.items() if s.code_overlay}
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
                    switch = _switch_targets(root.data, pos, seen)
                    if switch:
                        table, entries = switch
                        for k in range(entries):
                            word = base + ((table + 2 * k) & 0xFFFF)
                            if not 0 <= word <= root.size - 2:
                                break
                            work.append(base + struct.unpack_from("<H", root.data, word)[0])
                    break  # unconditional: direct target queued above, other indirect jumps stop
            pos += insn.size
    _REACH[key] = (oracle, seen)
    return seen


# Immediates that may carry a data address: MOV r16/m16, imm16 and PUSH imm16.
# Branch displacements, RET/RETF n, ENTER and arithmetic constants never qualify.
_ADDRESS_IMMEDIATE = ("mov", "push")


_STRING_USES = {"movs": {"si", "di"}, "cmps": {"si", "di"}, "lods": {"si"}, "stos": {"di"}, "scas": {"di"}}


def immediate_is_address(oracle, space_name, at):
    """True when the original instruction at `at` loads or pushes an imm16 that the following code uses as a near
    address: a register holding it (also through register moves and index arithmetic) is dereferenced or drives a
    string operation, or it is pushed directly after `push ds` (a DS:offset argument). Straight-line code and direct
    unconditional jumps only. Anything else (a stored, compared, multiplied or plainly pushed number) is ambiguous."""
    cs, xc = _capstone()
    engine = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_16)
    engine.detail = True
    data = oracle.spaces[space_name].data

    def decode(pos):
        return next(engine.disasm(bytes(data[pos:pos + 16]), pos), None)

    def reg(insn, op):
        return insn.reg_name(op.reg) if op.type == xc.X86_OP_REG else ""

    insn = decode(at)
    if insn is None or not insn.operands:
        return False
    after_ds = at > 0 and data[at - 1] == 0x1e
    if insn.mnemonic == "push":
        return after_ds
    if insn.mnemonic != "mov" or not reg(insn, insn.operands[0]) or insn.operands[0].size != 2:
        return False
    held, pos = {reg(insn, insn.operands[0])}, at + insn.size
    for _ in range(24):
        insn = decode(pos)
        if insn is None or not held:
            return False
        ops, name = insn.operands, insn.mnemonic.split()[-1]
        for op in ops:
            if op.type == xc.X86_OP_MEM and name != "lea" and op.mem.segment in (0, xc.X86_REG_DS) and \
                    {insn.reg_name(r) for r in (op.mem.base, op.mem.index) if r} & held and \
                    name[:4] not in _STRING_USES:
                return True
        if name[:4] in _STRING_USES:
            return bool(held & _STRING_USES[name[:4]])
        if name == "push":
            if reg(insn, ops[0]) in held:
                return after_ds
            after_ds = reg(insn, ops[0]) == "ds"
        elif insn.bytes[0] in (0xe9, 0xeb):
            delta = int.from_bytes(insn.bytes[1:], "little", signed=True)
            pos += insn.size + delta
            continue
        elif set(insn.groups).intersection((1, 2, 3, 4, 5, 7)):
            return False
        elif name == "xchg" and all(reg(insn, o) for o in ops):
            a, b = reg(insn, ops[0]), reg(insn, ops[1])
            held = {b if r == a else a if r == b else r for r in held}
        else:
            sources = set()
            if ops and reg(insn, ops[0]) and ops[0].size == 2:
                if name == "mov" and len(ops) == 2:
                    sources = {reg(insn, ops[1])}
                elif name == "lea":
                    sources = {insn.reg_name(r) for r in (ops[1].mem.base, ops[1].mem.index) if r}
                elif name in ("add", "sub", "inc", "dec"):
                    sources = {reg(insn, o) for o in ops}
            keeps = bool(sources & held)
            for r in insn.regs_access()[1]:
                held.discard(insn.reg_name(r))
            if keeps:
                held.add(reg(insn, ops[0]))
        pos += insn.size
    return False


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
    found = {"segments": module.segment_defs, "groups": module.groups,
             "publics": module.publics, "externals": module.externals}
    if getattr(module, "communals", None):  # near COMDEFs are pinned with the other declarations
        found["communals"] = module.communals
    return found


def fixes(module):
    return [e for r in module.fixups for e in r["resolved"] if e["kind"] == "fixup"]


def index_extent(raw, base):
    """Length N of the byte array at DGROUP offset `base` that an original bounds-checked index proves, else None.

    The window ends `cmp bl,N; jae X; add bx,base; <byte access [bx]>` with BH cleared earlier in it and nothing but
    BL-only operations on BX in between: the program addresses exactly base..base+N-1 as one array."""
    _, xc = _capstone()
    rows = instructions(raw)
    if len(rows) < 5:
        return None
    compare, branch, add, access = rows[-4:]
    if not (compare.mnemonic == "cmp" and compare.operands[0].type == xc.X86_OP_REG and
            compare.operands[0].reg == xc.X86_REG_BL and compare.operands[1].type == xc.X86_OP_IMM and
            branch.bytes[0] == 0x73 and bytes(add.bytes) == b"\x81\xc3" + struct.pack("<H", base)):
        return None
    memory = [o for o in access.operands if o.type == xc.X86_OP_MEM]
    if not (len(memory) == 1 and memory[0].size == 1 and memory[0].mem.base == xc.X86_REG_BX and
            memory[0].mem.index == memory[0].mem.disp == 0 and memory[0].mem.segment in (0, xc.X86_REG_DS)):
        return None
    cleared = False
    for insn in rows[:-4]:
        require(not set(insn.groups).intersection((1, 2, 3, 4, 5, 7)), "Index window must be straight-line before the bound")
        if bytes(insn.bytes) in (b"\x2a\xff", b"\x32\xff", b"\x28\xff", b"\x30\xff"):   # sub/xor bh,bh
            cleared = True
        elif registers_written(insn) & {"bx", "bh"}:
            cleared = False
    return compare.operands[1].imm & 0xFF if cleared else None


def pointer_extent(raw, base):
    """4 when the single original instruction `raw` reads a far pointer stored at DGROUP offset `base` (les/lds r,[base]
    or call/jmp far [base], default DS or the equal SS), else None: the program treats [base, base+4) as one pointer."""
    _, xc = _capstone()
    rows = instructions(raw)
    if len(rows) != 1 or rows[0].mnemonic not in ("les", "lds", "lcall", "ljmp"):
        return None
    memory = [o for o in rows[0].operands if o.type == xc.X86_OP_MEM]
    ok = (len(memory) == 1 and memory[0].mem.base == memory[0].mem.index == 0 and rows[0].disp_size == 2 and
          memory[0].mem.disp & 0xFFFF == base and memory[0].mem.segment in (0, xc.X86_REG_DS, xc.X86_REG_SS))
    return 4 if ok else None


def fill_extent(raw, base):
    """Bytes the straight-line original window `raw` fills from DGROUP offset `base`, else None.

    The last instruction is REP STOSW/STOSB with ES = DS loaded in the window (`push ds; pop es` or `mov ax,ds;
    mov es,ax`), DI = base (a constant, possibly through registers) and CX = a constant. Earlier fills in the window
    are followed (DI then unknown, CX 0). The program then clears or sets [base, base+L) as one unit, as a block copy."""
    _, xc = _capstone()
    rows = instructions(raw)
    sizes = {b"\xf3\xab": 2, b"\xf3\xaa": 1}
    value, es_ds, ds_ok, prev = {}, False, True, b""
    require(rows and bytes(rows[-1].bytes) in sizes, "Fill window must end in REP STOS")
    for insn in rows:
        b, ops, source = bytes(insn.bytes), insn.operands, None
        if insn is rows[-1]:
            count = value.get("cx")
            return count * sizes[b] if es_ds and count and value.get("di") == base else None
        if b in sizes:
            value.pop("di", None)
            value["cx"], prev = 0, b
            continue
        require(not set(insn.groups).intersection((1, 2, 3, 4, 5, 7)), "Extent witness window must be straight-line")
        if b == b"\xfd":
            return None
        if insn.mnemonic == "mov" and len(ops) == 2 and ops[0].type == xc.X86_OP_REG and ops[0].size == 2:
            source = (ops[1].imm & 0xFFFF if ops[1].type == xc.X86_OP_IMM else
                      value.get(insn.reg_name(ops[1].reg)) if ops[1].type == xc.X86_OP_REG else None)
        written = registers_written(insn)
        for name in written:
            for full in ("ax", "bx", "cx", "dx"):
                if name in (full[0] + "l", full[0] + "h"):
                    value.pop(full, None)
            value.pop(name, None)
        if "ds" in written:
            ds_ok = False
        if "es" in written:
            es_ds = ds_ok and ((b == b"\x07" and prev == b"\x1e") or (b == b"\x8e\xc0" and prev == b"\x8c\xd8"))
        if source is not None:
            value[insn.reg_name(ops[0].reg)] = source
        prev = b
    return None


def base_extent(raw, base):
    """k + width when the original window `raw` starts with `mov r,base` (BX, SI or DI), never changes r and ends in
    one DS access [r+k] of that width; else None. Conditional branches may lie between (the window is the path that
    falls through them), no call, jump or return. The program then uses memory up to base+k+width as one object."""
    _, xc = _capstone()
    rows = instructions(raw)
    first, last = rows[0], rows[-1]
    if len(rows) < 2 or first.mnemonic != "mov" or first.imm_size != 2 or first.operands[0].type != xc.X86_OP_REG or             first.operands[1].imm & 0xFFFF != base:
        return None
    name = first.reg_name(first.operands[0].reg)
    parts = {"bx": {"bx", "bl", "bh"}, "si": {"si"}, "di": {"di"}}.get(name)
    if parts is None:
        return None
    for insn in rows[1:]:
        branch = 0x70 <= insn.bytes[0] <= 0x7F
        require(branch or not set(insn.groups).intersection((1, 2, 3, 4, 5, 7)), "Base window must fall through")
        if (insn is not last and registers_written(insn) & parts) or "ds" in registers_written(insn):
            return None
    memory = [o for o in last.operands if o.type == xc.X86_OP_MEM]
    if len(memory) != 1 or last.mnemonic == "lea" or memory[0].mem.index or             last.reg_name(memory[0].mem.base) != name or memory[0].mem.segment not in (0, xc.X86_REG_DS):
        return None
    return memory[0].mem.disp + memory[0].size if memory[0].mem.disp >= 0 else None


def record_stride(raw, base, es=False, construct=False):
    """d when the straight-line original window `raw` ends in one access [r+base] (default DS; ES: with `es`) whose index
    register holds d (>= 2) times one unknown value, computed inside the window; else None.

    The program then steps through memory at `base` in records of d bytes, so it addresses an array there. Only
    register moves, adding or subtracting a register, shifts by a constant and multiplication by a constant are
    followed; anything else makes the register a new unknown of factor 1. Every unknown may itself be a multiple of
    something, so the real record can only be longer than d.

    Two more endings (2026-11-24). With `es`, a string store through ES:DI whose DI was last written by
    `lea di,[r+base]` in the window (a record copied into the array). With `construct`, `add r,base` as the last
    instruction: the near half of a far pointer to one record (the caller checks the segment half behind it)."""
    _, xc = _capstone()
    rows = instructions(raw)
    halves = {"ax": ("al", "ah"), "bx": ("bl", "bh"), "cx": ("cl", "ch"), "dx": ("dl", "dh")}
    names = ("ax", "bx", "cx", "dx", "si", "di")
    fresh = iter(range(1, 10000))
    value = {n: (next(fresh), 1) for n in names}     # (unknown, factor); unknown 0 is the constant 1
    half = {}                                        # constants in byte registers
    lea = None                                       # (value, displacement) of the LEA that last wrote DI

    def constant(op, insn):
        if op.type == xc.X86_OP_IMM:
            return op.imm
        name = insn.reg_name(op.reg) if op.type == xc.X86_OP_REG else ""
        return half.get(name) if op.size == 1 else value[name][1] if name in value and value[name][0] == 0 else None

    require(len(rows) >= 2, "Record window needs the index computation")
    for insn in rows[:-1]:
        require(not set(insn.groups).intersection((1, 2, 3, 4, 5, 7)), "Record window must be straight-line")
        ops, name, result, low = insn.operands, insn.mnemonic, None, None
        target = insn.reg_name(ops[0].reg) if ops and ops[0].type == xc.X86_OP_REG else ""
        source = insn.reg_name(ops[1].reg) if len(ops) == 2 and ops[1].type == xc.X86_OP_REG else ""
        if name == "mov" and target in value:
            result = value.get(source) or ((0, ops[1].imm & 0xFFFF) if ops[1].type == xc.X86_OP_IMM else None)
        elif name == "mov" and target and ops[0].size == 1 and ops[1].type == xc.X86_OP_IMM:
            low = ops[1].imm & 0xFF
        elif name in ("add", "sub") and target in value and source in value:
            (a, m), (b, k) = value[target], value[source]
            if a == b:
                result = (a, m + k if name == "add" else m - k)
        elif name in ("shl", "sal") and target in value:
            count = 1 if len(ops) == 1 else constant(ops[1], insn)
            if count is not None and 0 < count < 16:
                result = (value[target][0], value[target][1] << count)
        elif name in ("imul", "mul") and len(ops) == 1:
            k = constant(ops[0], insn)
            if ops[0].size == 1:
                k = half.get("al") if k is None else k
                result = (next(fresh), k) if k else None
            elif k is not None:
                result = (value["ax"][0], value["ax"][1] * k)
            elif value["ax"][0] == 0:
                other = value.get(target, (next(fresh), 1))
                result = (other[0], other[1] * value["ax"][1])
            target = "ax"
        written = registers_written(insn)
        if "di" in written:
            mem = ops[1].mem if name == "lea" else None
            only = [r for r in (mem.base, mem.index) if r] if mem is not None else []
            lea = ((value[insn.reg_name(only[0])], mem.disp & 0xFFFF)
                   if len(only) == 1 and insn.disp_size == 2 and insn.reg_name(only[0]) in value else None)
        for full in names:
            if written & {full, *halves.get(full, ())}:
                value[full] = (next(fresh), 1)
                for part in halves.get(full, ()):
                    half.pop(part, None)
        if result is not None:
            value[target] = result
        if low is not None:
            half[target] = low
    access = rows[-1]
    if construct:
        ops = access.operands
        if access.mnemonic != "add" or len(ops) != 2 or ops[0].type != xc.X86_OP_REG or ops[0].size != 2 or \
                ops[1].type != xc.X86_OP_IMM or ops[1].imm & 0xFFFF != base:
            return None
        unknown, factor = value.get(access.reg_name(ops[0].reg), (0, 0))
        return factor if unknown and 2 <= factor < 0x8000 else None
    if es and bytes(access.bytes) in (b"\xa4", b"\xa5", b"\xaa", b"\xab"):
        if lea is None or lea[1] != base:
            return None
        return lea[0][1] if lea[0][0] and 2 <= lea[0][1] < 0x8000 else None
    memory = [o for o in access.operands if o.type == xc.X86_OP_MEM]
    if len(memory) != 1 or access.disp_size != 2 or memory[0].mem.disp & 0xFFFF != base or access.mnemonic == "lea" or             (memory[0].mem.segment != xc.X86_REG_ES if es else
             _not_dgroup(memory[0]) or memory[0].mem.segment not in (0, xc.X86_REG_DS)):
        return None
    used = [r for r in (memory[0].mem.base, memory[0].mem.index) if r]
    if len(used) != 1 or access.reg_name(used[0]) not in value:
        return None
    unknown, factor = value[access.reg_name(used[0])]
    return factor if unknown and 2 <= factor < 0x8000 else None


def call_arguments(raw):
    """Stack words a straight-line original window passes to the far CALL that ends it, as {disp: value}: disp is the
    callee's [bp+disp] after `push bp; mov bp,sp` (6 for the last word pushed), value the pushed constant or "ds" for
    `push ds` while DS is unchanged. Other words are absent. The CALL may also be `push cs; call rel16`, the same far
    call inside one segment (2026-11-24)."""
    _, xc = _capstone()
    rows = instructions(raw)
    same = len(rows) >= 3 and rows[-1].bytes[0] == 0xe8 and bytes(rows[-2].bytes) == b"\x0e"
    body = rows[:-2] if same else rows[:-1]
    require(len(rows) >= 2 and (same or rows[-1].bytes[0] == 0x9a) and
            not any(set(r.groups).intersection((1, 2, 3, 4, 5, 7)) for r in body),
            "Call window must be straight-line code ending in one far CALL")
    value, stack = {"ds": "ds"}, []
    for insn in body:
        ops = insn.operands
        if insn.mnemonic == "push":
            stack.append(value.get(insn.reg_name(ops[0].reg)) if ops[0].type == xc.X86_OP_REG else
                         ops[0].imm & 0xFFFF if ops[0].type == xc.X86_OP_IMM else None)
            continue
        source = None
        if insn.mnemonic == "mov" and len(ops) == 2 and ops[0].type == xc.X86_OP_REG and ops[0].size == 2:
            source = (ops[1].imm & 0xFFFF if ops[1].type == xc.X86_OP_IMM else
                      value.get(insn.reg_name(ops[1].reg)) if ops[1].type == xc.X86_OP_REG else None)
        written = registers_written(insn)
        require("sp" not in written, "Call window moves the stack pointer")
        for name in written:
            for full in ("ax", "bx", "cx", "dx"):
                if name in (full, full[0] + "l", full[0] + "h"):
                    value.pop(full, None)
            value.pop(name, None)
        if source is not None and insn.reg_name(ops[0].reg) != "ds":
            value[insn.reg_name(ops[0].reg)] = source
    return {6 + 2 * k: v for k, v in enumerate(reversed(stack)) if v is not None}


def block_copy_extent(raw, at, base, arguments=None):
    """Length L of one original block copy anchored at DGROUP offset `base`, else None.

    `raw` holds straight-line original instructions starting at image offset `at`. SI or DI holds
    exactly `base` (`mov r,base`, `lea r,[base]`, `lea r,[bx+base]`, or a register move of such a
    value) when the first uninterrupted run of string moves starts: `rep movsw`/`rep movsb` with
    `mov cx,n`, and/or unrolled `movsw`/`movsb`, with nothing between them. The run copies L bytes
    through that register. A SI source needs DS unchanged since the window began; a DI destination
    needs ES loaded from that DS inside the window (`push ds; pop es` or `mov ax,ds; mov es,ax`).
    No control transfer, no `std`. The program then treats [base, base+L) as one copied unit (a
    struct assignment): any address in it belongs to the same aggregate as base (2026-11-22,
    docs/dgroup-binding.md "Aggregate extents"; unrolled runs and DS tracking 2026-11-23).

    With `arguments` (a call_arguments result) `raw` starts at a callee's entry `push bp; mov bp,sp`,
    and `mov r,[bp+disp]` loads the passed constant."""
    _, xc = _capstone()
    require(arguments is None or raw[:3] == b"\x55\x8b\xec", "Callee window must start with its stack frame")
    moves = {b"\xa5": 2, b"\xa4": 1, b"\xf3\xa5": 2, b"\xf3\xa4": 1}
    parent = {"al": "ax", "ah": "ax", "bl": "bx", "bh": "bx", "cl": "cx", "ch": "cx", "dl": "dx", "dh": "dx"}
    value, count, es_ds, ds_ok, prev, run, copied = {}, None, False, True, b"", None, 0
    for insn in instructions(raw):
        b = bytes(insn.bytes)
        if b in moves:
            if run is None:
                run = (ds_ok and value.get("si") == base) or (es_ds and value.get("di") == base)
            if b[0] == 0xf3:
                if count is None:
                    return None
                copied, count = copied + count * moves[b], 0
            else:
                copied += moves[b]
            continue
        if run is not None:
            break
        require(not set(insn.groups).intersection((1, 2, 3, 4, 5, 7)), "Extent witness window must be straight-line")
        if b == b"\xfd":
            return None
        ops = insn.operands
        source = None
        if insn.mnemonic in ("mov", "lea") and len(ops) == 2 and ops[0].type == xc.X86_OP_REG:
            if insn.mnemonic == "lea":
                mem = ops[1].mem
                if mem.segment == mem.index == 0 and mem.base in (0, xc.X86_REG_BX) and insn.disp_size == 2:
                    source = mem.disp & 0xFFFF
            elif ops[1].type == xc.X86_OP_IMM:
                source = ops[1].imm & 0xFFFF
            elif ops[1].type == xc.X86_OP_REG:
                source = value.get(insn.reg_name(ops[1].reg))
            elif arguments and ops[1].type == xc.X86_OP_MEM and ops[1].mem.base == xc.X86_REG_BP and \
                    ops[1].mem.index == ops[1].mem.segment == 0:
                source = arguments.get(ops[1].mem.disp)
                source = None if source == "ds" else source
        written = registers_written(insn)
        if arguments is not None and any(o.type == xc.X86_OP_MEM and xc.X86_REG_BP in (o.mem.base, o.mem.index)
                                         for o in ops[:1]):
            return None  # an argument slot may have been written
        if "bp" in written and bytes(insn.bytes) != b"\x8b\xec":
            return None
        for name in written:
            value.pop(parent.get(name, name), None)
            if parent.get(name, name) == "cx":
                count = None
        if "ds" in written:
            ds_ok = False
        if "es" in written:
            es_ds = ds_ok and ((b == b"\x07" and prev == b"\x1e") or (b == b"\x8e\xc0" and prev == b"\x8c\xd8"))
        if source is not None and ops[0].size == 2:
            value[insn.reg_name(ops[0].reg)] = source
            if insn.reg_name(ops[0].reg) == "cx":
                count = source
        prev = b
    return copied if run else None


def far_read_extent(raw, base, arguments):
    """Bytes a callee reads in sequence through a far pointer argument that is DS:base, else None.

    `raw` starts at the callee's entry (`push bp; mov bp,sp`) and is straight-line; `lds si,[bp+a]` loads the argument
    (`arguments[a] == base`, `arguments[a+2] == "ds"`: the caller's DS, which is DGROUP) and every following LODSW/LODSB
    reads the next bytes. Nothing else writes SI, DS or the frame, and there is no STD."""
    _, xc = _capstone()
    require(raw[:3] == b"\x55\x8b\xec", "Callee window must start with its stack frame")
    loaded, length = False, 0
    for insn in instructions(raw)[2:]:
        b = bytes(insn.bytes)
        require(not set(insn.groups).intersection((1, 2, 3, 4, 5, 7)), "Extent witness window must be straight-line")
        ops, written = insn.operands, registers_written(insn)
        if b == b"\xfd" or "bp" in written:
            return None
        if not loaded and ops and ops[0].type == xc.X86_OP_MEM and insn.mnemonic != "push":
            return None  # an argument slot may have been written
        if insn.mnemonic == "lds" and insn.reg_name(ops[0].reg) == "si":
            mem = ops[1].mem
            if loaded or mem.base != xc.X86_REG_BP or mem.index or mem.segment or \
                    arguments.get(mem.disp) != base or arguments.get(mem.disp + 2) != "ds":
                return None
            loaded = True
        elif b in (b"\xad", b"\xac"):
            length += (2 if b == b"\xad" else 1) if loaded else 0
        elif loaded and {"si", "ds"} & written:
            return None
    return length or None


def walk_extent(raw, base):
    """Bytes [base, base+L) a complete original loop steps through, as L, else None.

    `raw` is the whole loop from `mov r,base` to its closing backward branch. It is executed: the constant registers
    follow MOV/ADD/SUB/INC/DEC with constants, each conditional branch directly follows a CMP of a constant register with
    a constant, and every other instruction may not change a constant register that is still used. One access instruction
    through the pointer must touch base, base+d, base+2d, ... (d >= 2, at least two records) and nothing else, so the
    program uses one array of d-byte records there (2026-11-24)."""
    _, xc = _capstone()
    rows = {i.address: i for i in instructions(raw)}
    first = rows[0]
    if not (0xB8 <= first.bytes[0] <= 0xBF and first.size == 3):
        return None
    value, touched, compare, pos, sites = {}, [], None, 0, set()
    for _ in range(20000):
        if pos == len(raw):
            break
        insn = rows.get(pos)
        if insn is None:
            return None
        ops, name = insn.operands, insn.mnemonic
        target = insn.reg_name(ops[0].reg) if ops and ops[0].type == xc.X86_OP_REG and ops[0].size == 2 else ""
        constant = (ops[1].imm & 0xFFFF) if len(ops) == 2 and ops[1].type == xc.X86_OP_IMM else None
        if name in ("jb", "jae", "jne", "je", "jbe", "ja"):
            if compare is None or not 0 <= ops[0].imm < pos:
                return None
            a, b = compare
            taken = {"jb": a < b, "jae": a >= b, "jne": a != b, "je": a == b, "jbe": a <= b, "ja": a > b}[name]
            pos = ops[0].imm if taken else pos + insn.size
            compare = None
            continue
        if set(insn.groups).intersection((1, 2, 3, 4, 5, 7)):
            return None
        compare = (value[target], constant) if name == "cmp" and target in value and constant is not None else None
        memory = [o for o in ops if o.type == xc.X86_OP_MEM]
        if memory:
            mem = memory[0].mem
            used = [insn.reg_name(r) for r in (mem.base, mem.index) if r]
            if len(memory) != 1 or _not_dgroup(memory[0]) or len(used) != 1 or used[0] not in value or name == "lea":
                return None
            touched.append((value[used[0]] + mem.disp & 0xFFFF, memory[0].size))
            sites.add(pos)
        written = registers_written(insn) & set(value) | ({target} if target in value and name != "cmp" else set())
        if name == "mov" and target and constant is not None:
            value[target] = constant
        elif name in ("add", "sub") and target in value and constant is not None:
            value[target] = (value[target] + (constant if name == "add" else -constant)) & 0xFFFF
        elif name in ("inc", "dec") and target in value:
            value[target] = (value[target] + (1 if name == "inc" else -1)) & 0xFFFF
        elif name != "cmp":
            for register in written:
                value.pop(register, None)
        pos += insn.size
    else:
        return None
    if len(touched) < 2 or len(sites) != 1 or touched[0][0] != base:
        return None
    d, width = touched[1][0] - base, touched[0][1]
    if d < 2 or any(at != base + k * d or size != width for k, (at, size) in enumerate(touched)):
        return None
    return (len(touched) - 1) * d + width


def argument_writes(raw, arguments):
    """Byte ranges [lo, hi) a callee stores through near-pointer arguments in the straight-line window `raw`:
    `mov r,[bp+a]` loads a passed constant (`arguments`), `mov [r+k],value` stores through it. Nothing else may write the
    pointer register or the stack frame."""
    _, xc = _capstone()
    value, ranges = {}, []
    for insn in instructions(raw):
        require(not set(insn.groups).intersection((1, 2, 3, 4, 5, 7)), "Argument write window must be straight-line")
        ops = insn.operands
        require(insn.mnemonic == "mov" and len(ops) == 2, "Argument write window may hold only MOV")
        if ops[0].type == xc.X86_OP_MEM:
            mem = ops[0].mem
            require(not mem.segment and not mem.index and insn.reg_name(mem.base) in value,
                    "Argument write window stores elsewhere")
            lo = value[insn.reg_name(mem.base)] + mem.disp & 0xFFFF
            ranges.append((lo, lo + ops[0].size))
            continue
        for name in registers_written(insn):
            require(name not in ("bp", "ds"), "Argument write window changes its frame")
            value.pop(name, None)
            value.pop({"bl": "bx", "bh": "bx"}.get(name, name), None)
        if ops[1].type == xc.X86_OP_MEM and ops[1].mem.base == xc.X86_REG_BP and not ops[1].mem.index and \
                not ops[1].mem.segment and ops[0].size == 2:
            passed = arguments.get(ops[1].mem.disp)
            if type(passed) is int:
                value[insn.reg_name(ops[0].reg)] = passed
    return ranges


def fits_once(position, length, low, high, even):
    """True when `position` is the only place (even addresses only, with `even`) for `length` bytes in [low, high)."""
    places = [p for p in range(low, high - length + 1) if not even or p % 2 == 0]
    return length > 0 and places == [position]


def fit_bounds(below, above):
    """(low, high) between two original instructions: the end of a direct access, and the address of another access."""
    from disasm import _capstone
    memory = _capstone()[1].X86_OP_MEM
    brows = instructions(below)
    operands = [o for o in brows[0].operands if o.type == memory] if len(brows) == 1 else []
    require(len(operands) == 1 and not _not_dgroup(operands[0]) and not operands[0].mem.base and
            not operands[0].mem.index and brows[0].disp_size == 2, "Fit's lower neighbour is not a direct access")
    arows = instructions(above)
    require(len(arows) == 1, "Fit's upper neighbour is not one instruction")
    upper = [o for o in arows[0].operands if o.type == memory]
    require(len(upper) == 1 and not _not_dgroup(upper[0]) and arows[0].disp_size == 2 and
            arows[0].mnemonic != "lea", "Fit's upper neighbour is not a memory access")
    return (operands[0].mem.disp & 0xFFFF) + operands[0].size, upper[0].mem.disp & 0xFFFF


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

    def witness(spec, draft_ok=False, anywhere=False):
        # v4 witnesses may sit in overlay code (reached from that overlay's own entries);
        # every other mode and the DGROUP frame witness stay resident.
        overlay_ok = binding["mode"] == OPERAND_MODE and "entry" in spec and spec["space"] in oracle.spaces
        require(spec["space"] == "root" or overlay_ok, "Unsupported cross-space data witness")
        wspace = oracle.spaces[spec["space"]]
        at, raw = spec["image_offset"], bytes.fromhex(spec["bytes_hex"])
        same = spec["space"] == target["space"]
        inside = ([] if anywhere and claimed is not None else
                  [(lo, hi) for lo, hi in claimed] if draft_ok and claimed is not None else [(start, end)])
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
            # Aggregate extent: an original block copy anchored at S proves [S, S+L) is one unit.
            length = 0
            if "extent" in symbol:
                spec = symbol["extent"]
                kind = spec.get("kind")
                require(set(spec) <= {"length", "witness", "call", "kind", "next", "writes"} and
                        {"length", "witness"} <= set(spec) and
                        ((symbol["witnesses"] or internal and "call" not in spec) and kind is None or
                         kind in ("index", "pointer", "record", "fill", "base", "walk") and "call" not in spec or
                         kind == "far-read" and "call" in spec and symbol["witnesses"] or
                         kind == "writes" and "call" in spec and internal) and
                        ("next" not in spec or kind == "record") and ("writes" in spec) == (kind == "writes"),
                        "An aggregate extent needs {length, witness} and an independently witnessed base")
                # In a group the copy may itself lie inside a claim (2026-11-23): its length is an
                # original fact about [S, S+L). An index witness is S's only evidence, so it must stay outside
                # the claims. A copy or pointer access inside one leaves S to internal consensus when S has no
                # witness: module_gate requires two equal claimed S+0 sites before any in-extent member is used.
                at, raw = witness(spec["witness"], draft_ok=True, anywhere=kind != "index")
                require("entry" in spec["witness"] and at in reachable_from_entry(oracle, spec["witness"]["entry"]),
                        "Extent witness is not reached from its entry")
                arguments = None
                if "call" in spec:
                    # The copy is in a callee and the base is one of its stack arguments: an original reached far
                    # CALL to that callee passes exactly S in the word the callee copies through.
                    cat, craw = witness(spec["call"], draft_ok=True, anywhere=True)
                    wspace, target_at = oracle.spaces[spec["call"]["space"]], cat + len(craw) - 4
                    c_off, c_seg = struct.unpack_from("<HH", craw, len(craw) - 4)
                    callee = oracle.spaces[spec["witness"]["space"]]
                    if craw[-4:-2] == b"\x0e\xe8" and kind == "writes":
                        # push cs; call rel16 inside the caller's own segment
                        reached = cat + len(craw) + struct.unpack_from("<h", craw, len(craw) - 2)[0]
                        called = wspace is callee and reached == at
                    else:
                        called = (sum(r["image_offset"] == target_at + 2 for r in wspace.relocations) == 1 and
                                  (c_seg - callee.link_segment) * 16 + c_off == at and callee.name == "root")
                    require("entry" in spec["call"] and cat in reachable_from_entry(oracle, spec["call"]["entry"]) and
                            called, "Extent call is not a reached, relocated far CALL of the copying routine")
                    arguments = call_arguments(craw)
                    require(offset in arguments.values(), "Extent call does not pass the symbol's address")
                if kind == "record":
                    # The program steps through records of d bytes at S. With a second window that steps through the
                    # same records at S+d (element v+1), records 0 and 1 both belong to the array.
                    length = record_stride(raw, offset)
                    if "next" in spec and length is not None:
                        nat, nraw = witness(spec["next"], draft_ok=True, anywhere=True)
                        require("entry" in spec["next"] and nat in reachable_from_entry(oracle, spec["next"]["entry"]) and
                                record_stride(nraw, (offset + length) & 0xFFFF) == length,
                                "Record extent's next window does not step through the same records")
                        length *= 2
                elif kind == "writes":
                    # The callee stores through its pointer arguments: the stored ranges, taken from several reached
                    # windows of that callee, join to [S, S+L) without a gap.
                    require(raw == b"\x55\x8b\xec", "Argument writes need the callee's stack frame")
                    ranges = []
                    for row in spec["writes"]:
                        wat, wraw = witness(row, draft_ok=True, anywhere=True)
                        require(wat in reachable_from_entry(oracle, row["entry"]) and
                                row["entry"] == spec["witness"]["entry"] and row["space"] == spec["witness"]["space"],
                                "Argument write is not reached from the callee's entry")
                        ranges += argument_writes(wraw, arguments)
                    from index_floor import frame_writes
                    c_base = (int(spec["witness"]["entry"].split(":")[1], 16) - callee.link_segment) * 16
                    pointers = [a for a, v in arguments.items() if type(v) is int]
                    require(not any(lo < a + 2 and a < hi for lo, hi in frame_writes(callee, at, c_base) for a in pointers),
                            "The callee overwrites a pointer argument")
                    reached = offset
                    for lo, hi in sorted(set(ranges)):
                        require(offset <= lo <= reached, "Argument writes leave a gap")
                        reached = max(reached, hi)
                    length = reached - offset
                elif kind == "walk":
                    length = walk_extent(raw, offset)
                else:
                    length = (index_extent(raw, offset) if kind == "index" else pointer_extent(raw, offset)
                              if kind == "pointer" else far_read_extent(raw, offset, arguments)
                              if kind == "far-read" else fill_extent(raw, offset)
                              if kind == "fill" else base_extent(raw, offset)
                              if kind == "base" else block_copy_extent(raw, at, offset, arguments))
                require(length is not None and length == spec["length"],
                        "Extent witness is not a block copy, fill, bounded index, pointer or base access, record array, "
                        "argument read or argument write of that length at the symbol")
            def fits(fit, position, size, even):
                """One object of `size` bytes has no other place between the two named neighbours."""
                require(set(fit) == {"below", "above"} and internal, "Unsupported fit evidence")
                bat, braw = witness(fit["below"], draft_ok=True, anywhere=True)
                require("entry" in fit["below"] and bat in reachable_from_entry(oracle, fit["below"]["entry"]),
                        "Fit's lower neighbour is not reached")
                aat, araw = witness(fit["above"], draft_ok=True, anywhere=True)
                require("entry" in fit["above"] and aat in reachable_from_entry(oracle, fit["above"]["entry"]),
                        "Fit's upper neighbour is not reached")
                low, high = fit_bounds(braw, araw)
                require(fits_once(position, size, low, high, even),
                        "The object fits in another place between its neighbours")
            if "fit" in symbol:
                require(not symbol["witnesses"] and length, "A fitted symbol needs an extent and no witness")
                fits(symbol["fit"], offset, length, True)
            for m in (m for m in members if "fit" in m):
                require(not m["witnesses"] and symbol["witnesses"] and m.get("extent", {}).get("kind") == "fill",
                        "A fitted member needs a fill extent of its own and a witnessed symbol")
                fits(m["fit"], (offset + m["offset"]) & 0xFFFF, m["extent"]["length"], False)
            for m in (m for m in members if "floor" in m):
                # The folded constant part of an index: the lowest byte the original access reaches is S or a
                # witnessed member of S (tools/index_floor.py).
                from index_floor import index_floor
                row = m["floor"]
                require(set(row) == {"space", "entry", "image_offset"} and internal and not m["witnesses"] and
                        row["entry"].split(":")[0] == row["space"] and
                        row["image_offset"] in reachable_from_entry(oracle, row["entry"]),
                        "Index floor is not a reached original access")
                fspace = oracle.spaces[row["space"]]
                f_seg, f_off = (int(x, 16) for x in row["entry"].split(":")[1:])
                found = index_floor(fspace, fspace.position(f_seg, f_off), (f_seg - fspace.link_segment) * 16,
                                    row["image_offset"])
                anchored = {offset} if symbol["witnesses"] else set()
                anchored |= {(offset + q["offset"]) & 0xFFFF for q in members if q["witnesses"]}
                require(found is not None and found[0] == (offset + m["offset"]) & 0xFFFF and
                        (found[0] + found[1]) & 0xFFFF in anchored,
                        "Index floor does not reach a witnessed address of the symbol")
            require(all(type(m.get("offset")) is int and 0 < m["offset"] < 65536 and
                        type(m.get("witnesses")) is list and
                        (m["witnesses"] or internal or m["offset"] < length or
                         (type(m.get("unproven")) is str and m["unproven"].strip() and not m["witnesses"]))
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
                memory = [o for o in rows[0].operands if o.type == _capstone()[1].X86_OP_MEM]
                require(field != "displacement" or not any(_not_dgroup(o) for o in memory),
                        "Witness displacement is relative to ES, CS or the stack frame, not DGROUP")
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
                target_ref["kind"] == "external" and
                (target_ref["method"] == 6 and fix["displacement"] is None or
                 # MASM `sym+k`: the addend is the fixup's target displacement (T2), under the DGROUP frame.
                 dgroup_frame and target_ref["method"] == 2 and type(fix["displacement"]) is int) and
                (fix["frame"] == {"kind": "target_frame", "target": target_ref} or dgroup_frame),
                "Unsupported DGROUP fixup equation")
        require(0 <= at <= len(actual)-2 and not occupied.intersection((at, at+1)),
                "Overlapping/out-of-range bound fields")
        symbol = symbols[target_ref["name"]]
        if mode == OPERAND_MODE:
            fields = [operand_field(i, at, candidate=True) for i in rows if operand_field(i, at, candidate=True)]
            require(len(fields) == 1, "v4 FIXUPP is not the disp16/imm16 field of one decoded instruction")
            addend = (struct.unpack_from("<H", actual, at)[0] + (fix["displacement"] or 0)) & 0xFFFF
            # A non-zero addend must be a member whose own address S+k is independently
            # witnessed; a free addend would let the binding re-fit S (2026-11-21 review).
            if symbol.get("kind") == "dgroup-public":
                # An accepted provider's data public: S is fixed by the provider's unique placed
                # content, so a member addend is not free to re-fit S; it must stay inside the
                # public's span (to the next data public or its segment end).
                insn = next(i for i in rows if operand_field(i, at, candidate=True))
                direct = [o for o in insn.operands if o.type == xc.X86_OP_MEM and o.mem.base == o.mem.index == 0]
                width = direct[0].size if fields[0] == "displacement" and direct else 1
                # `p < array + n`: the one-past-the-end address, only as a compared immediate.
                past = addend == symbol["span"] and fields[0] == "immediate" and insn.mnemonic == "cmp"
                require(addend + width <= symbol["span"] or past, "Addend leaves the provider data public's span")
            else:
                require(addend == 0 or addend in {m["offset"] for m in symbol.get("members", [])},
                        "v4 proves zero-addend fields or independently witnessed members only")
                member = next((m for m in symbol.get("members", []) if m["offset"] == addend), None)
                if member is not None and not member["witnesses"] and "extent" in symbol and \
                        addend < symbol["extent"]["length"]:
                    # An extent-grounded member: the whole access stays inside the copied unit.
                    insn = next(i for i in rows if operand_field(i, at, candidate=True))
                    direct = [o for o in insn.operands if o.type == xc.X86_OP_MEM and o.mem.base == o.mem.index == 0]
                    width = direct[0].size if fields[0] == "displacement" and direct else 1
                    require(addend + width <= symbol["extent"]["length"], "Member access leaves the aggregate extent")
            value = (symbol["offset"] + addend) & 0xFFFF
            observed = struct.unpack_from("<H", expected, at)[0]
            if value != observed:
                differences.append(at)
            equations.append({"offset": at, "symbol": target_ref["name"], "operand": fields[0],
                              "encoded_addend": addend, "grounded_dgroup_offset": symbol["offset"],
                              "linked_value": value, "original_value": observed, "equal": value == observed,
                              **({"provider": symbol["provider"]} if symbol.get("kind") == "dgroup-public" else {})})
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
