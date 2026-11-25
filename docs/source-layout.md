# Source layout and names — 2026-11-21

Canonical source follows the original source files, which the FM Towns debug
symbols name ([fmtowns-symbols.md](fmtowns-symbols.md)).
[evidence/modules.json](../evidence/modules.json) maps DOS code frames and
overlay regions to those modules:

- **Root:** 21 of 30 frames anchored, 1 probable.
- **Overlays:** 22 of 24 regions anchored, 2 probable. Overlay attribution uses
  the 36 RTLink vectors.
- **Function pairs:** 104 DOS–FM Towns pairs anchored, 852 probable by ordered
  alignment.

**Layout.**

- `src/MODULE/` and `asm/MODULE/` use the lowercase original module name, e.g.
  `src/ctrl/`, `src/draw/`, `asm/mcga1/`.
- `include/MODULE.h` is the interface of a module: its types, its variables in the order of their addresses, its functions
  in alphabetical order. `types.h` (scalars, `Rect`, `Handle`), `level.h`, `char.h` and `game.h` hold the records of the
  game, `graf.h`, `shape.h`, `resc.h`, `heap.h`, `snd.h`, `system.h`, `window.h` the library, `tables.h` and `english.h` the
  data modules. A name is declared once. Headers include nothing themselves and have no guards; a file includes what it
  needs before it, after the compiler's own headers.
- The order of the includes, and a header that a file does not use, are not free: they give the symbol numbers behind the
  operand order of the original code (codegen-rules FRAME-7). LOAD declares its private functions itself for the same reason.
- Components whose module is not anchored stay at the top of `src/`.
- Recipes and bindings mirror the same paths (`recipes/ctrl/…`,
  `evidence/bindings/ctrl/…`).

**Names.**

- **CamelCase** is an original name. FM Towns (debug symbols, 1,788 publics) and the Macintosh version (MacsBug names) list
  a module's functions in the order our files have them, so most names follow from the place in the file; MOVE agrees in
  86 of 86 functions, FINAL in 56 of 56. `evidence/targets.json` records each as `original_name` with the reason in
  `original_name_evidence`. The spelling is that of the other build; DOS itself keeps no names.
- **snake_case** is a descriptive name of ours, e.g. `find_option`, where no build names the function (static, inlined
  or DOS-only code).
- Globals follow the same rule. FM Towns names (`Dir`, `SelDir`, `FChar`, `Kid`,
  `Char`) are used only where an anchor ties the DOS address to them.

`python tools/relocate.py OLD --name module/new --rename OLD=New` moves an
accepted component and renames identifiers. It re-proves exactness through the
original gate and keeps every binding witness.

**What is known to differ between builds:**

- Frame `1101` is one DOS code segment whose functions correspond to several FM
  Towns modules (XLATGRAF, GRAF, PTOOL, RESC, UNPACK, ...).
- Frame `052d` spans FM Towns DIALOG and DIALOG2.
- FM Towns inlines some DOS helpers, such as the delta update in `BoostMeter`.

Source order inside a module follows the FM Towns public order where pairs are
anchored, with recorded exceptions. Use it when drafting whole runs for the group
gate.
