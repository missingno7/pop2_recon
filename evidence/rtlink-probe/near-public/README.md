# Source-only same-public near-call controls

Run `python evidence/rtlink-probe/near-public/probe.py` from the repository.
All output goes to `build/workers/near_public/`; the command freshly compiles
and assembles source, links four controls, and inspects the generated files.
It never runs the generated executable or reads original bytes as link input.
The compact checked [receipt](receipt.json) pins sources, complete objects,
compiler flags, runner, linker and emitted executables. Full raw declarations,
FIXUPP records and logs remain in scratch.

| Control | Observation |
| --- | --- |
| Combined caller/callee, 34 bytes | F5/T6 self-relative LOC1 at +18 resolves to +2 |
| Four-byte prefix before combined source | Whole contribution moves +4; the relative field stays +2 |
| Separate caller/callee objects, combined UNIT_TEXT | Caller retains five-byte CALL FAR, plus its segment-word MZ relocation |
| Callee definition before caller, 34 bytes | Relative field at +30 resolves to -32 (`ffe0`) |

The script checks every ordinary contribution byte, every emitted field equation
and exact MZ relocation sites. Synthetic data definitions use offsets 0, 1 and
2; these are not original data-placement claims. The original grouped candidate
and accepted isolated callee whole-object hashes reproduce unchanged.

The observed equation is `S - (P + 2)` modulo 16 bits for the two-byte field,
with zero encoded addend, omitted displacement, and caller/callee in the same
linked CS. RTLink 6.10 is hash-pinned from the earlier local research fixture;
this does not identify the game's original linker or all its options. No
grouped-public acceptance mode, original TU identity or ownership follows from
these controls. The current C gate continues to refuse the combined object.
