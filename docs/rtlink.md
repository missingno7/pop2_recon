# RTLink overlay structure investigation

## The v1.1 target (2026-11-23)

The project target is `assets/pop2_11/PRINCE.EXE` (v1.1, 259,583 bytes, SHA-256 `c92298b6...`), locked in
`layout/oracle.lock.json`. The earlier target (the leaked pre-1.0 "IR" build, binary-patched in `SHOW_DIALOG_NOTICE`)
is frozen at tag `oracle-dos-1565`; the text below this section describes that file.

- MZ root image 151,979 bytes after an 8,192-byte header; entry `220e:0678`; 2,036 MZ relocations. The overlay table
  (16 records of 18 bytes, IDs 2 to 17) is at file offset 0x24bfb = 2000h + 220eh*16 + 0b1bh, the location the manager's
  `getsectionptr` computes. The first overlay relocation block starts at 160,176, after five padding bytes.
- Descriptor flags: 24h (overlays 2 to 15: code, compressed), 04h (overlay 16: code, stored), 01h (overlay 17: preloaded
  data). Field `f8` is the memory extent and the last field the file extent, both in paragraphs; relocation tables are
  stored uncompressed and name positions in the expanded image.
- A compressed payload is expanded by the original `$$RTLEXPANDSECTION` at `22f2:1296`, which OVLMGR calls (root
  `220e:030b` tests `cs:[bx+7],20h`) instead of reading the overlay itself. The expander is an adaptive-Huffman LZ decoder
  (stream magic 9CB5h). `tools/overlay_expand.py` runs those original instructions in a CPU emulator, requires that they
  write one contiguous block filling the declared memory paragraphs, and compares it with the SHA-256 pins in
  `layout/oracle-format.json`. Every overlay relocation lies inside the expanded image.
- DGROUP is paragraph `3302` in overlay 17 (position 0x2940 of its payload); overlay 17 again ends one byte short of its
  declared last paragraph.

Evidence carried over from the earlier build was translated by instruction alignment (1,654 of 1,683 reviewed targets
located) and every binding, owner and runtime pin was proven again from the v1.1 bytes by the unchanged gates.

## Whole link of v1.1 (2026-11-24)

`python tools/whole_link.py` compiles every accepted component under its module basename, adds the data modules of
`link/` and links everything once with pinned RTLink/Plus 6.10, the unmodified `MLIBCR.LIB` and `LIBH.LIB`, and the script
described by `layout/link-plan.json`. It compares the result with the original per space and grants no ownership.
`--check` fails when the result is worse than `evidence/whole-link.json`; the integrating writer updates that record with
`--record`. A new or renamed component must also be added to the plan (`tests/test_whole_link.py`).

Result: 279,812 of 280,476 image bytes equal. The MZ header, the descriptors (except file positions after overlay 11), all
link segments, section sizes, relocation counts and relocation sets equal the original, and the compressed payloads of
overlays 2-10 and 12-15 are identical. All differences lie in three functions whose C is open (DUMP_HEAP,
CONVERT_SOUND_HANDLE, HEAD_VISIBILITY). The order of the relocation records still differs (symbol ids, o9/o11).

Facts the link settled:

- **Libraries.** RTLink loads the members a library pass needs in library order and searches the libraries in order, then
  again. `GAMEA, MLIBCR, LIBH, GAMEB` with the members in code order reproduces every address of root, DGROUP and far
  data: the C runtime data sits between the two game libraries' data, its last members (strlen, strchr, strdup, then
  `_fstrcmp`) come from the second pass. No order is forced; a few names are only declared by some original module
  (`link/mode_table.asm`, plan `force`).
- `COMPRESS ON` inside each overlay section (not in the root, not for the preloaded section) reproduces the compressed
  payloads, including overlay 16 staying stored because it does not shrink. `STACK 1500` (hexadecimal).
- A call goes through an overlay vector only when its external *name* is the vectored one (`VECTOROFF`, `ALWAYS`), so a
  caller's descriptive alias calls the overlay directly. Validation refuses such aliases (`vector-alias`, and `data-alias`
  for data publics). PRINCE.C calls `show_init_message` (overlay 2), TOP.C `reset_overlay_state` (overlay 3); both are
  251b:0000.
- `220d:0000` is the C runtime's `EMULATOR_TEXT` (crt0dat.asm); its word is the empty far segment `EMULATOR_DATA` (313b).
  The 16 zero bytes before `_TEXT` are the DOSSEG padding.
- DGROUP after `_DATA`: the EMS/XMS workspace `BACKING_DATA` 2696 (COMMON), XIQC 26a4, DBDATA 26a6, CDATA 26b4, XP 26c2,
  CONST 26c6, HDR 26de, MSG 26e6, PAD, EPAD, `_BSS` 27c0, c_common 2b90, `_end` 6b74, 512 bytes of class STACK (two
  interrupt stacks), STACK 6d80 (1500h bytes).
- `expand.asm` (7,317 bytes at root 23140) is pulled in by the compressed sections. The shipped 6.10 member differs in two
  immediates (+1c27, +1c37) and `rtcommon` is 467 bytes instead of 495: the original used another RTLUTILS.LIB revision.
  The plan's `rtlutils` patches are labelled stand-ins for those two members.

