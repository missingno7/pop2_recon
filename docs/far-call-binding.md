# External root CALL FAR binding

`external-far-call-v1` proves one complete initialized CODE segment, one
owning public at zero and no secondary storage through the existing
`search.py`, `promote.py --binding`, recipe and fresh `validate.py` flow.
It is a component proof, with a deliberately finite root-only scope.

Each far obligation must be segment-relative OMF LOC3 pointer32, external T6,
F5 target-derived frame, omitted explicit displacement and four zero raw
LEDATA bytes. Its entire operand must belong to one decoded, unprefixed
five-byte `9A` CALL instruction. Near/indirect calls, literal calls, interrupts,
far jumps, other FIXUPP shapes and nonzero addends are refused. Unreferenced
EXTDEFs and FIXUPP thread declarations remain in the full object proof.
Nothing rewrites the object or constructs a normalized linked buffer.

A `far-code-entry` symbol records an exact root segment:offset alias, a
reviewed `entry_target_id`, its original extent hash, and independent original
CALL witnesses. The referenced `evidence/targets.json` row must have
`boundary_status: REVIEWED` and `entry_kind: direct-code`; `get_target` freshly
rechecks the extent, hash and recorded callers. Root vectors/thunks must not
be classified as direct entries. The callee extent cannot overlap the owner.
At least one frozen original resident `9A` outside the owner must independently
contain that exact pair, with exactly one touching relocation at its segment
word. Its bytes, file position and original relocation index are rechecked.
Neither the candidate's own call nor an equivalent linear address with a
different segment:offset representation can establish the alias. Callee and
witness context grant zero ownership. Existing accepted entries need this
explicit direct-entry review and independent witness before use in this mode.

For independently established `S = 16*segment + offset`, frame `F = 16*segment`,
raw addend `A = 0` and omitted `D`, the offset equation is
`S - F + A + D = offset`; the segment word is `F/16 = segment`. Both original
words must equal these values. The generated loader obligation is exactly the
segment word, operand offset +2. Every original relocated word touching the
owner, including one starting a byte before it, must correspond one-to-one to
these fields. The exact site set, one-to-one correspondence, original table
indices and within-target-symbol FIXUPP order are checked without sorting or
rewriting either sequence. Original symbol groups projected onto the owner must
be contiguous. Indices remain frozen in original table order in the binding,
recipe and owner; reports separately retain generated FIXUPP traversal indices.
All remaining bytes compare literally. Validation freshly compiles, checks raw
OMF identity, witnesses, equations, order qualification and indices; metrics
report the covered index list and `relocs=X/N` for each original space.

The original v1 whole-subsequence equality was over-strict. The independently
reproduced [RTLink order probe](../evidence/rtlink-probe/relocation-order/README.md)
shows target-symbol buckets within a **combined output CODE segment**, across
input object modules and LEDATA records. Another segment in the same object
has separate buckets. References to different public names at the same address
remain separate buckets. Each module/segment/symbol projection keeps raw OMF
FIXUPP traversal exactly. Thus a whole single-CODE component can prove its site
bijection and within-symbol order; it cannot determine the symbol bucket
permutation or full output-segment placement from its own object.

Reports use `relocation_order: EXACT` when whole FIXUPP traversal happens to
match the original subsequence, otherwise `GROUPED` after all group checks
pass. A GROUPED report and recipe explicitly retain `relocation_order_debt`:
between-target bucket order and combined-segment placement need structural
linker proof. Validation refuses a missing or changed qualification/debt. Old
recipes without these fields mean EXACT and still pass unchanged. Neither
qualification proves original TU membership or a natural whole-program link.

Composition uses the same `symbols` table with `near-data-alias` entries and a
`data` object containing the resident DGROUP `mode` and `frame` from
[dgroup-binding.md](dgroup-binding.md). Both zero-addend v1 and independently
witnessed two-word v2 reuse the existing grounding and field checks. Only
already-proven far calls relax the old call prohibition; direct data operands
still require symbolic fixups, segment changes remain refused, and data fields
generate no MZ relocations. Equations hold for every DS value; a call's runtime
DS preservation, original declarations and behavior are not proved. Legacy
standalone data modes still refuse all calls and relocations.

Original facts for the exemplar:

- Wrapper `root:1e23:04c6` is exactly `[0x1e6f6,0x1e70a)`, 20 bytes.
  The preceding procedure ends in `RETF 6`; the next BP frame starts at
  `0x1e70a`. A separate `PUSH CS; CALL near` at root `0x1e602` enters the
  wrapper. Its frame forwards the far pointer at BP+6/BP+8 and ends in `RETF 4`.
- Callee `root:1101:25e4`, `[0x135f4,0x1360a)`, has a BP frame, nullable far
  pointer test, one conditional join and `RETF 4`, followed by a new frame.
  For a non-null pointer it clears the top two bits of the word six bytes
  before the pointer. Eighteen relocated root calls use this exact alias.
  Independent witness `root+0x589a`, `9a e4 25 01 11`, has relocation index
  584 at `0x589d`; it is outside the accepted wrapper.
- The wrapper's CALL at `0x1e6ff` has operand offset 10. Its segment word at
  `0x1e702` is root MZ relocation **1764**, the first covered original
  relocation. That initial milestone had root **1/2,029**, all spaces **1/4,716**.

