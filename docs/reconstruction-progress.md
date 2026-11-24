# Reconstruction progress

> Historical: this page describes the earlier build (tag `oracle-dos-1565`). Its addresses, hashes and counts are not those of the v1.1 target; evidence files it cites are at that tag.

The immutable target remains `assets/PRINCE.EXE`, SHA-256
`5bf733c56441258e69303102b0e08388484a9400e4d5cdad9bd517478e150314`.
Release identity is unconfirmed. All 62 supplied asset identities, root/overlay
structure and 4,716 ordered relocation records remain unchanged.

## Current ownership and debt (2026-10-23)

| Category | Bytes |
| --- | ---: |
| Matching C | 2,349 |
| Matching ASM | 51 |
| Pinned runtime | 1,166 |
| External drivers | 0 |
| Unowned payload (identified and unclassified) | 267,822 |

Fresh validation of the canonical ownership baseline passes 220 invariant tests
and rebuilds 57 C components, one ASM component and twenty-one pinned runtime
members. Overlay C owns 884 bytes across eighteen complete components; resident
ownership is 2,682 bytes. There are 105 proven object fixup fields and 11/4,716
covered original relocation records. Structural RTLink closure remains
UNRECOVERED and behavior validation remains NOT_STARTED.

2026-11-21: the [symbolic runtime mode](runtime-fixups.md) pins eighteen further
fixup-bearing medium-model library members (2,529 bytes, 72 proven fields, 9
relocations), placing and comparing their DGROUP storage; 28 more matched members
stay refused with named blockers.

These totals are a dated snapshot. `layout/manifest.json` is authoritative
ownership; `python tools/validate.py` freshly checks it and writes the report,
including metrics. `python tools/metrics.py` refreshes the diagnostic metrics
without rebuilding and is not an acceptance command. Unowned payload includes
code, data and linker structures, not just game code.

Validation and both publication paths now retain explicit failure/interruption
state and recoverable publication journals; see [acceptance](acceptance.md).
The proof rules, original assets, toolchain pins and accepted ownership are
unchanged by this reliability milestone.

The subsequent [larger-function follow-up](larger-function-frontier.md) adds
the 160-byte fallback at `root:02cc:016a`, with thirteen proven byte-data fields.
It is the largest accepted C component so far. Complete source, branch shape,
outside data witnesses and fresh validation agree under the existing rules;
the near-call/TU investigation remains separate research.

## Historical milestones

The wave counts and validation totals below describe their respective historical
milestones, not current ownership. Use the dated summary above and fresh reports.

## Ninth far-call milestone

The [root far-call mode](far-call-binding.md) proves F5/T6 zero-addend LOC3
CALL operands to separately reviewed direct entries with independent relocated
original callers. It checks exact ordered segment-word relocation coverage and
composes with both resident DGROUP data modes. Overlay/vector and TU routing
remain separate debts. The complete 20-byte far-pointer wrapper at
`root:1e23:04c6` adds one C component and covers root relocation index 1764.
The independently reviewed 22-byte callee grants no ownership. Source names and
resource/memory interpretation remain hypotheses; SUPPORT_CODE is kept separate
from identified game helpers. Full validation and `python -m pytest -q tests`
pass all 172 tests. No assets or locks changed.

## Eighth bounded wave

The recorded rectangle operand-orientation proposal was tested once under
MSC600 `/c /AM /O /Gs /Zl /Gc`. Its equivalent signed relations reproduce the
same complete 38-byte CODE and entire OMF object as the previous best source.
The first load remains the bottom word at +6; the predicted +2 top-word load
does not appear. The worker stops on this known complete-object repetition.
This closes the proposed discriminator in that compiler context; the original
early AX=1, CX/DX coordinate temporaries and single exit remain unexplained.
No second trial or source ownership is claimed.

Two complete medium-model conversion members now pass the existing literal
runtime gate. The 84-byte `atox.asm` parser at `1fe6:1672` is reached through an
independently relocated far call to the preceding three-byte conversion stub,
whose original near equation enters the parser. Its selected caller path is
reviewed; an indirect switch on another branch remains open. The stub and
alignment stay unowned. The 95-byte `xtoa.asm` member at `1fe6:19a4` retains
both publics and its shared setup/core, with both entry CFGs reviewed. Original
`itoa` handoff context establishes its register ABI, while the separate
fixup-bearing wrapper still lacks the independent incoming witness required by
the near gate. Provider ownership does not grant wrapper ownership.

