# Convergence review: evidence model, structural blockers, lessons from other projects

Date: 2026-11-22. Baseline `5b54402` (1,401 exact functions); see the end for the state after this work.
Research inputs (scratch, not tracked): `build/research/siblings_toolchain.md` (SimAnt DOS/Win16, Stunts,
Rayman), `build/research/siblings_wholeimage.md` (duke1, dave, sam, tales, blupi, lemmings, ugh, gp),
`build/research/public_projects.md` (civd, F-15 SE2, Restunts, Duke2, NY00123 builds, Alien Arcade, LEGO
Island/reccmp/ReproBit, decomp-permuter, N64/GC tooling), `build/workers/o6_compiler/findings.md` (MSC C2
internals), `build/workers/o7_bss/report.md` (BSS layout), `build/workers/o4_dgroup/` (initialized DGROUP map).

## 1. What made other projects converge

Verified against their trees, commits or write-ups (details and citations in the research files):

1. **A whole-image check through the original linker is the decisive lever.** Every project that rebuilt a
   whole binary byte for byte has one: civd (MSC 5.1 + LINK 5.01, 23 overlays), Stunts (LINK 3.65 + EXEPACK,
   `histbuild.py` BUILT_EXACT), duke1, dave, sam, Duke2, NY00123's builds. Projects without it plateau (F-15
   SE2, Rayman's playable-but-not-identical link, SimAnt's missing RTLink 4.15/5.00 release). The link settled
   BSS/communal order, runtime placement and relocation order in one stroke (Stunts `ee1ff50`, `9ef93e8`).
2. **Bind by name first, prove the table later.** duke1, sam and dave accepted functions relative to a table of
   address-named symbols and proved that table with translation-unit and link checks. None of them proved a
   single-use BSS address at function level. Data pointers to not-yet-rebuilt functions bound to the target's
   discovered location (duke1 proved G2's `_DATA` while G1 was 251/257).
3. **Compiler behaviour that looked random was deterministic** and was found by tracing the unmodified compiler
   (SimAnt 6.00AX: declaration-number context repeating every 17 ids; Rayman/SimAntW allocators; civd: spelling
   and declaration order change local allocation and the order of uninitialized globals).
4. **Linker behaviour was read out of the linker, not inferred**: SimAnt decoded RTLink 4.14/6.10 relocation
   group order (four-word key, sum of ROR16(byte_j, j) mod 101, head insertion; 22 held-out links, 10,680
   relocations predicted exactly; `evidence/toolchain/rtlink-order/model.py`, commit `f10701f`).
5. **Trial links of accepted objects** with the real linker exposed wrong accepted modules (SimAnt placed 83 of 85
   modules; the two misses were real errors).
6. **Masked per-function equality is not reference identity.** A DOS project found "byte-identical" functions that
   referenced the wrong globals; ReverseApoc requires relocation-target checks plus splicing into the EXE. reccmp
   keeps STUB/LIBRARY/"effective match" separate; dtk/objdiff separate Matching/NonMatching/Equivalent.
7. **What did not converge:** broad permuters (Rayman 1/26, 1/62; decomp-permuter has no x86-16 and is weak on
   stack differences), backlog-wide declaration resync (SimAntW 0/173), profile sweeps without object evidence
   (Rayman 0/168; our own 64-file flag sweep: 0), large orchestration layers (SimAntW stalled at 72%). LEGO
   Island's "byte for byte" relies on ~800 committed ReproBit interventions (donor bodies, register bijections,
   post-link repacks) that our rules would refuse.

## 2. Blocker taxonomy for PRINCE.EXE