Compiler observation: pinned MSC 6.00 with
`/c /AM /Oe /Gs /Zl /Gc` emits the complete 20-byte segment, public
`FORWARD_FAR_BLOCK_FLAGS`, and exactly one F5/T6 LOC3 at offset 10.
[src/forward_far_block_flags.c](../src/forward_far_block_flags.c) and its
[binding](../evidence/bindings/forward_far_block_flags.json) reproduce 16
ordinary bytes plus the independently grounded four-byte equation.
Names, `void` result, resource/heap interpretation and original TU/library
membership are source hypotheses; the callee remains unowned SUPPORT_CODE.

Overlay observation: overlay-2 offset `0x63d7` also contains
`9a e4 25 01 11`; overlay relocation index 766 targets its segment word at
`0x63da`. Thus at least this overlay-to-root call uses the direct root alias
and an overlay relocation record. This does not establish all overlay routing.
The root calls at `0xe6f2` and following sites instead address manager entries
such as `2203:0d21`: its original bytes begin
`e8 34 f8 ea 0e 00 da 25` (near manager call, then far jump). These stubs
need a separate vector/dispatch proof. V1 refuses overlay owners, overlay
callees, overlay witnesses and entries reviewed as vectors; it implements no
vector or overlay placement rule. No new historical linker probe was needed;
the existing [pointer32 link receipt](../evidence/toolchain/binding-link-probes.json)
already records the segment-word relocation equation.

The private immutable-byte discovery census decoded **432 distinct**
`PUSH CS; E8 rel16` instruction pairs (426 root, six overlay), including the
independently reviewed wrapper caller above. This is compatible with same-CS
far-return calls; it does not prove original TU membership or that MSC emitted
every observed site. A separate MSC 6.00 `/AM /Oe /Gs /Zl /Gc` probe with two
far Pascal functions in one source emits `PUSH CS; E8` and a self-relative
LOC1 external F5/T6 fixup at offset 20. The source and full hashes are recorded
in the [review receipt](../evidence/matching/wave9-far-call-review.json).
No TU binding mode is added. Raw byte-pattern scanning
finds more pairs and is not counted as decoded code.

The same conservative census has 249 root tentative extents containing far
calls; **91** end in a return, contain only direct far calls, explain their
relocation sites and address entries below the manager region. These are a
plausible root review pool, not 91 accepted functions: boundaries, direct
callee grounding, data shapes, compiler output and strict relocation order
still need proof. The separately supplied ~592-function frontier uses a
broader recovery scope; this count is not a recount or acceptance of it.
Remaining debts include overlay vectors, near/TU calls, other fixups, natural
placement, complete RTLink closure and runtime behavior.


## Multi-callee 62fe qualification

The [original order receipt](../evidence/toolchain/far-call-order-original.json)
rechecks 1,070 increasing site pairs among 2,028 consecutive root relocation
pairs. Calls to `1101:4faa` occur at indices 466..475 and 1453; calls to
`1101:4fca` at 1426, 1654, 1719 and 1720. These address-based observations are
consistent with segment/symbol buckets; they do not recover original symbol
names, TU boundaries or the linker version. The synthetic probe separately
establishes the tested ordering mechanism rather than importing SimAnt's rule.

`root:1000:62fe` is the existing reviewed 24-byte wrapper at root+0x162fe.
It pushes BP+6, calls `1101:4faa`, pushes AX, calls `1101:4fca`, restores the
frame, executes RETF 2, then has the original alignment NOP. The next frame
starts at 0x16316. Six independently recorded callers and both reviewed callee
entries are rechecked. Pinned MSC 6.00 `/c /AM /Oe /Gs /Zl /Gc` emits the whole
24 bytes and F5/T6 LOC3 fields at offsets **13, 7**. Original relocation indices
**1453, 1654** instead identify segment words at root+0x16307 and root+0x1630d.
Each symbol has one field, so its group order is exact. The complete
[src/load_and_exchange_attribute.c](../src/load_and_exchange_attribute.c) is
CODE_EXACT with GROUPED debt pinned in its recipe. Both indices gain coverage;
no code byte, fixup or relocation record is changed.

The separate 300-call compiler contrast emits three LEDATA chunks: starts
0, 745, 1490; lengths 745, 745, 12; fixup counts 149, 149, 2. FIXUPPs descend
inside each chunk and continue in LEDATA traversal order across chunks. MASM's
probe references ascend. Sorting by site, globally reversing FIXUPPs, or
requiring a universal descending sequence would therefore be incorrect.
All non-order v1 restrictions and the root-only scope remain in force.


## RTLink relocation order and debug line info (2026-11-21)

Source-only RTLink 6.10 controls ([rule](../evidence/rtlink-probe/bucket-order/rule.md))
show the order of MZ relocation records:

- **Within one linked output segment,** RTLink groups records into buckets by
  resolved target: an external symbol, a group or an input SEGDEF.
- **Inside a bucket** it keeps raw FIXUPP traversal order, LOC2 before LOC3.
  This is the within-symbol rule the gate already checks.
- **The bucket permutation depends on link context.** Two links with identical
  load images produced different bucket orders, so between-bucket order stays
  recorded `GROUPED` debt.

The original CHECKNIS.C within-bucket orders match when the same source is
compiled with `/Zd`, or `/Zi`, because line-number records split MSC's LEDATA
chunks and so change FIXUPP traversal. The code bytes are unchanged. The
original game was therefore very likely built with debug line information.

The strict gate accepts `/Zd` LINNUM records (`0x94`), which carry no code or
fixups. Use `/Zd` when a within-symbol relocation order is refused. `/Zi`
(CodeView `$$SYMBOLS`/`$$TYPES` segments) is not yet supported by the OMF
reader.