The scanner's one-public restriction had hidden the complete shared conversion
member. The diagnostic now preserves all in-extent CODE publics while retaining
one complete zero-fixup CODE segment and empty secondary storage. Two tests
cover complete shared-entry discovery and an out-of-extent public. This matches
the existing literal-runtime member rules; acceptance itself is unchanged.
The refreshed two-library census finds 23 whole literal research matches, all
overlapping accepted ownership after these publications. Further runtime work
needs a new library/context or symbolic-provider lead.

Fresh strict checks and serial publication add 179 pinned runtime bytes.
[Original review](../evidence/matching/wave8-runtime-review.json) and
[the wave receipt](../evidence/matching/grinding-wave8.json) retain complete
member pins, original paths, the closed compiler discriminator and explicit
remaining debts. Full validation passes 148 invariant tests and freshly checks
31 C components, one ASM component, twenty-one runtime members and every asset
and historical tool identity. Source/ASM totals and the five unresolved helpers
are unchanged; original relocation coverage remains 0/4,716.

## Seventh bounded wave

The remaining adjacent three-value predicate `root:078a:28a4` is reconstructed
as [is_three_kind.c](../src/is_three_kind.c). Independent original review closes
both RETF2 exits, both NOPs, the byte argument ABI and the complete overlay caller
path to its relocated far entry. The ordered natural comparisons 11, 26, 15 match
all 36 bytes on one compiler trial using MSC600 `/c /AM /Oe /Gs /Zl /Gc`.
One preceding search refused a mislabeled reviewed callsite before compilation;
correcting the evidence schema cleared it without changing source or acceptance.
Fresh strict verification, serial publication and full validation passed.

The medium-model library lead yields seven further complete runtime members:
strcat 63, strcpy 50, strlen 27, strncpy 40, strchr 42, strupr 30 and BIOS timeofday 46.
All 298 bytes compare literally, with no fixups or secondary storage. Original
entry encodings and caller paths are independently reviewed; all members pass
fresh `pin_runtime.py --verify-only` and serial publication. Their near-pointer
ABIs coexist with distinct accepted large-model members. Global CRT model,
original TU/library ordering and execution assumptions remain unproved. The
[runtime review](../evidence/matching/wave7-runtime-review.json) retains whole
archive/member pins and compact original context.

The successful census is now `tools/runtime_scan.py`, with required worker-local
output. It scans complete members before the archive dictionary and reports
whole literal CODE matches, original relocations and ownership overlaps. It
excludes sparse code, secondary storage, multiple publics and actual fixups;
it grants no ownership. Five new refusal/bounds tests cover the diagnostic's
important failure cases. Existing acceptance gates are unchanged.

The newly reviewed rectangle predicate `root:1101:4d6e` has a complete 34-byte
extent, signed word comparisons and a far-pointer Pascal ABI, with no fixups.
Four Luna trials stop after repeated 40-byte outputs. A four-trial Sol compiler
context review produces 48-byte MSC510 local-result code and identical 38-byte
short-OR CODE under MSC510/MSC600 `/O`; MSC600 `/Oeg` repeats known 40-byte CODE.
The short-OR baseline already uses transient BX and result AX. Its remaining
differences include coordinate operand order, late Boolean materialization and
return/branch arrangement; the original initializes AX to 1 early and uses CX/DX for
coordinates. Swapping equivalent relational operands is a concrete uncompiled
resume lead, not an accepted explanation. The 34 bytes remain unresolved, joining
the four older parked helpers. Full objects and failed sources stay private.

The [wave receipt](../evidence/matching/grinding-wave7.json) separates original
review, compiler observations and hypotheses. Current totals add 36 C bytes and
298 runtime bytes. Full validation passes 146 invariant tests and freshly checks
31 C components, one ASM component, nineteen pinned runtime members, all assets
and historical tool identities. Original relocation coverage remains 0/4,716.

## Sixth bounded wave

Independent original-byte review closed two resident byte predicates and the
reserved overlay-3 kind/value predicate. The full caller paths, both resident
return exits, the overlay's single exit and all compiler alignment bytes are
retained in [original review](../evidence/matching/wave6-original-review.json).
The outside overlay entry encoding remains ambiguous between overlapping spaces;
the proof is of static overlay-3 file-space bytes, not runtime dispatch.

