# Original-byte review: direct-DS root leaves and predicate

All observations below come from locked Oracle bytes. Discovery entries were
used only to locate possible entries; extents and callers are checked here
from original returns, instructions, and relocation records. No candidate was
compiled in this review. Public names and high-level meanings remain unknown.

## `root:0000:4052` and `root:0000:4072`: adjacent byte-state helpers

The first body starts after a distinct `RETF` at `0x4050` and alignment NOP
at `0x4051`. It occupies 32 bytes `[0x4052,0x4072)` and ends in `RETF` at
`0x4071`. The next body starts immediately at `0x4072`, has its own BP-frame
prologue, and occupies 32 bytes `[0x4072,0x4092)`, ending in `RETF` at
`0x4091`. Neither body calls another routine, switches segments, reads
arguments, or contains an Oracle relocation site. Both use a one-byte local
slot only to exchange two direct DS bytes.

```text
4052 55                push bp
4053 8bec              mov bp,sp
4055 83ec02            sub sp,2
4058 f61e305e          neg byte ptr [5e30]
405c a08c62            mov al,[628c]
405f 8846ff            mov [bp-1],al
4062 a08d62            mov al,[628d]
4065 a28c62            mov [628c],al
4068 8a46ff            mov al,[bp-1]
406b a28d62            mov [628d],al
406e 8be5; 4070 5d; 4071 cb                frame restore; RETF

4072 55                push bp
4073 8bec              mov bp,sp
4075 83ec02            sub sp,2
4078 f61e315e          neg byte ptr [5e31]
407c a08e62            mov al,[628e]
407f 8846ff            mov [bp-1],al
4082 a08f62            mov al,[628f]
4085 a28e62            mov [628e],al
4088 8a46ff            mov al,[bp-1]
408b a28f62            mov [628f],al
408e 8be5; 4090 5d; 4091 cb                frame restore; RETF
```

The exact stack shape is consistent with no-argument far procedures. A
bounded root caller at `0x4008` has its own BP prologue and `RETF` at `0x4050`.
It reads byte `[0x5c01]`; if zero, it executes `PUSH CS; CALL 0x4052` at
`0x4018..0x401b`. It conditionally executes `PUSH CS; CALL 0x4072` at
`0x4023..0x4026` when word `[0x5e8e]` is nonzero. After intervening calls it
repeats those two conditional calls at `0x403e..0x4041` and `0x4049..0x404c`.
These are same-segment calls with explicit pushed CS to match the leaf
`RETF`; there is no MZ fixup for their relative call displacement. Their
conditional instruction paths are visible in the caller body, though
reachability of the caller's own entry is not asserted here.

Independent field witnesses occur outside the two bodies. Root `0x3f82..0x3f88`
zeros direct byte fields `[0x5e30]` and `[0x5e31]`. Root code at `0x40d4`
compares and writes `[0x628c]` and `[0x628d]` while testing `[0x5e30]`; code
from `0x4117` similarly uses `[0x628e]`, `[0x628f]`, and `[0x5e31]`. A separate
copy helper at `0x40a4` sets `SI=0x628c`, then executes two `MOVSW` and one
`MOVSB`, reading the contiguous byte state through offset `0x6290`. These
instructions independently support the byte offsets and widths. Classification
can safely stay at “two no-argument far byte-state update helpers” until
source evidence establishes more specific state names or purpose.

## `root:0000:4092`: clear state and return a byte

This is a 17-byte body `[0x4092,0x40a3)`, immediately after the preceding
`RETF` at `0x4091`; a NOP at `0x40a3` precedes the next entry at `0x40a4`.
The full 18-byte original extent is therefore body plus one alignment NOP.
It contains no call, segment switch, stack argument, or relocation:

```text
4092 2ac0       sub al,al
4094 a28f62     mov [628f],al
4097 a28e62     mov [628e],al
409a a28c62     mov [628c],al
409d a28d62     mov [628d],al
40a0 b001       mov al,1
40a2 cb         retf
40a3 90         nop
```

