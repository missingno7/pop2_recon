"""Expand RTLink-compressed overlays by running the executable's own $$RTLEXPANDSECTION.

RTLink/Plus marks a compressed overlay with descriptor flag 20h; OVLMGR then calls the expander with the DOS file
handle, the memory paragraphs and the load segment instead of reading the payload itself. The expander only reads
through INT 21h AH=3Fh and writes the load segment, so a 16-bit CPU emulator running the original bytes gives the
exact overlay image. Each expanded image is pinned by SHA-256 in layout/oracle-format.json; build/oracle/expanded is
a cache that is used only while its file equals that pin.
"""
import struct

from common import ROOT, atomic_write, require, sha

COMPRESSED = 0x20
LOAD = 0x1000          # program load paragraph
DESTINATION = 0x8000   # overlay load paragraph; the stack and the return HLT lie elsewhere
STACK, RETURN = 0x5000, 0x4000
HANDLE = 5
CACHE = ROOT / "build/oracle/expanded"


def emulate(data, mz, entry, payload, paragraphs):
    """Run the expander at link address `entry` on `payload`; return exactly the bytes it wrote."""
    from unicorn import Uc, UC_ARCH_X86, UC_HOOK_INTR, UC_HOOK_MEM_WRITE, UC_MODE_16
    from unicorn import x86_const as x86
    segment, offset = entry
    image = bytearray(mz.load_image(data))
    for row in mz.relocations:
        word = struct.unpack_from("<H", image, row["image_offset"])[0]
        struct.pack_into("<H", image, row["image_offset"], (word + LOAD) & 0xffff)
    require(LOAD * 16 + len(image) <= RETURN * 16 and paragraphs <= 0x1000, "Expander layout does not fit")
    cpu = Uc(UC_ARCH_X86, UC_MODE_16)
    cpu.mem_map(0, 0x110000)
    cpu.mem_write(LOAD * 16, bytes(image))
    cpu.mem_write(RETURN * 16, b"\xf4")
    state, written = {"read": 0}, set()

    def interrupt(mu, number, _):
        ax = mu.reg_read(x86.UC_X86_REG_AX)
        require(number == 0x21 and ax >> 8 == 0x3f and mu.reg_read(x86.UC_X86_REG_BX) == HANDLE,
                f"Expander made an unexpected request {number:#x}/{ax:#x}")
        count = mu.reg_read(x86.UC_X86_REG_CX)
        chunk = payload[state["read"]:state["read"] + count]
        mu.mem_write(mu.reg_read(x86.UC_X86_REG_DS) * 16 + mu.reg_read(x86.UC_X86_REG_DX), chunk)
        state["read"] += len(chunk)
        mu.reg_write(x86.UC_X86_REG_AX, len(chunk))
        mu.reg_write(x86.UC_X86_REG_EFLAGS, mu.reg_read(x86.UC_X86_REG_EFLAGS) & ~1)

    def wrote(mu, access, address, size, value, _):
        if DESTINATION * 16 <= address:
            written.update(range(address - DESTINATION * 16, address - DESTINATION * 16 + size))

    cpu.hook_add(UC_HOOK_INTR, interrupt)
    cpu.hook_add(UC_HOOK_MEM_WRITE, wrote)
    stack = struct.pack("<5H", 0, RETURN, DESTINATION, paragraphs, HANDLE)  # return address, DS, paragraphs, handle
    sp = 0xf000 - len(stack)
    cpu.mem_write(STACK * 16 + sp, stack)
    for register, value in ((x86.UC_X86_REG_SS, STACK), (x86.UC_X86_REG_SP, sp), (x86.UC_X86_REG_DS, DESTINATION),
                            (x86.UC_X86_REG_ES, DESTINATION), (x86.UC_X86_REG_AX, HANDLE),
                            (x86.UC_X86_REG_CS, segment + LOAD), (x86.UC_X86_REG_IP, offset)):
        cpu.reg_write(register, value)
    cpu.emu_start((segment + LOAD) * 16 + offset, RETURN * 16, count=50_000_000)
    require(cpu.reg_read(x86.UC_X86_REG_CS) * 16 + cpu.reg_read(x86.UC_X86_REG_IP) == RETURN * 16 and
            cpu.reg_read(x86.UC_X86_REG_AX) == 0, "Expander did not finish successfully")
    size = len(written)
    require(size and written == set(range(size)) and paragraphs * 16 - 16 < size <= paragraphs * 16,
            "Expanded bytes are not one block filling the declared memory paragraphs")
    return bytes(cpu.mem_read(DESTINATION * 16, size))


def expand(data, mz, profile, overlay_id, payload, paragraphs):
    """Pinned expanded image of one compressed overlay."""
    pin = profile["expanded_sha256_by_overlay"][str(overlay_id)]
    cached = CACHE / f"overlay-{overlay_id}.bin"
    if cached.is_file():
        image = cached.read_bytes()
        if sha(image) == pin:
            return image
    image = emulate(data, mz, profile["expander_entry"], payload, paragraphs)
    require(sha(image) == pin, f"Expanded overlay {overlay_id} differs from its pinned identity")
    try:
        atomic_write(cached, image)
    except OSError:
        pass  # a read-only checkout simply runs the expander again
    return image
