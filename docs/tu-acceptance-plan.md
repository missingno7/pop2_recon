# Plan: whole-module (TU) acceptance — 2026-11-20

> Historical: this page describes the earlier build (tag `oracle-dos-1565`). Its addresses, hashes and counts are not those of the v1.1 target; evidence files it cites are at that tag.

Status: **plan, not implemented**. Nothing here changes ownership or the gate.

## Why

The current gate accepts one public owning one complete CODE segment. After 35
commits this has produced 57 C components (2,349 bytes) out of a 144 KB resident
image and 16 overlays. Two measured facts make the single-function gate the main
limit:

- The root image has about 1,416 `PUSH CS; CALL near` sites, and overlays 2 and 3
  have 488 and 218 more. Under MSC 6.00 that encoding is emitted only when the
  callee is defined in the same source file (SimAnt rule TU-1; reproduced here in
  [tu-state-step.md](tu-state-step.md), including the RTLink 6.10 controls). Every
  function on either side of such a call is out of reach for a one-public object.
- MSC 6.00 code generation depends on how many identifiers were declared earlier
  in the file ([convergence audit](convergence-audit.md) reproduced SimAnt's
  SYM-1 contrast under `/AM /Oe /Gs /Zl /Gc`). Isolated files cannot reproduce
  that context, so several parked near-misses may simply be out of context.

`D:\Prog\simant_recon` uses the same MSC 6.00 binaries (identical CL/C1/C2/C3
hashes) and the same RTLink/Plus linker family. It reached 1,617 of 1,640
functions byte-exact by accepting whole modules. Its gate is the model for this
plan. Its large-model specifics, the 6.00AX compiler result (VER-3) and its
data/vector rules are leads that must be reproduced here before use.

## Original evidence already in hand

A census of root far-call relocations (opcode `9A` with a relocated segment word)
finds 40 distinct target frames. Their spans look like source files: frame
`0000` ≈ 0x1690 bytes, `02cc` ≈ 0x1da0, and small frames such as `1ec9` (96
bytes), `21fe` (80) and `1f61` (304). Frame `1101` spans about 50 KB and is
probably the shared `_TEXT` of the C runtime. In medium model each `.C` file gets
its own `NAME_TEXT` segment, so **frame = file** is the working hypothesis. It is
not yet verified for this linker.

## Phase 0: clean baseline (integrator)

1. Settle the uncommitted wave-9 targets in `evidence/targets.json` (five
   `root:02cc:*` entries). Commit them or drop them, and commit the removal of
   `to_delete/resolved-predicate/`.
2. Remove the stale worktrees `D:/Prog/pop2_recon_wt_farcall` (`wave9-farcall`,
   nothing ahead of main) and `build/workers/reliability_worktree` (same content
   as main HEAD). This needs the user's OK first.
3. Update the model routing in AGENTS.md (see "Model routing" below).

Done on 2026-11-20.

## Phase 1: module census and the frame = file proof (research, no ownership)

Done for root on 2026-11-20; see [tu-census.md](tu-census.md). Overlay frames
remain unresolved.

- `tools/tu_census.py` (worker-local output by default) builds a per-space module
  table:
  - far-call target frames and their relocation indices;
  - each frame's extent, bounded by the next frame and by link fill, plus a
    WORD-alignment/`00`-fill check at each boundary (SimAnt CODEALIGN-1);
  - the `PUSH CS; CALL near` graph, with every caller and callee attributed to a
    frame;
  - existing owners and reviewed targets inside each frame;
  - a fixup-class histogram per frame (DGROUP, root far call, near self-relative,
    overlay vector, other).
- RTLink control: compile two tiny medium-model files and link them with pinned
  RTLink 6.10 through the existing `probe_rtlink.py`. Confirm that each file gets
  its own frame and how `00`/`90` fill appears between them. Record a receipt
  under `evidence/rtlink-probe/`.
- Output: `evidence/modules.json`, the reviewed module list. Keys are
  `SPACE:FRAME[@OFF]`, with an extent and boundary evidence. As with
  `targets.json`, discovery is a hypothesis until it is reviewed.

## Phase 2: module gate (tooling worktree, integrator)

