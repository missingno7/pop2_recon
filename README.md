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
The [bootstrap report](docs/bootstrap-status.md) records the verified target,
compiler comparisons, exact C/runtime totals, remaining debt and next targets.
`layout/manifest.json` is canonical ownership; generated discovery does not own
bytes. `build/validation/report.json` and `build/metrics.json` report fresh local
status. Compiler findings and next targets are recorded in docs/ as evidence
becomes available.

The [current reconstruction milestone](docs/reconstruction-progress.md) includes
fourteen complete overlay C matches, including independently proved overlay
data fields. Totals are 1,174 C bytes, 51 ASM bytes, and 666 pinned runtime bytes.
Fifty-two source data fixups and the first
[runtime near-jump equation](docs/runtime-near-binding.md) are proven; original
relocation coverage remains 0/4,716. Far-call binding and natural RTLink
placement remain unresolved.
