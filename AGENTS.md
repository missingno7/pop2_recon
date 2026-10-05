# Historical reconstruction workers

Read README.md and docs/acceptance.md. The immutable supplied `assets/` and
`layout/oracle.lock.json` are authority. Never edit originals, refresh a lock from
a candidate, patch an object/executable, embed original code bytes as C, trim a
mismatching extent, mask fixups, or count static-recompiled C as reconstruction.

Run `python tools/oracle.py --extract` for unrelocated verification fixtures.
Run `python tools/discover.py` for the tentative inventory and
`python tools/context.py SPACE:SEG:OFF --asm` for original-byte context. Review
boundaries, callers, ABI and classification before submitting a target for
`evidence/targets.json`; discovery is a hypothesis, never acceptance.

Each worker owns `build/workers/NAME/`. Canonical source, target evidence, recipes,
manifest and acceptance tools have one integrating writer. Submit readable
historical C, exact compiler flags, public symbol, full emitted segment extent,
and independent target-boundary evidence. No worker overwrites another's source.

Use `python tools/search.py ID build/workers/NAME/candidate.c --profile PROFILE
--symbol _PUBLIC --worker NAME --flags /c /AM /O /Gs /Zl` for local comparison.
Compiler profiles and file identities are pinned under layout/ and recipes/.
MS-DOS Player is the normal compiler/linker runner; no tool binaries belong in Git.

Use `python tools/promote.py ID build/workers/NAME/candidate.c --profile PROFILE
--symbol _PUBLIC --name canonical_name --flags ... --verify-only` for a fresh
strict check. The integrating writer publishes by removing `--verify-only`, then
runs `python tools/validate.py`. Accepted code is complete component CODE_EXACT,
not original-TU or whole-link proof. Current acceptance supports one complete
initialized CODE segment. Relocation-free components need no binding proof.
Unreferenced compiler externals and FIXUPP thread declarations are retained in
the object proof. For the narrow external DGROUP offset16 modes (resident zero addend or
independently witnessed two-word alias; overlay byte/word zero addend), provide
`--binding evidence/bindings/NAME.json`; see docs/dgroup-binding.md. Every field
equation and independent original witness is rechecked, with no byte masking.
Other fixup-bearing components remain blocked on their symbolic proof.

For whole historical runtime members, submit an independent archive/member pin
as a worker recipe. Run `python tools/pin_runtime.py build/workers/NAME/recipe.json
--verify-only` for a fresh check; the integrating writer publishes by removing
`--verify-only`. The restricted resident F0/T6 near-offset mode uses independently
anchored provider-public context; see docs/runtime-near-binding.md. Provider
signatures grant zero ownership. A whole member with secondary storage cannot
be promoted by selecting its matching prefix. Runtime recipes and any binding
files are checked freshly by validation.

For reviewed hand-assembly hypotheses, use readable MASM source and select
`--language asm --profile masm510` for search/promotion. The same whole-segment
and symbolic-obligation gate applies. No opcode byte blobs, output edits, or
synthetic trailing bytes are acceptable. Canonical ASM lives in `asm/` and is
freshly assembled by validation. Symbolic FIXUPP inspection supports research;
only the explicitly implemented binding mode can grant fixup-bearing ownership.

Run full validation at tooling/acceptance milestones, not every hypothesis.
Keep original facts, compiler observations, external hints and source hypotheses
separate. Report C, ASM, proven runtime, drivers and unresolved payload separately.
Do not call unknown payload all game code. Structural RTLink recovery and runtime
behavior validation are separate debts. Do not begin an SDL port.

Commit useful bounded milestones. Preserve local assets, tools, reference clones
and worker evidence; never run blanket `git clean -fdx`.

## Bounded agent waves

The supervisor selects independent reviewed targets from one canonical HEAD,
reads previous negative evidence, and owns all canonical writes. Give each worker
one target, its extent/ABI/classification, supported binding, profile shortlist,
verification command, and a private `build/workers/NAME/task.json`. The card has
`target_id`, `canonical_head`, `max_trials`, `stagnation_limit`, and optional
`known_code_hashes`; add the source/ABI lead and evidence paths in plain fields.
Assign a target to only one grinding worker per wave. Do not create worker chats,
a daemon, shared writable queue, or speculative canonical source variants.

Use gpt-6-luna with xhigh for bounded source/profile searches. Use gpt-6.1-sol
with high for boundary/ABI review, stagnant near-matches, new proofs, overlays,
and structural/TU/linker decisions. Luna must not design acceptance mechanisms.
Use `--output build/workers/NAME/discovery.json` for worker discovery and
`--output build/workers/NAME/context.json` for worker context; their CLI defaults
write shared inventory diagnostics. Root refreshes shared oracle fixtures.
Workers write only their assigned scratch directory, never canonical directories,
acceptance tools or another worker's files. Use escalated shell execution if the
Windows sandbox helper fails; this does not change the owned write scope.

Append `--trial LABEL` to the existing search command to produce worker-local
`trials.json` and `summary.json`. This requires the supervisor card and source
inside that worker directory. The journal records compact diagnostics, freezes
one best source, deduplicates complete raw CODE hashes, distinguishes changed
binding/fixup identities, and stops at exact, budget or repeated-output limits.
A repeated CODE hash is no proof of equal bindings. These files grant no ownership;
search still compiles freshly, and promotion/validation never consume a cache.
Stop when requested by the card. Do not retry a stagnant target without a new
reasoning lead; inspect the existing negative receipts first.

Collect worker summaries, review full objects and independent original evidence,
then run fresh `promote.py --verify-only` and serial publication. Run full
validation at the wave boundary. Keep one canonical source/recipe and concise
useful evidence; failed variants and bulk disassembly remain ignored scratch.
Do not preserve every experiment as a tracked document. Preserve existing local
assets/tools/reference clones; no blanket cleanup. Workers may discard only their
own regenerable attempts, retaining the best source/report and stop reason.
