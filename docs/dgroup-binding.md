# External data binding proofs

Examples with addresses recorded before 2026-11-24 refer to the earlier build (tag `oracle-dos-1565`); the rules are unchanged.

`tools/binding.py` accepts deliberately small resident OMF modes: LOC1 offset16,
segment-relative, external T6, F5 target-derived frame, omitted displacement
and zero raw LEDATA addend in `external-dgroup-offset16-v1`. The source must own one complete initialized CODE
segment, no secondary storage, and exactly one public at zero. Its declared
segments, group, publics, externals and ordered resolved fixups are frozen in
the binding evidence. LOCAT kind is retained independently of field width;
loader-resolved offset16 is not accepted as ordinary offset16.

Each address field is checked with `S - F + A + D`, for the independently
observed relative alias `S = F + offset`. In the accepted zero-addend mode this
equals `offset` for every frame F; no absolute symbol address is inferred.
Every remaining byte is compared directly. Raw object bytes are never edited,
masked or returned as a fabricated linked buffer. Reports retain the different
raw-code and original-code hashes, record each equation and count both its
fields and ordinary bytes. A matching literal cannot replace an external fixup.
The supported offset operation generates no MZ relocation, so any original
relocated word touching the contribution fails, including a word starting one
byte before its boundary. Other fixup modes remain rejected.

The binding file belongs to the immutable EXE hash and reviewed target. The
original MZ entry's exact straight-line prefix must reach a relocated
`MOV r16,paragraph; MOV DS,r16`. Independent original instructions outside the
candidate establish each direct DS operand's offset and access width. Those
witness bytes and decoded operands are checked afresh. Witnesses establish
relative offsets and widths, not runtime DS reaching definitions. The candidate must use
the corresponding direct unprefixed DS accesses and preserve segment context;
calls, interrupts and segment-register writes are refused in this initial mode.

For [apply_adjustment](../src/apply_adjustment.c), the original MZ entry is
`2203:0678`, at root position `0x226a8`. Its first 15 bytes reach the DS load at
`0x226b2`; the immediate segment word has original relocation index 1981 at
`0x226b3`. A byte store at `0x20af` independently accesses DS:6253. A word store
at `0x970a` and a separate read at `0x4596` access DS:6254. These references
ground address aliases and widths; the original symbol names, storage
declarations, group spelling and broader structures remain unknown. The data
can be in allocated uninitialized memory beyond the on-disk resident payload;
no data bytes are owned by this code proof. Shared DS lifetime and the association
of each original witness with the startup paragraph remain hypotheses. The
component byte proof holds for every DS value; it does not claim those absolute
data addresses or prove program behavior. Context annotations are review notes,
not gate inputs establishing runtime DS.

The helper's caller at `0x2233` supplies one word and calls alias `02cc:037a`,
which maps to root `0x303a`. Its segment word is relocated at `0x2236`. The
independently reviewed extent is exactly `[0x303a,0x3054)`, ending in RETF 2;
the next prologue starts at `0x3054`. Pinned MSC 6.00 with
`/c /AM /Oe /Gs /Zl /Gc` emits a complete 26-byte CODE segment. Twenty bytes
compare directly; three two-byte external operand fields satisfy their linker
equations. All three fixups are retained in emitted order, 20, 17, 8.
`volatile` declarations reproduce the observed accesses; original qualifiers
and names are source hypotheses.

[Synthetic historical link receipts](../evidence/toolchain/binding-link-probes.json)
use independent C/MASM sources, MS LINK 3.65 and RTLink Plus 6.10. Both emit
identical load images for external data offsets, a near relative call and a far
pointer. They retain raw LEDATA addends separately from explicit FIXUPP
displacements. A +2 MASM expression uses explicit D=2; an MSC array expression
can use raw A=3 with D omitted. The first acceptance mode allows neither
nonzero form. The synthetic links do not establish the original linker version.

The dedicated MSC 6.00 F5/T6 followup emits `A=0`, omitted D, and an external
word reference. Both linkers resolve it to 6, with no MZ relocations. The
independent data definition places the word after a filler; the map locates it
at linear 6. Its paragraph frame is 0, even though RTLink displays the group's
first initialized byte at 4. Frame subtraction uses the DS paragraph base,
not the first byte's layout origin. This distinction is preserved in
[the F5 receipt](../evidence/toolchain/binding-link-msc600.json).

