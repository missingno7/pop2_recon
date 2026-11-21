# External data binding proofs

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
