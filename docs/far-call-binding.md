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
these fields. V1 requires the entire original relocation subsequence to equal
far FIXUPP traversal order, and freezes its original indices in the binding,
recipe and owner. It does not infer RTLink symbol-group ordering or reorder a
mismatch. All remaining bytes compare literally. Validation freshly compiles,
checks raw OMF identity, witnesses, equations and indices; metrics report the
covered index list and `relocs=X/N` for each original space.

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
  `0x1e702` is root MZ relocation **1764**, now the sole covered original
  relocation: root **1/2,029**, all spaces **1/4,716**.

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
