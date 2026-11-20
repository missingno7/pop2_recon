# FM Towns debug symbols — 2026-11-21

The supplied FM Towns release (`assets/fmtowns/`, ignored) contains
`PRINCE2/PRINCE.EXE`, 415,279 bytes. It is a 16-bit Borland C build with Turbo
Debugger 3.10 symbol tables appended after its MZ load image. None of the
bundled Windows tools or emulator files were run. The CD image is read directly,
and the tables are written to [symbols.json](../evidence/fmtowns/symbols.json):

```
python tools/fmtowns_symbols.py
```

## What is original fact

The tables name the following, in link order:

- **82 source modules**: 71 C and 11 assembly. There are platform modules (CD,
  DOS, PCM, MIDI, FONT, FAST*), engine modules (DRAW, GRAF, SHAPE, HEAP,
  XMALLOC, RESC, LOAD), and an alphabetical run of game modules (ACT, ADD,
  ALTAR, ANIM, AUTO, AUTOGUAR, AUTOHEAD, AUTOSHAD, BRIDGE, CHECKNIS, COLLIDE,
  CTRL, ...).
- **Their source file names** (`ACT.C`, `UNPACK.ASM`, ...).
- **1,788 public symbols** with segment and offset. These are game functions
  (`@JumpSeq`, `@DoJumpUp`, `@CheckBarr`, `@DrawCaveBlock`, ...), about 290
  DGROUP globals, far data tables, and the Borland runtime.
- **Exact segment and offset extents for the ASM modules.** Several share one
  segment; for example, UNPACK owns `UNPACKLZH/LZV/RLEH/RLEV`.

## What is a hypothesis

C modules have no segment records. The tool assigns each C code segment to a C
module by link order: a dynamic program that keeps order and scores how far
module and function names agree. Every one of the 67 C code segments lands on a
plausible module. ENGLISH, ERR, FRAMES and TABLES come out with no code, which
fits their names. Each module row states its `mapping_basis`.

## How to use it for DOS

This is a different build of the same game: a different compiler (Borland, not
MSC 6.00), a different platform layer, and a different link. Bytes and offsets
do not transfer. Two things do:

- **Names.** Under AGENTS.md naming rules a function name becomes a DOS name only
  after an independent anchor in PRINCE.EXE: matching strings, constants, data
  tables, call graph or argument use.
- **Module membership.** DOS frames are one per medium-model file (see
  [tu-census.md](tu-census.md)). Matching a DOS frame to an FM Towns module
  gives the original file name and its function list. The function order in a
  module is evidence for source order, which the group gate depends on. Module
  contents may still differ between platforms.

Version caveat: the FM Towns release is from 1994 (Interprog), later than the
DOS 1.0 target. Code may have changed between them.
