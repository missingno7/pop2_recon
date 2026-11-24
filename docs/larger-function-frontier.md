# Larger-function follow-up — 2026-10-23

> Historical: this page describes the earlier build (tag `oracle-dos-1565`). Its addresses, hashes and counts are not those of the v1.1 target; evidence files it cites are at that tag.

The complete 160-byte `FALLBACK_TILE_KIND` at `root:02cc:016a` is accepted as
CODE_EXACT through the existing resident DGROUP gate. This raises reconstructed
C from 2,189 to 2,349 bytes across 57 components; the prior largest component was
88 bytes. No acceptance code, toolchain lock or original asset changed.
Fresh full validation passes all 220 invariant tests, recompiles all accepted
C/ASM and rechecks every pinned runtime member.

## Why this target worked

The tentative discovery of its 122-byte caller stopped after 90 bytes at an
unfollowed jump. Reviewing the full CFG exposed its zero-room branch, which
pushes two byte arguments and calls this fallback using `PUSH CS; CALL near`.
The fallback itself has no outgoing calls. Its complete interval is bounded by
the previous `RETF 6; NOP`, its own `RETF 4`, and the next independently called
entry at `02cc:020a`.

The [source](../src/fallback_tile_kind.c) selects byte 0 or 20 with a sparse
switch and nested guards. All thirteen fixup fields use four byte aliases with
independent outside witnesses. The [binding](../evidence/bindings/fallback_tile_kind.json)
proves those fields; the other 134 bytes compare literally. There are no
original relocation words in the full 160-byte extent.

The first source expressed case 4 as one OR condition. MSC600 `/Oe` emitted
158 bytes, reusing an earlier JNE through a backward jump. Expressing its two
successful conditions as separate early `break` exits emitted the original
160-byte branch shape. `/O` was a negative control at 162 bytes. The bounded
journal stopped at exact on trial four; publication and validation then
compiled freshly. The [receipt](../evidence/matching/larger-function-followup.json)
preserves controls, hashes, branch checkpoints and the separate record-helper
negative result. Source names and gameplay interpretation remain hypotheses.

## What remains for its caller

| Dependency of `root:02cc:00f0` | Current result |
| --- | --- |
| Near call to `02cc:07c6` | Accepted 54-byte signed-byte helper |
| Near call to `02cc:016a` | Accepted 160-byte fallback from this milestone |
| Near call to `02cc:0002` | Unowned recursive neighbor adjustment; indexed DS array bases at `44be..44c1` need separate evidence/proof |
| Far call to `0f77:0002` | Unowned table-pointer setup; immediate address construction using `2ce4/304a` is outside the current data modes |

The caller has three same-CS near calls, not the two visible in the truncated
discovery. A grouped-public/near-call extension alone would not close the whole
source unit: contiguous peers, indexed data bases and pointer construction still
need work. Accepted peer bodies cannot be sliced from their objects and pasted
into a larger ownership claim.

The state-step [RTLink controls](../evidence/rtlink-probe/near-public/README.md)
now verify the forward/backward relative equation and distinguish same-file
code generation from simple linked adjacency. They grant no ownership and do
not change the current gate. The useful next structural target is the recursive
`02cc:0002` provider and its array declarations; other leaf peers with supported
bindings can proceed through the current matching loop in parallel with that
research. There is no function-size limit preventing larger leaf matches.
