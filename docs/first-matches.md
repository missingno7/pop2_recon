# First function matching work: overlay-2 pair swap

The best function-boundary candidate examined here is `overlay-2:28a1:0e68`, the same byte position as `overlay-2` link segment `0x2344`, offset `0x6438`. The supplied executable identity is SHA-256 `5bf733c56441258e69303102b0e08388484a9400e4d5cdad9bd517478e150314`. Oracle mapping puts the entry at overlay-relative `0x6438` and file offset `0x2c558` (181,592).

The external Supermedo reference checkout at commit `e2c407fa8409ad891bf78c8e98c3e052dd00869b` lists `o2 28a1:0e68` in `recomp/extra_entries.txt`. Independent address arithmetic maps that alias into overlay 2. Linear 16-bit decoding reaches `RETF 2` exactly at byte 44, with no branch leaving the 45-byte extent. The following byte is `90`, then another `55 8b ec` prologue begins. There are no original overlay relocation sites in the 45-byte extent.

The bytes move the two words at near-pointer offsets 0 and 2 to offsets `0x2e` and `0x30`, then exchange the pairs. This supports a structure-pair swap hypothesis. The semantic classification as game logic remains provisional because original names and an independently reviewed caller have not been established.

The isolated source in [swap_candidate.c](../evidence/matching/swap_candidate.c) expresses that pair swap with one local pair. MSC 6.00, flags `/c /AM /Oe /Gs /Zl /Gc`, emits one public at offset zero, a complete initialized CODE segment, no fixups, and a 50-byte CODE extent. It does **not** match. Byte 14 is the first difference: the candidate spills both words to `[BP-4]` and `[BP-2]`; the original keeps the first in `CX` and spills only the second. The candidate also carries a trailing `90` after `RETF 2`. The full 50-byte segment remains the comparison extent; it was not trimmed to make a match.

An alternate scalar-local formulation compiles to a 32-byte segment, and `/O`, `/Og`, `/Ox`, `/Os`, and `/Ot` aggregate variants did not match. The full target bytes, oracle hash, entry evidence, compile receipt, object hash, and exact comparison are in [first_matches.json](../evidence/matching/first_matches.json). No source has been promoted or added to canonical ownership.

The next complete-component comparison should own all 46 original bytes including the observed alignment NOP; the 45-byte instruction-body comparison above is diagnostic only. A register aggregate local still emitted the same 50 bytes under /Oe, /Ox, and /Oe /Or.
