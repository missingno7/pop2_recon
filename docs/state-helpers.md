# Four complete state-helper C matches — 2026-10-05

Four independently reviewed resident helpers now pass CODE_EXACT under pinned
MSC 6.00 `/c /AM /Oe /Gs /Zl /Gc`. They add 104 bytes to the preceding
[data-helper milestone](data-helpers.md), bringing matching C to 216 bytes
across eight complete contributions. The external DGROUP gate is unchanged.

| Readable source | Original interval | Complete CODE | Data equations | Ordinary bytes |
| --- | --- | ---: | ---: | ---: |
| [update_pair_4052.c](../src/update_pair_4052.c) | root `[0x4052,0x4072)` | 32 | 5 | 22 |
| [update_pair_4072.c](../src/update_pair_4072.c) | root `[0x4072,0x4092)` | 32 | 5 | 22 |
| [clear_state_bytes.c](../src/clear_state_bytes.c) | root `[0x4092,0x40a4)` | 18 | 4 | 10 |
| [conditional_negative.c](../src/conditional_negative.c) | root `[0x424e,0x4264)` | 22 | 1 | 20 |

## Original boundaries, callers and ABI

The [independent locked-byte review](../evidence/matching/state-helpers-original-review.md)
records the original instructions separately from compiler observations.
Both pair helpers have distinct BP prologues, one byte local, direct byte
negation, and a three-assignment exchange. Their RETF instructions immediately
precede the next independent entries. There are no calls, branches or original
relocations within either complete 32-byte interval.

A separate resident caller occupies `[0x4008,0x4051)`. Its decoded conditional
paths execute PUSH CS followed by near CALL at `0x4019`/`0x403f` to `0x4052`,
and at `0x4024`/`0x404a` to `0x4072`. No argument is pushed; explicit CS supplies
the segment consumed by RETF. The caller surrounds two other far calls with
these updates. Its own entry reachability and runtime execution remain unproved.
Separate direct byte instructions at `0x3f84`, `0x3f87`, and `0x40d4` onward
supply the negative-field and exchange-field witnesses, outside the targets.
The [pair receipt](../evidence/matching/update-pair-helpers.json) preserves both
worker objects and canonical proofs.

The clear helper stores zero to byte offsets `628f`, `628e`, `628c`, `628d`,
then returns AL=1. Its RETF at `0x40a2` and NOP at `0x40a3` precede a distinct
copy entry at `0x40a4`; the compiler emits the whole 18 bytes including the NOP.
Thirty relocation-backed far encodings target `02cc:13d2`: 27 in overlay 3
and one each in overlays 8, 10 and 16. The reviewed overlay-3 sequence at
`0x32a8` tests byte `628f`, conditionally calls at `0x32af`, then stores the
returned AL at `0x32b4`. Independent reads and writes in the pair helpers ground
all four relative byte aliases. The [receipt](../evidence/matching/clear-state-bytes.json)
retains the complete reference list; reference presence does not prove execution.

The conditional-negation helper reads one word at BP+6, negates DX if byte
`5c01` is zero, and returns AX with RETF 2. An original NOP at `0x4263` aligns
the next BP prologue at `0x4264`; the complete comparison is 22 bytes. The
relocated overlay-3 call starts at `0x3cdf`, with the segment word at `0x3ce2`
(index 329). SUB AX,AX; PUSH AX passes zero; MOV SI,AX immediately consumes
the returned word. Independent byte access at root `0x32f0` and `0x20ac`
grounds the flag. The [receipt](../evidence/matching/conditional-negative.json)
preserves that ABI context and the one data equation.

## Complete source and binding proof

Every accepted source freshly emits one public at zero in one fully initialized
CODE segment and empty secondary storage. Every ordinary byte compares
literally; each two-byte data operand uses the existing zero-addend external
F5/T6 LOC1 equation and separate original offset/width witnesses. No field is
masked or changed. The startup DS witness, declarations, ordered fixups and
source/binding/full object hashes are checked freshly.

Canonical public names are distinct. Byte aliases keyed by relative offset
agree across these helpers; they recover no original symbols or data storage.
The worker submissions remain unchanged under their original identifiers and
module basenames, with separate whole-object pins. Identifier changes alter
canonical OMF identity, but the complete emitted CODE bytes remain unchanged.
Both worker and canonical objects are recorded. Source qualifiers, signedness,
state meanings, original translation units and the runtime DS association remain
hypotheses. Historical narrowing and negation behavior has not been validated
with runtime traces or a portable arithmetic proof.

## Unowned 80-byte predicate

The newly reviewed target `root:0284:0096` covers root `[0x28d6,0x2926)`:
a 79-byte body, RETF at `0x2924`, and the original alignment NOP at `0x2925`.
Four far CALL opcodes start at root `0x0dff`, `0xa7c1`, `0xa7e4`, `0xe937`;
segment-word relocations are at `0x0e02`, `0xa7c4`, `0xa7e7`, `0xe93a`.
A separate PUSH CS; near CALL at `0x2ba6`/`0x2ba7` reaches the same entry,
then normalizes returned AX before passing it onward. These are static contexts.

The [readable non-owning replay](../recipes/probes/state_predicate.c) checks
three word guards and four literal byte-pair cases. Fresh MSC 6.00 `/Oe`
emits a complete 80-byte segment. Eleven independent data equations agree,
but four ordinary instruction bytes differ because the candidate uses DX
for the zero/result where the original uses BX. Strict search reports
CANDIDATE_C, first difference at byte 1, and grants zero ownership.

The [negative receipt](../evidence/matching/state-predicate-negative.json)
retains full declarations, original witnesses, equations and 34 bounded
source/flag trials. Explored MSC 5.10 outputs use stack storage or a saved SI;
MSC 6.00 contrasts include 79-, 80-, 81-, 82- and 96-byte complete extents.
All remain mismatches. Neither shortening an extent nor forcing an unrelated
pointer interpretation supplies an accepted source. A distinct natural source
or compiler-generation lead is still needed; the experiments do not exclude
other source/TU/compiler possibilities.

## Validation and accounting

Fresh search, verify-only promotion and publication pass for all four helpers.
Full `python tools/validate.py` passes 108 invariant tests, freshly compiles all
eight C functions, freshly assembles the one ASM function, checks all eleven
whole runtime members, and rechecks all original assets, layout and tool pins.
No acceptance tooling was changed in this milestone.

| Category | Bytes |
| --- | ---: |
| Matching C | 216 |
| Matching ASM | 51 |
| Pinned runtime | 666 |
| External drivers | 0 |
| Unresolved identified game-helper hypotheses | 152 |
| Unknown/unclassified payload | 270,303 |

The identified helper subtotal is 368 bytes, including three unresolved targets:
the 46-byte overlay swap, 26-byte signed-value helper and 80-byte predicate.
All 933 owned bytes remain resident. Twenty-nine source data fixups and one
runtime near fixup are proved; the predicate's diagnostic equations are not
included. Original MZ relocation coverage remains 0/4,716. Structural RTLink
linkage remains UNRECOVERED and behavior validation remains NOT_STARTED.
