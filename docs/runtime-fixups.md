# Runtime members with symbolic fixups (runtime-symbolic-v1)

## Scope â€” 2026-11-21

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
- **Signature-anchored code providers** (`code_anchors`, 2026-11-22). A code external
  may instead name an anchor: an identity-pinned library member and a frame offset `o`.
  Its complete initialized CODE segment must lie in the grounded frame window and have
  at least 16 ordinary (non-field) bytes. Every ordinary byte must equal the original
  at `F*16+o`, and that must be the only position in `[F*16, F*16+64K)` where they all
  match. Its own-segment fields (rel16 inside the segment, own CS offsets, rel16/LOC3 to
  its own publics) must satisfy their equations there. Its external and DGROUP fields
  remain *its own* open obligations and are only counted. The anchor must not overlap
  the judged member. It may overlap an owner only when that owner is the
  PINNED_RUNTIME recipe of the same member hash, segment and position, so a later pin of
  the anchored member keeps earlier proofs valid. A named anchor must declare the
  public, agree with any pinned declaration of it, and be used. An anchor owns zero
  bytes. This generalises the provider-signature precedent of
  [runtime near binding](runtime-near-binding.md).
- **Class runs** (`class_runs`, mode `dgroup-class-run-v1`). This is an LF evidence file
  under `evidence/bindings/`, pinned by hash. It lists, in order, every contribution of
  one DGROUP segment class: segment name plus one identity-pinned member, or (for a
  common-combine segment) every contributor. Each contribution must be complete,
  initialized, fixup-free and in DGROUP, and each entry's contributors must agree on
  combine and alignment. Placements follow `P(i+1) = align(P(i) + len(i))` from the
  listed start, and alignment gaps must be zero. Every contributor's bytes must equal
  the original DGROUP bytes at its entry's placement. The run must end exactly at the
  original DGROUP payload end (`home.size - base`), so no unknown contribution can sit
  inside it. A common-combine segment (combine 6) is placeable only through a class run,
  and any listed segment must use the run's placement. `check_layout` lets common
  contributions of one segment name share one start; any other overlap still refuses.
  An unused run refuses.
- **Same-library selection** (`same_library_selection`, 2026-11-22). The binding may
  opt in, symbol by symbol, to binding a public that several PINNED_RUNTIME recipes
  declare. It binds only when exactly one declaring recipe comes from the judged
  member's own archive (`library_sha256`), and that recipe is the one named. The
  provider equations then still have to hold. A selected symbol with a single
  declaration, a named recipe from another archive, two same-archive declarations or
  an entry missing from `code_providers` refuses.
- **Accepted external code providers** (`external_providers`, 2026-11-22). A code
  external may name an accepted exact resident owner (`MATCHING_C`/`CODE_EXACT` or
  `MATCHING_ASM`/`ASM_EXACT`) by `target_id`:
  - Its claimed public must equal the external name. The only other accepted form is
    MASM's default uppercasing, for an `asm` recipe; it is recorded as `case_folded`.
  - Its frame is the segment word of one reviewed `far_call` or `far_pointer` call
    site of that target row. `match.get_target` rechecks it freshly: the relocated
    segment word, landing exactly on the entry, outside the judged member.
  - LOC3 fields use `S : F_p` with the provider's own frame `F_p`. Near forms (rel16,
    CS offsets) are refused unless `F_p` is the runtime frame.
  - The provider must not overlap the judged member, and it must not shadow a pinned
    or already bound provider. It owns nothing here, and must be used.

## Equations (A = LEDATA addend, D = FIXUPP displacement)

| Field | Value |
| --- | --- |
| LOC1 self-relative, own `_TEXT` (E8/E9/Jcc operand) | `m + D + A - (m + at + 2)`, target inside the member |
| LOC1 self-relative, code external (A = D = 0) | `S - (m + at + 2)`, `S` the provider's frame offset |
| LOC1 offset, CODE frame (F0/F4/F5/F2) | own: `m + D + A`; external: `S + D + A` |
| LOC1 offset, DGROUP (F1, or F5/F2 to a DGROUP public) | own placed segment `P + D + A`; provider public `P_s + off + D + A` |
| LOC1 offset, absolute external | `value + D + A` |
| LOC2 base, DGROUP group | `G`, plus an original relocation at the field |
| LOC3 far pointer, code external (A = D = 0) | `S : F`, plus an original relocation at field+2 (an external provider uses its own witnessed `F_p`) |
| LOC1 offset in placed data, code external (CODE-segment frame, F5, F2 = target) | `S + D + A`, no relocation |
| LOC3 far pointer in placed data, code external (A = D = 0) | `S : F`, plus an original relocation at field+2 in the DGROUP home space |

