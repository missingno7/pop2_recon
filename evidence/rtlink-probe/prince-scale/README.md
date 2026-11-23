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