Search and promotion accept `--binding evidence/bindings/NAME.json`. Promotion
requires that evidence cone; recipes pin its file hash and validation rechecks
the original witnesses, declarations, equations, complete extent, source, tool
and raw object identity. No original translation unit, original object-record
identity, natural RTLink placement, overlay binding or behavioral proof follows
from this component match. The earlier near-call CRT diagnostic in
[runtime-close-binding-candidate.json](../evidence/matching/runtime-close-binding-candidate.json)
has now led to the independently grounded
[whole close-member proof](runtime-near-binding.md). The supporting provider
prefix still owns zero bytes.

The [next data-helper milestone](data-helpers.md) reuses this gate without
changes for complete 10-byte and 56-byte C contributions. Their two and nine
external data fields are checked against separate original instructions outside
each owner. Shared semantic aliases do not grant data ownership or prove original
global declarations, runtime DS association, TU membership or natural placement.

## Independently witnessed word pair

`external-dgroup-word-pair-offset16-v2` additionally permits a four-byte
near-data alias with exactly two ordered word members at offsets 0 and 2.
Each member must have its own original direct DS word-access witness outside
the candidate. The complete four-byte span cannot overlap another alias.
Candidate accesses must be direct words at these members; raw LEDATA A is
only 0 or 2, explicit D remains omitted, and every field satisfies
`S - F + A = alias_offset + A`. Byte accesses, odd/outside addends, missing
members and candidate-supplied witnesses fail. Other v1 restrictions remain.
The legacy mode still rejects four-byte aliases and nonzero addends.

[is_special_state.c](../src/is_special_state.c) uses a natural unsigned-long
zero guard to reproduce MSC 6.00's BX allocation. Independent original reads
at root `0x6065` and `0x6062` establish DS offsets `0x5bfc` and `0x5bfe`,
respectively. The [canonical proof](../evidence/bindings/is_special_state.json)
checks all eleven fields and 58 ordinary bytes in the complete 80-byte segment,
including its alignment NOP. The +2 raw addend is retained unedited.

These witnesses establish the two address/width equations; grouping the words
into a long, its signedness, original storage/type, source names and purpose
remain hypotheses. No data ownership, runtime DS proof, overlay binding,
far-call binding or MZ relocation coverage follows. Nine additional refusal
and equation tests preserve the narrow member rules and legacy behavior.

## Resident immediate string bases

`external-dgroup-offset16-v3` is a standalone resident mode adding only
`MOV SI/DI,imm16` fields with LOC1 offset16, F5/T6, raw A=0 and omitted D:
`S - F + A = alias_offset` for every F. Complete-segment, empty-secondary-storage,
frozen-object, startup-DS, literal-byte and touching-MZ-word checks still apply.
F1/group frames, segment targets and other immediate instructions remain refused.
The v1, v2, overlay and composed far-call modes keep their existing rules.

A `near-data-address-alias` has an offset and **no width or members**. Its
independent original witness must be outside the complete owner: either an
unprefixed direct DS access at the exact offset, or `MOV SI/DI,imm16` with a
frozen `base_use` block that consumes it in the checked string idiom. The block
must contain the witness on an instruction boundary and have no touching MZ
word. Its `transfer_bytes` is rechecked from string widths and literal counts.
This grounds an address/transfer footprint, never aggregate allocation bounds,
original type, forward direction/DF, data ownership or runtime DS lifetime.
Neither the instruction nor its context may overlap the candidate.

Straight-line code must reach exactly one adjacent, unprefixed `push ds; pop es`
pair before any string. Every other segment write (including FS/GS and explicit
LDS/LES/LFS/LGS/LSS loads), system context transfer, call, interrupt and branch fails. Only MOVSB/MOVSW, F3 MOVSW and F3 STOSB encodings are supported;
other strings, overrides, operand/address-size prefixes, F2 and LOCK fail.
REP needs a reached positive literal CX count. Every string pointer must still
come from a symbolic immediate SI/DI base, and every field must feed a string.
Literal address replacements, intervening pointer writes and instructions after
return (except alignment NOPs) fail. General pointer dataflow remains unsupported.

