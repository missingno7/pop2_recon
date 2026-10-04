# Bootstrap status — 2026-10-04

The bootstrap produces real component proofs, not a reconstructed executable.
One game C function is CODE_EXACT; ten independent historical library members
are PINNED_RUNTIME. `python tools/validate.py` freshly verifies every component
and passes 45 invariant tests. Original assets remain unchanged.

## Target and immutable oracle

The target is the supplied DOS `assets/PRINCE.EXE`, 290,415 bytes, SHA-256
`5bf733c56441258e69303102b0e08388484a9400e4d5cdad9bd517478e150314`.
Exact release/build identity is unconfirmed. `PRINCE2.EX_` has SHA-256
`2a4268b4c42dd808947edcc4e4fe9646ef33f1bfb4e5361b6267fadbaf112dbd`;
five changed bytes alter a one-time initialization guard. Neither is asserted
to be retail. Several resource identities agree with a published early-release
manifest while both executable hashes differ; see external-evidence.md.

All 62 supplied files, totaling 7,029,758 bytes, are SHA-locked in
`layout/oracle.lock.json`, including resources, drivers, configuration and
support executables. Copy timestamps are observations, not build dates.

MZ header: 8,192 bytes; DOS-declared file image: 152,637 bytes; resident payload:
144,445 bytes. Initial link CS:IP is `2203:0678`; runtime load segment remains a
separate variable. Root has 2,029 ordered relocation records. The 16 attached
spaces (IDs 2–17) contain another 2,687 records: **4,716 total**. Their unmodified
payload plus root totals 271,388 bytes. The final overlay physically lacks one
byte of its paragraph allocation, and three bytes before overlay 2 are explicit
file-alignment debt. No bytes are fabricated or normalized.

The table at file offset 150,347 is a guarded, target-specific bootstrap offset
in `layout/oracle-format.json`; a runtime-derived locator is still unresolved.
`layout/structure.lock.json` independently freezes all payload and ordered
relocation hashes. `tools/oracle.py --extract` writes only verification fixtures
under ignored `build/oracle`; they never feed candidate construction.

## Compiler and component results

Pinned MSC 5.10, MSC 6.00 and the nearby 6.00A C2L-bound installation run under
MS-DOS Player with `-e -v5.00`. Nine independent source probes cover arithmetic,
signed/unsigned branches, switches, register locals, far pointers and helper
fixups, with near/medium model and optimizer contrasts. Tool and source hashes,
full declarations and FIXUPP evidence are in `evidence/toolchain/probes.json`.
The 6.00A large-model control timed out; its tested near-model profile works.

`root:02cc:08c2` (root position `0x3582`) owns 20 bytes. The independently reviewed
far CALL at root position `0x1999`, MZ segment relocation at `0x199c`, argument
pushes and complete RETF 4 body establish its ABI and boundary. The accepted C
uses a register signed word and a near output pointer, decrements negative
values, then stores them. `src/floor_adjust.c` and `recipes/floor_adjust.json`
produce exactly all 20 bytes under MSC 6.00:

```
/c /AM /Oe /Gs /Zl /Gc
```

No actual fixups or original relocation obligations occur in this component.
Unused externals and FIXUPP thread declarations remain recorded, never masked.
MSC 5.10 rejects `/Oe` and emits 22 differing bytes under `/Ox`. MSC 6.00A also
matches the target under an explicit far Pascal function in small model. This
supports the MSC 6 generation without uniquely identifying revision, global
memory model, original TU flags or original symbol name.

Ten full nofixup CODE segments in the independently installed MSC6 `LLIBCR.LIB`
match resident bytes: string functions and internal heap helpers. Runtime
recipes pin the archive, entire OMF member, declarations and complete code.
They cover **653 bytes**, separately counted as PINNED_RUNTIME. Partial startup
similarity is diagnostic only and owns zero bytes. Whole CRT release and
contiguous runtime boundaries remain unproved.

The overlay-2 pair swap at `28a1:0e68` remains nonmatching. It has a 45-byte
instruction body plus observed alignment NOP, making a provisional 46-byte owned
extent. Natural aggregate C emits 50 bytes, first differing at byte 14: two
spills replace the original CX/stack split. Scalar, register aggregate/long and
optimizer variants did not solve it. Readable failed source and full output
evidence are retained under `evidence/matching/`; no ownership is promoted.

## Infrastructure and current debt

Implemented MZ/root/overlay parsing and extraction; ordered relocation handling;
separate file/link/runtime addresses; hash and structure locks; strict OMF
inspection with full initialization coverage; isolated historical compilation;
context, search, promote and validation commands; runtime pin verification;
ownership overlap checks; and per-space debt metrics.

Initial discovery has 2,184 entry hypotheses, 976 traced entries, 1,249 call
edges and 1,208 prologue-only hints. They remain UNKNOWN and have tentative
extents. Reviewed targets and canonical ownership are separate. Regression
tests cover CS:IP aliases, wrapping near calls, far segments, original/candidate
independence, fixup debt, complete extents, holes, overlaps and stale objects.

| Ownership | Bytes |
| --- | ---: |
| Reconstructed C, CODE_EXACT | 20 |
| Reconstructed ASM | 0 |
| Pinned historical runtime | 653 |
| Unresolved/unclassified payload | 270,715 |

All accepted bytes currently reside in the root. Overlay reconstruction and
generated relocation coverage remain zero. Unknown payload includes data and
linker structures, so it is not all labeled game code. One nonmatching C target
is retained. `build/metrics.json` provides the full per-overlay breakdown.

The binary directly contains RTLink/Plus runtime text. A clean RTLink 6.10
synthetic build confirms the same 18-byte descriptor format and paragraph
arithmetic, but uses a documented temporary runtime-library filename alias.
It is a structural experiment, not historical closure. The 4.00 invocation
reached positional prompts and produced no comparable output. Exact original
RTLink version, module order, overlay grouping, vectors and natural final
placement are unresolved.

The pinned public Supermedo checkout supplies independently published parser
and entry hints; its generated CPU emulation C is never canonical source. Layout
agreement does not prove executable hash identity. First-person public reports
support the FM Towns symbols lead, but no symbol-bearing material is available
locally and no cross-version names have been assigned to DOS functions.

Next useful work:

1. Resolve the overlay-2 pair-swap register/stack split and independently review
   its callers; use its complete 46-byte owned extent.
2. Implement independently grounded DGROUP/global and call binding, preserving
   every FIXUPP and ordered relocation. The current gate rejects actual fixups,
   COMDEF/unsupported storage, and nonzero secondary data/BSS.
3. Extend runtime-member verification to startup and compiler helpers with
   symbolic fixups; do not promote normalized prefix similarity.
4. Probe original TU grouping and flag families around the exact helper, while
   investigating RTLink versions and a runtime-derived descriptor locator.
5. Correlate legally supplied/public FM Towns symbol metadata by behavior and
   call graph if it becomes available.

Original DOS behavioral traces have not started. No hybrid image, reconstructed
whole executable or SDL port is claimed or constructed.