`S` is a pinned provider's or an anchor's public in the grounded frame; a code address
stored in data is never read from the data operand. The original relocations inside the
member extent must be exactly the generated segment words, in FIXUPP order per target
(for placed data, inside the placed extent of the home space). Back-patched (overwritten) LEDATA is
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

## Extensions of 2026-11-22: anchors, data code pointers, MSG class run

**Original facts.** DGROUP (paragraph `30f3`) starts at overlay-17 +`0x2940`. The
overlay payload ends at DGROUP `0x292f` (the known one-byte shortfall). The MSG class
tiles `[0x284e, 0x292f)` with no gaps:
- nmsghdr `HDR` `<<NMSG>>` (8 bytes);
- crt0msg `MSG` (130), crt0fp `MSG` (47), chksum `MSG` (37);
- common `PAD` `ff ff` at `0x292c` (contributed by crt0msg, crt0fp, chksum and nmsghdr);
- `EPAD` `ff` at `0x292e`.

[runtime_msg_run.json](../evidence/bindings/runtime_msg_run.json) records this run.

Two placed data segments carry code addresses:
- crt0 `_DATA` at `0x2256` holds `dw __exit` = `0x0238` (crt0dat at `1fe6:016c`, plus `0xcc`);
- cmiscdat `_DATA` at `0x249c` holds six `1fe6:0316` (`__fptrap`) far pointers, with
  overlay-17 relocations 116-121.

**Anchors.** Each anchor matches exactly once in the 64 KiB `1fe6` window and has at
least 18 ordinary bytes:

| Member | Anchor offset |
| --- | --- |
| crt0 | `0074` |
| crt0dat | `016c` |
| crt0msg | `02f4` |
| chkstk | `031c` |
| nmsghdr | `0574` |
| stdalloc | `05d6` |
| `_flsbuf` | `07c4` |
| `_open` | `0918` |
| `_sftbuf` | `0a00` |
| output | `0b7a` |
| stream | `1062` |

The malloc/free trampolines (one ordinary byte) and crt0fp (four) cannot be anchors.

**Members proven by these rules** (worker recipes; 1,741 bytes):

| Member | Bytes | Rules and providers |
| --- | ---: | --- |
| `fopen` | 66 | E1: `_open`, `stream` anchors |
| `fprintf` | 56 | E1: `output`, `_sftbuf` anchors |
| `sprintf` | 90 | E1: `output`, `_flsbuf` anchors; `_BSS` from the two-addend rule at `0x2ce2` |
| `stdenvp` | 125 | E1: `stdalloc` anchor |
| `nheapchk` | 28 | E1 + E3: crt0 `_DATA` provider, `__exit` from the crt0dat anchor |
| `nmemmax` | 45 | E1 + E3: crt0 `_DATA` provider, `__exit` from the crt0dat anchor |
| `crt0fp` | 6 | E1 + E4 |
| `crt0msg` | 34 | E1 + E4 |
| `chksum` | 36 | E1 + E4; `NULL` placed literally at DGROUP 0 |
| `output` | 1,255 | E1 + E3: cmiscdat far pointers; needs `crt0fp` pinned first |

**Hypotheses and limits.** An anchor proves a provider's position only: that member's
own external, DGROUP and secondary-storage obligations remain open, and it owns
nothing. A class run proves gap-free placement against the payload end, not natural
link order. The pairing of each contribution with its historical member is a library
correspondence. The remaining blockers below are unchanged by these rules: `__bufin`,
`_end`/`_edata`, crt0 `DBDATA`/`C_ETEXT` and crt0/crt0dat startup.