Fresh [MSC 6.00 contrasts](../evidence/toolchain/immediate-dgroup-msc600.json)
with `/c /AM /Oe /Gs /Zl /Gc` and `/O` give identical objects: five-byte extern
struct assignment emits MOVSW/MOVSW/MOVSB; 20/64-byte assignment emits REP MOVSW;
`#pragma intrinsic(memset)` plus `memset(state, 0, sizeof(state))` emits REP STOSB.
Extern fields use F5/T6, A=0, D omitted. Same-file initialized/static definitions
instead use F1/T4 _DATA/_BSS and 40 bytes of secondary storage; tentative
external-linkage definitions emit COMDEF. All these storage forms remain refused.

[save_twenty_byte_state.c](../src/save_twenty_byte_state.c) proves all 18 bytes at
`root:02cc:1c3e` (two equations, 14 literal bytes); separate inverse helper
`0x4910` grounds both bases and a 20-byte transfer.
[clear_five_byte_state.c](../src/clear_five_byte_state.c) proves all 16 bytes at
`root:02cc:1404`, including the NOP (one equation, 14 literal bytes); separate
copy `0x40a4` grounds its base and five-byte transfer. Boundaries, incoming
callers and absent MZ obligations were independently rechecked. Seven of the ten
reviewed helpers fit this shape; parameter/zero pointers and DX-to-DI propagation
remain outside v3. Further ownership still requires independent witnesses and
complete fresh compiler equality.

## Overlay byte and word fields

`external-overlay-dgroup-offset16-v1` applies the same zero-addend F5/T6 LOC1
equations to a reviewed overlay contribution. Its only alias widths are one
and two bytes, with independent direct DS witnesses in the resident original.
The unchanged startup DS witness remains in root. Owner positions use their
actual overlay space; overlap checks compare both space identity and offsets.
A root witness at the same numeric offset is not inside the overlay owner.
The resident v1/v2 modes still refuse overlay targets; this overlay mode refuses
resident targets, word pairs, nonzero addends and overlay-supplied witnesses.

[overlay_state_pair.c](../src/overlay_state_pair.c) freshly reproduces the full
26 bytes at `overlay-4:29cf:0a12`, including its alignment NOP. Original root
read `0x1ecd` establishes byte DS:5c0e; root comparison `0x381e` establishes
word DS:5c19. The two operand fields satisfy the equations and all 22 other
bytes match literally. Eight additional space/refusal tests pass.

Calls, interrupts, segment writes, indexed absolute globals and every original
MZ word touching the owner remain rejected. The equations are DS-relative and
hold for every DS value. They establish no runtime DS lifetime, shared original
symbol identity, data ownership, overlay load/placement, natural link or behavior.
Far pointers and relocation-generating code remain unsupported.


## Composition with root far calls

The explicit [external-far-call-v1](far-call-binding.md) mode reuses these
resident byte/word and reviewed word-pair equations. Its `data` object chooses
the original data mode and startup frame; the combined `symbols` table keeps
each independent witness. Proven CALL FAR operands may coexist with data
fields, and only their segment words create original MZ obligations. The
standalone data modes, including overlay v1, keep their existing refusals.


## v4: byte-level offset fields (2026-11-21, revised after review)

`external-dgroup-offset16-v4` proves the bytes of DGROUP offset fields in any
instruction shape:

- **Field.** The FIXUPP (LOC1, F5/T6, no displacement) must be the address field
  of exactly one decoded instruction: a memory disp16, or the imm16 of `MOV` or
  `PUSH`. Branch displacements, `RET n` and arithmetic immediates never count.
  MASM's `ASSUME DS:DGROUP` emits F1 instead: a frame naming the object's GRPDEF
  `DGROUP` gives the same value `S - DGROUP + A`, so it is accepted; an F1 frame
  naming any other group is refused.
- **Value.** The linked value is `S + A`. `A` must be 0, or a member offset `k`
  whose own address `S + k` is independently witnessed (`members`). A free
  addend would let a binding re-fit `S` to any witnessed value and leave the
  source's struct offsets unproven, as an Opus review demonstrated.
