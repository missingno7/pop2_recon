# Reconstruction continuation — 2026-10-05

The immutable target remains `assets/PRINCE.EXE`, SHA-256
`5bf733c56441258e69303102b0e08388484a9400e4d5cdad9bd517478e150314`.
Release identity is unconfirmed. All 62 supplied asset identities, root/overlay
structure and 4,716 ordered relocation records remain unchanged.

## Current ownership and debt

| Category | Bytes |
| --- | ---: |
| Matching C | 1,174 |
| Matching ASM | 51 |
| Pinned runtime | 666 |
| External drivers | 0 |
| Unresolved identified game helpers | 186 |
| Unknown/unclassified payload | 269,311 |

There are 27 accepted C components, one ASM component and eleven whole pinned
runtime members. Overlay C owns 664 bytes across fourteen complete components;
resident ownership is 1,227 bytes. The identified game-helper subtotal is
1,360 bytes. Fifty-two source data-offset fixups and one runtime near fixup
are proven. Original MZ relocation coverage remains 0/4,716. Structural RTLink
closure remains UNRECOVERED and behavior validation remains NOT_STARTED.

## Fifth bounded wave

Sol reviewed three new complete targets and their independent original caller
paths. Two isolated Luna searches found exact resident C components; fresh
strict `--verify-only`, serialized publication and full validation accepted
all 120 bytes.

| Reviewed target | Canonical source | Complete bytes | Search outcome |
| --- | --- | ---: | --- |
| `root:052d:1dc6` | [make_index_rect.c](../src/make_index_rect.c) | 78 | Natural ternary clamp replaces the rejected local-variable form |
| `root:078a:28fe` | [is_four_kind.c](../src/is_four_kind.c) | 42 | Ordered byte predicate; both original return paths and NOPs retained |

Pinned MSC 6.00 `/c /AM /Oe /Gs /Zl /Gc` reproduces each complete segment.
Their actual Pascal publics are `MAKE_INDEX_RECT` and `IS_FOUR_KIND` at offset
zero. Neither component has object fixups or original MZ obligations. The
rectangle helper takes a near word-record pointer and signed index; the
predicate reads the low byte of its word argument slot. Original symbols,
source TU, natural placement and runtime behavior remain unproved.

The overlay setter `overlay-3:2344:0042` has a reviewed 34-byte extent and an
independent root DS:5c13 byte witness. Seven strict search trials produced
three distinct complete CODE outputs of 30 or 32 bytes. Plain/inverted forms,
volatile qualifiers, `/Oe`, `/O` and the final `/Oer` shared-exit check did not
reproduce its SI preservation, separate stores and single SP-restoring return.
All outputs remain rejected and unscored; no prefix, padding or binding change
granted ownership. The finite family is parked pending a new register/source
lead. Its 34 bytes join the prior 152 unresolved identified helper bytes;
this reclassification reduces unknown payload without claiming a match.

The structural Sol task established an independent original-instruction table
locator: MZ entry/relocated manager selector, count 16 and `BX=18*AX+0b1b`
locate file offset 150,347. The original loader distinguishes memory paragraphs
from file paragraphs (overlay 17: 3,026 versus 1,319), and derives its relocation
base from startup ES plus `0x10`. [RTLink documentation](rtlink.md) and the
[compact original manager proof](../evidence/rtlink-manager.json) now separate
these facts from the reference loader's fixed `0x0100` observation model. Pinned
local 6.10 source layout agrees; the supplied 4.00 non-debug structure differs.
Exact historical version, arbitrary CRT frames, whole-link closure and behavior
remain unproved. This static research grants zero ownership or MZ coverage.

The [wave receipt](../evidence/matching/grinding-wave5.json) keeps accepted
source/object hashes and compact rejected CODE identities/resume conditions.
The [original review](../evidence/matching/wave5-original-review.json) keeps
full target bytes, boundaries and independently checked caller/witness paths.
Each accepted component has one source and recipe. Failed sources, objects and
bulk contexts stay ignored; existing acceptance and journal tools were sufficient.

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

The [third wave](../evidence/matching/grinding-wave3.json) added four more
overlay components and isolated the membership `/Or` shared-exit near-match.
Its whole runtime scan established concrete memmove provider/frame, calloc
revision and filelength provider-tree blockers. The retained receipts supply
known output hashes and resume conditions; they are diagnostic evidence, not
acceptance caches.

The [fourth wave](../evidence/matching/grinding-wave4.json) added four scalar/data
helpers and repaired fresh rejected-CODE identity retention without weakening
acceptance. Its compiler-frame experiments and string-search far-provider
blockers remain recorded; neither tree received new blind grinding this wave.

Earlier resident source evidence is in [state helpers](state-helpers.md),
[data helpers](data-helpers.md) and the [bootstrap report](bootstrap-status.md).
The 51-byte readable MASM byte-block reversal is CUSTOM_ASM, separately counted;
its DF and behavioral preconditions remain untraced. The complete 13-byte
runtime close member uses the [restricted near proof](runtime-near-binding.md);
its provider context grants zero ownership. [Symbolic OMF inspection](omf-fixup-resolution.md)
and [RTLink research](rtlink.md) remain separate from acceptance.

## Next reasoning leads

The unresolved identified helpers are the 46-byte overlay-2 swap, 26-byte
resident signed-value helper, 80-byte membership predicate and new 34-byte
overlay-3 setter. The [swap matrix](first-matches.md), prior wave receipts and current frame-option evidence
record exhausted outputs. Further Luna work requires a new lead;
repeating known source/flag families does not advance these targets.

Far-call work also needs independent provider identity/frame evidence. The
earlier strnicmp alias appears only inside its prospective owner; a separate
strlen caller has a still-open near/far provider tree. The current runtime scan
adds concrete object/revision blockers. Runtime DS, original TU grouping,
RTLink version and overlay placement remain debts; the original descriptor
locator is now independently established. The reserved overlay-3 predicate
`2344:237e` is a practical next boundary-review lead, not yet a canonical target.

Full `python tools/validate.py` passes all 141 invariant tests, freshly compiles
27 C components, assembles one ASM component, checks all eleven independent
runtime members and rechecks every supplied asset/structure and historical tool
pin. Whole extents, symbolic equations and canonical ownership pass.