## Extensions of 2026-11-22: same-library selection and accepted providers

**Original facts.**
- The near-heap routines `1fe6:001a` and `1fe6:003b` are accepted ASM_EXACT owners
  (group `nmalloc/nmalloc`, publics `__NFREE` and `__NMALLOC`). Their reviewed relocated
  far calls include root `+0x13fa` (`001a`) and root `+0x1387` (`003b`, MZ 65).
- The original `strdup` far pointers encode `1fe6:20f4` and `1fe6:1614`. Those are the
  mlibcr (`crt_strlen_medium`, `crt_strcpy_medium`) entries. The llibcr pins at
  `2000:1d34`/`2000:1d4c` declare the same names.

**Members proven** (worker recipes in `build/workers/e67_runtime/`; 177 bytes):

| Member | Bytes | Rule |
| --- | ---: | --- |
| `malloc.asm` `_malloc` | 5 | E7 `__nmalloc` (MZ 1931) |
| `free.asm` `_free` | 5 | E7 `__nfree` (MZ 1915) |
| `calloc.asm` `__ncalloc` | 46 | E7 `__nmalloc` (MZ 1932) |
| `_freebuf` | 44 | pinned `_free` |
| `stdalloc` | 37 | pinned `_malloc`, crt0 anchor `__amsg_exit`, growseg `_DATA` `__amblksiz` |
| `strdup` | 40 | E6 `_strlen`/`_strcpy`, pinned `_malloc` |

The three E7 members prove against the current manifest. `_freebuf` needs `free`
published first; `stdalloc` and `strdup` need `malloc`.

**Hypotheses.**
- The `__nmalloc` ↔ `__NMALLOC` name correspondence (MASM default case, case-insensitive
  link), the selection of the `__ncalloc` member over the same-code `_calloc` sibling,
  and the historical archive pairing remain correspondence hypotheses. Each field
  value is proven.
- Two `_strcpy` definitions cannot both carry that name in one link. The llibcr pins'
  public names are themselves hypotheses.

## Refused, with the exact blocker

- `crt0` (program start address, STACK, common `XIQC`, `_end`/`_edata`, `_main`),
  `crt0dat` (second CODE segment `EMULATOR_TEXT`; `__cintDIV`, `__fptrap`,
  `__nullcheck` providers missing).
- `__bufin` near communal in `_file.c`'s `_DATA` (only zero-addend references; not
  placeable): blocks `__iob`/`__iob2`/`__lastiob` and therefore `stream.c`,
  `_open.c`, `fflush.c`, `fclose.c`, `_flsbuf`, `_getbuf`, `_sftbuf`, `printf.c`.
  (Anchors only give their code positions.)
- `_end` (linker-defined) in `chkstk`'s `_DATA`: blocks `chkstk` and `stackava`
  (`STKHQQ`).
- crt0's common `DBDATA` (code offsets, `C_ETEXT` base): blocks `nmsghdr`, `d_rdwr`,
  `write`.
- `chkstk` also needs crt0's `__amsg_exit` (available as an anchor) besides `_end`.

Original TU, natural RTLink placement, library order and behaviour remain unproved;
the placed DGROUP bytes are evidence, not owned data.

## RTLink 6.10 overlay-manager members (rtlink-manager-v1)

`tools/rtlink_fixups.py` pins whole members of RTLink/Plus 6.10 `RTLUTILS.LIB`
(SHA-256 `c718bac0..`, the library recorded for the RTLink probe). They link into the
public segments `$$OVLMGR` (frame `2203`) and `$$OVLMGR_UTILS` (frame `22e5`) rather than
`_TEXT`/DGROUP. A recipe names `segment_class`; only the pinned RTLink library may own a
`$$` manager class, and the member must have exactly that one public-combine segment at
its declared alignment, initialized at both ends. Interior uninitialized LEDATA (MASM
`4 dup (?)` in `ovlmgr_m.asm`) links as zero; a real 6.10 link (`PROBE.EXE`) and the
original both hold zeros there. Fixup-free members use the literal mode.

**Output-segment frames.** `segment_frames` grounds every segment a member names:

