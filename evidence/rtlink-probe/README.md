# RTLink 6.10 controlled overlay link

This fixture keeps a C entry point and two far-return assembly functions tiny enough that the emitted overlay records are easy to inspect. `resident.c` compiles with the pinned Microsoft C 5.10 profile using `/c /O /AL`. `ovone.asm` and `ovtwo.asm` assemble with Microsoft MASM 5.10 using `MSDOS.EXE -e MASM.EXE ovone.asm,ovone.obj,NUL,NUL;` (likewise for `ovtwo`). The response file links the C module in the root and each assembly module in its own overlay section.

The successful local RTLink 6.10 invocation was run with current directory `build/workers/rtlink-probe/v6`, `PATH` and `MSDOS_PATH` set to that directory, and `TEMP`, `TMP`, and `MSDOS_TEMP` set to `.`:

```
C:\tools\nmlgcdos\msdos.exe -e C:\tools\rtlink-plus-6.10\installed\RTLINK.EXE @PROBE.ARF
```

The runner SHA-256 is `f7f6cb0a3e816c5edb13112d327c1bddbf7463fe7bf9a005ca1eb5317751bd02`; RTLINK.EXE is `ea6f57a470a26801086593d54aa99f6bf983dd024fc19b5e13361a07f43e17fb`; output `PROBE.EXE` is SHA-256 `b24f859cade94fbb3bdf31760dfc40ace748a33ba3542a111b018c9fd4327156`. The linker emitted no unresolved-symbol or duplicate-start warnings.

This machine did not contain `LLIBCE.LIB`, which the MSC510 object requests. For this structural probe only, `C:\tools\msc-5.10\LLIBCR.LIB` was copied in the scratch directory under the expected filename `LLIBCE.LIB` (source SHA-256 `7961eafce689dbf17acc3fd6fe34be6bc000f60d0f02fb1af2602b09112f3d2e`). RTLink 6.10's own `RTLUTILS.LIB` was also copied to scratch (SHA-256 `c718bac09b0b1204a259494446acfa8bfffb129144cc7d36799855621f4fdccb`). Do not treat the aliased runtime as an authentic toolchain closure; it only let the linker emit and validate its overlay layout.

`tools/probe_rtlink.py build/workers/rtlink-probe/v6/PROBE.EXE --descriptor-offset 4763 --count 2` validates the emitted 18-byte descriptors. The table at 4763 (`0x129b`) contains IDs 2 and 3, each with zero relocations and one paragraph of code. Their file ranges are `[6496,6512)` and `[6512,6528)`. The MZ root ends at 6487, leaving nine bytes of alignment before the first overlay.

The local RTLink 4.00 binary identifies itself as Version 4.00 and its bundled sample `LINKOUT2` contains the same version banner. A minimal 4.00 response-file invocation was attempted in scratch but its input was interpreted as positional prompts and produced no executable, so this note does not claim an equivalent 4.00 probe. Bundled 4.00 and 6.10 overlay-manager source files describe an in-memory `info_section` record; that manager structure is distinct from the emitted on-disk descriptors.

The compiler wrapper call that produced the resident object was:

```
python -c "import sys; sys.path.insert(0, 'tools'); from compiler import compile_c; from pathlib import Path; r=compile_c(Path('evidence/rtlink-probe/resident.c'), 'msc510', flags=['/c','/O','/AL'], basename='PROBE', workdir=Path('build/workers/rtlink-probe/rootc')); print(r.ok, r.source_sha256, r.obj_sha256)"
```

The matching MASM calls, from `build/workers/rtlink-probe`, were:

```
C:\tools\nmlgcdos\msdos.exe -e C:\tools\masm-5.10\files\MASM.EXE ovone.asm,ovone.obj,NUL,NUL;
C:\tools\nmlgcdos\msdos.exe -e C:\tools\masm-5.10\files\MASM.EXE ovtwo.asm,ovtwo.obj,NUL,NUL;
```

A first 6.10 probe with an assembly root was discarded after it produced duplicate start and unresolved `_main` warnings. The final link used the C `main` fixture above, and the final link log has no warnings.
