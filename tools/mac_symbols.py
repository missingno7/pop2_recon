"""Extract MacsBug function names from the Macintosh PoP2 application's CODE resources.

The supplied Mac release (`assets/mac/*.sit`, StuffIt 5) is unpacked with The
Unarchiver's `unar` (pinned below) into ignored build/references/mac/ with
resource forks kept as visible `.rsrc` files (AppleDouble). THINK C/MPW place a
MacsBug name after each function's RTS, RTD or JMP (A0). The names are original
symbols of the Mac build. Segment grouping follows the Mac CODE resources, not
source files. Like the FM Towns names, they are external leads for DOS until
anchored in PRINCE.EXE.

Reproduce:
  C:/tools/unar-windows/unar.exe -k visible -o build/references/mac assets/mac/<archive>.sit
  python tools/mac_symbols.py
"""
import argparse
import re
import struct
from pathlib import Path

from common import ROOT, require, sha, write_json

FORK = ROOT / "build/references/mac/Prince of Persia 2/Prince of Persia 2.rsrc"
ARCHIVE = ROOT / "assets/mac/Prince-of-Persia-2-The-Shadow-The-Flame_Mac_EN.sit"
UNAR_SHA256 = {"unarWindows.zip": "61a6b299606282f72f51c278801eac11d3dccfac83e2d68bccce33539912e0dd"}
DEFAULT_OUTPUT = "evidence/mac/symbols.json"


def resource_fork(data):
    """The resource fork from an AppleDouble file (entry 2), or the data itself."""
    if data[:4] == b"\x00\x05\x16\x07":
        for k in range(struct.unpack_from(">H", data, 24)[0]):
            entry, offset, length = struct.unpack_from(">III", data, 26 + 12 * k)
            if entry == 2:
                return data[offset:offset + length]
        require(False, "AppleDouble file has no resource fork")
    return data


def code_resources(fork):
    data_offset, map_offset, _, map_length = struct.unpack_from(">IIII", fork, 0)
    rmap = fork[map_offset:map_offset + map_length]
    types_at, names_at = struct.unpack_from(">HH", rmap, 24)
    rows = []
    for t in range(struct.unpack_from(">H", rmap, types_at)[0] + 1):
        kind, count, ref = struct.unpack_from(">4sHH", rmap, types_at + 2 + 8 * t)
        if kind != b"CODE":
            continue
        for r in range(count + 1):
            rid, name_off, attr = struct.unpack_from(">hhI", rmap, types_at + ref + 12 * r)
            name = None
            if name_off != -1:
                length = rmap[names_at + name_off]
                name = rmap[names_at + name_off + 1:names_at + name_off + 1 + length].decode("mac_roman")
            at = data_offset + (attr & 0xFFFFFF)
            size = struct.unpack_from(">I", fork, at)[0]
            rows.append((rid, name, fork[at + 4:at + 4 + size]))
    return sorted(rows)


def macsbug_names(code):
    """(end offset, name) after RTS / JMP (A0) / RTD #n, variable-length MacsBug format."""
    out = []
    for i in range(0, len(code) - 2, 2):
        word = struct.unpack_from(">H", code, i)[0]
        if word not in (0x4E75, 0x4ED0, 0x4E74):
            continue
        j = i + 2 + (2 if word == 0x4E74 else 0)
        if j >= len(code):
            continue
        first = code[j]
        if 0x80 < first < 0xA0:
            text = code[j + 1:j + 1 + first - 0x80]
        elif first == 0x80 and j + 1 < len(code):
            text = code[j + 2:j + 2 + code[j + 1]]
        else:
            continue
        if text and re.fullmatch(rb"[A-Za-z_][A-Za-z0-9_]*", text):
            out.append({"end_offset": j, "name": text.decode("ascii")})
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fork", default=str(FORK))
    parser.add_argument("--output", default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    raw = Path(args.fork).read_bytes()
    fork = resource_fork(raw)
    segments = []
    for rid, name, code in code_resources(fork):
        names = macsbug_names(code)
        segments.append({"id": rid, "name": name, "size": len(code), "functions": names})
    report = {"schema": 1,
              "authority": "Mac release MacsBug symbols; names are external leads, not DOS facts",
              "source": {"archive": ARCHIVE.relative_to(ROOT).as_posix(),
                         "archive_sha256": sha(ARCHIVE.read_bytes()) if ARCHIVE.is_file() else None,
                         "application_fork_sha256": sha(fork), "extractor": UNAR_SHA256},
              "segments": segments}
    write_json(ROOT / args.output, report)
    print(f"{len(segments)} CODE resources, {sum(len(s['functions']) for s in segments)} MacsBug names "
          f"-> {args.output}")


if __name__ == "__main__":
    main()
