"""Lowest index of an original indexed access (docs/dgroup-binding.md "Index floors").

MSC folds a constant part of an index into the displacement: `cells[column - 1]` with a column that starts at 1
becomes `[bx+cells-1]`, so the displacement names no object. The original code bounds the index register from below
(a counter that starts at 1, a comparison that guards the access), and displacement + lowest index is the first byte
the access can reach. This module computes that bound from the original bytes of the routine alone."""
import struct

from disasm import _capstone

_WORD = {"ax": "a", "bx": "b", "cx": "c", "dx": "d", "si": "si", "di": "di"}
_HALF = {half + part: half + "x" for half in "abcd" for part in "lh"}
_LIMIT = {1: (-128, 127), 2: (-32768, 32767)}
_FLIP = {"jl": ("lt", "ge"), "jge": ("ge", "lt"), "jg": ("gt", "le"), "jle": ("le", "gt"), "je": ("eq", "ne"),
         "jne": ("ne", "eq")}


def _signed(value, size):
    value &= (1 << (8 * size)) - 1
    return value - (1 << (8 * size)) if value >> (8 * size - 1) else value


class _Routine:
    """Interval analysis of one routine: signed ranges of registers and of BP stack slots before each instruction.

    Anything not followed is unknown. Arithmetic that could leave the signed range of its size is unknown, so a wrapped
    value never counts. Calls keep SI, DI and stack slots (the compiler's own convention, which the original loop
    relies on) and lose the other registers; a routine that takes the address of a stack slot keeps no slot."""

    def __init__(self, space, start, base):
        cs, self.xc = _capstone()
        self.space, self.start, self.base = space, start, base
        self.engine = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_16)
        self.engine.detail = True
        self.cs, self.rows, self.states = cs, {}, {}
        self.slots = True
        self._solve()

    def decode(self, pos):
        if pos not in self.rows:
            data = self.space.data[pos:pos + 16]
            self.rows[pos] = next(self.engine.disasm(bytes(data), pos - self.base), None)
        return self.rows[pos]

    def location(self, insn, op):
        if op.type == self.xc.X86_OP_REG:
            name = insn.reg_name(op.reg)
            return name if name in _WORD or name in _HALF else None
        if op.type == self.xc.X86_OP_MEM and op.mem.base == self.xc.X86_REG_BP and not op.mem.index and \
                op.mem.segment in (0, self.xc.X86_REG_SS) and self.slots and op.size in (1, 2):
            return ("slot", op.mem.disp, op.size)
        return None

    def read(self, state, insn, op):
        if op.type == self.xc.X86_OP_IMM:
            value = _signed(op.imm, op.size)
            return value, value
        return state.get(self.location(insn, op))

    @staticmethod
    def kill(state, location):
        if isinstance(location, tuple):
            lo, hi = location[1], location[1] + location[2]
            for key in [k for k in state if isinstance(k, tuple) and k[1] < hi and lo < k[1] + k[2]]:
                del state[key]
        else:
            state.pop(location, None)
            for half, full in _HALF.items():
                if location == full:
                    state.pop(half, None)
                elif location == half:
                    state.pop(full, None)

    def write(self, state, location, value, size):
        self.kill(state, location)
        if value is not None and _LIMIT[size][0] <= value[0] <= value[1] <= _LIMIT[size][1]:
            state[location] = value

    def step(self, state, insn):
        """State after a non-branching instruction."""
        from binding import registers_written
        xc, ops, name = self.xc, insn.operands, insn.mnemonic
        state = dict(state)
        if name in ("lcall", "call"):
            for full in ("ax", "bx", "cx", "dx"):
                self.kill(state, full)
            return state
        target = self.location(insn, ops[0]) if ops else None
        size = ops[0].size if ops else 0
        result, known = None, False
        if target is not None and size in (1, 2):
            a = state.get(target)
            b = self.read(state, insn, ops[1]) if len(ops) == 2 else None
            same = len(ops) == 2 and ops[0].type == ops[1].type == xc.X86_OP_REG and ops[0].reg == ops[1].reg
            if name == "mov":
                known, result = True, b
            elif name in ("sub", "xor") and same:
                known, result = True, (0, 0)
            elif name == "add":
                known, result = True, (a[0] + b[0], a[1] + b[1]) if a and b else None
            elif name == "sub":
                known, result = True, (a[0] - b[1], a[1] - b[0]) if a and b else None
            elif name in ("inc", "dec") and len(ops) == 1:
                k = 1 if name == "inc" else -1
                known, result = True, (a[0] + k, a[1] + k) if a else None
            elif name in ("shl", "sal"):
                count = (1, 1) if len(ops) == 1 else b
                known = True
                if a and count and count[0] == count[1] and 0 < count[0] < 15:
                    result = (a[0] << count[0], a[1] << count[0])
            elif name == "lea" and size == 2:
                mem = ops[1].mem
                parts = [state.get(insn.reg_name(r)) for r in (mem.base, mem.index) if r]
                known = True
                if parts and all(parts):
                    d = _signed(mem.disp, 2)
                    result = (sum(p[0] for p in parts) + d, sum(p[1] for p in parts) + d)
        if name == "cbw" or bytes(insn.bytes) == b"\x98":
            low = state.get("al")
            self.kill(state, "ax")
            if low:
                state["al"] = low
                state["ax"] = low
            return state
        if name == "mul" and size == 1:
            factor = state.get("al")
            self.kill(state, "ax")
            if factor and factor[0] == factor[1] and factor[0] >= 0:
                state["ax"] = (0, factor[0] * 255)   # the other factor is one unsigned byte
                if state["ax"][1] > _LIMIT[2][1]:
                    del state["ax"]
            return state
        if name in ("cmp", "test", "push", "nop"):
            return state
        if known:
            self.write(state, target, result, size)
            return state
        for op in ops[:2] if name == "xchg" else ops[:1]:
            if op.type == xc.X86_OP_MEM and xc.X86_REG_BP in (op.mem.base, op.mem.index):
                where = self.location(insn, op)
                for key in [where] if where else [k for k in state if isinstance(k, tuple)]:
                    self.kill(state, key)     # an unfollowed write to one stack slot, or to any of them
        for written in registers_written(insn):
            if written in _WORD or written in _HALF:
                self.kill(state, written)
        return state

    def refine(self, state, compare, relation):
        location, value = compare
        lo, hi = state.get(location, _LIMIT[self.compare_size(location)])
        if relation == "lt":
            hi = min(hi, value - 1)
        elif relation == "le":
            hi = min(hi, value)
        elif relation == "gt":
            lo = max(lo, value + 1)
        elif relation == "ge":
            lo = max(lo, value)
        elif relation == "eq":
            lo, hi = max(lo, value), min(hi, value)
        else:
            lo, hi = lo + (lo == value), hi - (hi == value)
        if lo > hi:
            return None
        return {**state, location: (lo, hi)}

    @staticmethod
    def compare_size(location):
        return location[2] if isinstance(location, tuple) else 2 if location in _WORD else 1

    def compare(self, insn):
        if insn is None or insn.mnemonic != "cmp" or insn.operands[1].type != self.xc.X86_OP_IMM:
            return None
        location = self.location(insn, insn.operands[0])
        return None if location is None else (location, _signed(insn.operands[1].imm, insn.operands[0].size))

    def edges(self, pos, state, compare):
        """(successor, state, pending compare) of the instruction at `pos`."""
        from binding import _switch_targets
        insn = self.decode(pos)
        if insn is None:
            return None
        groups, name = set(insn.groups), insn.mnemonic.split()[-1]
        after = pos + insn.size
        if groups.intersection((self.cs.CS_GRP_RET, self.cs.CS_GRP_IRET)) or name in ("ljmp", "hlt"):
            return []
        if name == "jmp":
            if insn.operands[0].type == self.xc.X86_OP_IMM:
                return [(self.base + (insn.operands[0].imm & 0xFFFF), state, None)]
            switch = _switch_targets(self.space.data, pos, set(self.rows))
            if not switch:
                return None
            table, count = switch
            return [(self.base + struct.unpack_from("<H", self.space.data, self.base + ((table + 2 * k) & 0xFFFF))[0],
                     state, None) for k in range(count)]
        if self.cs.CS_GRP_JUMP in groups or name.startswith("loop") or name == "jcxz":
            out = []
            if insn.operands[0].type != self.xc.X86_OP_IMM:
                return None
            rows = [(self.base + (insn.operands[0].imm & 0xFFFF), 0), (after, 1)]
            if name.startswith("loop"):
                state = dict(state)
                self.kill(state, "cx")
            for successor, side in rows:
                refined = state
                if compare is not None and name in _FLIP:
                    refined = self.refine(state, compare, _FLIP[name][side])
                if refined is not None:
                    out.append((successor, refined, compare))
            return out
        following = self.step(state, insn)
        pending = self.compare(insn)
        if pending is None and name == "nop":
            pending = compare
        return [(after, following, pending)]

    def _solve(self):
        # First pass: find the routine's instructions and whether a stack address is taken.
        for self.slots in (True, False):
            self.rows, self.states = {}, {}
            if not self._iterate():
                self.states = {}
                return
            taken = any(i is not None and (i.mnemonic == "lea" and i.operands[1].mem.base == self.xc.X86_REG_BP or
                                           i.mnemonic == "mov" and i.operands[1].type == self.xc.X86_OP_REG and
                                           i.reg_name(i.operands[1].reg) in ("bp", "sp") and
                                           i.reg_name(i.operands[0].reg) not in ("bp", "sp"))
                        for i in self.rows.values())
            if not taken:
                return

    def _iterate(self):
        work, visits, thresholds = [self.start], {}, {-32768, -128, 0, 127, 32767}
        self.states = {self.start: ({}, None)}
        while work:
            pos = work.pop(0)
            state, compare = self.states[pos]
            out = self.edges(pos, state, compare)
            if out is None:
                return False
            for insn in (self.rows[pos],):
                if insn.mnemonic == "cmp" and insn.operands[1].type == self.xc.X86_OP_IMM:
                    k = _signed(insn.operands[1].imm, insn.operands[0].size)
                    thresholds |= {k - 1, k, k + 1}
            for successor, new, pending in out:
                if not 0 <= successor < self.space.size:
                    return False
                if successor not in self.states:
                    self.states[successor] = (new, pending)
                    work.append(successor)
                    continue
                old, old_pending = self.states[successor]
                visits[successor] = visits.get(successor, 0) + 1
                joined = {}
                for key in old.keys() & new.keys():
                    lo, hi = min(old[key][0], new[key][0]), max(old[key][1], new[key][1])
                    if visits[successor] > 8:         # widen to the next constant the routine compares with
                        if lo < old[key][0]:
                            lo = max(t for t in thresholds if t <= lo)
                        if hi > old[key][1]:
                            hi = min(t for t in thresholds if t >= hi)
                    joined[key] = (lo, hi)
                merged = pending if pending == old_pending else None
                if (joined, merged) != (old, old_pending):
                    self.states[successor] = (joined, merged)
                    if successor not in work:
                        work.append(successor)
        return True


