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

## Grounding a callee with no outside caller (2026-11-21)

When a reviewed resident `far-code-entry` callee has no original relocated
CALL FAR outside the candidate object, `ground_symbol` accepts, in order:

1. **accepted exact entry**: a canonical CODE_EXACT, ASM_EXACT or
   PINNED_RUNTIME owner starts at the same position (the strongest form,
   reached by matching callees first);
2. **structural entry** (`grounded_by: structural-entry`, weaker, recorded as
   such with its anchor and frame witness). Every condition must hold:
   - the entry precedes the RTLink manager/vector region (`2203:0000`);
   - it starts with the BP-frame prologue `55 8B EC`, so frameless callees
     still need an outside caller or an accepted owner;
   - it lies strictly inside no reviewed or accepted extent, and its own
     extent overlaps no accepted owner;
   - a reviewed or accepted extent (the anchor) ends at `entry - pad`, where
     pad is 0, or one `90`/`00` byte touched by no relocation. The anchor's
     linear decode ends exactly there with RET, RETF, RET iw or RETF iw;
   - another relocated `9A` outside the candidate object uses the same
     segment word for a different reviewed entry. This witnesses the 16:16
     representation independently of the candidate.

These conditions come from measurements on the original image. Of the 493
relocated far-call targets, 84% are preceded by a return plus at most one pad
byte. Accepted C, however, contains 11 `RETF n; NOP` labels in the middle of a
routine, because MSC duplicates frameless epilogues (e.g. `078a:2830` at
`0xa104`). Raw byte matching also fires at 2.25% of root positions. None of
the 24 such interior boundaries starts with `55 8B EC`, and no prologue
preceded by a return lies inside a reviewed or accepted extent. A missing
anchor is supplied by reviewing the preceding routine as a target, not by
pattern matching. Overlay callees and RTLink vectors keep the outside-witness
or accepted-owner rule.

Every far-code-entry and overlay-code-entry callee must also lie outside
accepted owners, except one that starts exactly at the entry.

## Near-code-entry (groups)

A same-source group may call a near routine outside its object (MASM `EXTRN
NAME:NEAR`, LOC1 self-relative F0/T6 or F5/T6, zero addend, `CALL near` with or
without `PUSH CS`). The binding names it as a `near-code-entry` symbol: `space`,
`segment`, `offset`, `entry_target_id`, `entry_sha256` and `witnesses`. The field
must equal `callee - (field + 2)` mod 65536 in linear positions.

The callee position is grounded independently of the candidate:

- the reviewed target row is rechecked (extent hash, exact 16:16 alias,
  `entry_kind: direct-code` or an accepted owner); it lies in the object's space,
  outside the whole object, and inside no accepted code except an owner that
  starts there;
- **accepted exact entry:** a CODE_EXACT, ASM_EXACT or PINNED_RUNTIME owner
  starts at the position, or
- **original near CALL:** each witness is `E8 rel16` (optionally with its
  `PUSH CS`) at a recorded image offset outside the whole object. Its bytes are
  rechecked, and the instruction is reached by recursive descent from an
  independent `entry`: a relocated CALL FAR destination, a reviewed direct-code
  entry or an overlay vector target. Its rel16 must land on the callee in that
  entry's CS window. The candidate's own call never counts.

The candidate's call is then proven only inside the claim's witnessed CS window,
as for same-object calls ([acceptance.md](acceptance.md)). External near callees
are never assumed to preserve segment context. Limits: no overlay-to-root or
cross-space near calls, no near jumps to external code, and no frame inferred
for a component entered only by near calls.

## RTLink vectors without an outside caller (2026-11-21)

An `rtlink-vector` callee with empty `witnesses` is grounded by its own
record (`grounded_by: vector-record`). The census decoder checks the record's
manager-intercept CALL, the relocated far JMP and the overlay descriptor.
The decoded destination must start a reviewed target row, or an accepted
owner, in the destination overlay, so the destination has independent
boundary review. An outside caller would add nothing beyond the record
itself. A destination without a reviewed row is refused.

## Draft-region far witnesses in groups (2026-11-21)

In a same-source group, an independent relocated CALL FAR witness must lie
outside every claimed extent rather than outside the whole compiled object.
A call in an unclaimed draft region is an original fact the claims are not
judged on, matching the v4 draft-region rule in docs/dgroup-binding.md.
When the group later claims that region, the witness is refused and the
binding must switch to another grounding. The callee entry itself must
still lie outside the whole object. Structural-entry frame witnesses keep
the whole-object rule.

## Overlay owners in groups (2026-11-21)

A same-source group may claim code in an RTLink code overlay (`overlay-N`). The
single-function far gate (`external-far-call-v1`) and the structural-entry rule
stay root-only; a one-claim group covers an overlay owner instead.

**CS frame evidence.** Overlay code runs only while its section is loaded at its
link address, and several overlays share link segments (2/3, 4–7/10, ...). A far
entry therefore names an overlay claim's CS only if it provably reaches this
overlay. `module_gate._frame_evidence` accepts, per claim:

- an **RTLink vector record** (decoded by `tu_census.overlay_vectors`, the format
  the manager checks at `2203:064c`) whose descriptor index names this overlay and
  whose segment:offset is the claim entry. The manager loads that section and
  far-jumps with that segment;
- a reviewed **same-overlay CALL FAR**, relocated by this overlay's own table and
  outside the whole object. Its caller is resident, and the manager evicts whole
  sections, so the destination is this overlay's copy;
- a reviewed **cross-space CALL FAR** (root or another overlay) only when no other
  space holds that link address.