| Category | Evidence/breakthrough that resolves it | State |
|---|---|---|
| Source-level matching (codegen near-misses with correct extent) | Better compiler knowledge + targeted search, not volume | Ongoing; ~60 left |
| Compiler knowledge | C2 internals decoded: sortnode operand key from C1 symbol id (FRAME-7), register weights/picker (REG-11), first-fit BP slots (FRAME-9), C1 id costs. Not decoded: 6.00A load-before-INC, ov1 register remap, /Og CSE numbering | Largely solved; emulated C2 available |
| Initialized data layout | One `_DATA` piece per object in link order; FM Towns order; data-only modules | Mapped (o4_dgroup); ENGLISH, TABLES published; several misattributions corrected |
| Uninitialized data (BSS/communal) | Original map, CodeView data or source. Global constraints (link order, communal passes, alignment, crt0 anchors) fix order and parity only | 34 single-site game fields genuinely underdetermined |
| Linker reconstruction | RTLink relocation-group order model (SimAnt), trial link with real RTLink 6.10, whole-image link with stand-ins | Not started here; highest structural lever |
| Over-coupled evidence | Separate code exactness from binding proof; prove target identity separately from target implementation | Fixed this session (CODE_MATCH, code-match entries, aggregate extents, data-only groups) |
| Genuinely insufficient original information | Single-site BSS fields; callback targets in overlays never called directly | Recorded explicitly as CODE_MATCH obligations |

## 3. Evidence model after this work

The smallest coherent separation that removed the coupling, without a new framework:

- **CODE_EXACT** (owner, counted): code bytes and every fixup equation equal, every binding independently proven.
- **CODE_MATCH** (recipe `code_matches`, never an owner, reported separately): code bytes and every field equation
  equal under the declared value, with an explicit list of unproven bindings (`"unproven": reason` on a v4 symbol,
  one v4 member, or a far code entry that still names its reviewed row). Nothing is masked; validation refuses any
  change, including a match whose bindings became provable (republish it as a claim). Matches never support other
  claims' internal consensus. Validation prints "N code matches (B bytes) with unproven bindings, not counted".
- **Entry identity vs implementation.** A far call or far pointer needs only the callee's entry. A recorded
  CODE_MATCH proves where its function starts (its own unproven data does not bear on that), so
  `far_binding.ground_symbol` grounds it as "accepted code match entry". Four matching pointer bytes alone never
  ground anything: placed-data far pointers still require a grounded symbol and an original MZ relocation.
- **Initialized data**: placed pieces compared byte for byte, including linked near pointers, far code pointers,
  far segment words and single-byte alignment holes; data-only modules (`promote_data.py`) provide data publics.
- **Aggregate extents**: a member S+k is grounded when an original straight-line block copy anchored exactly at the
  witnessed base S copies L > k bytes. Assumption, stated explicitly: one fixed-length copy unit is one C object
  (a struct assignment). It holds for the cases used (MOBS 13-byte records, the 64-byte character records, LEVEL =
  FM Towns `_Level`), but it is an inference about object identity, not about member names.
- **Debt ledgers** (shrink-only, validated): placement debt, `_segname` CONST layout debt, case aliases, overlap
  pairs. Exact metrics are unchanged in meaning.

## 4. Changes made (all with focused tests and full validation)

- Refused fitting devices: new `_segname` placement; one external under two case spellings.
- Solved CS frames from CS-relative words (graphics mode code; +19 functions).
- Placed-data linker equations: near pointers, far code pointers, far segment words, alignment holes.
- Data-only groups and their submission support; ENGLISH and TABLES published, correcting INIT, MOVERUIN,
  toggle_sound, DIALOG, LOAD 0c98 and the ACT/FINAL `COLEDGEX` binding; DIALOG left the CONST debt list.
- Aggregate extents (+9 functions: GET_MOB_TYPE_ITEM, MARK_GUARD_UPDATES, MOB_LOOSE, KNOCK_LOOSE,
  GET_CHAR_STRIKE_RANGES, DRAW_METERS, RESET_FRAME_VARS, DRAWOBJSPIKES, CHECK_DART_HIT).
