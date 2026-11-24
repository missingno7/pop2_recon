# RTLink 6.10 relocation order at PRINCE scale (o11_relorder, 2026-11-23)

These are research receipts. Nothing here grants acceptance. They were produced from the o10 whole-image trial
link (`build/workers/o10_trial`) and from probe links with the pinned RTLink 6.10 under MS-DOS Player. The
scratch scripts and the full notes are in `build/workers/o11_relorder/`; `model.py` is copied here.

## Model, checked on the trial link

The model has zero free parameters. Checked against the trial link (`receipt_candidate_model.json`):
- 50 of 51 final-segment relocation tables reproduce every group that targets a named symbol: 1,646 groups,
  in segments holding 4,565 of the 4,717 relocations.
- The one exception is the overlay manager's `$$OVLMGR` table: its RTLUTILS member registration order is
  unknown.
- 105 groups that target a segment or group are keyed per input SEGDEF contribution, and the data cannot
  pin them.

The model needed four corrections to the bucket-order rule (`../bucket-order/rule.md`). Each was confirmed by a
probe link:

1. **ALWAYS prefix.** Symbol ids start with the names on `ALWAYS` lines, in ALWAYS order (1..36). Then come 10
   linker names. The first object symbol gets id 47, measured by linking a probe of 200 fresh publics first.
2. **Case-insensitive ids.** `_malloc` and `_MALLOC` are one id. They differ only through point 3.
3. **Flags depend on COMENT class A1.** A1 is the MSC 6 OMF-extensions comment.
   - Fixups from an object that carries A1 key their groups with flags 0x048D (base16) and 0x0C8D (pointer32).
   - Objects without A1 use 0x048B and 0x0C8B: MASM, stand-ins and probes.
   - The frame method and the target method make no difference.
4. **Library members register their names in load order.** MODULE members load first. The rest follow the
   registration order of undefined symbols.

## Original PRINCE.EXE

**Vectored publics: solved** (`receipt_vector_ids.json`). They have ids 1..36 in `$$OVLVEC` order, with
0x?8D flags.
- The evidence is 28 ordered pairs in 9 original tables.
- Of the offsets 0..4999, only 4 fit, and one of them is the predicted offset 1. With 0x?8B flags no offset
  fits.
- So the original code that references the vectors came from MSC 6 objects, and the vector names were
  registered first, as an ALWAYS list does.

**Other ids: not solved.**
- Consistent propagation narrows each symbol's domain to a median of 380 of 4,095 values.
- No accepted module's names fit as one consecutive block.
- Most callee names were registered before the accepted modules that call them. Either the original
  processing order differs from the MAP order of the accepted objects, or unreconstructed modules came first.
  This is evidence, not a solved order.
- With original target addresses as identities, every accepted segment's groups are contiguous. Candidate
  names break this: PIECELEFTID in DRAWRUIN and in DRAWTEMP are different symbols.

## v1.1, natural whole link (2026-11-24)

`tools/whole_link.py` links v1.1 without generated symbols, so the same model applies to it (scratch
`build/workers/o21_reloc/`). Library members register in library file order. The model reproduces 60 of the 64
tables of our own link, and all but the library `GAMELIB_TEXT`, `_TEXT`, `CATALOG_FAR` and `_DATA` of them completely.

Against the original:
- Every original table is one run per symbol under the accepted names (no symbol is split), except `GAMELIB_TEXT`.
- With ids counted from the original's own start (36 vectors, 10 linker names), PRINCE's FIXUP-registered symbols 98-162
  match exactly. This fixes the number of PRINCE's publics and communals.
- From there a small number of extra or missing symbols per object explains every other C table. Each one is a fact about
  the source: PRINCE has two names less (the sound byte pair is one variable, the debug position is one), ADD defines the
  three far tables and one `PeelInfo`, CUSTOM has twelve public plates more (the FM Towns names), LOAD two tables, MOVE
  separate `MobHeight`/`MobWidth`.
- PUBDEF and COMDEF records come out in symbol-number order modulo 64 (probe), so the order of a file's publics in the
  original tells in which order it declared them. The ids the tables allow for single publics are few (often one).

Later the same day:
- The five THRONE entries are not among the first 40 registered names (their table positions exclude ids 36-40, all
  others confirm theirs). Linking them through VECTORON at their section gives the identical image and vector table. The
  original's first object id (51) still needs 40 + 10 earlier names, so five unknown names come before it; the plan holds
  placeholders.
- Eight names are registered between DIAG and INIT: the eight publics of the resident resource module, which is therefore
  a root file after DIAG (identical image when linked that way).
- DIAG has nine more publics: exactly its nine debug integers.
- With the counts below, the FIXUP-registered symbols of all 34 C files (1,287 table groups against each other) fit with
  one descent, and for all but four files an order of the file's publics exists that satisfies every table.

Counts the source does not reproduce yet (fit): CHECKNIS, MOVESPIK, AUTO one public less, DRAW two, NIS eleven (probably
static functions); MISC or ENGLISH and one of FINAL/AUTOHEAD/AUTOGUAR one public more; CTRL one more symbol before
`GRAB_ROOF_SHIP`; DIAG refers to no new `TimeVal`; INIT two publics more and two references less; NIS no `MIDI_CUE`.

Why the order itself is not reproduced: a file's publics are numbered by symbol number modulo 64 in the original compile.
The solved orders mix functions and variables in no source order (PRINCE: `DoBlackOut`, `CheckMeters`, `BlackOut`,
`clear_keyboard`, ...), as follows from large shared headers. Declaring the names in that order in each file would be a
construct for the compiler only; the real way is the original header text, which is not known.
