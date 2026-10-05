# Component acceptance

`tools/match.py` reads expected bytes solely from the SHA-locked original EXE.
`evidence/targets.json` freezes reviewed extents and their independent boundary
evidence. A whole emitted CODE SEGDEF belongs to one public at offset zero.
Declared length, every initialized byte, padding, all publics, secondary storage,
records and FIXUPP subrecords are inspected. CODE holes, multiple publics,
nonzero secondary data/BSS, unknown storage records and 32-bit records fail.

The relocation-free gate accepts components with **zero actual object fixups
and zero original relocation obligations**. FIXUPP thread definitions are linker
declarations, not relocation requests; they are decoded and retained. Unused
EXTDEF declarations (including Microsoft CRT markers and a self-public) are
retained in reports and the exact object hash. A separate narrow
[external DGROUP gate](dgroup-binding.md) proves offset16 fields using frozen,
independent original witnesses and explicit linker equations; no bytes are
masked, patched or normalized. A separate
[runtime near-code gate](runtime-near-binding.md) supports anchored F0/T6
self-relative offset16. Unsupported fixup modes remain refused.

Comparisons use the complete emitted segment, never an oracle-sized slice.
Byte similarity and first-difference offsets are diagnostic. Promotion reruns
the historical compiler on a frozen independent source copy. Validation freshly
recompiles every accepted source and checks full OMF identity, complete code,
tool hashes, original asset/structure identities and non-overlapping ownership.
Missing tools and compilation failures never fall back to cached objects.

`tools/runtime.py` separately verifies pinned historical library members. Each
recipe hashes the independent library archive and entire OMF member, owns the
full initialized CODE segment, and checks declarations, original bytes and
fixup/relocation obligations. Ten relocation-free members cover 653 bytes; the
13-byte close member additionally verifies an external near equation against
independent zero-owned provider context. These eleven members cover 666 bytes
as PINNED_RUNTIME, separate from reconstructed C or ASM. Partial startup/provider
signatures remain evidence only and contribute no accepted bytes. Runtime
publication uses `tools/pin_runtime.py` with a reviewed worker submission and
fresh strict verification; whole members cannot be trimmed to a matching prefix.

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

The [current state-helper milestone](state-helpers.md) brings matching C to 216
bytes across eight complete contributions, with twenty-nine external DGROUP
fields proved. Matching ASM remains 51 bytes and pinned runtime remains 666
bytes. The reviewed 26-byte signed-value target is still unowned: its full
24-byte candidate fails the extent gate. The new 80-byte predicate remains
unowned despite agreeing data equations: four ordinary register-encoding bytes
differ. Source hypotheses cannot inherit ownership from neighboring accepted
helpers, agreeing equations or matching individual instructions.

Canonical source and binding file pins use the repository's LF checkout policy.
Source publication normalizes its frozen independent copy to LF, then compiles
and pins that copy; worker input is unchanged. JSON output uses explicit UTF-8
bytes. Source/runtime publication refuse CRLF binding files so Git cannot
silently change their pinned identity. This file-format rule does not change
original bytes, object bytes, fixup equations or component extents.
