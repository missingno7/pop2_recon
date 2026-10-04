# Component acceptance

`tools/match.py` reads expected bytes solely from the SHA-locked original EXE.
`evidence/targets.json` freezes reviewed extents and their independent boundary
evidence. A whole emitted CODE SEGDEF belongs to one public at offset zero.
Declared length, every initialized byte, padding, all publics, secondary storage,
records and FIXUPP subrecords are inspected. CODE holes, multiple publics,
nonzero secondary data/BSS, unknown storage records and 32-bit records fail.

The initial gate accepts only components with **zero actual object fixups and
zero original relocation obligations**. FIXUPP thread definitions are linker
declarations, not relocation requests; they are decoded and retained. Unused
EXTDEF declarations (including Microsoft CRT markers and a self-public) are
retained in reports and the exact object hash. No referenced external or
relocation can pass this gate. Nothing is masked or normalized.

Comparisons use the complete emitted segment, never an oracle-sized slice.
Byte similarity and first-difference offsets are diagnostic. Promotion reruns
the historical compiler on a frozen independent source copy. Validation freshly
recompiles every accepted source and checks full OMF identity, complete code,
tool hashes, original asset/structure identities and non-overlapping ownership.
Missing tools and compilation failures never fall back to cached objects.

CODE_EXACT means the recorded compiler/profile reproduces the complete reviewed
component code with its proven fixup obligations. It does not establish original
translation-unit membership, historical object-record identity, natural final
placement, library order, overlay vectors or whole RTLink closure. Those remain
separate structural proof. No hybrid executable is constructed in this bootstrap.

The final overlay is physically 21,103 bytes although its descriptor allocates
21,104 paragraph bytes. The guarded profile records one absent terminal byte;
oracle extraction neither reads past EOF nor supplies a synthesized zero. The
three file bytes before overlay 2 relocations remain unowned alignment debt.

Source semantics, game/runtime classification, names and original build/version
are evidence-qualified claims independent of byte equality. Compiler probes can
establish code-generation facts without proving the compiler used by every TU.