**Status 2026-11-21.** The gate's first version is implemented: `tools/module_gate.py`,
`tools/promote_group.py`, and group support in validation, publication and context.
Its rules are in [acceptance.md](acceptance.md#same-source-groups).

- **Review.** GPT 6.2 Sol reviewed it independently and found eleven issues. A
  re-check found four variants. All are fixed, each with a regression test in
  `tests/test_module_gate.py`.
- **Pilot.** The state-step pair (`state_step_pair`, 34 bytes) is published and
  migrated `UPDATE_STEP_DELTA` into it.
- **Not yet supported.** Complete-module status (reviewed file boundaries), root
  CALL FAR and overlay claims are refused for now.
- **Accepted limit.** Publication and validation have no file locks, so a
  concurrent edit in the instant before a retirement unlink is not excluded.
  Validation re-checks every live pin at the end.

Implement this in a separate git worktree, following AGENTS.md. Reuse the
existing binding modes per claim instead of inventing new ones.

**Compile.** The whole file is compiled once with a pinned profile and flags. The
object must have exactly one CODE segment, and the same OMF record and declaration
retention rules apply as now.

**Claims.** A claim is a public that exists in the object and has a reviewed
target. Its extent runs from its public to the next public (or to the module end,
including at most one trailing `90` that equals the oracle).

- The claim's offset in the compiled segment must equal its original offset minus
  the module start ("one placement"). A permutation of individually exact
  functions therefore fails.
- Each claim is compared over its complete extent: bytes, fixup equations and
  relocation obligations.

**Fixups.** Supported, fail-closed per claim:

- existing DGROUP offset16 modes and root external `CALL FAR`;
- **new:** a self-relative near call to a public in the same object. Its equation
  is computed from the object's own public offsets and must equal the original
  displacement. The callee must be in the same frame.
- **new:** an own-segment far reference (same-file `CALL FAR` with a relocated
  frame word). The relocation must carry the module frame.
- Anything else makes that claim non-exact. It never makes it maskable.

**Partial vs complete module.**

- A **partial module** owns only its exact claims. Every other public stays an
  in-place *draft*, reported as debt and owning nothing. Not-yet-written callees
  may sit in a marked `/* SCAFFOLD */` block; a scaffolded module can never be
  complete.
- A **complete module** additionally requires:
  - claims tile the reviewed extent;
  - the object's publics equal the claims;
  - no scaffold;
  - link fill after the extent;
  - cross-claim relocation order equals FIXUPP order.
  
  Partial modules carry cross-claim order as explicit pending debt, as the
  existing `GROUPED` mode does.

**Promotion.** `promote.py --module KEY --claim ...`:

- A promotion that makes any existing claim of the module non-exact is refused.
- Single-function owners inside the module are **migrated** in the same
  publication transaction: the old recipe is retired and the owner is replaced,
  never counted twice.
- The journal and recovery in `publication.py` stay the only write path.

**Validation.** `validate.py` recompiles every module freshly and reports
separately: complete-module bytes, partial-module claimed bytes, draft debt and
pending order debt.

**Refusal tests** (purposeful ones only): permuted functions, an unclaimed extra
public in a complete module, a regressed existing claim, a near call to another
frame, a near call to a non-public, double ownership after migration, a scaffold
in a complete module, and a mismatched trailing fill.

**Overlays.** The first version allows overlay modules only with
relocation-free, same-object near-call and overlay-DGROUP claims. Vector and
cross-overlay calls stay refused until a separate proof exists (SimAnt VEC-1 is
the lead).

## Phase 3: pilots

1. **Partial module, frame `0000`.** Use the state-step pair at
   `0000:0e78`/`0e8e` (34 bytes, already observed exact with controls). This
   migrates `UPDATE_STEP_DELTA` into the module.
2. **First complete module.** Use the smallest frame whose census shows only
   supported fixup classes. Candidates are `21fe` (80 bytes), `1ec9` (96) and
   `1f61` (304).
3. **Frame `02cc`.** It already holds several owners, including
   `FALLBACK_TILE_KIND` and the 54-byte signed-byte helper. Its caller `00f0` is
   blocked only by near calls and two data dependencies
   ([larger-function-frontier.md](larger-function-frontier.md)).

Run full validation after each pilot.

## Phase 4: codegen rules under pop2 flags

Confirm the SimAnt rules with a positive control and a negative contrast under
`/AM /Oe /Gs /Zl /Gc`. Add a rule to [codegen-rules.md](codegen-rules.md) only
when it is confirmed. Priority order:

1. TU-1 (near call only for same-file definitions; mostly done);
2. the declaration count, and whether it repeats every 17 identifiers;
3. which identifiers count toward it;
4. first-appearance position versus repetition;
5. DATA-1 (string literals and data emission order);
6. CODEALIGN-1 (WORD alignment and `00`/`90` fill);
7. MSC 6.00 versus 6.00AX on a multi-function file, since `/Oe` may behave
   differently from SimAnt's `/Os`.

## Phase 5: module waves

The unit of work becomes a module, not a function. A worker card adds:

- the module key and extent;
- the original function order;
- the existing claims (which must stay exact);
- the fixup classes the census says are supported;
- near-call edges;
- known parked near-misses inside the module.

Only one worker drafts a module per wave. Add a `search.py --module` report:
per-claim exact/first difference from one compile, which is diagnostic only.

## Model routing (user decision 2026-11-20)

- **GPT 6.2 Sol (codex):** module drafting, source/profile search, near-misses,
  boundary/ABI review, independent review of the gate's refusal tests.
- **GPT 6 Luna (codex):** mechanical work only: census runs, card generation,
  prototype/declaration extraction, ledger and receipt chores. Luna never
  designs acceptance rules.
- **Opus 5.5:** only blockers that Sol stalls on with negative receipts
  recorded.
- The gate itself (Phase 2) is written by the integrating session, because it is
  an acceptance mechanism.

## Open decisions for the user

1. **Steered source.** SimAnt and Rayman accept a match that needs an unnatural
   construct, tagged as *steered* and listed as debt. pop2 currently requires
   readable historical C only. Should module promotion allow a `--steered` tag?
2. **Partial credit.** This plan counts exact claims in partial modules as owned.
   The alternative is to count only complete modules.
