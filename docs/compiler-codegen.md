# Historical compiler and OMF probes

`tools/compiler.py` runs external, locally installed Microsoft C compilers through
the pinned MS-DOS Player. `layout/toolchain.lock.json` records the runner,
compiler pass paths, and SHA-256 values. The executable files stay outside the
repository. `verify_profile()` checks every pinned file before each compile, and
`compile_c()` checks the compiler output again as a strict 16-bit OMF object.

The verified runner arguments are `-e -v5.00`. Quote `-v5.00` as one native
argument in PowerShell: an unquoted call can split that token. Compiler switches
follow the path to `CL.EXE`. The working probe flags are `/c /O /AS /Gs /Zl`.
They request near-model optimized code, suppress stack-probe calls, and omit
default-library directives. The separate `/AL` profile remains available as a
large-model contrast. These are experimental recipes; no probe establishes the
original game's model or complete translation-unit settings.

The current profiles pin compiler executables and runner files, but no C headers.
The driver therefore refuses source-level `#include` directives instead of
letting CL search an unpinned include directory.

Run the probe set with:

```powershell
python tools/probe_toolchain.py
```

The script compiles independent increment, external-call, signed and unsigned
branch, switch, far-pointer, long multiply, long divide, and register-local
sources under MSC 5.10 and MSC 6.00. It also compiles increment and external-call
controls under the nearby MSC 6.00A C2L-bound tree, plus medium-model `/AM`
switch and far-pointer contrasts under 5.10 and 6.00. Generated sources, objects,
logs, and receipts stay under ignored `build/workers/toolchain/`. The structured
report is [probes.json](../evidence/toolchain/probes.json).

The 5.10, 6.00, and 6.00A compilers all ran when MS-DOS Player was given the
quoted `-v5.00` switch. MSC 6.00A's `/AL` control timed out after its version
banner; the near-model C2L-bound probes completed. The general profiles in
`recipes/compiler-profiles.json` are keyed to the tested profile names; its
separate `variants` object defines switch sets without claiming historical
provenance.

For `int increment(int value) { return value + 1; }` under
`/c /O /AS /Gs /Zl`, each version emitted a complete ten-byte `_TEXT` extent:

| Compiler | Public | Complete CODE bytes | Object SHA-256 |
| --- | --- | --- | --- |
| MSC 5.10 | `_increment` | `55 8B EC 8B 46 04 40 5D C3 90` | `5ec84e059f1cd9c959eabcc5e288fbe6f3580d2eacb38b0daa1b4d04c91e0a62` |
| MSC 6.00 | `_increment` | `55 8B EC 8B 46 04 40 5D C3 90` | `c87d00c39111b76ae42a6bdfacf5fce4159cbfbc4b85ebb9c97b938183a2c6e1` |
| MSC 6.00A C2L-bound | `_increment` | `55 8B EC 8B 46 04 40 5D C3 90` | `c87d00c39111b76ae42a6bdfacf5fce4159cbfbc4b85ebb9c97b938183a2c6e1` |

All three emitted the same initialized CODE bytes. Their OMF metadata differs:
5.10 retains external declarations for `__acrtused` and `_increment`; 6.00 and
6.00A retain `_increment`. Each also emits a FIXUPP record containing only
thread definitions. The reader preserves those declarations and records.

The report also contains a bounded MSC 6.00 Pascal-call experiment
(`/c /AM /Oe /Gs /Zl /Gc`) against the increment control. It compiles and
publishes the name `INCREMENT` rather than `_increment`, consistent with the
Pascal naming switch. The recipe scopes this hypothesis to the accepted floor
candidate family; the control does not establish that setting as a global game
profile.

The separate `external_call.c` probe provides a real fixup control. Under the
near-model flags, `invoke_external` has a near-offset fixup at CODE offset 7.
The target is `_helper`, represented by EXTDEF index 2 in 5.10 and index 1 in
6.00/6.00A. The 32-bit multiply and divide probes also emit helper-call fixups.
These objects expose binding work and are not treated as final code bytes.

`tools/omf.py` is a small independent reader, written for this project from the
published 16-bit OMF record layout. It verifies record lengths and checksums,
keeps the complete ordered record list, reads SEGDEF extents, PUBLIC/EXTDEF and
GRPDEF declarations, expands LIDATA, records initialized ranges, and preserves
and decodes FIXUPP thread/fixup subrecords. It rejects 32-bit records, unknown
storage records, repeated SEGDEF names, overlapping data records, data beyond a
declared segment, records after MODEND, and incomplete modules. A bytearray's
zero fill is not treated as initialized data: callers must check
`initialized_ranges`.

The reader does not perform linker frame/target thread rebinding or apply
relocations. FIXUPP thread definitions are identified separately from actual
fixup fields; an unanchored pre-data FIXUPP record stays marked as unanchored.
`tools/inspect_omf.py OBJECT.OBJ` prints the complete structure, ranges,
declarations, fixup records, and record types. The candidate gate remains
fail-closed for actual fixups until symbolic binding has its own proof.

`tools/compiler.py` exposes:

```python
verify_profile(profile) -> dict
compile_c(source: str | Path, profile: str, flags=None, *, workdir=None, ...) -> CompileResult
extract_function(module_or_result, public_name) -> bytes
```

`CompileResult.obj` is the generated object path; `obj_bytes` and `parsed` expose
its verified bytes and OMF view. `source_sha256` fingerprints the supplied source
file bytes, while `staged_source_sha256` fingerprints the CRLF DOS input.
`extract_function()` returns the whole owning
initialized CODE extent only when one PUBLIC begins at offset zero, other
SEGDEFs are empty, all bytes are covered without gaps, and there are no actual
fixup fields. It never accepts a caller-supplied size or trims to an oracle
length. External declarations and thread-only records remain inspectable.
