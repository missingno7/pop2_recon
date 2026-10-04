# Reconstruction continuation — 2026-10-04

The immutable target remains `assets/PRINCE.EXE`, SHA-256
`5bf733c56441258e69303102b0e08388484a9400e4d5cdad9bd517478e150314`.
Release identity is still unconfirmed. Root/overlay structure, all 62 asset
identities, and the 4,716 relocation records remain unchanged. The
[bootstrap report](bootstrap-status.md) is the earlier milestone snapshot.

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
This does not apply relocations or accept fixup-bearing components.

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
| Matching C | 20 |
| Matching ASM | 51 |
| Pinned runtime | 653 |
| External drivers | 0 |
| Unresolved identified game helper | 46 |
| Unknown/unclassified payload | 270,618 |

There are two accepted functions and ten pinned runtime components. All accepted
bytes remain resident. The identified game subtotal is 66 bytes, including the
medium-confidence 46-byte swap hypothesis; only 20 of those bytes are accepted.
No overlay has accepted ownership, and generated relocation coverage remains
0/4,716. Natural RTLink linkage and original runtime traces remain unresolved.

The next highest-leverage work is independently grounding symbolic call/DGROUP
bindings so fixup-bearing C helpers and CRT startup can be compared without
masking fields. The swap needs a new source/TU or compiler-generation lead:
the documented aggregate and scalar families do not explain its CX/stack split.
RTLink version, TU grouping and runtime-derived descriptor location remain
separate structural investigations.

Final `python tools/validate.py` passes all 67 invariant tests, freshly compiles
the C function, freshly assembles the ASM function, verifies all ten independent
runtime members, and rechecks every original asset/structure and tool pin.
Source, object, byte extent, relocation and ownership checks all pass.