- **Grounding.** Each `S` (and each `S + k`) is grounded by original instructions
  outside the candidate whose address field equals it.
  - A witness with an `entry` must be reached by recursive descent from a
    relocated CALL FAR destination or a reviewed direct-code entry. The descent
    follows branches, loops and direct near calls.
  - Entries in overlays count too, within that overlay's own space: an RTLink
    vector destination, or a relocated CALL FAR into the overlay.
  - A witness without an entry must be a v1-shape direct, unprefixed DS memory
    operand.
- **Finding witnesses.** `python tools/dgroup_witness.py OFFSET --exclude START:END`
  lists the reachable ones.
- **Scope.** v4 proves code bytes only. Data extents, types and the runtime DS
  association are not claimed.
- **Owners.** Root or overlay code (2026-11-21); the frame witness stays the root
  startup DS load. See [overlay owners](far-call-binding.md#overlay-owners-in-groups).

## v4 internal consensus and draft-region witnesses in groups (2026-11-21)

A field holds `S + k`, and `S` is a link-time unknown. One claimed site
fits `S` to the claimed bytes. A second equal original field at a different
instruction overdetermines `S`. This proves address equality only, not symbol
identity, type or storage class. Same-source groups (v4 only) therefore accept
these support sites for each linked value `S + k`:

- **outside witness**: as before, an original instruction outside the whole
  object;
- **draft-region witness**: an original instruction inside the object but
  outside every claimed extent, with the same witness shape (reached from an
  entry, or a v1 direct DS operand). Only the original bytes count, never the
  draft's compiled FIXUPP. When the group later claims that region, the
  witness is refused, so the binding must switch to internal consensus. A
  claim never silently loses its support;
- **internal consensus**: a symbol or member with `"witnesses": []`. Every
  claimed field with that symbol and `k` is proven only if equal fields of the
  same symbol and `k` occur at two or more distinct original instruction
  starts, all inside CODE_EXACT claims. The check runs to a fixpoint, so a
  claim that loses exactness withdraws its sites. Different `k` never support
  each other.

Every v4 linked value `S + k` must differ from every other symbol's `T + j`.
A v4 address may not fall inside the object's own placed segment. Each
equation records `support` as `witnessed` or `internal consensus`, and
validation recomputes it. Single-function gates are unchanged.

Root measurements: 306 BSS addresses at 0x2930 and above are reached as
direct DS operands. 142 of them are used by one code frame only. Of those,
65 have two or more sites, and 77 single-site addresses stay unprovable.

## Indirect calls through memory (2026-11-21)

The disp16 of an indirect CALL or JMP through memory (`FF /2`..`/5`, for example
`call far [1fe8]`) addresses the stored pointer. v4 treats it as a data
address field like any other memory operand, on both the candidate and the
witness side. Direct branch targets, returns, interrupts and
register-indirect calls have no such field.

## Provider data publics (2026-11-22)

A group often addresses data that another file defines. MOVETEMP.C, for example, stores
into MOVE.C's mob height/width table. A v4 symbol of kind `dgroup-public` grounds such an
EXTDEF through the accepted group that defines it:

- **Value.** The linker resolves the EXTDEF by name to the provider's PUBDEF. Its frame is
  DGROUP (F5 through the public's group, or F1 DGROUP), so the field is `P + o + A`. `P` is
  the provider's placement of the segment and `o` the public's offset in its freshly
  validated object. `A` is the consumer's encoded addend. The binding states `P + o`, and
  the gate checks it.
- **Provider.** The provider must be an accepted same-source group whose recipe lists the
  name in `data_publics` with `unique: true`. That means the segment is initialized and its
  content occurs exactly once in the original DGROUP image, the existing rule that fixes
  `P`. The gate rechecks that window in the original image: it occurs once and holds no
  relocated word. It also checks the provider binding's pinned hash. A placement fixed only
  by repeated content, or an uninitialized (BSS) one, never provides.
- **Name identity.** The name is defined exactly once among accepted components: code
  publics, drafts, data publics and runtime member publics. The consumer must not define
  it itself. Validation refuses any accepted data public whose name is defined twice.
- **Addend.** `A` (unsigned, so a negative addend fails) must lie inside the public's span.
  For a direct memory operand, the whole access `A + width` must fit. The span runs to the
  next data public or the segment end, which bounds the declared object from above. A
  static between two publics stays inside the earlier span.
- **One definition per byte.** The provider's placed segment may not overlap the
  consumer's own placed segments, or any other accepted group's placed segment. Under the
  existing v4 aliasing rule, every linked value `P + o + A` the consumer writes must differ
  from every other v4 symbol value. A v4 address that lies inside a provider span without
  equalling a used value is an alias: an address without extent, not a definition. It is
  recorded as `provider_span_aliases` debt in the recipe and revalidated. Expressing it as a
  member of the provider public retires the debt.

Fields grounded this way report support `accepted provider public`. The proof stays
byte-level CODE_EXACT. It does not prove the original declaration, type, file boundary
or that the provider is the original defining file. It proves only that the reconstructed
sources, linked together, would write the original bytes. The typed-member-stride idea
(element size from an original indexed access) is not adopted: one indexed access does not
fix an array's bounds or the layout around it.

Research runs: the provider's `promote_group.py --verify-only --stage DIR` writes
`DIR/provider.preview.json` when the object has data publics. A consumer's verify-only
run may cite it with `--extra-provider DIR/provider.preview.json`, before the provider is
published. `submission.py --verify-only` passes the preview of each verified group to the
groups listed after it. Validation never uses previews.

## DGROUP class order (RTLink 6.10 probe, 2026-11-22)

Linker observations (synthetic MSC 6.00 objects linked with the pinned RTLink 6.10; receipt in
evidence/rtlink-probe/dgroup-class-order/receipt.json):
- Segments of one name combine into one block, each object's piece contiguous, in link order;
  blocks are placed by class in first-seen order. With DOSSEG: NULL (BEGDATA) | all `_DATA` |
  all CONST | MSG | `_BSS` | STACK. Reversed link order reverses pieces inside each block. /Zi does
  not change it. Overlay modules' DGROUP pieces go into the preloaded data section after the
  root modules' pieces in the same class block. Classes never interleave per object.
- MSC 6.00 / 6.00A put string literals, initialized arrays, `const` arrays and static const data
  in `_DATA` (with or without /Zi, /Zd, /Od, /Ox, /Gt4); CONST receives only floating-point
  constants and explicit `_based(_segname("_CONST"))` data.

Consequences: a file's initialized data is one contiguous `_DATA` piece at its link-order
position; two files cannot interleave. Existing canonical placements that use
`_based(_segname("_CONST"))` (custom, dialog, draw, load, movecave) prove their code bytes but
are not a linkable model of the data layout (they sit among `_DATA` pieces); they are layout
debt, not precedent. They are listed in evidence/const-layout-debt.json, which validation keeps equal
to the accepted C sources that use `_segname`; `promote.py` and `promote_group.py` refuse `_segname`
in any other C source, so the list only shrinks. Data another file uses is that file's data public (see "Cross-file data
publics"), not a second placement.

## Near pointers inside placed initialized data (2026-11-22)

A file's initialized `_DATA` may hold tables of near pointers to its own data (MSC emits an
offset16 FIXUPP in `_DATA`, frame DGROUP, target `_DATA` or another placed segment of the
object, offset stored in the LEDATA bytes). Such fixups are judged by the placement, not by
any claim: the gate writes the value the linker writes, P(target) + stored offset + D mod 64K,
into the segment's bytes and compares the whole linked segment with the original DGROUP bytes
at its placement (and uses the linked bytes for the uniqueness test). Every byte is compared;
no field is skipped. Any other fixup in a data segment (far pointers with a segment word,
pointers to externs or to code, a non-DGROUP frame, a segment without a placement) is
refused. The placement report records `pointer_fields` for such segments.

Since 2026-11-23 a near pointer may also target another file's data, i.e. an external, when the binding grounds
that external independently: a v4 symbol with its own original witnesses, or a provider data public. The linked
value is then the grounded offset + stored offset + D. Such a symbol counts as used by the group. A symbol marked
`unproven`, or one the binding does not ground, is refused: pointer bytes that happen to equal the original never
prove the symbol. DRAW's level-spec table at DS:04e0..04ea needs this.

## Alignment holes in placed initialized data (2026-11-22)

MSC word-aligns items inside `_DATA` by skipping a byte without emitting LEDATA for it; the linker
writes zero there. A placed initialized segment may therefore have single-byte holes between
initialized ranges (it must still start at 0 and reach its end). The holes stay in the compared
content as zero bytes, so every byte of the segment is compared with the original DGROUP image;
wider gaps remain refused.

## Far code pointers inside placed initialized data (2026-11-22)

Callback and dispatch tables in a file's `_DATA` hold far code pointers: MSC emits a LOC3 (16:16)
FIXUPP framed by its external target, with a zero encoded addend. The placement writes the value
the linker writes, the target symbol's offset and segment, into the segment's bytes and compares
the whole linked segment with the original. The target must be a far code symbol of the group's
binding, grounded independently exactly as for a far CALL (accepted exact entry or outside
relocated CALL FAR witnesses); its segment word must carry an original MZ relocation, and the
segment must contain no other original relocation. Nonzero addends and pointers to this object's own code remain refused.

MSC far data adds a second form: code loads ES from a CONST word holding the segment of an external
far array (`mov es, [seg word]`), emitted as a LOC2 base16 FIXUPP framed by its target. The placement
writes the far segment symbol's paragraph (a `far-data` or `segment-base` binding symbol, grounded
independently as usual) and the original must relocate that word. These are the only relocations a
placed segment may contain.

