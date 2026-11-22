# o9_linktools: OMF writer, RTLink 6.10 harness, stand-ins (track B tooling)

Worker scratch only. Nothing here is canonical. Nothing here grants reconstruction
or acceptance. Stand-ins are scaffolding for structural link proof. They are
labelled `STANDIN_*` in THEADR and carry a translator COMENT, and they never
count as reconstruction.

## Files

| File | Role |
|---|---|
| `omf_writer.py` | 16-bit OMF writer: THEADR, COMENT, LNAMES, SEGDEF (align, combine, length, overlay name, absolute frame), GRPDEF, EXTDEF/LEXTDEF, COMDEF (near/far), PUBDEF/LPUBDEF (group, absolute), LEDATA (auto or explicit chunks, kept in global record order), FIXUPP (F0/F1/F2/F4/F5 frames; T0-T2 with displacement, T4-T6 without; LOC lobyte/offset16/base16/pointer32/hibyte/loader-offset16; self-relative), MODEND (main, start address). Also `read`, `normalize`, `lift` (re-emit a real object) and `standin`. |
| `link_harness.py` | `LinkJob`/`run_link`: stages inputs (copied) into an emptied directory under this worker. It writes a FREEFORMAT response file and runs the pinned RTLINK.EXE 6.10 under the pinned MS-DOS Player. It runs one process, waits for it, and kills it on timeout. It returns the EXE, MAP, log, input hashes and warnings. Parsers: `parse_mz`, `find_descriptor_table`, `parse_overlays`, `parse_map` (sections, origins, publics). `diff_exe` and `summary` give the structured diff. The CLI diffs against PRINCE.EXE. |
| `rtlink_order.py` | Relocation-descriptor order model (hash, head insertion, bucket walk), symbol registration (`register_symbols`), per-segment prediction. |
| `standin_exe.py` | Builds stand-ins from a linked EXE + MAP. It covers root-only and single-area overlay links; the overlay manager comes from RTLUTILS.LIB. Every relocated word is a FIXUPP over a zero addend. |
| `probes.py` | Synthetic probes. It writes `receipts/*.json` and links under `scratch/probes/`. |
| `probe_src/` | Probe sources: START/DEFS/OV2 (MASM), ROOTC/OV1 (MSC 6.00). |
| `tests/` | 27 pytest tests. `O9_LINK=1` also runs the two RTLink integration tests. |

## Usage

```
python build/workers/o9_linktools/probes.py                 # all probes, about 6 s (about 60 links)
python -m pytest build/workers/o9_linktools/tests -q -p no:cacheprovider   # O9_LINK=1 for links
python build/workers/o9_linktools/link_harness.py CAND.EXE --candidate-map CAND.MAP --output build/workers/o9_linktools/diff.json
```

```python
w = ow.standin("SEG1")                                   # or ow.OmfWriter(name) for non-stand-ins
s = w.segment("X_TEXT", "CODE", align="word", data=image_bytes_with_zero_addends)
w.public(s, "ENTRY", 0)
w.fixup(s, 0x11, "pointer32", ("external", "TARGET"))  # F5 by default; displacement=None -> T6
job = lh.LinkJob("OUT", root=["START", "SEG1"], areas=[[("ONE", ["OV1"])]],
                 inputs={"START.OBJ": b"...", "SEG1.OBJ": w.to_bytes(), "OV1.OBJ": ..., "RTLUTILS.LIB": lh.RTLUTILS})
r = lh.run_link(job, Path("build/workers/o9_linktools/scratch/x"))
report = lh.diff_exe(r.exe, lh.PRINCE.read_bytes(), candidate_map=r.map_text)
```

Every LEDATA record is at most 1024 bytes and never splits a fixup field. The
writer refuses to emit an object whose per-record FIXUPP traversal would differ
from the declared fixup order (`strict_fixup_order`). RTLink preserves that
traversal inside each relocation group.

## Verified (receipts/)

- **Writer read-back.** Every record kind and fixup form round-trips through
  `tools/omf.py` (`tests/test_omf_writer.py`). `lift` re-emits MASM objects byte
  for byte. MSC 6.00 and 6.00A `/Zi` objects keep the same structure; threads and
  CodeView are dropped.
- **(a) Hand-written stand-ins re-link to known EXEs** (`a_handwritten.json`):
  - The real START+A+B+DEFS link reproduces the existing relocation-order
    receipt (`ddfc11bd…`).
  - Hand-typed stand-ins for all four modules give the identical EXE in four
    FIXUPP encodings (F2/T6, F5/T6, F2/T2+0, F5/T2+0).
  - The MSC `MANY` link (`07290c8c…`, also as in the receipt) is identical with a
    hand stand-in that uses the compiler's fixup order and LEDATA boundaries.
  - The same declared order with 1024-byte LEDATA gives 151/300 relocations in
    place. Ascending order gives 2/300. Both have the same image and the same set.
- **Lifted real objects re-link identically** (`a_lift.json`). This holds for the
  MASM root link, an MSC/MASM root link with DGROUP, communals and far data
  pointers, and the overlay link.
