# Runtime external near-code proofs

## Medium-model DOS open — 2026-10-11

The unchanged gate also accepts the complete 23-byte `__dos_open` member from
`C:/tools/msc-6.00/LIB/mlibcr.lib`, pinned archive SHA-256
`03a85ba8ac1752b64c82eff829bd2de34f776630e1aa90874155b55f187d64f0`.
The entire 181-byte `dos\d_open.asm` member at archive offset 120016 is pinned;
it has one public at CODE offset zero and no secondary storage. Its far cdecl
ABI uses a near filename, a mode word and a near output-handle pointer. The
large-model member is 26 bytes and differs; this proves the selected member,
not a global CRT model or library order.

Original entry `1fe6:1bcc` is independently encoded by the relocated far call
at root image offset 24314. All 21 ordinary bytes compare literally and the
single F0/T6 field at CODE offset 21 satisfies
`0x0604 - 0x1be3 = -5599 = 0xea21`. The complete member ends at `1be3`; the
following original alignment byte remains unowned, and the next entry is `1be4`.

To keep the provider witness outside this new owner, its binding uses the
complete original close wrapper at `1fe6:1b94`, the outside relocated entry at
root 80524, and the original straight-line path to its near tail. The same
independently pinned provider signature/public context is checked afresh. These
are immutable original witnesses, not candidate output or ownership-dependent
fallbacks; provider context still owns zero bytes. Both near-bound members now
cover 36 bytes, separately from ten relocation-free members covering 653 bytes.
See [the open recipe](../recipes/runtime/crt_dos_open.json) and
[binding](../evidence/bindings/crt_dos_open.json). No new fixup mode, MZ coverage,
whole-link closure or behavioral proof is introduced.

## Initial close proof — 2026-10-05

The complete MSC 6.00 archive member `dos\d_close.asm` now owns 13 resident
CODE bytes as PINNED_RUNTIME. The independent archive and entire 174-byte OMF
member are hashed; its CODE SEGDEF is fully initialized, has one public at zero,
and owns no secondary data. Its only actual fixup is a self-relative LOC1
offset16, framed by location segment `_TEXT` (F0), targeting external
`__dosreturn` (T6). The raw field is zero and the explicit displacement is
omitted. All 11 ordinary bytes compare directly; the remaining two bytes are
checked by their near-transfer equation. Neither object nor EXE is edited.

The original member lies at root `[0x219f4,0x21a01)`. A relocated far-call
encoding at `0x13a8c` specifies `1fe6:1b94`; its segment word is relocated at
`0x13a8f`. The earlier diagnostic packet incorrectly used the latter coordinate
for the opcode/file offset; [the corrected evidence](../evidence/matching/crt-dos-close.json)
and the binding gate keep them distinct. A `2000:` window address is an alias,
not evidence of the frame encoded by that reference. Reachability and execution
of the call have not been traced.

The destination is grounded independently of the close jump. The pinned
`dos\dosret.asm` member declares `__dosreturn` at `_TEXT+8`. Its 40-byte context
prefix is unique in the original resident image at `0x2045c`; every ordinary
byte matches, and its three internal near-call fields satisfy the provider's
own equations (26, 14, 1). The public entry is at `0x20464`, immediately after
RETF; subsequent public metadata and return boundaries agree. An unrelated
far-call encoding enters a DOS wrapper at `1fe6:1bcc`. A checked conditional
path reaches its E9 at `1fe6:1be0`, which targets `1fe6:0604`, the same public
position. Each selected path step must start an instruction and follow a valid
fallthrough or direct conditional edge; calls, far flow and returns on the
selected path are refused. The error condition's runtime truth is not claimed.

The provider owns **zero bytes**. Its whole 85-byte CODE and 20-byte DATA
contributions still have unresolved obligations outside the signature prefix.
The prefix is solely a public-identification witness, not a trimmed accepted
component. Original public spelling and original archive provenance remain
correspondence hypotheses backed by installed-library metadata and code.

In the encoded `1fe6` frame, the owned field's next IP is `1ba1`, and the
independently grounded target IP is `0604`:

```
0604 - 1ba1 = -5533 = ea63 (encoded 16-bit word)
```

The original E9 field contains `63 ea`. The equation uses the independently
grounded public and frame, not the observed field to select an address.
Self-relative offset16 creates no MZ relocation, so any original relocated word
touching the owner, wrapper span or signature prefix fails, including a word
straddling the start. Far-call anchors require exact segment-word relocation
membership. The verified anchor encodings determine their CS destination if
executed; caller instruction boundaries and dynamic execution are not claimed.

[Independent synthetic links](../evidence/toolchain/near-jump-link-probe.json)
verify precisely this LOC1/F0/T6, zero-A/omitted-D operation with a backward E9.
MASM 5.10 emits F0 for `JMP _TEXT:_helper`; an unqualified jump emits F2, which
the current gate rejects. Its LEDATA begins at segment offset 10, so the raw
FIXUPP record's location 1 resolves to complete segment offset 11. MS LINK 3.65
and RTLink Plus 6.10 both emit signed displacement -15 to the independent
provider public, identical load images and zero MZ relocations. This tests
linker arithmetic, not original version, order or placement.

`tools/near_binding.py` implements this restricted resident proof. Omitted D,
explicit D=0 and nonzero raw A remain distinct; only the first with A=0 is
accepted. Unsupported modes, wrapping displacements, ambiguous provider
signatures, changed declarations/publics/fixups, unowned secondary storage,
holes, length mismatches and literal address substitutions cannot pass.
`tools/pin_runtime.py build/workers/NAME/recipe.json --verify-only` performs a
fresh check. Removing `--verify-only` publishes a canonical recipe after
ownership validation; a rejected physical alias overlap removes the proposed
recipe and leaves the manifest unchanged. Binding files are confined to
`evidence/bindings`, use LF newlines for stable Git identity, and are pinned by
file hash. Validation rereads the independent
archive, whole member, provider, original witnesses and equations every time.

This is component proof with a signature-backed provider-public binding.
Complete provider recovery, original TU membership, library order, natural
RTLink placement, far-call fixup binding and behavioral traces remain separate
debts. Original assets, overlay ownership and relocation coverage are unchanged.
