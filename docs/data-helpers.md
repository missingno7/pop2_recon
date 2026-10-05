# Additional complete data-helper C matches — 2026-10-05

This records the earlier 112-byte C milestone. The later
[state-helper continuation](state-helpers.md) brings matching C to 216 bytes.

Two independently reviewed resident contributions now pass CODE_EXACT under
pinned MSC 6.00 `/c /AM /Oe /Gs /Zl /Gc`. Both use the existing external DGROUP
offset16 proof; no gate was weakened or extended. The original target remains
`PRINCE.EXE`, SHA-256
`5bf733c56441258e69303102b0e08388484a9400e4d5cdad9bd517478e150314`.

| Source hypothesis | Original interval | Whole CODE | Data equations | Ordinary bytes |
| --- | --- | ---: | ---: | ---: |
| [clear_steps.c](../src/clear_steps.c) | root `[0x3604,0x360e)` | 10 | 2 | 6 |
| [increase_step.c](../src/increase_step.c) | root `[0x3596,0x35ce)` | 56 | 9 | 38 |

## Clear two byte fields

The original SUB AL,AL and two direct stores zero DS offsets `5c0c` and `5c0d`,
then return far at `0x360c`. The nine-byte body is followed by an original NOP
at `0x360d` and the next independent BP prologue at `0x360e`. The frozen whole
comparison interval includes that NOP; the compiler emits all ten initialized
bytes. A preceding RETF at `0x3603` separates the entry from its neighbor.

The root far-call encoding at `0xe852` specifies `02cc:0944`; MZ relocation
1250 covers its segment word at `0xe855`. The root sequence from `0xe828`
decodes through that call among state-initialization stores. Twelve additional
encoded references appear across overlays 3, 6, 10, 11, 12 and 15. Their exact
addresses are retained in [the evidence](../evidence/matching/clear-steps.json).
These establish encoded references and static instruction contexts; caller
entry, reachability and dynamic execution are unproved.

Independent byte reads at root `0x35f3` and `0x35d6` establish the two relative
offsets and access widths. The [binding](../evidence/bindings/clear_steps.json)
checks them, the startup DS-load witness, the whole object declarations and two
zero-addend F5/T6 offset16 fields. Six ordinary bytes compare directly.

A nonvolatile signed-byte local initialized to zero reproduces SUB AL,AL and
both AL stores. The explored nonvolatile chained/local source forms
also emit ten bytes. Volatile declarations change several emitted forms and
remain separate diagnostics; neither original qualifiers nor signedness of
these two zero stores is recovered. The source names describe a hypothesis
supported by neighboring signed-byte reads.

## Increment and clamp one signed byte

The original entry follows the preceding helper's RETF 4 at `0x3593..0x3595`.
Five conditional branches stay in the complete 56-byte interval and terminate
at plain RETF at `0x35bb` or `0x35cd`; the next routine starts at `0x35ce`.
For state byte `5c0b` equal to 4 or 9, a nonzero word at `5e90` selects byte
increment and a signed upper comparison against 4. Zero selects addition of
3 and a signed upper comparison against 33. Names and gameplay purpose remain
hypotheses; classic compiler byte arithmetic/narrowing governs overflow.
No behavior trace or portable arithmetic proof is claimed.

Two root far-call encodings at `0xec4b` and `0xee09` specify `02cc:08d6`.
Their segment-word relocations are indices 1227/1228 at `0xec4e`/`0xee0c`.
A separate PUSH CS; near CALL at `0x3a9a` targets the helper, then calls the
following state-update routine. Those contexts support a far no-argument
helper; Pascal convention with no arguments is a source declaration hypothesis.

The [binding](../evidence/bindings/increase_step.json) grounds the state-byte
alias with the separate comparison at `0x35de`, the updated byte with the
read at `0x35d6`, and the word selector with the separate store at `0xe85c`.
Its nine external fields are checked individually; 38 other bytes compare
literally. The [receipt](../evidence/matching/increase-step.json) retains the
complete object proof and source/flag contrasts. `/Oe` and `/O` give identical
CODE for explored volatile/nonvolatile forms; adding `/Or` emits 58 bytes and
a shared exit. That output remains a mismatch and is never trimmed.

## Unowned predecessor

The independently decoded helper at `[0x3020,0x3039)` reads a word argument,
conditionally negates it using byte `5c01`, adds word `5c02`, and returns AX
with RETF 2. A NOP at `0x3039` precedes the distinct `0x303a` prologue; the
frozen comparison interval is 26 bytes including this alignment byte.
Decoded callers push a word and CS before a relative CALL, and store the
returned AX to the same relative word. Names, C signedness and purpose are
hypotheses.

The [fresh negative probe](../evidence/matching/signed-value-negative.json)
emits one complete 24-byte CODE segment under MSC 6.00 `/Oe`. The original
uses MOV AX,DX; ADD AX,[bias], while this candidate uses MOV AX,[bias];
ADD AX,DX. A [readable replay source](../recipes/probes/signed_value.c) is saved
as a non-owning probe, with its complete compiler identity in the receipt.
Strict comparison rejects the unequal whole extents. The worker
also tried conditional/ternary/compound/local/pointer forms and bounded MSC
5.10/6.00 flag contrasts without finding a complete match. These observations
grant no ownership; a distinct source or code-generation lead is needed.

## Stable publication identities

Final review found a Windows text-I/O issue: some earlier local source/proof
pins hashed CRLF bytes while Git's declared policy stores LF. Canonical source
publication now freezes LF input without editing the worker submission; JSON
writing uses explicit UTF-8 bytes, and both source/runtime publication refuse
CRLF binding files. Affected canonical file pins were migrated to their already
committed LF representation. Four regression tests cover stable hashes,
verify-only isolation and refusal before canonical mutation. Original assets,
locks, equations, DOS-staged input and full OMF identities are unchanged.

## Scope and validation

Fresh search, verify-only promotion, publication and full validation pass for
both accepted contributions. All 108 invariant tests pass; all four canonical
C functions are freshly compiled, the ASM function is freshly assembled, and
all eleven independent runtime members, original assets and tool pins are
rechecked. Matching totals are 112 C bytes, 51 ASM bytes and 666 pinned runtime
bytes. The fifteen proved object fixups (fourteen source data fields and one
runtime near field) generate zero MZ relocations; coverage remains 0/4,716.

The data aliases prove relative offsets and widths for every DS value. They
own no initialized or uninitialized data and do not establish original names,
absolute storage, runtime DS reaching definitions, translation units, library
order or natural RTLink placement. Structural linkage remains UNRECOVERED;
runtime behavior validation remains NOT_STARTED. The 46-byte swap and 26-byte
signed-value target remain unresolved identified helper hypotheses; 270,487
payload bytes remain unknown/unclassified.
