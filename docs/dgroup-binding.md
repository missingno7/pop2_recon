# First external data binding proof

`tools/binding.py` accepts one deliberately small OMF mode: LOC1 offset16,
segment-relative, external T6, F5 target-derived frame, omitted displacement
and zero raw LEDATA addend. The source must own one complete initialized CODE
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
from this component match. The near-call CRT candidate in
[runtime-close-binding-candidate.json](../evidence/matching/runtime-close-binding-candidate.json)
remains diagnostic with zero ownership until its independent symbol and frame
proof is completed and the runtime gate gains that mode.
