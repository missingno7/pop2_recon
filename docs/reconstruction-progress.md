# Reconstruction continuation — 2026-10-04

The immutable target remains `assets/PRINCE.EXE`, SHA-256
`5bf733c56441258e69303102b0e08388484a9400e4d5cdad9bd517478e150314`.
Release identity is still unconfirmed. Root/overlay structure, all 62 asset
identities, and the 4,716 relocation records remain unchanged. The
[bootstrap report](bootstrap-status.md) is the earlier milestone snapshot.

## First fixup-bearing C component

`root:0000:303a` owns the independently reviewed 26-byte helper ending in
RETF 2 at `0x3054`. A relocated far caller passes a signed word. The accepted
[apply_adjustment.c](../src/apply_adjustment.c) conditionally negates that word,
adds it to a volatile word, and returns the resulting word. Names, qualifiers
and gameplay purpose remain hypotheses.

Pinned MSC 6.00 `/c /AM /Oe /Gs /Zl /Gc` reproduces its complete CODE extent.
The new [DGROUP proof](dgroup-binding.md) checks twenty ordinary bytes directly
and three two-byte external offset fields by explicit linker equations. It
preserves raw OMF bytes and hashes, freezes declarations/ordered fixups, and
rechecks the original startup DS load and independent data references. The
binding file and source are pinned by the canonical recipe. No data bytes,
original symbol names, original TU or natural final placement are claimed.

Independent synthetic links with MS LINK 3.65 and RTLink Plus 6.10 confirm
data-offset, near-relative and far-pointer equations. A dedicated MSC 6.00
F5/T6 probe verifies the precise zero-addend/omitted-displacement mode used by
this helper. The paragraph frame is distinct from a map's first initialized
group byte. Only the external DGROUP mode is enabled for acceptance; the other
linker observations remain diagnostic. A complete 13-byte `__dos_close` library
member is a promising next near-call target, with unresolved independent target
symbol/frame proof and zero ownership.

## First accepted ASM component

`root:0000:1344` owns the complete 51-byte range `[0x1344,0x1377)`.
Its relocated far caller at root position `0x1f5b4`, preceding argument pushes,
BP-frame loads and RETF 8 establish a far-buffer/two-word ABI. A separate
prologue starts immediately at 0x1377; no internal relocation is present.

Readable [reverse_block_bytes.asm](../asm/reverse_block_bytes.asm), freshly
assembled with pinned MASM 5.10 under MS-DOS Player, emits exactly all 51 bytes.
The strict recipe pins the canonical source and entire canonical OMF object;
the object has one complete initialized CODE segment, one public at zero,
empty secondary DATA, no externals and no fixups. The
[worker submission](../evidence/matching/reverse-block-bytes.json) independently
records caller/boundary and initial object evidence. Its REVBLOCK module basename
differs from canonical UNIT.ASM; validation pins the canonical OMF identity.

DS switching, LES, STOSB, LOOP and the AH/AL byte exchange support a handwritten
assembly hypothesis. MASM 5.10 reproduces the component; the original assembler
revision, source symbol, source-unit membership and final placement are unknown.
The semantic name suggests reversing each fixed-length buffer block. This is
CUSTOM_ASM with unresolved game/resource subsystem, separately counted as ASM.
It does not enlarge the identified game-code subtotal.

The source preserves the original word counter wrap behavior and lacks CLD.
DF=0 and valid-input preconditions have not been established by runtime traces;
no behavioral-validation claim is made.

## Grinder and evidence

Search, promotion and validation now support `--language asm --profile masm510`.
The assembler verifies tool hashes, stages ASCII/CRLF source in scratch, refuses
unpinned includes and extra output switches, removes stale objects before each
run, and fails closed on missing output, timeout, malformed OMF or differing
full extents. The existing byte/fixup/relocation acceptance gate is retained.

[Symbolic OMF resolution](omf-fixup-resolution.md) preserves all raw FIXUPP
records and resolves thread, frame and target identities for inspection. Actual
MSC 5.10/6.00 near-call, far-call and initialized-global probes verify field widths,
LEDATA-based offsets, self-relative calls, external symbols and DGROUP frames.
Undefined threads/indexes, unsupported methods and out-of-bounds fields fail.
The resolver does not apply relocations. The separate narrow binding gate above
is the only supported fixup-bearing acceptance mode.

Discovery now normalizes external `o2` names to Oracle `overlay-2`. A bounded
CFG pass links relocated calls missed by the linear sweep, probing only explicit
extra-entry targets with multiple relocated calls. It recovers the swap's six
calls from two callers. Same-space context aliases share caller and ownership
metadata while retaining the requested CS:IP in disassembly. The regenerated
inventory has 2,263 tentative entries and 1,354 call edges; no discovery row
grants ownership.

The overlay 2 swap now has six independently verified far calls and a reviewed
46-byte complete target. [277 recorded trials](first-matches.md) over 39 source
forms remain mismatches. Two additional local MSC revision chains failed to
produce objects; their pins and failure logs remain explicit hypotheses.
The bundled primary MSC help confirms the meanings of /Oe and /Or; no global
historical flag family is inferred from these experiments.

## Current ownership and debt

| Category | Bytes |
| --- | ---: |
| Matching C | 46 |
| Matching ASM | 51 |
| Pinned runtime | 653 |
| External drivers | 0 |
| Unresolved identified game helper | 46 |
| Unknown/unclassified payload | 270,592 |

There are three accepted functions and ten pinned runtime components. All accepted
bytes remain resident. The identified game subtotal is 92 bytes, including the
medium-confidence 46-byte swap hypothesis; 46 of those bytes are accepted.
No overlay has accepted ownership, and generated relocation coverage remains
0/4,716. Three candidate offset fixups are proven and generate no MZ relocations;
these counts are reported separately. Natural RTLink linkage and original runtime
traces remain unresolved.

The next highest-leverage work is extending independently grounded binding to
near/far calls, beginning with the library close/return pair, then expanding the
small data-helper family. The swap needs a new source/TU or compiler-generation lead:
the documented aggregate and scalar families do not explain its CX/stack split.
RTLink version, TU grouping and runtime-derived descriptor location remain
separate structural investigations.

Final `python tools/validate.py` passes all 85 invariant tests, freshly compiles
both C functions, freshly assembles the ASM function, verifies all ten independent
runtime members, and rechecks every original asset/structure and tool pin.
Source, object, byte extent, relocation and ownership checks all pass.
