"""Probe the embedded MS-LINK/RTLink overlay descriptor and relocation layout.

This is a small evidence/probe utility, independent of the decompiler oracle.
It validates the checked-in PRINCE executable and offers a synthetic fixture
for the descriptor, paragraph, and relocation-page calculations.
"""
from __future__ import annotations

import argparse
import json
import struct
from dataclasses import asdict, dataclass
from pathlib import Path

DESC_FMT = '<HHHBBHHHHH'
DESC_SIZE = struct.calcsize(DESC_FMT)
DEFAULT_DESC_OFFSET = 150347
DEFAULT_OVERLAYS = 16


@dataclass
class Overlay:
    descriptor_offset: int
    link_segment: int
    f2: int
    relocation_position_paragraphs: int
    high_position_byte: int
    flags: int
    f8: int
    relocation_count: int
    fc: int
    overlay_id: int
    size_paragraphs: int
    relocation_offset: int
    relocation_page_bytes: int
    code_offset: int
    declared_code_end: int
    available_code_bytes: int
    short_by: int
    relocation_sites: int


def mz_end(image: bytes) -> int:
    if len(image) < 28 or image[:2] != b'MZ':
        raise ValueError('missing/truncated MZ header')
    cblp, cp = struct.unpack_from('<HH', image, 2)
    if cp == 0 or cblp > 511:
        raise ValueError('invalid MZ page fields')
    end = (cp - 1) * 512 + cblp if cblp else cp * 512
    if end > len(image):
        raise ValueError(f'MZ image ends at {end}, beyond physical file size {len(image)}')
    return end


def parse_overlays(image: bytes, descriptor_offset: int = DEFAULT_DESC_OFFSET,
                   count: int = DEFAULT_OVERLAYS, allowed_final_short: int = 1) -> list[Overlay]:
    end = mz_end(image)
    table_end = descriptor_offset + count * DESC_SIZE
    if descriptor_offset < 28 or table_end > end:
        raise ValueError(f'descriptor table [{descriptor_offset},{table_end}) outside MZ image [0,{end})')
    result: list[Overlay] = []
    for i in range(count):
        doff = descriptor_offset + i * DESC_SIZE
        seg, f2, pos, hi, flags, f8, nrel, fc, oid, size = struct.unpack_from(DESC_FMT, image, doff)
        if oid != i + 2:
            raise ValueError(f'descriptor {i} at {doff}: expected overlay ID {i+2}, got {oid}')
        if size == 0:
            raise ValueError(f'overlay {oid} has zero code size')
        reloc = (pos + hi * 65536) * 16
        reloc_page_bytes = ((nrel + 3) // 4) * 16
        code = reloc + reloc_page_bytes
        code_end = code + size * 16
        if reloc < end:
            raise ValueError(f'overlay {oid} relocations start {reloc} before MZ image end {end}')
        if code > len(image) or reloc + nrel * 4 > code:
            raise ValueError(f'overlay {oid} relocation block outside physical file')
        available = max(0, min(code_end, len(image)) - code)
        short_by = max(0, code_end - len(image))
        if short_by and (i != count - 1 or short_by > allowed_final_short):
            raise ValueError(f'overlay {oid} declared code ends at {code_end}, physical size {len(image)} (short {short_by})')
        sites = 0
        for j in range(nrel):
            roff, rseg = struct.unpack_from('<HH', image, reloc + 4 * j)
            site = (rseg - seg) * 16 + roff
            if site < 0 or site + 2 > size * 16:
                raise ValueError(f'overlay {oid} relocation {j} site {site} outside {size*16}-byte code')
            sites += 1
        result.append(Overlay(doff, seg, f2, pos, hi, flags, f8, nrel, fc, oid,
                              size, reloc, reloc_page_bytes, code, code_end,
                              available, short_by, sites))
    for current, following in zip(result, result[1:]):
        if current.declared_code_end != following.relocation_offset:
            raise ValueError(f'overlay {current.overlay_id} declared end {current.declared_code_end} '
                             f'does not meet overlay {following.overlay_id} relocation start '
                             f'{following.relocation_offset}')
    return result


def synthetic_fixture() -> bytes:
    """Create a 2-overlay image exercising 16-byte relocation page rounding."""
    root_end, table = 320, 32
    overlay_data = bytearray(64)
    # Overlay 2: one 4-byte relocation in a 16-byte relocation page, then 1 paragraph.
    struct.pack_into('<HHHH', overlay_data, 0, 0, 0x2000, 0, 0)
    # Overlay 3: no relocations, starts immediately after overlay 2's one-paragraph code.
    # Both extents are contiguous: 320 -> reloc-page 336 -> code 352 -> end 368.
    image = bytearray(root_end + len(overlay_data))
    image[:2] = b'MZ'
    struct.pack_into('<HHH', image, 2, root_end, 1, 0)  # cblp, cp, crlc
    struct.pack_into('<H', image, 8, 2)                 # two header paragraphs
    struct.pack_into('<H', image, 24, 28)               # relocation table offset
    struct.pack_into(DESC_FMT, image, table, 0x2000, 0, 20, 0, 4, 1, 1, 0xFFFF, 2, 1)
    struct.pack_into(DESC_FMT, image, table + DESC_SIZE, 0x2000, 0, 22, 0, 4, 2, 0, 0xFFFF, 3, 1)
    image[root_end:] = overlay_data
    return bytes(image)


def run_self_test() -> None:
    overlays = parse_overlays(synthetic_fixture(), descriptor_offset=32, count=2, allowed_final_short=0)
    assert [(x.overlay_id, x.relocation_offset, x.code_offset, x.declared_code_end, x.relocation_sites)
            for x in overlays] == [(2, 320, 336, 352, 1), (3, 352, 352, 368, 0)]
    print('synthetic RTLink layout probe: PASS')


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('image', nargs='?', type=Path, default=Path('assets/PRINCE.EXE'))
    ap.add_argument('--descriptor-offset', type=int, default=DEFAULT_DESC_OFFSET)
    ap.add_argument('--count', type=int, default=DEFAULT_OVERLAYS)
    ap.add_argument('--self-test', action='store_true')
    args = ap.parse_args()
    if args.self_test:
        run_self_test()
    image = args.image.read_bytes()
    overlays = parse_overlays(image, args.descriptor_offset, args.count)
    print(json.dumps({
        'image': str(args.image), 'physical_size': len(image), 'mz_end': mz_end(image),
        'descriptor_offset': args.descriptor_offset,
        'descriptor_table_end': args.descriptor_offset + args.count * DESC_SIZE,
        'overlays': [asdict(x) for x in overlays],
    }, indent=2))


if __name__ == '__main__':
    main()
