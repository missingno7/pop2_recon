# Runtime members with symbolic fixups (runtime-symbolic-v1)

## Scope — 2026-11-21

`tools/runtime_fixups.py` lets a whole pinned library member own its CODE even when it
carries fixups and secondary DGROUP storage. It is the medium-model counterpart of the
large-model design used for another reconstruction: every FIXUPP field is computed by
the linker equation from independently grounded values. Nothing is masked, no field
value is read back from the original operand, and no matching prefix is selected.
Recipes still pin the archive and the whole OMF member (`pin_runtime.py`), and the
binding is pinned by hash under `evidence/bindings/` (LF only).

The PoP2 runtime is the medium-model MSC 6.00 C library (`mlibcr.lib`, SHA-256
`03a85ba8..`; `libh.lib`, `a4492e8f..`) linked as one `_TEXT` run in frame `1fe6`
(root `0x1fed4`-`0x21feb`), monotone in archive order. Its data lives in DGROUP,
paragraph `30f3`, inside the preloaded overlay 17.

## Grounded values

- **CODE frame `F`.** Original relocated `CALL/JMP FAR` encodings outside the member
  whose segment word is `F` and whose offset lands exactly on a public of another
  PINNED_RUNTIME member (unique across the manifest). Every member's code must lie in
  `[F*16, F*16+64K)`; its frame offset is `m = position - F*16`.
- **Code providers.** A code external is bound to a named PINNED_RUNTIME recipe in the
  manifest; the public must be declared by exactly one pinned recipe (an ambiguous
  name refuses). Its position is that recipe's proven position plus the PUBDEF offset.
  The member's own publics need no provider. A provider that is not yet pinned refuses
  the member, so publication goes leaf-first.
- **DGROUP paragraph `G`.** Original relocated words storing `G`, as for placed group
  data; the one space holding the paragraph (overlay 17) supplies DGROUP bytes.
- **Placed DGROUP segments.** Every non-empty secondary segment of the member must be
  placed at a DGROUP offset `P` (`placements`). A data external comes from a
  `data_providers` entry: an identity-pinned library member (code or data-only) and
  one of its segments with `P`; the symbol must be a DGROUP-grouped public of that
  segment. A placed segment must be completely initialized, public-combine and not
  STACK/CODE. Its bytes compare literally with the original DGROUP bytes at `P`
  except its own fixup fields, which are proven by the same equations (own segment
  offsets, DGROUP base). The original overlay relocations inside it must be exactly
  its generated base fields. An uninitialized own segment (`_BSS`) needs two distinct
  proven code addends instead. Provider segments own zero bytes.
- **Absolute publics.** `absolute_providers` name an identity-pinned member whose
  PUBDEF defines the symbol with frame 0 (e.g. `dos\diffhlp.asm` `__AHINCR = 1000h`).

## Equations (A = LEDATA addend, D = FIXUPP displacement)

| Field | Value |
| --- | --- |
| LOC1 self-relative, own `_TEXT` (E8/E9/Jcc operand) | `m + D + A - (m + at + 2)`, target inside the member |
| LOC1 self-relative, code external (A = D = 0) | `S - (m + at + 2)`, `S` the provider's frame offset |
| LOC1 offset, CODE frame (F0/F4/F5/F2) | own: `m + D + A`; external: `S + D + A` |
| LOC1 offset, DGROUP (F1, or F5/F2 to a DGROUP public) | own placed segment `P + D + A`; provider public `P_s + off + D + A` |
| LOC1 offset, absolute external | `value + D + A` |
| LOC2 base, DGROUP group | `G`, plus an original relocation at the field |
| LOC3 far pointer, code external (A = D = 0) | `S : F`, plus an original relocation at field+2 |

The original relocations inside the member extent must be exactly the generated
segment words, in FIXUPP order per target. Back-patched (overwritten) LEDATA is
accepted only where no FIXUPP field lies in an overwritten range. Validation also
checks that one contribution has one placement across all runtime proofs and that
distinct placed contributions do not overlap (`check_layout`). Any other location,
frame or target, a communal, a program start address, an unbound or unused provider
binding refuses the member.

## Pinned with this mode (18 members, 2,529 bytes)

`dosret` (85, own `_DATA` + crt0dat `_DATA` provider), `stdargv` (402), `growseg`
(253), `open.asm` (444, also `txtmode` `__fmode`), `close` (32), `lseek` (122),
`unlink` (13), `d_creat` (42), `flength.c` (134, three far calls), `splitpat.c`
(340, five far calls), `itoa` (27), `alshl` (29), `setseg` (73), `scanseg` (74),
`_fmemcmp` (91), `_fmemcpy` (94), `_fmemset` (72) and `_fmemmove` (202, `__AHINCR`
and a far call to `__aFahdiff`). The crt0dat `_DATA` provider places 82 bytes at
DGROUP `2274` with its DGROUP base word proven against overlay-17 relocation 84.

## Refused, with the exact blocker

- `crt0` (program start address, STACK, common `XIQC`, `_end`/`_edata`, `_main`),
  `crt0dat` (second CODE segment `EMULATOR_TEXT`; `__cintDIV`, `__fptrap`,
  `__nullcheck` providers missing).
- `__bufin` near communal in `_file.c`'s `_DATA` (only zero-addend references; not
  placeable): blocks `__iob`/`__iob2`/`__lastiob` and therefore `stream.c`,
  `_open.c`, `fopen.c`, `fflush.c`, `fclose.c`, `_flsbuf`, `_getbuf`, `_sftbuf`,
  `printf.c`.
- `_end` (linker-defined) in `chkstk`'s `_DATA`: blocks `chkstk`, `stackava`
  (`STKHQQ`) and through `__chkstk` `write`, `output` (also `__cfltcvt_tab` holds a
  far `__fptrap` pointer), hence `fprintf`, `sprintf`.
- crt0's common `DBDATA` (code offsets, `C_ETEXT`): blocks `nmsghdr`, `d_rdwr`,
  `write`; crt0's `_DATA` holds `dw __exit` (crt0dat code unpinned): blocks
  `__nheap_desc`, so `nheapchk`, `nmemmax`.
- `crt0msg`, `chksum`: MSG/PAD/`NULL` storage (common PAD, no references) and
  `__NMSG_WRITE`/`__FF_MSGBANNER` providers missing.
- `stdalloc`, `stdenvp`, `calloc`, `_freebuf`, `strdup`: `_malloc`/`_free` are 5-byte
  `malloc.asm`/`free.asm` far-jump stubs to `__nmalloc`/`__nfree` at `1fe6:003b`/`001a`
  (no pinned provider); `calloc`'s own call encodes `1fe6:003b`, not the stub.
  `strdup` also names `_strcpy`, declared by two pinned recipes (the large-model
  `crt_strcpy` pin at `2000:1d4c` and `crt_strcpy_medium`). `stdalloc` and `chkstk`
  also need crt0's `__amsg_exit`.

Original TU, natural RTLink placement, library order and behaviour remain unproved;
the placed DGROUP bytes are evidence, not owned data.
