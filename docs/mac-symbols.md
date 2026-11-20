# Macintosh MacsBug symbols — 2026-11-21

The supplied Mac release (`assets/mac/*.sit`, StuffIt 5, Arsenic compression) was
unpacked with The Unarchiver's command-line `unar`. It is the official Windows
build from `cdn.theunarchiver.com/downloads/unarWindows.zip`, SHA-256
`61a6b299…12e0dd`, installed in `C:/tools/unar-windows`. Resource forks are kept
as visible AppleDouble `.rsrc` files under ignored `build/references/mac/`.
Nothing from the archive was executed.

```
C:/tools/unar-windows/unar.exe -k visible -o build/references/mac assets/mac/<archive>.sit
python tools/mac_symbols.py
```

## What it holds

The application's resource fork has 24 CODE resources. THINK C/MPW stored a
MacsBug name after each function's return. [symbols.json](../evidence/mac/symbols.json)
lists **1,377 function names** by CODE segment, in segment order:

- **Game segments:** Main, Characters 1/2, Modular Set 1/2, Ruins, Caverns,
  Final Battle, Temple, Desert, Rooftops, NISs, PoP Tools.
- **Mac platform libraries:** Mohawk Libs 1–4, Mac Libs 2, Anim & Audio. These
  include C++ mangled names.

Mac segments are memory-management groupings of several source files, not files
themselves. The FM Towns module list in [fmtowns-symbols.md](fmtowns-symbols.md)
is the source-file authority between the two.

## How it compares with FM Towns

- **554 names appear in both builds.** They are strong cross-platform evidence
  for the original function name.
- **823 names are Mac-only.** They include static functions, which a public symbol
  table cannot list, and Mac platform code.
- **791 names are FM Towns-only.** They include FM Towns platform code and
  functions absent from, or renamed in, the Mac build.

Both builds are later than the DOS 1.0 target, and neither shares DOS bytes. A
name becomes a DOS name only with an independent anchor in PRINCE.EXE.
