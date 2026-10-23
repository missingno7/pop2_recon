# State-step caller/callee cluster (2026-10-23)

Three fresh MSC 6.00 compilations support a grouping of the caller at
`root:0000:0e78` and accepted `UPDATE_STEP_DELTA` at `root:0000:0e8e`.
The complete combined CODE segment is 34 bytes, with publics at offsets 0 and
22. Every literal byte agrees with the independently bounded original interval;
five DS-relative field equations and one same-object near-call equation also
agree. This is **research evidence**, not CODE_EXACT or original-TU proof.
No ownership, target entry, binding mode or acceptance rule was changed.

The [receipt](../evidence/tu/state-step.json) records original bytes and witnesses,
readable candidate/control source, source/object hashes, complete CODE identity,
all segment/public/external declarations, FIXUPP order and thread declarations.
Generated objects and bulk contexts remain in `build/workers/tu_cluster/`.

## Independent original evidence

The caller occupies `[0x0e78,0x0e8e)`: a signed byte load, increment promoted to
16 bits, upper clamp to 12, byte store, `PUSH CS; CALL near`, `RETF` and a NOP.
The preceding function ends with `RETF 2` at `0x0e74` and a NOP at `0x0e77`.
The callee occupies `[0x0e8e,0x0e9a)`, including its trailing NOP; its existing
reviewed extent and accepted recipe independently establish that boundary.
The following entry at `0x0e9a` starts with saved DI/SI and has a separate
relocated incoming far call. No original MZ relocation word touches the cluster.

The far call at root image offset `0x46bc` targets `0000:0e78`; its segment word
is original relocation index 343. It lies at the second destination of a
six-case CS-relative switch table at `0x4696`, using CS `02cc` and entries
`19e2,19fc,1a04,1a0c,1a14,1a3a`. The original bounds the selector to 0..5 before
doubling it. Invocation of that dispatcher remains untraced; this is static
call/table evidence, not execution evidence.

For the prospective combined extent, the accepted callee's witness inside
`0x0e78` would no longer be independent. The investigation instead uses direct
DS accesses in the next function: `0x0ea1` observes byte `5c14`, `0x0ea8` loads
byte `5c12`, and `0x0ead` loads byte `5c13`. That function is independently
entered by the far call at `0x195f`, with original relocation index 150.
These observations ground relative byte offsets, not data allocation, original
symbol names or runtime DS identity.

## Compiler contrast

All three probes use pinned `msc600` and `/c /AM /Oe /Gs /Zl /Gc`.

| Source context | Whole CODE | Observation |
| --- | ---: | --- |
| Caller and callee together | 34 bytes | Callee public at +22; `PUSH CS; CALL near` at +16; caller `RETF; NOP` at +20 |
| Caller with callee prototype only | 22 bytes | `CALL FAR` at +16; `RETF` at +21, with no tail NOP |
| Accepted callee alone | 12 bytes | Entire OMF object equals its existing recipe; complete callee CODE is unchanged in the combined source |

The isolated and contextual caller contributions are both 22 bytes. Length
alone therefore cannot distinguish their call encoding: one byte of instruction
size is exchanged for alignment padding. The contextual near field at +18 has
raw addend zero; the callee public supplies `22 - (18 + 2) = 2`, agreeing with
the original displacement. The five data fields at +1, +14, +23, +27 and +30
have zero raw addends and agree with the outside DS-relative observations.
Neither the object nor original bytes were patched or masked. Literal intervals
and explicit field equations account for all 34 bytes; they grant no acceptance.

The source preserves the accepted callee's unsigned byte declarations and uses
an explicit signed-byte cast for the caller's load. Its `int next` keeps the
increment before truncation: input 127 clamps to 12, while input -128 becomes
-127. The caller's return is declared `char` to forward AL, but its original
caller ignores that value; a `void` declaration remains plausible. Names and
the original declarations are hypotheses.

## Remaining proof

The existing extractor correctly refuses this object because it has two
publics. Reconstructed-C acceptance also lacks the F5/T6 self-relative LOC1 mode
targeting another public in the same object. The already accepted 12-byte callee
overlaps this proposed grouping; a future group must migrate that ownership,
not count it twice. The whole cluster would add at most 22 source-owned bytes.

Do not resume source grinding: the predicted encoding, extent and callee
stability have been observed with controls. The next useful investigation is
the surrounding functions and same-CS callers, to constrain whether these two
functions belong to a larger unit. Original TU boundaries, full declarations,
data ownership, natural RTLink placement and runtime behavior remain open.
A grouped-public/near-call proof would require its own reviewed implementation
and refusal tests; this investigation deliberately leaves the gate unchanged.
