# RTLink 6.10 relocation-order contrasts

Run from the repository root:

```
python evidence/rtlink-probe/relocation-order/probe.py
```

The script freshly assembles the four readable MASM sources, compiles a
300-call C source generated from three extern declarations, and links three
root-only synthetic programs under the pinned MS-DOS Player. The compiler and
assembler profiles are checked against `layout/toolchain.lock.json`; the
RTLink executable hash is checked against the existing 6.10 pin. No original
or reconstruction candidate bytes are link inputs. The programs are never
executed. START supplies a normal DOS entry and stack, and all three final
links have zero warnings. The component acceptance subset is not widened to
allow START's MODEND entry or the probe's multiple CODE segments/publics.

The sources separate four possible scopes:

- A has SHARED and SECOND CODE segments. SHARED crosses a MASM LEDATA boundary.
- B contributes more bytes to the same public-combine SHARED output segment.
- DEFS gives PROBE_X and ALIAS_X the same public address, and PROBE_Y another.
- MANY has 300 alternating calls to these names and three compiler LEDATA chunks.

AB links START,A,B,DEFS; BA reverses only A and B. MSC links START,MANY,DEFS.
The checked [receipt](receipt.json) records input/output identities, response
files, segment/symbol bucket sequences, complete-order checksums and short
operand/index sequences. Full diagnostics stay in
`build/workers/wave9_farcall_sol/order-probe/`, including the freshly emitted
objects, executables, maps, logs, generated C, raw-report.json and summary.json.
The checked receipt is a frozen research observation; acceptance never consumes
it or a cached probe object.

In both AB and BA, all SHARED references to PROBE_Y precede SHARED PROBE_X,
then SHARED ALIAS_X. Each bucket includes both modules before the next symbol;
reversing A/B reverses those module contributions inside each bucket. SECOND,
in the same input object A, starts separate Y/X/ALIAS buckets. A's SHARED
references from both LEDATA chunks remain in the same bucket. Thus the observed
scope is the combined output source segment, across input modules and chunks,
rather than the input module, LEDATA record, or whole executable. Names with
equal addresses remain separate symbol buckets.

Every complete module/segment/symbol projection equals raw FIXUPP traversal.
MANY emits chunks at offsets 0,745,1490 (lengths 745,745,12), with respectively
149,149,2 descending far FIXUPPs. RTLink preserves those orders inside each
symbol group, including chunk transitions; MASM's ascending references also
survive. Descending order is a compiler observation, not an OMF/linker rule.

These contrasts justify the component gate's exact site bijection, contiguous
target-symbol projection and within-symbol order. They do not determine a
general symbol-bucket permutation, original symbol names, original linker
version, original TU partition, combined output placement, or whole RTLink
closure. The observed three-name Y/X/ALIAS permutation is not promoted to a
generic ordering algorithm. `GROUPED` reports/recipes retain that structural
debt explicitly; unchanged byte equations and relocation coverage still apply.
