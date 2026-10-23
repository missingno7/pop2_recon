# Prince of Persia 2 historical reconstruction

Recover maintainable historical C/ASM that independently recompiles to the
supplied DOS executable's machine code. This repository starts with component
matching; whole RTLink structural closure is a separate investigation. No SDL
port has been started.

Target: local `assets/PRINCE.EXE`, 290,415 bytes, SHA-256
`5bf733c56441258e69303102b0e08388484a9400e4d5cdad9bd517478e150314`.
Its exact release identity is unconfirmed. The supplied `PRINCE2.EX_` differs
at five bytes and has a separate locked identity; matching targets the requested
`PRINCE.EXE` without undoing that difference.

The oracle contains a 144,445-byte resident load image, 16 attached spaces
(overlays 2-17), and 4,716 ordered relocation records. File offsets, link
segment:offset pairs and runtime addresses stay distinct. Overlay spaces overlap
in memory and remain separately identified. See [RTLink evidence](docs/rtlink.md),
[external leads](docs/external-evidence.md), and [sibling workflows](docs/sibling-workflows.md).

Run from the repository using Python 3.10 or newer:

```
python tools/inventory.py
python tools/oracle.py --extract
python tools/discover.py
python tools/context.py root:2203:0678 --asm
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

Validation on 2026-10-23 records 56 complete C components (2,189 bytes), one ASM
component (51 bytes), and 21 pinned runtime members (1,166 bytes). Eighteen
complete overlay C components are included. There are 92 proven object fixup
fields and 11 covered original relocation records. See
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