- CODE_MATCH and code-match entry grounding.
- Compiler-model use: AUTOGUAR hub (SHAKE_BONES via real declaration position), perm3 v2 (declaration moves
  predicted by the sortnode key made GET_MOB_TYPE_ITEM byte-exact).

## 5. Contradictions found in accepted evidence (code stays proven; declared storage cannot link)

- `_BSS` placements of MOVE (3fc2), LOAD (455a), MOVECAVE (6a9a) and UTIL (6ac8) lie in communal storage
  [2cf0,6cd6); FINAL's 2cdb is odd; 33 extern-modelled v4 symbols sit in the per-file `_BSS` block and must be the
  referencing file's statics; `ROOM_TILES` at 2ce4 lies inside sprintf's `_BSS` (2ce4 = LEVEL 2d02 - 0x1e).
- Remaining `_DATA` debt files: CUSTOM, DRAW, LOAD, MOVECAVE (`_segname`); their correct pieces are mapped. DRAW
  and LOAD wait on far pointers to overlay functions (now groundable through code matches once those functions are
  recorded).

## 6. Recommended next steps, by leverage

1. **Whole-image path with the real RTLink 6.10.** Port SimAnt's RTLink relocation-order model and test it on our
   probes; then trial-link accepted objects plus stand-ins built from original bytes (dave `tools/omf_writer.py`,
   gp's link plan) in original link order. This turns GROUPED relocation debt, BSS/communal order and placement
   into checked facts and exposes wrong accepted modules.
2. **Record code matches** for every byte-exact function blocked only by bindings (callee or single-site data),
   starting with FINAL's RUN_FINAL_FRAME, which unblocks LOAD's corrected data model, and the overlay draw entries
   DRAW's level tables point to. This makes conditional coverage visible and grounds callback tables.
3. **BSS refutation rule R** (o7_bss): placed C `_BSS` pieces inside [_edata, xontab), even, ordered; C communals
   even below 6ccc or in the byte tail; no symbol inside another placed segment; existing violations as a
   shrinking debt list. **Rule G**: `linker-anchor` symbols for `_end`/`_edata`/`__bufin` (unblocks 4 runtime items).
4. **Compiler-model-guided matching** per file: use keyscan/weightlog to choose real declaration positions and
   local structure; decode the remaining 6.00A load-before-INC rule in C2 (localized to codegen/regMD/globregs).
5. Replace remaining `_segname` and misattributed BSS placements with the mapped real pieces as files are revisited.

## 7. Remaining uncertainty

- The aggregate-extent rule's object-identity assumption (section 3).
- 34 single-site BSS game fields cannot be proven without original symbol data; they remain CODE_MATCH obligations.
- Between-symbol relocation order and whole-program placement stay unproven until the RTLink model and a trial link
  exist ("structural link UNRECOVERED").

## 8. State after this work (main `d8e1423`)

- 1,411 CODE_EXACT functions (from 1,401), 139,679 C bytes, 40,003 ASM bytes; 2 CODE_MATCH functions (148
  bytes: FINAL RUN_FINAL_FRAME, LOAD SET_LEVEL_SPEC_PTRS) reported separately and not counted.
- The far-pointer path works end to end: FINAL's RUN_FINAL_FRAME is a recorded code match, so LOAD's level
  procedure table grounds all five callbacks; LOAD now publishes its real 656-byte `_DATA` at DS:0558 without
  `_segname` and left the CONST debt list (remaining: CUSTOM, DRAW, MOVECAVE).
- bss-layout-v1 is enforced with crt0 anchors; 383 existing violations are recorded as shrink-only debt
  (5 misattributed `_BSS` placements, 38 externs inside the per-file `_BSS` block, 340 odd communal starts that
  are members of larger variables).
- Not done here: the RTLink relocation-order model, the trial link and Rule G (linker anchors), which remain the
  highest-leverage structural steps (section 6).

## 9. Whole-image path (2026-11-23)

- **Trial link.** build/workers/o10_trial is the first RTLink 6.10 link of the accepted objects plus stand-ins
  built from original bytes. In the diag variant it reproduced 95.1% of the image bytes and placed every MAP
  segment at its original address. Every difference it found was classified (report.md there).
- **Whole-link refutation.** evidence/link-consistency-debt.json, shrink-only and checked by validation, holds:
  - duplicate publics;
  - one DGROUP name bound at two addresses;
  - far-call externs named differently from their definer (case-insensitive, since RTLink ids are);
  - one entry declared under two names in one file;
  - contributions that break their SEGDEF alignment.
- **Fixes since.**
  - 777 extern spellings were unified with their definers, plus 250 anchored DGROUP and public renames
    (w54_names: each FM Towns name carries a behavioural anchor in an anchored function pair).
  - Two overlapping group pairs were merged into one source each: PRINCE.C (toggle_sound) and FADE.C
    (palette_tail).
  - Eight alignment contradictions were resolved:
    - five genuine-assembly pieces now declare BYTE CODE segments, each starting right after the previous
      routine's RET or IRET;
    - three C data pieces now start at their real even address.
  - The ledger went from 1,214 entries to about 100. Every file was re-proved by the unchanged gate.
- **Regression.** The strict trial link links every accepted object where recorded, with no stand-ins for
  misplaced pieces. It went from 67.1% to 89.9% equal bytes, and relocations equal in site and frame went from
  1,332 to 4,367. The alignment cascade that shifted every overlay by one paragraph is gone.
- **RTLink relocation order.** Per evidence/rtlink-probe/prince-scale, the model reproduces 50 of 51 trial-link
  tables. In the original, the vector ids (1..36) are solved. The other original symbol ids are still open,
  and the evidence suggests that unreconstructed modules, or a different FILE order, registered most names first.
- **Open.**
  - DGROUP aggregates: 53 multi-address names are interior addresses of character, room and level records,
    and need real struct declarations.
  - The 7 same-entry files.
  - Misplaced CONST and _BSS pieces (const-layout and BSS ledgers).
  - Drafts.
  - The original symbol registration order.

### 9.1 State after the whole-image waves (2026-11-23, main f15a13c)

- **Exactness.**
  - 1,436 CODE_EXACT functions, up from 1,416.
  - 36 code matches, reported separately.
  - 7 claims with placement debt, down from 9.
- **Whole-link ledger (names, aliases, alignment): empty.** It started at 1,214.
  - Fixes:
    - extern names unified with their definers;
    - anchored DGROUP and public renames;
    - overlapping groups merged;
    - BYTE-aligned MASM segments;
    - real C data starts;
    - six string pins moved to LIBH;
    - calling conventions corrected from original bytes.
  - MOVE's resident wrapper at 0b41:14dc is a `_fastcall` forwarder to MOVETEMP. The sound2 MASM entries are
    cdecl.
- **Other ledgers.**
  - BSS layout: 383 to 53 entries.
  - Case aliases: 2 to 0.
  - `_segname` CONST: 4 files to 1 (MOVECAVE).
- **DGROUP aggregates.** The aggregates are now real structs across the game files: CHARREC Char, Opp, Kid and
  RoomOpp; ROOMREC; CELLREC; FCHARREC; FRAMEREC; MOBREC; LEVELREC; the coordinate tables.
  - Member access was code-identical in most files.
  - In CTRL, ACT, MOVE, LOAD and NIS, the correct declarations changed codegen until the real declaration
    positions were found. That is evidence the earlier scalar sources fit by accident.
  - New exact functions came from it: GENERIC_FIGHT, APPLY_INPUT, MAKE_RUBBLE.
- **Gate additions**, all narrow:
  - near pointers in placed data to independently grounded DGROUP symbols;
  - near COMDEF communals;
  - `pin_runtime --replaces`;
  - relocate renames in place.
- **Strict trial link:** 67.1% to 90.9% equal bytes; relocations equal in site and frame 1,332 to 4,370.
