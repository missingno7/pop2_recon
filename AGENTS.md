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
the object proof. For the narrow external DGROUP offset16 mode, provide
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
