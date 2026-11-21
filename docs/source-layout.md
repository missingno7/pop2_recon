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
- Components whose module is not anchored stay at the top of `src/`.
- Recipes and bindings mirror the same paths (`recipes/ctrl/…`,
  `evidence/bindings/ctrl/…`).

**Names.**

- **CamelCase** is an original name anchored in the DOS code, e.g. `GetUndefineCellId`
  or `JaffarSetToNextPos`. `evidence/targets.json` records it as `original_name`,
  with `original_name_evidence` from FM Towns and/or Mac.
- **snake_case** is a descriptive name of ours, e.g. `copy_kid_to_char`, kept
  where no build names the function (inlined in FM Towns, unlabelled on the Mac)
  or the pairing is only probable.
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