Anything else is listed in the claim's `frame_evidence_refused` (for example
`link address also held by overlay-2`) and grants nothing. A vector naming
another entry does not witness this claim. All claims must agree on one frame;
same-object near calls, external near calls and own-segment words then use that
64 KB window exactly as in root (own-segment words additionally need the whole
claim inside the window). Inheritance through equal near calls is unchanged.

**Far calls.** `far-code-entry` (resident), `overlay-code-entry` and
`rtlink-vector` callees ground as before (reviewed direct entry plus outside
witnesses, or an accepted exact owner). A direct call into a *different* overlay
must be to one that can be resident together with the caller's overlay
(`tu_census.can_coexist`); otherwise the original call could not reach it. The
field equation depends only on the grounded 16:16 alias. Its segment word is an
obligation of the owner's **own overlay relocation table**: the claim's touching
records (by overlay-table index) must biject to the generated segment words, with
the same within-symbol FIXUPP order and `GROUPED` debt as root. The overlay
tables show the same output-segment/target buckets as the MZ table (overlay 3:
four frame runs, no interleaved target run). Recipes and metrics record these
indices per space.

**Data.** `external-dgroup-offset16-v4` now accepts overlay owners (groups and
single functions): overlay code runs with the startup DGROUP in DS, the frame
witness stays the root startup DS load, and witnesses follow the existing
outside / draft-region / internal-consensus rules. Overlay witnesses still need
an `entry` reached in their own overlay (vector destination, same-overlay
relocated CALL FAR destination or reviewed entry). The older modes are unchanged:
resident v1/v2/v3 refuse overlay owners, overlay v1 keeps its byte/word rule.
Placed DGROUP segments remain root-only.

Real-image tests (`tests/test_overlay_group.py`) use byte-faithful synthetic
objects at reviewed overlay-3 targets: `25da:0016` proves three far calls with
overlay-3 relocations 207, 272, 354 and ten v4 fields; `2344:08f0` proves its near
call in frame 2344 through the same-overlay call at overlay-3+0x2f77.
The same draft-region rule applies to group `segment-base` and `far-data`
witnesses: relocated segment words and offset instructions in an unclaimed
draft region count, and those inside a claimed extent do not.

## Overlay site frames (2026-11-21)

Overlays 2 and 3 share link segments, so root calls into overlay 3 do not
reach a frame (`link address also held by overlay-2`). The overlay's own
relocation table names the frame directly. A controlled RTLink 6.10 link
([site-frames](../evidence/rtlink-probe/site-frames/README.md)) shows:

- every record stores the frame of the segment holding its site, which is the
  paragraph floor of that segment's start;
- every relocated far word into the segment stores that same frame;
- frames increase with position;
- a relocation-free file has a frame but no record;
- a segment inside a group is the exception: its records name its own floor,
  while its callers use the group frame.

The locked image agrees. Its 15 code-overlay tables are monotone, and all 447
relocated CALL FARs into overlay frame extents match a holder's site frame.

`module_gate._site_frame` adds the evidence kind `overlay site frame` to an
overlay claim when all of these hold:

- **Bracket.** The last record of the claim's own overlay table with a site
  before the claim's end, and the first with a site at or after its start, name
  one frame F. Both may be the claim's own far-call records, because site
  frames are linker-assigned and the candidate does not produce them. Since
  frames are monotone, every segment between those two sites has frame F, so
  the claim's segment does too (one reviewed function lies in one segment). If
  the two frames differ, the evidence is refused, because a relocation-free file
  may lie between them. The census extent alone is not enough.
- **No alias.** Every relocated far word that lands inside the whole object
  uses F. A word is the stored segment and the offset word before it, in any
  space; a decoded vector that names another overlay is the only exception. A
  word with another frame (for example root `2a4e:0000` landing in overlay 4)
  refuses the evidence.
- **Window.** F's 64K window holds the whole object.
- **Monotone.** The table is monotone.
- **No group.** The candidate object puts its CODE segment in no group.

Each refusal appears in `frame_evidence_refused`. Of the 64 reviewed
code-overlay targets, 47 get site-frame evidence. In the 11 that also have
another witness, the frames agree. 36 targets are newly witnessed, including ACT
`25da:047e/0092/01e0` and COLLIDE `2802:000e`. AUTO `2344:0c6e` brackets in
frame 2344 but has no reviewed target yet. Tests:
`tests/test_overlay_group.py` (real 08f0 claim with no far entry, the named
targets, an unbracketed `25da:000e`, the alias in overlay 9, and synthetic
straddle, alias, group and monotonicity refusals).

## Far data members (2026-11-21)

A `far-data` symbol may list `members: [{offset: k, offset_witnesses: [...]}]`.
A candidate offset field with encoded addend `k > 0` (for example
`ES:[BX+1]` into a far table) is proven only when `O + k` has its own
independent offset witnesses, under the same rules as `O`: reached,
paired with a relocated segment word, outside every claim. A free addend
would re-fit `O`, as in the v4 member rule.

A far-call fixup may also use frame method F2 when it names the same external
as its T6 target (MASM `call far ptr X`). That frames the pointer by X's own
segment, exactly as MSC's F5 target frame does. An F2 naming any other
external is refused.

## Reached near calls as root frame evidence (2026-11-21)

A root claim with no reviewed relocated far caller can take CS frame F from
an original near CALL (`E8 rel16`, optionally after PUSH CS) outside the
whole object. The call's rel16 must land on the claim entry, computed in F's
64K window. The call must also be reached by recursive descent from an
entry that an original relocated CALL FAR names exactly as `F:offset`.
Descent and near calls never change CS, so the claim is entered with
CS = F. This evidence is consulted only when the claim has no relocated far
caller, so it cannot introduce a conflicting alias for claims that already
have one. Overlay claims use the overlay rules above.
