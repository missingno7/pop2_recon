# Reconstruction continuation — 2026-10-05

The immutable target remains `assets/PRINCE.EXE`, SHA-256
`5bf733c56441258e69303102b0e08388484a9400e4d5cdad9bd517478e150314`.
Release identity is unconfirmed. All 62 supplied asset identities, root/overlay
structure and 4,716 ordered relocation records remain unchanged.

## Current ownership and debt

| Category | Bytes |
| --- | ---: |
| Matching C | 872 |
| Matching ASM | 51 |
| Pinned runtime | 666 |
| External drivers | 0 |
| Unresolved identified game helpers | 152 |
| Unknown/unclassified payload | 269,647 |

There are 21 accepted C components, one ASM component and eleven whole pinned
runtime members. Overlay C owns 576 bytes across twelve complete components;
resident ownership is 1,013 bytes. The identified game-helper subtotal is
1,024 bytes. Forty-nine source data-offset fixups and one runtime near fixup
are proven. Original MZ relocation coverage remains 0/4,716. Structural RTLink
closure remains UNRECOVERED and behavior validation remains NOT_STARTED.

## Third bounded wave

Sol reviewed four complete extents, independent inbound PUSH CS/near-call
paths, ABI and data witnesses before four isolated Luna searches began.
The supervisor rechecked every recorded caller CFG step, original context,
extent hash and six independent root data-witness paths. All four candidates
then passed fresh strict `--verify-only`, serialized promotion and full validation.

| Reviewed target | Canonical source | Complete bytes | Search outcome |
| --- | --- | ---: | --- |
| `overlay-11:2c3e:11a0` | [in_remainder_window63.c](../src/in_remainder_window63.c) | 54 | One distinct CODE; corrected public lookup on second recorded trial |
| `overlay-3:2344:2836` | [is_scalar_modulo_window.c](../src/is_scalar_modulo_window.c) | 88 | Four trials, three distinct CODE outputs |
| `overlay-12:2c3e:0042` | [set_overlay_state_bytes.c](../src/set_overlay_state_bytes.c) | 16 | First source trial; two data equations |
| `overlay-4:29cf:0a54` | [overlay_state_window.c](../src/overlay_state_window.c) | 48 | Second trial; five data equations |

Pinned MSC 6.00 `/c /AM /Oe /Gs /Zl /Gc` reproduces all 206 added bytes,
including reviewed alignment NOPs. The scalar source keeps a signed local
index and normalizes the unsigned value parameter in place. Its second
modulo path can return 2; the source preserves that observed word result.
The two data helpers use the existing [overlay binding mode](dgroup-binding.md):
50 ordinary bytes compare literally and seven zero-addend fields satisfy
independent DS-relative equations. Original types/names, data storage, runtime
DS association, source TU and natural link placement remain unproved.

Sol tested three documented compiler leads for the stagnant 80-byte membership
predicate. `/Or`, documented in the shipped compiler help as disabling inline
return, produces a complete 78-byte output whose first 73 bytes agree. It still
omits original `MOV SP,BP`. `/Os` additionally removes the alignment NOP;
`/Og` repeats the older 94-byte switch output. The supervisor independently
reproduced the 78-byte full CODE/object hashes. The single
[natural C probe](../recipes/probes/overlay_membership.c) now retains that better
source; all outputs remain non-owning. No artificial stack allocation or padding
was introduced. Resume requires independent lifetime/compiler/TU evidence.

The runtime frontier scan strictly parsed 678 whole members from three pinned
archives. Three substantive whole CODE candidates agree at ordinary bytes:
134-byte filelength, 46-byte calloc and 202-byte memmove. These are diagnostics,
not accepted matches: their symbolic obligations remain open. Memmove's
independently pinned hdiff provider has no outside original CS alias witness
and its object contains one overlapping initialized byte rejected by the gate.
The absolute `__AHINCR=4096` definition is independently pinned but does not
resolve that frame. Calloc's original allocator adds an indirect new-handler
call absent from the pinned provider revision. Filelength's lseek tree mixes
data, interrupts and a near tail. No speculative checker or ownership followed.

The [wave receipt](../evidence/matching/grinding-wave3.json) retains compact
compiler/output identities, archive/provider pins, blockers and resume conditions.
The [original review](../evidence/matching/wave3-original-review.json) retains
full original bytes, instruction boundaries, branches and caller/witness paths.
Bulk disassembly, objects, failed source variants and proof drafts stay ignored.
Each accepted component has one canonical source, recipe and any required binding.
Existing cards, journals, search and promotion needed no infrastructure changes.

## Earlier milestones

The [first wave](../evidence/matching/grinding-wave1.json) established four
relocation-free overlay C components and resolved the resident predicate using
the narrow independently witnessed word-pair mode. The
[second wave](../evidence/matching/grinding-wave2.json) accepted three more
overlay leaves and the first overlay data-bound predicate. Its refusal tests
preserve resident gates, reject overlay word pairs/nonzero addends, and explicitly
reject LDS/LES segment writes. The [binding document](dgroup-binding.md) records
the exact proof limits. Earlier failed predicate material is isolated under
`to_delete/resolved-predicate/`, outside normal indexing/build/validation.

The initial audit found the existing oracle, runners and serial manifest writer
sufficient. `search.py --trial` adds only private CODE-hash/binding-identity
journals, a frozen best source and budget/stagnation stops. AGENTS.md specifies
model roles and private CLI output paths; no shared queue or scheduler exists.

Earlier resident source evidence is in [state helpers](state-helpers.md),
[data helpers](data-helpers.md) and the [bootstrap report](bootstrap-status.md).
The 51-byte readable MASM byte-block reversal is CUSTOM_ASM, separately counted;
its DF and behavioral preconditions remain untraced. The complete 13-byte
runtime close member uses the [restricted near proof](runtime-near-binding.md);
its provider context grants zero ownership. [Symbolic OMF inspection](omf-fixup-resolution.md)
and [RTLink research](rtlink.md) remain separate from acceptance.

## Next reasoning leads

The unresolved identified helpers are the 46-byte overlay-2 swap, 26-byte
resident signed-value helper and 80-byte membership predicate. The
[swap matrix](first-matches.md), prior wave receipts and current `/Or` evidence
record exhausted outputs. Further Luna work requires a new lead;
repeating known source/flag families does not advance these targets.

Far-call work also needs independent provider identity/frame evidence. The
earlier strnicmp alias appears only inside its prospective owner; a separate
strlen caller has a still-open near/far provider tree. The current runtime scan
adds concrete object/revision blockers. Runtime DS, original TU grouping,
RTLink version, overlay placement and descriptor location remain separate debts.

Full `python tools/validate.py` passes all 137 invariant tests, freshly compiles
21 C components, assembles one ASM component, checks all eleven independent
runtime members and rechecks every supplied asset/structure and historical tool
pin. Whole extents, symbolic equations and canonical ownership pass.