Since 2026-11-23 a table may also point into the same object's own CODE (a level table of the file's own functions).
The frame is the claims' independently witnessed CS frame F, so the linker writes the contribution's offset in F +
the stored offset + D, then F. The segment word must carry an original MZ relocation, and the entry must lie inside
the emitted CODE. Without a witnessed frame the field is refused.

## Data-only groups (2026-11-22)

Some original source files define initialized DGROUP data and no code (FM Towns ENGLISH.C,
TABLES.C; see build/workers/o4_dgroup for the DGROUP ownership map). `tools/promote_data.py SOURCE
--name NAME --profile P --flags ... --binding B [--verify-only]` compiles such a file and judges it
with `module_gate.check_data_group`: the CODE segment must be empty, every segment with content is a
placed DGROUP segment judged exactly as in a group (content equal to the original bytes; linked
near/far pointer and segment-word fields; single-byte alignment holes), and every placement must be
fixed by content that occurs once in DGROUP. The binding holds `placements`, `dgroup_segment`,
the frozen `fixups`/`declarations` and only far symbols. Its data publics ground other groups'
externs (`dgroup-public` provider symbols, as for a group). The manifest lists the recipe under
`data_groups`; validation recompiles it and includes its placements in the overlap check, so a
correctly attributed data module can never coexist with a misattributed placement of the same bytes.

