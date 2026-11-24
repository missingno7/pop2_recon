# Prince of Persia 2 historical reconstruction

Recover maintainable historical C/ASM that independently recompiles to the
supplied DOS executable's machine code. This repository starts with component
matching; whole RTLink structural closure is a separate investigation. No SDL
port has been started.

Target: local `assets/pop2_11/PRINCE.EXE` (v1.1, the last release), 259,583 bytes, SHA-256
`c92298b62fc04fe66a0270fbaf082d7d7a5e37d0b5b7ed5adafccd43b26a67d7`, with its asset directory locked in
`layout/oracle.lock.json`. Until 2026-11-23 the target was the leaked pre-1.0 build in `assets/DOS`; that
reconstruction (1,565 exact components) is frozen at tag `oracle-dos-1565`.

The oracle contains a 151,979-byte resident load image and 16 attached spaces (overlays 2-17); the 14
compressed overlays are expanded by the original's own expander (`tools/overlay_expand.py`). File offsets, link
segment:offset pairs and runtime addresses stay distinct. Overlay spaces overlap
in memory and remain separately identified. See [RTLink evidence](docs/rtlink.md),
[external leads](docs/external-evidence.md), and [sibling workflows](docs/sibling-workflows.md).

Run from the repository using Python 3.10 or newer:

```
python tools/inventory.py
python tools/oracle.py --extract
python tools/discover.py
python tools/context.py root:220e:0678 --asm
python tools/runtime_scan.py C:/tools/msc-6.00/LIB/mlibcr.lib --output build/workers/review/runtime.json
python tools/metrics.py
python tools/validate.py
```

Historical tools are read from `C:\tools`; hashes are pinned in
`layout/toolchain.lock.json`. Capstone 5.0.3 at `C:\tools\capstone-5.0.3` provides
diagnostic disassembly. Tool binaries, original assets, reference checkouts and
generated bulk output are ignored. The small function targets, sources, recipes,
proof tools, tests and durable evidence are tracked.

Follow [AGENTS.md](AGENTS.md) for bounded worker waves, optional `search.py --trial`
output deduplication, isolation and serialized publication, and
[acceptance.md](docs/acceptance.md) for exactness and current gate limits.
The [bootstrap report](docs/bootstrap-status.md) records the initial target and
compiler comparisons; its ownership totals and next steps are historical.
`layout/manifest.json` is canonical ownership; generated discovery does not own
bytes. `build/validation/report.json` and `build/metrics.json` report fresh local
status. Compiler findings and next targets are recorded in docs/ as evidence
becomes available.

Validation on 2026-11-21 records 58 exact C functions (2,371 bytes), one ASM
component (51 bytes), and 21 pinned runtime members (1,166 bytes). The C functions
include the first [same-source group](docs/acceptance.md#same-source-groups): two
functions linked by a same-file near call, from one compilation. Eighteen
complete overlay C components are included. There are 11 covered original
relocation records. See
[current progress](docs/reconstruction-progress.md) for the dated baseline;
the manifest and a fresh validation report take precedence over these snapshots.
[Root far-call binding](docs/far-call-binding.md) now supports exact
within-symbol relocation order with explicit GROUPED structural debt for
between-symbol permutations. Original relocation coverage is 11/4,716;
overlay vectors, original TU identity and natural RTLink placement remain open.

The [convergence audit](docs/convergence-audit.md) compares the current blockers
with SimAnt's workflow, confirms declaration-context sensitivity under our
compiler flags, and records a working AX contrast and the next proof priorities.
The [state-step TU investigation](docs/tu-state-step.md) tests an original
caller/callee cluster with isolated compiler controls; its grouping remains
research evidence with no additional accepted ownership.
The [larger-function follow-up](docs/larger-function-frontier.md) adds a complete
160-byte fallback through the unchanged gate and identifies the remaining
dependencies of its 122-byte caller.