- `far_call`: an original relocated CALL/JMP FAR outside the member, whose segment word
  is the frame and whose offset lands exactly on a group-free public of another pinned
  member of that segment name (an ungrouped public's far pointer stores its segment frame).
- `site_record`: an original MZ relocation record outside the member, with that frame.
  The site-frames probe shows that RTLink 6.10 stores the paragraph floor of the start of
  the output segment holding the site. It does this for grouped segments too, such as
  `$$OVLVEC` in group `$$OVLMGR`.
  The output segment holding the site therefore starts in `[F*16, F*16+15]` and reaches
  the site. If the site follows the start of a contribution that reaches past `F*16+15`
  (the member itself or a named pinned member), that segment overlaps the contribution.
  It is therefore the contribution's segment. For example, record 1975 (`0x221a6`) gives
  `ovlmgr_m` frame `2203`, and record 2028 (`0x22e9e`) gives `cnv_val`'s segment `22e5`.
  Records of other frames, such as 1946 `2202:0000`, refuse.

The MZ entry `2203:0678` agrees, but it is not used as a witness. A start frame may be a
group frame, and calls through the vectors use the `$$OVLMGR` group frame `2203`.

**Equations** (A = LEDATA addend, D = FIXUPP displacement, `B(s)` = frame*16 of segment s;
the FIXUPP frame (F0 own segment, F5, F2 = target) must be the target's own segment):

| Field | Value |
| --- | --- |
| LOC1 self-relative rel16 (own segment, target inside member) | `p + D + A - B - (m + at + 2)` |
| LOC1 offset, own segment / provider public | `p + D + A - B(own)` / `S + D + A - B(seg S)` |
| LOC2 segment base (own segment / provider) | frame, plus an original relocation at the field |
| LOC3 far pointer | `(target + D + A.lo - B) : frame`, original relocation at field+2 |

Providers are named PINNED_RUNTIME recipes (`code_providers`), unique by public. Groups,
communals, a start address, secondary storage, unbound/unused providers and foreign frames
refuse. Original relocations inside the member must be exactly the generated words.

### Pinned (7 members, 1,373 bytes)

| Member | Root | Bytes | Fixups | Frame witness |
| --- | --- | ---: | ---: | --- |
| `dcache.asm` | `22e5:0005` | 2 | 0 | literal; three manager CALL FARs reach it |
| `cnv_val.asm` | `22e5:0007` | 48 | 0 | literal |
| `ddata.asm` | `22e5:020a` | 13 | 0 | literal |
| `rtcomm_m.asm` | `22e5:0217` | 112 | 4 | far call `22e5:0007` (reloc 1965) to cnv_val |
| `pathsrch.asm` | `22e5:0287` | 820 | 7 | same; `$$RTLPATHSEARCH_MESSAGES` from pathsr_m |
| `pathsr_m.asm` | `22e5:05bb` | 50 | 3 | same; ends at the resident image end `0x2343d` |
| `ovlmgr_m.asm` | `2203:0000` | 328 | 11 | site record 1975 |

### Refused

- `ovlmgr.asm` (`2203:0150`, 2,491 bytes, 346 fixups) has no pinned provider for
  `$$MAIN`, `$$RTL_FATAL_ERROR`, `$$RTL_PRELOAD`, `$$RTL_REGISTER`, `$$RTL_TERM_INIT`,
  `$$RTOVEXEOFFSET` or `$$RTLEXPANDSECTION`. A research-only run read the first six
  provider positions back from the original far pointers: `22e5:0037..01b3` in the
  rtcommon-like range, and `$$MAIN` = `1fe6:0074`. With those positions, every byte and
  28 MZ relocations agree. The original stores `0000:0000` with no relocation for
  `$$RTLEXPANDSECTION`, which is linker-defined or absent. Its frame `2203` is groundable
  from record 1947, and `$$OVLMGR_UTILS` from record 2028 via cnv_val.
- `rtcommon.asm` (root `0x22e87`-`0x2305a`) differs from the 6.10 member (495 versus 467
  bytes; 135 non-fixup bytes differ). Linker-generated `0x22b3b`-`0x22e55` (`$$OVLINFO`,
  descriptor table, filename record and `$$OVLVEC` vectors) stays unowned.
