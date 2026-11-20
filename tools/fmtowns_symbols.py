"""Extract the Turbo Debugger symbol tables from the FM Towns PoP2 release.

The FM Towns CD (supplied, ignored under assets/fmtowns/) holds PRINCE2/PRINCE.EXE,
a 16-bit Borland C build with TD 3.x debug info appended after its MZ load
image. That info names 82 source modules in link order, about 1,800 public
functions and globals, and gives segment records for the assembly modules.

Original facts (read directly): module names, languages and order; source file
names; every symbol's name, segment and offset; ASM segment extents.
Hypothesis (marked in the output): C modules carry no segment records, so each
C module's code segment is assigned by link order, scored by how well module and
function names agree. Nothing here is DOS evidence: names need an independent
anchor in PRINCE.EXE before they become DOS names (AGENTS.md naming rules).
"""
import argparse
import re
import struct
from pathlib import Path

from common import ROOT, require, sha, write_json

CD_IMAGE = ROOT / "assets/fmtowns/Game/Prince of Persia 2 (J).bin"
EXE_PATH = ("PRINCE2", "PRINCE.EXE")
DEFAULT_OUTPUT = "evidence/fmtowns/symbols.json"
SECTOR, DATA_OFF, DATA_LEN = 2352, 16, 2048


def iso_file(image, path):
    """One file from the MODE1/2352 data track's ISO9660 tree (read-only)."""
    with open(image, "rb") as f:
        def read(lba, size):
            out = bytearray()
            for i in range((size + DATA_LEN - 1) // DATA_LEN):
                f.seek((lba + i) * SECTOR + DATA_OFF)
                out += f.read(DATA_LEN)
            return bytes(out[:size])

        pvd = read(16, DATA_LEN)
        require(pvd[1:6] == b"CD001", "Not an ISO9660 MODE1/2352 image")
        lba, size = struct.unpack_from("<I", pvd, 158)[0], struct.unpack_from("<I", pvd, 166)[0]
        for part in path:
            data, at, found = read(lba, size), 0, None
            while at < size:
                n = data[at]
                if n == 0:
                    at = (at // DATA_LEN + 1) * DATA_LEN
                    continue
                rec = data[at:at + n]
                at += n
                name = rec[33:33 + rec[32]].decode("ascii", "replace").split(";")[0]
                if name == part:
                    found = struct.unpack_from("<I", rec, 2)[0], struct.unpack_from("<I", rec, 10)[0]
                    break
            require(found, f"{part} not on the CD")
            lba, size = found
        return read(lba, size)


def td_info(exe):
    """Parse the TD 3.x tables used here; layout cross-checked against the counts."""
    cblp, pages = struct.unpack_from("<HH", exe, 2)
    base = (pages - 1) * 512 + cblp if cblp else pages * 512
    dbg = exe[base:]
    head = struct.unpack_from("<HHIHHHHHHHHHHHH", dbg, 0)
    (magic, version, names_size, names_count, types, members, symbols, globals_, modules,
     locals_, scopes, lines, sources, segments, correlations) = head
    require(magic == 0x52FB and version == 0x0310, "Not Turbo Debugger 3.10 symbol information")
    names = dbg[len(dbg) - names_size:].split(b"\0")[:names_count]
    require(len(names) == names_count, "Name pool count mismatch")

    def name(index):
        return names[index - 1].decode("ascii") if 0 < index <= names_count else None

    at = 80  # header plus extension; symbol table follows
    symbol_rows = [struct.unpack_from("<HHHHB", dbg, at + 9 * k) for k in range(symbols)]
    at += 9 * symbols
    module_rows = [struct.unpack_from("<HBBHHHHHH", dbg, at + 16 * k) for k in range(modules)]
    at += 16 * modules
    source_rows = [struct.unpack_from("<HI", dbg, at + 6 * k) for k in range(sources)]
    at += 6 * sources + 4 * lines + 2 * 0  # line table (line, offset) pairs, unused here
    segment_rows = [struct.unpack_from("<8H", dbg, at + 16 * k) for k in range(segments)]
    require(all(1 <= r[0] <= modules for r in segment_rows), "Segment table misaligned")
    return {"base": base, "header": dict(zip(
                ("magic", "version", "names_size", "names_count", "types", "members", "symbols", "globals",
                 "modules", "locals", "scopes", "lines", "sources", "segments", "correlations"), head)),
            "symbols": [{"name": name(n), "segment": s, "offset": o, "flags": f} for n, _, o, s, f in symbol_rows],
            "modules": [{"index": k + 1, "name": name(r[0]), "language": {0: "asm", 1: "c"}.get(r[1], r[1]),
                         "source_file": name(source_rows[r[5] - 1][0]) if r[5] else None,
                         "segment_records": [{"segment": s[1], "offset": s[2], "length": s[3]}
                                             for s in segment_rows if s[0] == k + 1]}
                        for k, r in enumerate(module_rows)]}


def _tokens(text):
    return {t.lower() for t in re.findall(r"[A-Z][a-z]+|[a-z]+|[A-Z]+(?![a-z])", text) if len(t) >= 3}


def assign_c_modules(info):
    """Order-preserving assignment of C code segments to C modules (a hypothesis)."""
    asm_segments = {r["segment"] for m in info["modules"] for r in m["segment_records"]}
    groups = {}
    for s in info["symbols"]:
        if s["segment"] and s["segment"] not in asm_segments:
            groups.setdefault(s["segment"], []).append(s)
    # Borland register-convention functions carry '@'; far data segments do not.
    code = sorted(seg for seg, rows in groups.items() if any(r["name"].startswith("@") for r in rows))
    modules = [m for m in info["modules"] if m["language"] == "c"]

    def score(module, seg):
        stem = module["name"].lower()
        words = set().union(*(_tokens(r["name"]) for r in groups[seg]))
        return sum(1 for w in words if w in stem or stem.startswith(w[:4]))

    # DP: every code segment goes to exactly one module, in order; modules may stay empty.
    m, n = len(modules), len(code)
    best = [[None] * (n + 1) for _ in range(m + 1)]
    best[0][0] = (0, None)
    for i in range(1, m + 1):
        for j in range(0, n + 1):
            options = []
            if best[i-1][j] is not None:
                options.append((best[i-1][j][0], (i-1, j, False)))
            if j and best[i-1][j-1] is not None:
                options.append((best[i-1][j-1][0] + score(modules[i-1], code[j-1]) + 0.01, (i-1, j-1, True)))
            if options:
                best[i][j] = max(options, key=lambda o: o[0])
    require(best[m][n] is not None, "C modules cannot cover the code segments in order")
    assignment, i, j = {}, m, n
    while i:
        _, (pi, pj, took) = best[i][j]
        if took:
            assignment[modules[i-1]["name"]] = code[j-1]
        i, j = pi, pj
    rows = []
    for module in info["modules"]:
        if module["language"] == "asm":
            segs = [r["segment"] for r in module["segment_records"]]
            # Several ASM modules share one segment; each owns only its offset range.
            funcs = [s for s in info["symbols"] for r in module["segment_records"]
                     if s["segment"] == r["segment"] and r["offset"] <= s["offset"] < r["offset"] + r["length"]]
            basis = "asm segment record (original fact)"
        elif module["name"] in assignment:
            segs = [assignment[module["name"]]]
            funcs = groups[segs[0]]
            basis = (f"link-order assignment, name agreement {score(module, segs[0])} (hypothesis)")
        else:
            segs, funcs, basis = [], [], "no code segment assigned (data-only or no public functions)"
        ordered = sorted(funcs, key=lambda s: (s["segment"], s["offset"]))
        rows.append(dict(module, code_segments=segs, mapping_basis=basis,
                         functions=[{"name": s["name"], "segment": s["segment"], "offset": s["offset"]}
                                    for s in ordered]))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", default=str(CD_IMAGE))
    parser.add_argument("--output", default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    exe = iso_file(args.image, EXE_PATH)
    info = td_info(exe)
    modules = assign_c_modules(info)
    placed = {f["name"] for m in modules for f in m["functions"]}
    report = {"schema": 1,
              "authority": "FM Towns release debug symbols; names are external leads, not DOS facts",
              "source": {"cd_path": "/".join(EXE_PATH), "exe_size": len(exe), "exe_sha256": sha(exe),
                         "debug_base": info["base"]},
              "header": info["header"], "modules": modules,
              "runtime_symbols": [s["name"] for s in info["symbols"] if s["segment"] == 0],
              "other_symbols": [{"name": s["name"], "segment": s["segment"], "offset": s["offset"]}
                                for s in info["symbols"] if s["segment"] and s["name"] not in placed]}
    write_json(ROOT / args.output, report)
    code = [m for m in modules if m["functions"]]
    print(f"{len(modules)} modules, {sum(len(m['functions']) for m in modules)} functions in "
          f"{len(code)} modules with code, {len(report['other_symbols'])} data/other symbols -> {args.output}")


if __name__ == "__main__":
    main()