Open: 473 variables no accepted source defines (the generated `VARIABLES` module gives them their accepted addresses), the
plan's `fills` (statics of five modules and 14 missing bytes of the CONVERT_SOUND_HANDLE draft), the declaring module of
the interrupt stacks and of the `link/` data, the two RTLUTILS members, and the relocation order.

## The earlier build (tag oracle-dos-1565)

Direct byte inspection confirms that `assets/DOS/PRINCE.EXE` has a conventional MZ root image of 152,637 bytes, followed by 137,778 bytes of appended overlay material. The root ends at byte `0x2543d`; the first overlay relocation block begins at 152,640, after three zero padding bytes. The overlay descriptor table is located inside the root at decimal offset 150,347 (`0x24b4b`), and contains 16 records of 18 bytes, IDs 2 through 17. The earlier offset `0x24a8b` is 192 bytes before that table and is not its start.

Each descriptor decodes as `<HHHBBHHHHH>`: link segment, filename-record pointer `f2`, relocation position in paragraphs, high relocation-position byte, flags, memory size in paragraphs `f8`, relocation count, ancestor table index `fc`, overlay ID `fe`, and file size in paragraphs. The relocation byte offset is `(pos + hi*65536) * 16`. A relocation record is four bytes (`<HH>`: offset, segment). The relocation area occupies `ceil(nrel/4) * 16` bytes, so code starts at `relocation_offset + ceil(nrel/4)*16`; the code length is `size*16` bytes. For each relocation, the site inside code is `(rseg-link_segment)*16 + roff`. The original relocation loop derives its addend from saved startup ES plus `0x10`; the fixed `0x0100` in the reference loader is an observation model. The original skips a write when adding the record segment to that base carries. This is static instruction evidence, without a runtime trace.

Concrete first entry (overlay 2): descriptor at 150,347; link segment `0x2344`; relocation position 152,640; flags 4; 821 relocations; ID 2; 1,675 paragraphs of code. Since `ceil(821/4)*16 = 3,296`, code starts at 155,936 and occupies 26,800 bytes, ending at 182,736, the next overlay's relocation area. The independently parsed table has 2,687 relocation records total; every relocation site is within its overlay code extent, and all overlay extents meet exactly.

The final row (overlay 17) begins its relocation block at 268,656; its 161 relocation entries occupy 656 padded bytes, so code begins at 269,312. The descriptor declares 1,319 paragraphs (21,104 bytes), ending at 290,416, while the physical file ends at 290,415. Both `PRINCE.EXE` and `PRINCE2.EX_` have this exact one-byte terminal shortfall. The two files have identical bytes from the root-image boundary onward; they differ in five root-image bytes at offsets 34,157, 34,158, and 34,161 through 34,163.

A real RTLink Plus 6.10 run through the local MS-DOS Player produced a small two-overlay executable from a Microsoft C 5.10 resident entry and two MASM far-return functions. Its table at offset 4,763 (`0x129b`) uses the same 18-byte record layout: overlay IDs 2 and 3, zero relocations, and one paragraph each, with file extents `[6496,6512)` and `[6512,6528)`. The MZ root ends at 6,487, leaving nine bytes before overlay data. Reproducible assembly/link sources and command details are in `evidence/rtlink-probe/README.md`; the output hash and exact descriptor values are in `evidence/rtlink-layout.json`.

The MSC 5.10 object requested `LLIBCE.LIB`, absent from the local installation. For this layout-only probe, the installed `LLIBCR.LIB` was copied under the expected name in the scratch directory, alongside RTLink 6.10's own `RTLUTILS.LIB`. The executable linked without diagnostics, but this alias is not evidence of a faithful runtime-library closure. The installed RTLink Plus 4.00 binary identifies itself as Version 4.00, and its bundled sample link log has the same banner. Attempts to run a comparable 4.00 response-file probe fell back to positional prompts and emitted no output, so the synthetic 4.00 case remains unresolved.

The [original manager proof](../evidence/rtlink-manager.json) independently locates this table. MZ entry `2203:0678` saves startup ES and loads the relocated manager selector. Initialization reads count 16 at `0b17`; `getsectionptr` at `0587` computes `BX = 18 * AX + 0b1b`. Thus `8192 + 0x2203*16 + 0x0b1b = 150347`. All sixteen destination-address fields have ordered original MZ relocations 1948 through 1963; observing them grants no reconstructed relocation ownership.

The original consumes these records directly. Offset `+8` controls memory extent; `+16` controls file extent. Overlay 17 declares 3,026 memory paragraphs and 1,319 file paragraphs, leaving a 27,312-byte memory remainder handled by the static clearing path. That remainder is separate from the one missing physical payload byte. The read-success path ignores the returned byte count, so this review does not prove that missing byte initialized. Ancestor index `+12` and user section number `+14` are separate: rows with user IDs 8 and 9 point to table index 5, whose user ID is 7. The filename pointer refers to a flag byte followed by `POP.EXE` and NUL.

Pinned local source hints remain separate from original facts. RTLink 6.10's non-debug `OVLMGR.INC` defines the same 18-byte `info_section`, including separate memory/file sizes; the supplied 4.00 non-debug structure has 16 bytes. The original stride, file-size read and compressed-loader branch distinguish these supplied structures. They do not identify the exact historical linker or prove an unmodified manager source. The original-instruction table locator is now established; original manager/member identity, natural linker placement, arbitrary CRT provider frames, whole-link closure and behavior remain open.
