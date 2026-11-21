# Component acceptance

`tools/match.py` reads expected bytes solely from the SHA-locked original EXE.
`evidence/targets.json` freezes reviewed extents and their independent boundary
evidence. For reconstructed C/ASM, a whole emitted CODE SEGDEF belongs to one
public at offset zero, or to a same-source group (below).

## Same-source groups

`tools/module_gate.py` lets one compiled object own several reviewed functions.

**Placement and claims.**
- The object has one complete CODE segment and the same record and storage rules
  as the single-function gate.
- Every public starts inside the initialized segment, and one public starts at
  offset 0, so no emitted byte is unaccounted for. Fixup fields must not overlap.
- Each *claim* pairs a reviewed target with the public that starts it.
- All claims must share one placement: the original position minus the public's
  object offset is the same for every claim. Permuted definitions therefore fail.
- The claims' reviewed, relocated incoming CALL FARs must all name one original
  frame.

**What each claim is judged on.** A claim's extent runs from its public to the
next public or the segment end, and must equal the reviewed target exactly. Its
bytes compare literally, except for these fixup fields:

- **Self-relative rel16 fields.** Every LOC1 self-relative field must be the
  rel16 operand of a decoded `E8`, `E9` or `0F 8x`. Its frame is the
  target-derived frame (MSC F5) or the object's own CODE segment (MASM F0/F4);
  the value, a difference of two positions in one frame, is the same for both.
  The field must equal `S - (P + 2)` mod 65536, the equation the RTLink controls
  in [tu-state-step.md](tu-state-step.md) observed, where `S` is:
  - **own-segment branch** (T0/T4 to this CODE segment, MASM's long `JMP`/`Jcc`
    to a label): target displacement plus the encoded LEDATA addend. `S` must be
    an instruction start of the same claim. A branch into another claim is
    refused. No CS window is needed: like a jump the compiler resolves itself,
    it stays inside one contiguous claim;
  - **same-object near call**: a public of this object, named by an external
    T6 (MSC) or an own-segment target (MASM), called by `PUSH CS; CALL near`
    or by a plain `CALL near`. T6 fields need a zero addend;
  - **external near call**: a `near-code-entry` symbol outside the whole object
    (F0/T6 or F5/T6, zero addend, `CALL near` with or without `PUSH CS`). The
    symbol grounds the callee position independently of the candidate; see
    [far-call-binding.md](far-call-binding.md#near-code-entry-groups).

  The linked value does not depend on CS, but whether the call reaches `S` does.
  Every call (both forms, same-object or external) is therefore proven only
  when a claim's frame is witnessed and the whole call, its `PUSH CS` and the
  callee lie in that frame's 64 KB window. A claim without its own relocated
  CALL FAR may inherit the frame only through an equal near call from a claim
  already proven in it. A component entered only by near calls has no
  independent frame evidence and is refused: another target's CALL FAR into the
  same code frame does not prove the CS its callers use (root frames such as
  `1000` and `1101` overlap linearly). External near callees are never assumed
  to preserve DS/ES/SS.
- **External DGROUP offset16.** The existing modes apply. The group binding is
  pinned to `group:NAME`, and every data witness must lie outside the whole
  emitted object, not just outside the claim. A data fixup whose symbol is also a
  CODE public of the object is refused, because the linker would resolve it to
  that code.
- **Calls before DGROUP accesses.** A claim with DGROUP accesses may call only
  claims that themselves provably preserve DS/ES/SS. This is checked as a
  fixpoint. A call into an unclaimed draft refuses those accesses.

- **Cross-file CALL FAR (root and overlay owners).** The field is the operand of a decoded
  `9A` with zero addend, and it equals an independently witnessed, reviewed
  direct entry, as in the single-function far gate. The entry and every witness
  lie outside the whole object. A callee with no outside relocated caller may
  instead be grounded by being the entry of an already accepted exact owner. The
  claim's original MZ relocations must be
  exactly these segment words, with within-symbol FIXUPP order (`EXACT`, or
  `GROUPED` with recorded debt). Far callees are taken to preserve DS/SS per the
  MSC ABI, as the single-function far gate already assumes.

- **Far data segment base (root).** A LOC2 base16 field (`mov ax, seg Table`) is
  proven as the segment value `N`. `N` is grounded by original relocated words
  outside the candidate that store `N` (symbol kind `segment-base`). The field's
  MZ relocation joins the claim's relocation bijection.

- **Far data pointer (root).** For a far data object `N:O`, the LOC2 field equals
  `N` and a LOC1 offset field equals `O` with zero addend (symbol kind
  `far-data`). `N` is grounded like a segment base. `O` is grounded by reached
  original instructions whose address field equals `O`, each within 16 bytes of
  a relocated word storing `N`.

- **DGROUP base (`mov ax, DGROUP`).** A LOC2 field to the DGROUP group equals the
  DGROUP paragraph, which the binding's startup DS-load frame witness grounds.
  Its MZ relocation joins the claim's relocation bijection.

- **File-private DGROUP data (placements).** A group binding may place the
  object's own `_DATA`/`CONST`/`_BSS` segments (`placements: {"_BSS": P, ...}`).
  Fields addressing them must use the F1 DGROUP frame (MSC and MASM both do) of
  a DGROUP member and equal `P + A`; any other frame would write `A` alone and
  is refused. Each placement must be
  over-determined:
  - an initialized segment's bytes equal the original DGROUP bytes at `P`. The
    C runtime reloads DS with the relocated paragraph `30f3` before `main`, so
    DGROUP's initialized data lives in the preloaded overlay 17. The binding names
    that paragraph (`dgroup_segment`, grounded by an original relocated word
    storing it). The compared range may contain no relocated word;
  - an uninitialized segment has at least two distinct addends among proven
    fields that agree on `P`.

  This proves code bytes only; the data bytes are not owned yet.

**What refuses a claim.** Any other fixup, a field crossing a claim boundary, or
an original MZ (or overlay-table) relocation not generated by a claimed field
refuses that claim. Such a claim cannot be published.

**Overlay owners.** Claims may lie in an RTLink code overlay (`overlay-N`); see
[far-call-binding.md](far-call-binding.md#overlay-owners-in-groups). Their CS
frame needs overlay-specific evidence, their relocation bijection uses that
overlay's own table, and v4 DGROUP fields use the same root startup DS witness.

**Unclaimed publics.** These are in-place *drafts*. They own nothing and are
recorded in the recipe as debt.

**MASM groups.** `promote_group.py --language asm --profile masm510` assembles
one reviewed MASM source instead (no flags). The recipe records `language: asm`,
the source lives under `asm/NAME.asm` (e.g. `asm/xlattxt4/xlattxt4.asm`), claims
are ASM_EXACT and owners MATCHING_ASM. Validation reassembles freshly. The same
gate applies; v4 data fields may use MASM's F1 DGROUP frame
([dgroup-binding.md](dgroup-binding.md)).

**Promotion and validation.** `tools/promote_group.py` publishes only when every
claim is CODE_EXACT (ASM_EXACT for MASM).
- Existing single-function owners inside the placement must be among the claims.
  Their owners are replaced in the same transaction, and their source, recipe
  and unshared binding are retired after the manifest commit (recoverable through
  the same journal).
- An owner that overlaps only an unclaimed draft keeps its bytes, because drafts
  own nothing. An owner overlapping a claim must itself be claimed.
- A group can grow under its own name. The larger group must reclaim every old
  claim; its files then replace the old ones, which are backed up in the journal
  and restored on rollback. The binding is published from any input path to
  `evidence/bindings/NAME.json`.
- Only files that no remaining recipe references are retired. Their pins are
  rechecked before the commit, and they are deleted only while they still match.
- Promotion and validation hash exactly the source and binding bytes they compile
  and parse.
- Validation recompiles each group once. Every claim must remain exact, and the
  object hash, placement, frame, extent and draft record must stay the same.

**What a group proves.** A group proves CODE_EXACT for each claim and their
relative placement from one compilation. It does not prove the original file
boundary, declarations before the group, unclaimed drafts, or link placement.
Declared length, every initialized byte, padding, all publics, secondary storage,
records and FIXUPP subrecords are inspected. CODE holes, multiple publics,
nonzero secondary data/BSS, unknown storage records and 32-bit records fail.

The relocation-free gate accepts components with **zero actual object fixups
and zero original relocation obligations**. FIXUPP thread definitions are linker
declarations, not relocation requests; they are decoded and retained. Unused
EXTDEF declarations (including Microsoft CRT markers and a self-public) are
retained in reports and the exact object hash. A separate narrow
[external DGROUP gate](dgroup-binding.md) proves offset16 fields using frozen,
independent original witnesses and explicit linker equations. Resident modes
support zero addends, reviewed word pairs, or explicit v3 immediate SI/DI
bases with the checked ES := DS string idiom; the explicit overlay mode permits
only byte/word zero addends with root witnesses. No bytes are
masked, patched or normalized. A separate
[runtime near-code gate](runtime-near-binding.md) supports anchored F0/T6
self-relative offset16. The narrow [root CALL FAR gate](far-call-binding.md)
supports external zero-addend F5/T6 LOC3 operands to reviewed direct entries
with independent original relocated callers. Every segment word must match
an exact one-to-one MZ relocation, with original table indices pinned in binding,
recipe and owner. Target-symbol groups must be contiguous and retain FIXUPP
order internally. A between-group permutation is reported and recipe-pinned
as `relocation_order: GROUPED` with explicit structural debt; old recipes mean
EXACT. It composes with the resident DGROUP modes. The far-call mode refuses
overlay owners, vectors and near/TU calls. Other fixup modes remain refused.

Comparisons use the complete emitted segment, never an oracle-sized slice.
Byte similarity and first-difference offsets are diagnostic. Promotion reruns
the historical compiler on a frozen independent source copy. Validation freshly
recompiles every accepted source and checks full OMF identity, complete code,
tool hashes, original asset/structure identities and non-overlapping ownership.
Missing tools and compilation failures never fall back to cached objects.

Validation replaces its previous report with `RUNNING` before any oracle or
compiler checks. Success is atomically published as `PASS`, with a run ID,
timestamps, the manifest hash and whether tests were requested. Exceptions write
`FAIL` or `INTERRUPTED`; a killed process may leave `RUNNING`, never a new success.
`--skip-tests` remains an explicitly recorded local check. Manifest changes
during validation or an unfinished publication prevent success.

Source and runtime publication use one recovery journal at
`build/publication-pending.json`. New source/recipe files are installed without
overwriting existing files, the unchanged manifest gate runs, and an atomic
manifest replacement commits the transaction. Publication invalidates the old
validation report. Ordinary failures roll back; after a process crash, stop the
publisher and run `python tools/publication.py --recover` before continuing.
Recovery removes only journaled, unchanged files when the old manifest remains,
or retains the complete publication when the new manifest was committed. An
external edit causes recovery to refuse without deleting files. Do not remove
the journal manually or run recovery against a live publisher. These controls
protect publication and reporting; they grant no additional code or binding proof.

`tools/runtime.py` separately verifies pinned historical library members. Each
recipe hashes the independent library archive and entire OMF member, owns the
full initialized CODE segment, and checks declarations, original bytes and
fixup/relocation obligations. Nineteen relocation-free members cover 1,130 bytes; the
13-byte close and 23-byte medium-model open members additionally verify external
near equations against independent zero-owned provider context. These twenty-one
members cover 1,166 bytes as PINNED_RUNTIME, separate from reconstructed C or ASM.
Partial startup/provider
signatures remain evidence only and contribute no accepted bytes. Runtime
publication uses `tools/pin_runtime.py` with a reviewed worker submission and
fresh strict verification; whole members cannot be trimmed to a matching prefix.
The literal, relocation-free runtime gate retains every public in a whole
multi-public member; it does not select a public's matching suffix or prefix.
The near-bound runtime gate still requires one owning public at zero.

`tools/runtime_scan.py` supplies diagnostic whole-member candidates. It reads
page-aligned members before the archive dictionary, requires one complete CODE
segment with at least one public at zero and empty secondary storage, retains
all CODE publics within that extent, excludes actual fixups,
and searches whole literal CODE bytes. It retains original relocations and
existing ownership overlaps in its worker-local report. A scan hit still needs
independent boundary/ABI/caller review and fresh `pin_runtime.py` verification;
the scan never publishes or supplies acceptance fixtures.

CODE_EXACT means the recorded compiler/profile reproduces the complete reviewed
component code with its proven fixup obligations. It does not establish original
translation-unit membership, historical object-record identity, natural final
placement, library order, overlay vectors or whole RTLink closure. Those remain
separate structural proof. No hybrid executable is constructed in this bootstrap.

The same complete-segment gate supports reconstructed ASM through
`tools/assembler.py`. `masm510` pins Microsoft MASM 5.10 and the runner;
assembly takes place as `UNIT.ASM` in isolated scratch. External include/library
inputs and unapproved assembler switches are refused. Failed or timed-out
assembly cannot reuse a stale object. Search and promotion select this path
with `--language asm`; accepted recipes declare that language, own source under
`asm/`, and are validated by fresh assembly and full OMF identity as ASM_EXACT.
An odd-length segment is valid when its entire initialized extent matches the
independently reviewed target. Original source language and assembler revision
remain evidence-qualified claims separate from this component proof.

The final overlay is physically 21,103 bytes although its descriptor allocates
21,104 paragraph bytes. The guarded profile records one absent terminal byte;
oracle extraction neither reads past EOF nor supplies a synthesized zero. The
three file bytes before overlay 2 relocations remain unowned alignment debt.

Source semantics, game/runtime classification, names and original build/version
are evidence-qualified claims independent of byte equality. Compiler probes can
establish code-generation facts without proving the compiler used by every TU.

Current component counts, proven fields and unresolved targets are recorded in
[reconstruction progress](reconstruction-progress.md), backed by canonical
layout/manifest.json and fresh validation metrics. The bounded wave receipts
record exhausted compiler outputs and concrete provider blockers. Far-call/MZ
acceptance is limited to the explicit root mode above; agreeing conditional
equations, shared archive declarations or partially closed provider trees grant no ownership.

Canonical source and binding file pins use the repository's LF checkout policy.
Source publication normalizes its frozen independent copy to LF, then compiles
and pins that copy; worker input is unchanged. JSON output uses explicit UTF-8
bytes. Source/runtime publication refuse CRLF binding files so Git cannot
silently change their pinned identity. This file-format rule does not change
original bytes, object bytes, fixup equations or component extents.

Placed-data fields may carry their offset in the FIXUPP displacement (MASM T0/T4
with an explicit D) as well as in the LEDATA bytes; the proven value is P + D + A.
Overlay owners may place file-private DGROUP data when the binding grounds the
runtime DGROUP paragraph (`dgroup_segment`). Overlay code runs with the same DS,
and initialized bytes are compared in whichever space holds that paragraph.
A claim may end in data it addresses itself, for example a literal after the
final RET reached by `mov si, OFFSET lit`. The tail from the lowest own-segment
field target T counts as data when the code before T decodes completely and
ends in a return or JMP, and no own-segment field lies inside the tail. The
tail bytes are compared exactly as ordinary bytes.
A claim may take the address of a public of its own object (a callback). The
offset half (`mov ax, offset F`, LOC1/LOC5 immediates only; a DS memory operand
naming code stays refused) uses the own-segment offset16 equation with
addend = F's object offset + A + D. The segment half (`mov ax, SEG F`, LOC2) must
equal the claim's witnessed CS frame paragraph and counts among the claim's
relocation sites.
