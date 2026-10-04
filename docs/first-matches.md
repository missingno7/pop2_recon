# First function matching work: overlay-2 pair swap

The best function-boundary candidate examined here is `overlay-2:28a1:0e68`, the same byte position as `overlay-2` link segment `0x2344`, offset `0x6438`. The supplied executable identity is SHA-256 `5bf733c56441258e69303102b0e08388484a9400e4d5cdad9bd517478e150314`. Oracle mapping puts the entry at overlay-relative `0x6438` and file offset `0x2c558` (181,592).

The external Supermedo reference checkout at commit `e2c407fa8409ad891bf78c8e98c3e052dd00869b` lists `o2 28a1:0e68` in `recomp/extra_entries.txt`. Independent address arithmetic maps that alias into overlay 2. Linear 16-bit decoding reaches `RETF 2` exactly at byte 44, with no branch leaving the 45-byte extent. The following byte is `90`, then another `55 8b ec` prologue begins. There are no original overlay relocation sites in the 45-byte extent.

The bytes move the two words at near-pointer offsets 0 and 2 to offsets `0x2e` and `0x30`, then exchange the pairs. This supports a structure-pair swap hypothesis. The semantic classification as game logic remains provisional because original names and an independently reviewed caller have not been established.

The isolated source in [swap_candidate.c](../evidence/matching/swap_candidate.c) expresses that pair swap with one local pair. MSC 6.00, flags `/c /AM /Oe /Gs /Zl /Gc`, emits one public at offset zero, a complete initialized CODE segment, no fixups, and a 50-byte CODE extent. It does **not** match. Byte 14 is the first difference: the candidate spills both words to `[BP-4]` and `[BP-2]`; the original keeps the first in `CX` and spills only the second. The candidate also carries a trailing `90` after `RETF 2`. The full 50-byte segment remains the comparison extent; it was not trimmed to make a match.

An alternate scalar-local formulation compiles to a 32-byte segment, and `/O`, `/Og`, `/Ox`, `/Os`, and `/Ot` aggregate variants did not match. The full target bytes, oracle hash, entry evidence, compile receipt, object hash, and exact comparison are in [first_matches.json](../evidence/matching/first_matches.json). No source has been promoted or added to canonical ownership.

The next complete-component comparison should own all 46 original bytes including the observed alignment NOP; the 45-byte instruction-body comparison above is diagnostic only. A register aggregate local still emitted the same 50 bytes under /Oe, /Ox, and /Oe /Or.

The continuation review independently found six relocated direct far calls from
overlay-2 callers `2344:48b4` and `2344:53a0`. Each passes SI once; BP+6 and
RETF2 confirm the callee-cleaned near-pointer ABI. The full 46-byte target is
now reviewed in `evidence/targets.json`; byte45 is the alignment NOP and byte46
begins the next prologue. See [swap-review.json](../evidence/matching/swap-review.json).
Game-helper classification is a medium-confidence hypothesis.

The [reproducible matrix](../evidence/toolchain/swap-matrix.json) records 277 trials
over 39 readable source forms and pinned compiler/profile contrasts. Aggregate,
long, float, union, scalar, qualifier and optimizer forms remain nonmatching.
MSC 5.10 outputs differ, while tested MSC 6.00/6.00A aggregate forms retain both
stack spills. A 46-byte scalar/long form uses DI and a different frame; equal
length is not equality. Complete segments and failures are in
[swap-results.json](../evidence/toolchain/swap-results.json). Nothing was trimmed.

```
python tools/probe_matrix.py evidence/toolchain/swap-matrix.json --worker swap_probe --trial 0
```

Omit `--trial` to rerun the entire diagnostic matrix. No trial grants ownership.
The additional local July 1990 bound 6.00A chain fails with C1059 near-heap
exhaustion; the local 6.00AX chain's selected pass reports it cannot run in DOS.
Neither failure supplies code-generation evidence or silently falls back.