def index_floor(space, start, base, site):
    """(displacement, lowest index) of the DGROUP access at `site` in the routine that starts at `start` (positions in
    `space`; `base` is the position of its CS window), or None. The index registers' signed ranges must be known and
    their lowest sum positive."""
    routine = _Routine(space, start, base)
    state = routine.states.get(site)
    insn = routine.rows.get(site)
    if state is None or insn is None:
        return None
    memory = [o for o in insn.operands if o.type == routine.xc.X86_OP_MEM]
    if len(memory) != 1 or insn.disp_size != 2 or insn.mnemonic == "lea" or \
            memory[0].mem.segment not in (0, routine.xc.X86_REG_DS):
        return None
    parts = [insn.reg_name(r) for r in (memory[0].mem.base, memory[0].mem.index) if r]
    if not parts or "bp" in parts:
        return None
    ranges = [state[0].get(name) for name in parts]
    if not all(ranges):
        return None
    low, high = sum(r[0] for r in ranges), sum(r[1] for r in ranges)
    return (memory[0].mem.disp & 0xFFFF, low) if 0 < low <= high < 0x8000 else None


def frame_writes(space, start, base):
    """[lo, hi) displacement ranges of BP-based memory the routine's own instructions may write; everything when an
    indexed BP access or an unfollowed jump makes it unknown."""
    routine = _Routine(space, start, base)
    xc, ranges = routine.xc, []
    if not routine.states:
        return [(-0x8000, 0x8000)]
    for insn in routine.rows.values():
        for op in insn.operands:
            if op.type != xc.X86_OP_MEM or op.mem.base != xc.X86_REG_BP or op.mem.segment not in (0, xc.X86_REG_SS) or                     insn.mnemonic in ("push", "cmp", "test", "lea") or not op.access & 2:
                continue
            ranges.append((-0x8000, 0x8000) if op.mem.index else (op.mem.disp, op.mem.disp + op.size))
        if insn.mnemonic == "lea" and insn.operands[1].mem.base == xc.X86_REG_BP:
            ranges.append((-0x8000, 0x8000) if insn.operands[1].mem.disp > 0 else (0, 0))
    return ranges