A data public of a pinned library member can be the provider too (`"provider": "runtime"` with an `archive` object:
library, member identity, segment, placement). Its segment must be fixup-free, fully initialized, at least 16 bytes
long and occur once in the original DGROUP image (`module_gate._archive_public`; MSC `__ctype` at DS:24ba). An addend
equal to a provider public's span is accepted only as a compared immediate, the one-past-the-end bound of a loop.

## Aggregate extents (2026-11-22)

A v4 member address S+k usually needs its own witness. Many single-use addresses are members
of records the program copies whole (MOVE's 13-byte mob records, the 64-byte character records):
`lea si,[bx+S]` (or `lea r,[S]` / `mov r,S`), `mov cx,n`, `rep movsw` (+ one `movsb`) or
`rep movsb`. Such an original copy proves that [S, S+L) is one copied unit, L = 2n(+1) or n.
A v4 symbol may therefore carry `"extent": {"length": L, "witness": {space, image_offset,
bytes_hex, entry}}`: `binding.block_copy_extent` re-decodes the witness window from the original
(reached from its entry, outside the candidate; straight-line; SI or DI loaded with exactly S; no
other write to that register or CX before the copy; a DI destination needs ES loaded from DS in
the window) and must yield exactly L. The base S must have its own independent witnesses. A member
k with 0 < k < L then needs no further witness, and every access to it must stay inside the unit
(k + width <= L); equations report support "aggregate extent". This proves the address lies in the
same aggregate as S; it says nothing about the member's name or type.

Extended 2026-11-23 (`binding.block_copy_extent`):
- The base may reach SI or DI through a register move (`mov ax,base; ...; mov di,ax`).
- The copied length is the whole uninterrupted run of string moves, so MSC's unrolled `movsw`/`movsb` struct copies
  count as well as `rep`. Any other instruction ends the run.
- DS and ES are tracked. A SI source needs DS unchanged since the window began, and a DI destination needs ES loaded
  from that DS inside the window. Before this, a DS reload inside the window was not refused; no accepted extent
  relied on that.
- In a group the copy may itself lie inside a claim. The length is an original fact about [S, S+L) and is not one of
  the fields the claim is judged on. S still needs its own witness outside the claims.

When every original S+0 reference lies in claims, S has no witness. In a group the copy (or pointer access) may then
itself lie in a claim, and S stands on internal consensus: no in-extent member is accepted unless S+0 has equal fields
at two or more distinct claimed instruction starts. First use: `MOBS` (DS:2aa4, 13-byte records copied by ANIM_MOBS).

A copy may also be made by a callee through a near-pointer argument. The extent then has a `call`: a reached original
window that ends in a relocated far CALL of a resident routine and passes exactly S, while `witness` starts at that
routine's entry (`push bp; mov bp,sp`) and copies through `mov r,[bp+disp]` with the matching stack slot. First use:
INIT_PORT copies 44 bytes into `ScreenPort` (DS:5e62). A base with a proven extent is a used address, so it needs no
other immediate evidence.

An extent may also come from a bounds-checked byte index (`"kind": "index"`): BH cleared, `cmp bl,N; jae`, then
`add bx,S` and a byte access at [bx]. The program addresses exactly S..S+N-1 as one array, which grounds S as well.
First use: the keyboard interrupt handler's `KEY_DOWN` (DS:1e7b, 59h bytes).

A far pointer variable has extent 4 (`"kind": "pointer"`) when one reached original instruction loads or calls through
it as a whole (`les`/`lds r,[S]`, `call`/`jmp far [S]`); its segment half S+2 is then a member. An index witness grounds
S as well, so it must lie outside the claims; after a pointer access inside a claim, S still needs an outside witness or
two equal claimed sites (internal consensus).

Added 2026-11-24 (each re-decoded from reached, straight-line original code; same rules for the base as a copy):
- `"kind": "record"` (`binding.record_stride`): an access [r+S] whose index is d times one unknown, the scaling visible in
  the window. The program steps through records of d bytes at S, so [S, S+d] is inside one array of two or more records;
  the extent is d. With a second window `next` that steps by the same d at S+d (element v+1), records 0 and 1 are both
  used and the extent is 2d. Unrelated unknowns, a second index register, LEA or ES refuse the window. First use: the
  5-byte `BELOW_TILES`/`ABOVE_TILES` records. The stride also shows that S is a used address, so a plain immediate S in
  other code may then be its witness (`ABOVE_TILES` passed by GETABOVEARY).
- `"kind": "fill"` (`binding.fill_extent`): REP STOS of a constant count with DI = S and ES loaded from DS. First use:
  `REDRAW_FORE_CELLS` (300 bytes cleared at DS:6248).
- `"kind": "far-read"` (`binding.far_read_extent`): a reached original call passes DS:S (`push ds; push S`) to a resident
  routine that loads it with `lds si,[bp+a]` and reads L bytes with LODS in straight-line code. S needs its own witness.
  First use: `SCREEN_BOUNDS` read by InsetRect.

- `"kind": "base"` (`binding.base_extent`): `mov r,S`, r unchanged on the fall-through path, and one DS access [r+k] of
  width w: extent k+w. First use: `__nheap_desc` (`cmp [si+8],bx` in `__nfree`), whose +6/+8 words ALLOCATE_EMPTY_HANDLE
  reads and writes directly.

For far data (docs/far-call-binding.md) a member O+k may carry `"record": {"base", "member"}`: one window steps through
d-byte records at O, the other at O+k with |k| < d. Two such arrays would overlap, so both are the same array. Each window
proves its own ES (a selector word, or a relocated `mov r16,N; mov es,r16`). First use: `MIDDLE_QUEUE[count - 1]` fields,
which the compiler folds to O-1 and O-2.

A provider data public's segment may hold relocated words when they are exactly the provider's own far-pointer and
segment-word fixups (LOAD's level procedure table at DS:06d4). Any other relocated word is refused.

## BSS layout (bss-layout-v1, 2026-11-22; refutation only)

Original anchors (C startup, root image): `_edata` = 27c0 (`mov di` at 0x1ffe7), `_end` = 6b74 (`mov cx` at
0x1ffea), xontab = 2b88 (`mov si`/`mov di` at 0x20147/0x2014a; the onexit table is empty). RTLink 6.10 with MSC
6.00 objects (probes in build/workers/o7_bss): each object's statics form its own `_BSS` piece in declaration
order, pieces in link order inside the per-file block [27c0, 2b88); non-static uninitialized globals are near
COMDEF communals in `c_common` [2b90, 6b74): word-aligned multi-byte communals first, then one-byte communals in a
tail from 6b6c (after the runtime's 512-byte `__bufin` at 696c). Consequently validation flags, per C group:
a placed `_BSS` piece outside [27c0, 2b88) or at an odd start; a v4 symbol starting inside [27c0, 2b88) (an address
there is some file's own static, never another C file's extern); a v4 symbol starting at an odd address in
[2b90, 6b6c) (a member of a larger variable, not a variable). Existing cases are listed in
evidence/bss-layout-debt.json, which validation keeps equal to the violations, so the list only shrinks: republish
the group with the real static, communal or member-of-base model. The rule proves nothing; single-use BSS fields
stay unproven (o7_bss: genuinely underdetermined without an original map), see CODE_MATCH in docs/acceptance.md.

## Communal definitions (near COMDEF, 2026-11-23)

MSC 6 emits a near COMDEF record (data type 0x62) for an uninitialized global that a file defines without
`extern`. The linker allocates communals in `c_common`, so the object contributes no bytes. The strict parser
accepts near COMDEFs in compiler output. They share the external index space, so every reference to a communal is
an external fixup that the group binding must ground like any extern: a witnessed v4 symbol, members or provider.
The communal names and lengths are pinned in the binding's `declarations` (`communals`). Far COMDEFs stay
library-only. The BSS layout rule applies to their grounded addresses as to externs.

## Immediate witnesses (2026-11-23; refutation only)

A MOV/PUSH imm16 witness proves an address only when the original code uses that value as one. Within straight-line
code and direct jumps, `binding.immediate_is_address` requires one of these:
- a register holding the value is dereferenced, also after register moves or index arithmetic;
- it drives a string operation;
- it is pushed directly after `push ds`.

Otherwise the immediate may be a plain number that equals the address by chance. Such witnesses had been accepted
(DS:0000 from `mov word [00b0],0`, DS:005a from `mov ax,90`). A symbol or member whose every witness is ambiguous is
listed in evidence/immediate-witness-debt.json (shrink-only, equal to `module_gate.immediate_witness_violations`).
Retire an entry with a memory-operand witness, a provider data public, internal consensus or an aggregate extent.

## Far-data selector words in CONST (2026-11-23)

MSC loads the segment of an external far object from a private CONST word (`mov es,[D]`, LOC2 base16 to the object).
Such words repeat (2fae, 2e5f), so content alone places nothing. `module_gate._exclusive_selectors` places a CONST
piece made only of selector words when both hold:
- every word of the piece is referenced only from inside the object's own code (raw byte search over every space)
  and is loaded there;
- every other CONST position with the same relocated content has a word loaded by reached code outside the object, so
  that position is another file's.

CONST starts at DS:26c6, the end of the C termination table that the exit code walks (root+20150/20153). The report
basis says `exclusive selector words`.

A witness displacement with an ES/CS override or a BP base (without a DS override) addresses something else and is
refused (`binding._not_dgroup`, 2026-11-23). A far-data member may carry `unproven` (a reason, no witness): every claim
that uses it is then a CODE_MATCH, as for v4 members.