- **(b) Overlay link with the shipped manager** (`b_overlay.json`):
  - The link has a root and sections ONE and TWO. RTLUTILS.LIB supplies 9
    members (ovlmgr_m, ovlmgr, …).
  - The MZ entry is `$$RTLOVLINIT` at manager offset 0678, the same IP as
    PRINCE.EXE (`2203:0678`).
  - The descriptor table has 3 rows. A last "preloaded data" section (flags 1)
    holds STACK and DGROUP and has its own relocation table.
- **(c) Stand-ins built from the linked EXE's own bytes re-link byte for byte**
  (`c_roundtrip.json`):
  - Root links: AB (24 relocations), MIX (11) and a 1200-relocation link.
  - Overlay links: the probe (42 root relocations, 3 vectors) and a 4-section
    link (185 root relocations, 60 vectors). Both match in the image, every
    section, every relocation table and vector order. This needs the three
    link parameters below.
- **Order model, held out** (`order_model.json`). Predictions were written before
  each link. There were 960 root relocations in 2 links and 749 overlay-link
  relocations in 2 links (root, section and multi-object-section tables), all
  predicted exactly. Vector order equals symbol registration order in both
  links.

## RTLink 6.10 behaviours relevant to PRINCE.EXE

1. **One relocation group per descriptor, per final combined segment.**
   - The key is `(flags, 0, id, id)`, hashed as in SimAnt's model. Flags are
     `0x048B` for base16 and `0x0C8B` for pointer32. The pointer32 value is new
     here; it was fitted on one link and then held out.
   - The frame method (F2 vs F5) and displacement presence do not change the key.
     Five encodings of 120 calls gave one EXE (`order_model.json` `encodings`).
     The offset16+base16 split gives the same relocation set with 0/120 in place.
   - base16 and pointer32 references to the same symbol are separate groups,
     not one contiguous run. The "LOC2 run, then LOC3 run" rule in
     `evidence/rtlink-probe/overlay-buckets` is a special case of that 2-symbol
     probe. With 90 or 150 symbols, a merged-descriptor hypothesis predicts only
     19 or 11 positions (`scratch/exp/exp4.py`).
   - Sites keep per-LEDATA FIXUPP traversal order.
2. **Symbol IDs.**
   - IDs are registration order: PUBDEF/COMDEF when processed, externals at
     their first FIXUPP, unused EXTDEFs at the module end, `$$MAIN` at the
     entry module's MODEND.
   - Names are matched case-insensitively.
   - Links with overlay sections pre-register 10 names before the first object.
     This was found on one link and held out on 2 more.
3. **Vectors.** References to overlay publics from other sections are keyed with
   the overlay public's own ID (pointer32 flags). The `$$OVLVEC` table is in
   registration order of the vectored publics. Each vector is
   `E8 rel16, EA off seg, dw index-in-area`.
4. **Library tables depend on the objects' registrations.** The overlay
   manager's (and any library member's) relocation order changes with the number
   of names registered before library search. In the sweep, only the original
   count (24) reproduces all 32 manager relocations; 23 and 25 give 22/32.
   `$$MAIN`'s ID matters too. The `$$OVLVEC` jump-word table order depends on
   the vectored publics' IDs. It is **not** explained by the descriptor model:
   the key forms tried in `scratch/exp/exp15-20.py` all failed. The table is
   reproduced only when those IDs equal the original's.
5. **Preloaded data.** DGROUP and STACK go to a trailing preloaded-data section
   (flags 1). This holds even when an overlay object contributes `_DATA`.
   PRINCE.EXE's section 17 (flags 1, 161 relocations) has this shape.

## Limits

- **PRINCE.EXE needs three integers per link that its bytes do not give:** the
  pre-library name count, `$$MAIN`'s ID, and every vectored public's ID. The
  harness can search them by relinking, but no PRINCE-scale search was run. The
  CRT library members (crt0 references `_main`) add the same kind of dependency.
- **`standin_exe` limits:**
  - It refuses nested overlay areas; PRINCE's ancestor index shows nesting.
  - It refuses references to library frames from user code.
  - It refuses relocated words that are no segment, group or vector frame.
  - Segment alignment is inferred from the MAP placement.
  - Near offsets stay raw bytes, because they produce no MZ relocation.
  - Aliases sharing a vector address are ambiguous and are refused.
- **Model coverage:**
  - The model covers external targets with target-derived frames. Segment and
    group targets in real MSC objects still link correctly, but their keys are
    not modelled.
  - The 10-name overlay prefix was only seen with one area, so it may depend on
    the manager configuration.
- **Other limits:**
  - A lifted MSC object drops thread records (their fixups become explicit) and
    CodeView records. The non-`/Zi` lifts re-linked identically. `tools/omf.py`
    documents the `/Zi` case; it was not linked here.
  - PRINCE.EXE is read only for the diff report (`prince_report.json`) and the
    descriptor-table test. It was never used as a link input.
