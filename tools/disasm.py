"""Pinned Capstone adapter for 16-bit x86 code in the supplied DOS image."""
from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator

CAPSTONE_ROOT = Path(r"C:\tools\capstone-5.0.3")
CAPSTONE_VERSION = "5.0.3"


def _capstone():
    """Load only the pinned Capstone distribution; never fetch dependencies."""
    if str(CAPSTONE_ROOT) not in sys.path:
        sys.path.insert(0, str(CAPSTONE_ROOT))
    try:
        import capstone
        from capstone import x86_const
    except ImportError as exc:
        raise RuntimeError(
            f"Capstone {CAPSTONE_VERSION} is required at {CAPSTONE_ROOT}; no download is attempted"
        ) from exc
    version = getattr(capstone, "__version__", "")
    if version != CAPSTONE_VERSION:
        raise RuntimeError(f"Expected Capstone {CAPSTONE_VERSION}, found {version or 'unknown'}")
    return capstone, x86_const


@dataclass(frozen=True)
class Instruction:
    offset: int
    size: int
    bytes_hex: str
    mnemonic: str
    operands: str
    groups: tuple[str, ...]
    flow: str | None = None
    near_target: int | None = None
    far_target: tuple[int, int] | None = None


def decode_one(data: bytes, offset: int, *, origin: int = 0) -> Instruction | None:
    """Decode one instruction at byte offset. ``origin`` is its real-mode IP base."""
    if offset < 0 or offset >= len(data):
        return None
    cs, xc = _capstone()
    engine = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_16)
    engine.detail = True
    engine.skipdata = False
    insn = next(engine.disasm(data[offset:offset + 15], origin + offset, count=1), None)
    return _adapt(insn, offset, xc) if insn is not None else None


def iter_instructions(data: bytes, start: int = 0, end: int | None = None,
                      *, origin: int = 0) -> Iterator[Instruction]:
    """Linear-sweep candidate bytes, stopping at the first undecodable byte."""
    if end is None:
        end = len(data)
    if start < 0 or end < start or end > len(data):
        raise ValueError("instruction range is outside the byte buffer")
    cs, xc = _capstone()
    engine = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_16)
    engine.detail = True
    engine.skipdata = False
    for insn in engine.disasm(data[start:end], origin + start):
        yield _adapt(insn, insn.address - origin, xc)


def _adapt(insn, offset: int, xc) -> Instruction:
    groups = tuple(insn.group_name(group) for group in insn.groups)
    flow = None
    if "call" in groups:
        flow = "call"
    elif "jump" in groups:
        flow = "jump"
    elif insn.mnemonic in {"ret", "retf", "iret", "iretd"}:
        flow = "return"

    near_target = None
    far_target = None
    if flow in {"call", "jump"} and insn.operands:
        op = insn.operands[0]
        if insn.mnemonic in {"lcall", "ljmp"} and len(insn.operands) >= 2:
            # Capstone's x86 detail order is segment, offset for immediate far flow.
            if all(item.type == xc.X86_OP_IMM for item in insn.operands[:2]):
                far_target = (int(insn.operands[0].imm) & 0xffff,
                              int(insn.operands[1].imm) & 0xffff)
        elif op.type == xc.X86_OP_IMM:
            # In 16-bit code, relative targets are IP offsets with 16-bit wrapping.
            near_target = int(op.imm) & 0xffff
    return Instruction(offset=offset, size=insn.size, bytes_hex=bytes(insn.bytes).hex(),
                       mnemonic=insn.mnemonic, operands=insn.op_str, groups=groups,
                       flow=flow, near_target=near_target, far_target=far_target)