| Target | Canonical source | Complete bytes |
| --- | --- | ---: |
| `root:078a:2830` | [is_seven_kind.c](../src/is_seven_kind.c) | 60 |
| `root:078a:28c8` | [is_six_kind.c](../src/is_six_kind.c) | 54 |
| `overlay-3:2344:237e` | [is_kind_value.c](../src/is_kind_value.c) | 88 |

The resident predicates match naturally on their first trials. The overlay's
combined Boolean baseline is 74 bytes. Five follow-up source variants fail full
extent checks; Sol review also found that their failed kind-10 cases incorrectly
reached the default branch. Those failures do not exclude correctly nested C.
Correct nested false paths plus a staged default Boolean assignment match all
88 bytes on one fresh trial. The independent root DS:5c19 witness proves its
single word field; every other byte compares literally. The source uses only
meaningful declarations and assignments, with no synthesized padding or bytes.

The whole 23-byte medium-model `dos\d_open.asm` runtime member also passes the
existing near gate. Its near-pointer ABI matches the original; the 26-byte
large-model member differs. The provider witness uses the complete original
close wrapper outside the new open owner. All 21 ordinary bytes and the
`0604 - 1be3 = ea21` field are rechecked. The following original alignment byte
stays unowned. This is [component runtime proof](runtime-near-binding.md), not a
claim about global CRT model, library order, whole placement or behavior.

The [wave receipt](../evidence/matching/grinding-wave6.json) retains compact
accepted hashes and useful negative/resume evidence. Reversing the signed-value
helper's real declarations repeats known 24-byte CODE. The documented
`check_stack(off)` pragma suppresses probes under `/Ge` but repeats membership's
78-byte CODE. A new staged-Boolean transfer check from the exact overlay emits
148/152 bytes and still omits SP restoration; its two-trial budget is closed.
The four previously parked helpers remain unresolved.

The allocator review deduplicates the 6.00A archive and stops after five new
archive trials at repeated-provider limit. MSC 7 has a handler hook, but a
59-byte allocator public, 92-byte CODE and four initialized DATA bytes differ
from the original 57-byte allocator and its saved BX/CX handler path. Its
outside far-call encoding is a useful CS/entry lead; caller-entry grounding and
complete provider correspondence remain open. No prefix or provider bytes are
accepted. Worker cards now carry discriminating original branch outcomes for
nested guards, preventing source mistakes from becoming compiler-exclusion
claims. Existing acceptance and journal tools suffice; no gate was weakened.

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
the exact proof limits. The earlier rejected predicate has been superseded;
the wave receipt retains its useful negative evidence.

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

## Remaining reasoning leads

The [state-step TU investigation](tu-state-step.md) reproduces a complete
34-byte caller/callee cluster under conditional field equations, with unchanged
accepted callee CODE. Its three controlled compilations isolate same-file call
lowering and alignment; the existing multi-public/near-call gate correctly
refuses acceptance. No bytes were added to ownership. The next discriminator
is larger-unit membership and symbolic proof, not more source permutations.

The four parked helper searches are the 46-byte overlay-2 swap, 26-byte
resident signed-value helper, 80-byte membership predicate and 34-byte resident
rectangle predicate (186 bytes total). They are a subset of unresolved reviewed
targets, not a complete inventory of remaining code. The overlay-3 setter is
accepted as `src/set_state_from_record.c`; `/O /Or` with an explicit register
pointer reproduced its 34-byte extent. The [swap matrix](first-matches.md), prior wave receipts and current frame-option evidence
record exhausted outputs. Further Luna work requires a new lead;
repeating known source/flag families does not advance these targets.

Far-call work also needs independent provider identity/frame evidence. The
earlier strnicmp alias appears only inside its prospective owner; a separate
strlen caller has a still-open near/far provider tree. The current runtime scan
adds concrete object/revision blockers. Runtime DS, original TU grouping,
RTLink version and overlay placement remain debts; the original descriptor
locator is now independently established. The formerly reserved overlay-3
predicate `2344:237e` is now reconstructed.

The medium-model census now includes complete multi-public members. All of its
current literal hits overlap accepted ownership. The conversion parser and
shared output core are pinned in full; `_itoa` still needs a distinct incoming
wrapper and a supported shared-public near proof. The rectangle operand-order
proposal repeats its known complete object. Further source trials need an
independent lifetime, type, TU or compiler-pass lead, rather than another
equivalent comparison rewrite.

The eighth-wave validation passed 148 invariant tests, compiled 31 C components,
assembled one ASM component and checked twenty-one runtime members. This is a
historical result; the current validation baseline is recorded at the top.
