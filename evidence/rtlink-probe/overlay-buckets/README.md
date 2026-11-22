# RTLink 6.10 overlay relocation ordering

RTLink 6.10 retained the resident ordering rule for an overlaid caller whose
external providers are resident. Each external symbol occupies a contiguous
run in the caller's relocation projection. The run contains LOC2 sites
first, then the segment words of LOC3 operands. Each list preserves raw OMF
FIXUPP traversal, including LEDATA transitions. The paired LOC5 offset word
links to the public's offset and consumes no relocation record.

All 54 links agreed with this rule for 279 caller relocations, including 36
overlay links. The three links of each caller object emitted identical
caller-relative sequences. Each object was linked resident, in its own
overlay with the provider listed first, and in its own overlay with the
provider listed last. X and Y share a provider segment and have distinct
entry offsets. The alias controls add a separate MASM public XADDR at exactly
X's address. Their output starts with the X call bucket, followed by the
XADDR split-address bucket. Thus the resolved segment:offset pair alone
cannot identify a bucket. The tests establish contiguity and within-bucket
order. Their between-symbol permutations are observations of these links.

## Synthetic contrasts

MSC 6.00A profile `msc600a_bound` used `/c /AM /Oe /Gs /Zl /Gc /Zi`.
Passing a far function pointer to an indirect sink emits adjacent `MOV AX`
(LOC5 offset) and `MOV DX` (LOC2 base), then `PUSH DX; PUSH AX`. Direct
calls emit `9A` with a LOC3 operand. The probe verifies both linked words
against the MASM publics, pairs sites one-to-one with on-disk table records,
and checks that no caller-overlay relocation is left over. The matrix varies
one versus two referenced functions, source order, and LEDATA placement.
The `*_kind_*` variants put all calls and all address loads in different
LEDATA chunks. The `*_across_*` variants test repeated references to a
symbol and kind across chunks. The `*_same_*` variants put the relevant
fixups in one verified LEDATA record.

Site offsets below are hexadecimal and relative to the caller's CODE
segment. X2 denotes X's LOC2 site and X3 denotes X's LOC3 segment word.
The raw projection skips the non-relocating LOC5 half.

| Variant | Raw FIXUPP sites in traversal order | Emitted overlay sites |
| --- | --- | --- |
| `cave_same` | X2@0011 X3@000b Y3@0006 | X2@0011 X3@000b Y3@0006 |
| `cave_across` | X3@0008 Y3@0003 X2@0015 | X2@0015 X3@0008 Y3@0003 |
| `temple_same` | X3@0027 Y2@001d X2@0012 X2@0007 | X2@0012 X2@0007 X3@0027 Y2@001d |
| `temple_across` | X2@0007 X3@0030 Y2@0026 X2@001b | X2@0007 X2@001b X3@0030 Y2@0026 |

In `xy_kind_call`, the LOC3 records all come from the earlier LEDATA at
`0000` and the LOC2 records from the later LEDATA at `001b`. The overlay
still emits all of X's LOC2 and LOC3 sites before any Y site. In the Temple
controls, the two X LOC2 sites remain descending when both originate in
one chunk. With the sites in separate chunks, the sequence follows the
earlier chunk first. Both cases keep the complete X bucket contiguous.

## Original sequences

`Oracle.load()` reads the original tables from the SHA-locked PRINCE.EXE.
The receipt includes the complete CS-frame runs, 92 records in overlay 6
(indices 68–159) and 75 in overlay 9 (77–151), with neighboring bytes and
the archived candidate FIXUPP projections from the two worker bindings.
The class and names in the projection come from those bindings. The key
split pairs and direct calls were checked independently in the locked bytes.
Functions A and B below resolve to resident addresses `0169:0358` and
`0169:023a`. The Cave binding names A `ADDFIMGTOFTABLE` and the Temple
binding names it `ADD_FOREGROUND_IMAGE`. Both name B `ADDBFIMGTOFTABLE`.

| Overlay | A LOC2 | B LOC2 | A LOC3 | B LOC3 |
| --- | --- | --- | --- | --- |
| 6, DRAWCAVE | 128–130 | 131–138 | 139–151 | 152–157 |
| 9, DRAWTEMP | 121–131 | 132–143 | 144–148 | 149 |

- **DrawCaveRevPlate.** B's split address at index 137 (`CS:0f64`) and
  direct call at 157 (`CS:0f58`) surround A's call at 150 (`CS:0f36`).
  The archived B FIXUPP projection puts `0f64` before `0f58`. Both full
  A and B groups show the interleaving in the table above.
- **DrawTemplePosts.** A's split sites at 129 (`CS:0bca`) and 130
  (`CS:0bae`) precede B's split site at 142 (`CS:0bf4`) and A's
  direct call at 148 (`CS:0c6b`). The archived candidate's /Zi cut
  gives A's split sites as 130 then 129. The synthetic cross-chunk test
  preserves that kind of reversal, so a historical input with that same
  FIXUPP order would still miss the original pair.

Both original frames project to contiguous runs keyed by (candidate symbol,
LOC). All 21 Cave runs and 21 of 22 Temple runs preserve the archived raw
order by kind. The Temple 129/130 swap is the sole within-kind discrepancy.
The verified RTLink 6.10 rule cannot produce the original interleaving from
one external identity per destination in one caller segment, regardless of
bucket permutation or LEDATA cuts. The split between address and call runs
is a property of these original projections. Other mixed symbols have
adjacent LOC2 and LOC3 runs, and the two affected frames contain direct-call
runs before the interleaved A/B runs. Historical OMF identity and the natural
link inputs needed to account for the original separation remain unknown.

## Reproduction

Run from the repository root.

```powershell
python -B build/workers/w42_ovlprobe/probe.py
```

[receipt.json](receipt.json) records exact argv, environment, response file,
input and output hashes, raw OMF fixups and decoded root and overlay records.
The script locates the overlay descriptor via the MAP public `$$OVLPBLOCK`
and reads physical sections with `tools/probe_rtlink.py`. The manager also
describes a stack-only preload with zero file paragraphs and zero relocations
at EOF. The receipt records that descriptor separately. `tools/mz.py`
decodes the resident tables. All compiles and links are synchronous and use
worker-local staging. The linked programs are structural fixtures.
