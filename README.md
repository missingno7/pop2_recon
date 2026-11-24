# Prince of Persia 2 historical reconstruction

C and assembly source that rebuilds the DOS version of Prince of Persia 2 (v1.1) with the original tools: Microsoft C 6.00A,
MASM 5.10/6.00 and RTLink/Plus 6.10. Each accepted function compiles to exactly the original's machine code, and everything
linked together gives the original program: every image byte is equal, and the file differs only in the order of its
relocation records. No SDL port has been started. What is still open is in the [roadmap](docs/roadmap.md).

Target: local `assets/pop2_11/PRINCE.EXE` (v1.1, the last release), 259,583 bytes, SHA-256
`c92298b62fc04fe66a0270fbaf082d7d7a5e37d0b5b7ed5adafccd43b26a67d7`, with its asset directory locked in
`layout/oracle.lock.json`. Until 2026-11-23 the target was the leaked pre-1.0 build in `assets/DOS`; that
reconstruction (1,565 exact components) is frozen at tag `oracle-dos-1565`. Game files and tools are not part of the
repository.

The executable contains a 151,979-byte resident load image and 16 attached spaces (overlays 2-17); the 14
compressed overlays are expanded by the original's own expander (`tools/overlay_expand.py`). File offsets, link
segment:offset pairs and runtime addresses stay distinct. Overlay spaces overlap
in memory and remain separately identified. See [RTLink evidence](docs/rtlink.md).

## Where things are

- `src/MODULE/` C and `asm/MODULE/` assembly, under the original module names ([source layout](docs/source-layout.md)).
  `CamelCase` names are original (FM Towns or Macintosh symbols), `snake_case` names are ours.
- `link/` data modules no accepted component owns yet, and `layout/link-plan.json`, the link script's contents.
- `recipes/`, `evidence/`, `layout/`: compiler flags, proofs of every address and pointer, and the list of owned bytes
  (`layout/manifest.json`, the only record of ownership).
- `tools/`, `tests/`: the proof tools. [acceptance.md](docs/acceptance.md) says what counts as exact.

## Commands

Python 3.10 or newer, from the repository:

```
python tools/validate.py            # compile and prove everything, link it, compare the whole file, run the tests
python tools/whole_link.py --check  # only the link and the comparison; writes build/whole_link/POP.EXE
python tools/context.py root:220e:0678 --asm   # original code of one address
```

Historical tools are read from `C:\tools`; hashes are pinned in
`layout/toolchain.lock.json`. Capstone 5.0.3 at `C:\tools\capstone-5.0.3` provides
diagnostic disassembly. Tool binaries, original assets, reference checkouts and
generated bulk output are ignored.

## Working on it

Follow [AGENTS.md](AGENTS.md) for worker waves, isolation and publication, and
[acceptance.md](docs/acceptance.md) for exactness and the gates' limits.
[Compiler rules](docs/codegen-rules.md) lists what is known about the code generator;
[external evidence](docs/external-evidence.md), [FM Towns](docs/fmtowns-symbols.md) and [Macintosh](docs/mac-symbols.md)
symbols are the outside sources. Pages marked historical describe the earlier build.