Relocation-backed far callers encode `9a d2 13 cc 02` (`02cc:13d2`, which
maps to this root entry); overlay-3 has 27 such segment-word fixups (indices
388–414), with additional encoded calls in other overlays. A particularly
clear overlay-3 context at `0x32a8` compares byte `[0x628f]` to zero and
branches over the call when equal. Otherwise `0x32af` executes that far call,
then `0x32b4` stores returned AL back to `[0x628f]`. This supplies both a
conditional call path and an immediate use of the returned byte. The
containing function's full entry reachability remains a separate question.

Independent external witnesses support the cleared fields: the separate
functions at `0x4052`/`0x4072` directly load bytes `[0x628c]`, `[0x628d]`,
`[0x628e]`, and `[0x628f]`; the copy helper at `0x40a4` reads the contiguous
state with two word moves and a byte move; and code at `0x40d4` directly
compares and updates those bytes. The observed ABI is no-argument FAR with a
byte result in AL. This is consistent with the integrating worker's unsigned
byte-return C shape, while the symbol and purpose are not identified by these
bytes.

## `root:0000:424e`: one-word conditional-negation helper

The preceding function ends with `RETF` at `0x424d`. This body is 21 bytes
`[0x424e,0x4263)`, ending in `RETF 2`; `0x4263` is an alignment NOP before the
next BP-frame entry at `0x4264`.

```text
424e 55             push bp
424f 8bec           mov bp,sp
4251 8b5606         mov dx,[bp+6]
4254 803e015c00     cmp byte ptr [5c01],0
4259 7502           jne 425d
425b f7da           neg dx
425d 8bc2           mov ax,dx
425f 5d             pop bp
4260 ca0200         retf 2
4263 90             nop
```

`[bp+6]` is one word argument after the far return address; `RETF 2` cleans
it. The exact far caller starts at overlay-3 `0x3cdf`: `9a 8e 15 cc 02`. The
preceding `PUSH AX` at `0x3cde` pushes zero (AX was cleared at `0x3cdc`). Its
segment-word MZ relocation (index 329) is at `0x3ce2`; Oracle maps
`02cc:158e` to root offset `0x424e`.
The byte `[0x5c01]` is independently used as a byte by root code, for example
`NOT byte ptr [0x5c01]` at `0x32f0`, and by other root conditionals. A
small far, one-word, callee-clean helper is supported; whether its input is
formally signed and what its public name should be remain open.

## `root:0000:28d6`: direct-state Boolean predicate

The body begins after a preceding `RETF 2` at `0x28d3` and occupies
`[0x28d6,0x2925)`, 79 executable bytes. It returns at `0x2924`; NOP `0x2925`
aligns the next BP prologue at `0x2926`, giving an 80-byte extent if alignment
is included. It has no parameters, calls, segment changes, or internal MZ
fixups. It returns 0/1 in AX after checking three word guards and four
byte-pair cases:

```text
if ([5bfe] != 0 || [5bfc] != 0 || [5e00] != 0) return 0;
return ([4562] == 27 && [4549] == 6)  ||
       ([4562] == 22 && [4549] == 10) ||
       ([4562] == 1  && [4549] == 14) ||
       ([6ccd] == 9  && [4549] == 8);
```

The original directly corroborates the widths. Outside the candidate, root
`0x6062` loads word `[0x5bfe]`, `0x6065` ORs word `[0x5bfc]`, and `0x6076`/
`0x6079` write those same words. Root `0x60dd` writes word `[0x5e00]`. Root
`0xe71f` loads byte `[0x4562]` and `0xe725` stores a byte into `[0x6ccd]`;
root `0x361a` compares byte `[0x4549]`. These establish direct offset/width
matches without relying on a source hypothesis.

Incoming calls include four relocation-backed far calls to `0284:0096`,
which Oracle maps to this entry. The actual `9A` instruction starts are root
`0x0dff`, `0xa7c1`, `0xa7e4`, and `0xe937`; their MZ segment-word relocations
are at `0x0e02`, `0xa7c4`, `0xa7e7`, and `0xe93a`, respectively. There is
also a same-segment `PUSH CS; CALL rel16 -> 0x28d6` at root
`0x2ba6..0x2ba9`. After it, caller code compares AX with 1 and normalizes it
to a Boolean using `SBB AX,AX`, corroborating the zero/one return contract.
These call encodings do not prove that every containing path executes. Keep
the helper unnamed and its four byte-pair conditions literal unless
independent higher-level evidence supports a semantic name.
