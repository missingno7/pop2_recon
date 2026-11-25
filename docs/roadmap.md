# Roadmap

Goal: the most useful source of Prince of Persia 2 v1.1 for DOS. It rebuilds the original program from a clean checkout,
reads like a program someone wrote, and is fully understood. A port (SDL) comes later and is not part of this repository.

Current numbers come from `python tools/validate.py`; this page holds only goals, open work and clues.

## What "done" means

1. **Rebuilds by itself.** One command builds PRINCE.EXE from the sources and the pinned tools, with no file or number that
   exists only to make the link work.
2. **Identical file.** Every byte equals the original. Today the order of the relocation records differs; nothing else.
3. **Every byte has an owner** in a source file or a named library, data included.
4. **Readable.** Shared headers as the authors had them, each type and global declared once; no `unknown_` fields; `goto` only
   where a plain form does not give the original code; names from the original where any build gives one.
5. **Understood.** Every function and global has a checked meaning; levels, sequences, sounds and resources are described.
6. **No clue left unused** (list below), and no accepted rule that an independent review would reject.

## Open work

| Area | State | Next step |
|---|---|---|
| Headers | Every type, variable and prototype is declared once, in `include/`; all 64 C groups use it. The include order is fitted per file. | Look for one include list for all files: it would give every file its symbol numbers and with them the relocation order. |
| Plain code | About 100 `do`-`while` loops, 26 `goto` and 25 local structs that imitate compiler temporaries are left. | codegen-rules DRAFT-7 and DRAFT-8: try the plain form, fit the includes, let the gate decide. |
| Relocation order | The order that fitted through CTRL came from hand-placed declarations in each file; the headers replaced them (2026-11-25), so few records are in place now. The records themselves are all equal. | Each file's EXTDEF order is its symbol ids modulo 64: read the original header order off the relocation tables, starting with ACT, and put the headers in it. |
| Data | About 20 KB of overlay 17 is linked from `link/` without an owner; ten link modules have an unsettled file. | Settle each with the neighbouring symbols in the FM Towns and Mac builds. |
| Link aids | Two forced symbols, five prefix names of unknown meaning, the `VARS` object. | Replace by real references once their users are known. |
| Recorded debt | One C routine kept as assembly, one ambiguous immediate witness (`evidence/*-debt.json`, `asm-origin-audit.json`). | Rebuild, or find a second witness. |
| Alignment | 18 hand-placed bytes in nine assembly files. | Check for the original `EVEN`/`ALIGN` and segment attributes. |
| Names | Descriptive names wherever no original one is anchored. | Pair the remaining FM Towns and Mac functions (`evidence/modules.json`), then rename with `tools/relocate.py`. |
| Fields | Some struct fields are `unknown_`. | Read every use; compare with the PoP1 and level-format knowledge. |
| `goto` | Left in ANIM (`AnimNew`), COLLIDE (`GetFwdDist`), FINAL (`JaffarGoToPos0`) and XLATGRAF (error exits). | Every plain form tried so far changes the code; ANIM's and XLATGRAF's are ordinary clean-up exits. |
| Documentation | Several pages in `docs/` describe finished investigations of the pre-1.0 build. | Keep what explains the current code, drop the rest (it stays in history). |

## Clues

| Source | What it gives | Used so far |
|---|---|---|
| FM Towns release (`assets/fmtowns`) | Module list, 1,788 public symbols, Borland code of the same functions | Module of nearly every function, recorded names; unused: locals and argument counts visible in its code, names of unpaired functions |
| Macintosh release (`assets/mac`) | 1,377 function names in segment order, 68k code | Names; unused: its data names, its code as a second opinion on control flow and types |
| DOS v1.0, v1.01 and the pre-1.0 build (`assets/pop2`, `pop2_101`, `DOS`) | The same source at other dates | Pre-1.0 reconstruction (tag `oracle-dos-1565`); unused: a difference list against v1.1 that explains each fix |
| RTLink/Plus 6.10, MSC 6.00A, MASM 5.10/6.00 | Exact tool behaviour | Symbol ids, record order, communals, run-time modules |
| Prince of Persia 1 (Apple II source, reverse-engineered DOS names) | Original vocabulary and structure of the shared engine | Names of character fields and sequences; unused: systematic comparison of routines |
| "POP2 Bible" (1991 design notes) and the level data | Names of rooms, objects and enemies | Not used systematically |
