# Runtime classification evidence

## Whole conversion members — 2026-10-11

The medium-model `atox.asm` and `xtoa.asm` members now pass the unchanged
literal-runtime gate at `1fe6:1672` (84 bytes) and `1fe6:19a4` (95 bytes).
Every initialized CODE byte matches, with no fixups or secondary storage.
The parser's original entry is reached through an independently relocated
three-byte conversion stub. The stub and its following zero alignment remain
unowned. The shared output core retains both archive publics, `__cltoasub+0`
and `__cxtoa+12`, and its entire 95-byte segment. Its first twelve bytes are
setup for an existing caller frame, rather than a separate far C function.

The original `itoa` wrapper independently establishes its register entry and
far caller frame; both conditional paths reach the shared core's public at 12.
This caller context supports the complete literal-member review. The separate
fixup-bearing `itoa` member remains unowned: its near proof lacks a distinct
incoming wrapper, and the provider's shared public is not after a RETF within
the member as required by the current near gate. Pinning a provider does not
satisfy those symbolic obligations. The [compact review](../evidence/matching/wave8-runtime-review.json)
retains exact pins, original paths and ABI limits; runtime execution is untraced.

The diagnostic scanner now retains all in-extent CODE publics of a complete
zero-fixup member. Its earlier one-public restriction hid `xtoa.asm` even though
the literal-runtime gate already supports whole multi-public members. Two new
tests verify complete shared-entry discovery and reject an out-of-extent public.
The combined medium and large census now yields 23 whole literal research
matches; after publication they all overlap accepted ownership. No acceptance
gate changed.

## Medium-model continuation — 2026-10-11

Whole members from `C:\tools\msc-6.00\LIB\mlibcr.lib` also match the resident
image. This archive is 142,539 bytes, SHA-256
`03a85ba8ac1752b64c82eff829bd2de34f776630e1aa90874155b55f187d64f0`.
The prior medium-model DOS open match supplied the reason to inspect this model.
Seven further members have complete literal CODE matches, empty secondary
storage, sole publics at zero, no fixups and independently relocated far entries.
Fresh strict publication owns all 298 bytes:

| Public | Logical root address | Complete bytes |
| --- | --- | ---: |
| `_strcat` | `1fe6:15d4` | 63 |
| `_strcpy` | `1fe6:1614` | 50 |
| `_strlen` | `1fe6:20f4` | 27 |
| `_strncpy` | `1fe6:1646` | 40 |
| `_strchr` | `1fe6:2110` | 42 |
| `_strupr` | `1fe6:1986` | 30 |
| `__bios_timeofday` | `1fe6:1b66` | 46 |

These far-call members use near data pointers. The distinct large-model string
members below remain accepted at their own extents. This establishes both ABI
families in specific components; it does not infer a global CRT model, source
translation units or library order. DS/DF and DOS/BIOS execution remain separate
behavior debts. Complete member pins, original CFGs, caller paths and limits are
in [the compact review](../evidence/matching/wave7-runtime-review.json).

The reusable diagnostic command is:

```
python tools/runtime_scan.py C:/tools/msc-6.00/LIB/mlibcr.lib --output build/workers/NAME/runtime.json
```

It reports whole literal matches, preserves original relocation obligations and
ownership overlaps, and excludes actual fixups, sparse CODE
and nonempty secondary storage. It cannot publish. The initial two-library run
examines 343 members per archive and reproduces all seven reviewed member pins.
At that milestone the `__catox` hit and near-bound `_itoa` research remained
unowned; the later conversion review above closes the whole parser member.

## Initial large-model comparison

Research date: 2026-10-04. This read-only comparison tests local MSC 6.0 large-model runtime objects against immutable `PRINCE.EXE` bytes. It establishes byte-level matches for specific library members; it does not pin the exact library release used to link the game or assign every byte in the resident image to the CRT.

The reference archive is `C:\tools\msc-6.00\LIB\llibcr.lib` (148,711 bytes, SHA-256 `3d0c6ae92972789e87def01fa96351c39216d731d5c880370155c5613d78c884`). I located member boundaries from OMF `THEADR` through `MODEND`, then used the repository's strict `OmfReader` on each relevant complete member. The OMF reader API is `OmfReader().read(member_bytes, label)`; its result exposes `segments`, `segment_defs`, `publics`, `externals`, `fixups`, and raw OMF records. The candidate root addresses and bytes come from the locked local executable SHA-256 `5bf733c56441258e69303102b0e08388484a9400e4d5cdad9bd517478e150314`.

These complete MSC runtime member `_TEXT` segments match root bytes exactly. Each listed member has no FIXUPP records, no decoded fixups, and no MZ relocation entries within the matched root extent:

| Public member | Archive member offset / extent | Member SHA-256 | `_TEXT` bytes / SHA-256 | Root link address | Root file offset |
|---|---:|---|---|---|---:|
| `__searchseg` (`searchsg.asm`) | `0x7040` / 257 | `5964e6c9dfd4de45ed54a592c35165c6dd851c32cca10ee25c6b652422c0a9e2` | 124 / `44a28c72af20a32d04143d0b08720a019f3f13486749a27da6b9e3b52b21fb39` | `2000:12ac` | 144044 |
| `__nmsize` (`nmsize.asm`) | `0x7450` / 139 | `7860299116f1a7dbb6b28fe5bd6e7c73d0d6b17cd39c9f1f9932458e52b8fe0a` | 13 / `65f2940dcea8f9a6757cce4f2770d99fc059f4b78f2bdc40e490933558a19a63` | `2000:1426` | 144422 |
| `_strcpy` (`strcpy.asm`) | `0x7650` / 179 | `d4d6a8b32590105b6f721818882355d512221b19d55bfa6b2055170e4a7f9f18` | 54 / `d2de043779a7128c098a270356177adb5e2b742da948b8e8a1b0d4abbed13569` | `2000:1d4c` | 146764 |
| `_strcmp` (`strcmp.asm`) | `0x7710` / 166 | `647bb9bde2215e9aa469cae5ee0092d60dc70c8d6f36aeddfb93800be6d0d07c` | 41 / `035ad47ddae9b512f38fe3759c5df5fa0e12c0dfdeeaf71d52c6919d3bdb40f5` | `2000:1fc2` | 147394 |
| `_strlen` (`strlen.asm`) | `0x77c0` / 148 | `36407dec2d2121638e1743614974f899b5c156e863373de4f9499e8c60d4a714` | 23 / `e5ff878eb7744dbfdb40f7b4993400d6bf9bbd6c7770524c57ddd5a38b537331` | `2000:1d34` | 146740 |
| `__headchk`, `__listchk` (`headchk.asm`) | `0xdb10` / 259 | `f6a0ac010766b5e631c8677b0c5e5f599b98c62c3107be89d2b44692dc94a427` | 117 / `7397326f984c6eb93d262a166cda397e6aa9b8e2bd850cf6b1cc4312acd8cd3e` | `2000:1698` | 145048 |
| `__growscan` (`growscan.asm`) | `0xe6e0` / 167 | `af22173718178583140192d6ef51dee8873a5179a53875c83f217a38269f43ff` | 21 / `0143ef3d243ee758839405385ad6115d6d9ca00c834e1d712c9d34b37aa2ab46` | `2000:17d0` | 145360 |
| `_strcmpi`, `_stricmp` (`stricmp.asm`) | `0x13fc0` / 209 | `c7384f78630166042dca86b0987b5586e2eb861b4882e03a6291e33253ba0f40` | 69 / `139b7c6d73baad3f586256404c04f228a0664dc315325cf23abf816646a4c7ef` | `2000:1cee` | 146670 |
| `_strnicmp` (`strnicmp.asm`) | `0x14180` / 219 | `49be8627f7594079023aa6d3c2a70c882e1c17f713cf900cdc6ba9481836b2fd` | 88 / `03d548a41c4cec445d438fd0af0928fde5e0dc96785c4449b53bbf7c4dc8cab9` | `2000:1efc` | 147196 |
| `_strstr` (`strstr.asm`) | `0x14a90` / 228 | `72b433a654b9a251bf06e8775cdf101d561938f88aa9566528c736e88b0803a5` | 103 / `96bacf99c5d1b777c600fbe722335dc43e0f348dee34b05352ece25c7c57176b` | `2000:1dca` | 146890 |

The `root:1101:bd0a` helper is not an exact match for the library's `_tolower` implementation. Its 20 bytes are `55 8b ec 8b 46 06 3c 41 72 06 3c 5a 77 02 0c 20 5d ca 02 00`, a far-call helper that folds ASCII `A`–`Z` by setting bit `0x20` and returns with `RETF 2`. Its root extent has no MZ relocation. The archive member `tolower.c` is at `0x8150`, extent 294, SHA-256 `92b4f2998f5e689c7c500010e88f5300d628ec81220ef345300465ea4bbf143a`; it exports `_tolower` at `_TEXT+12` and `__tolower` at `_TEXT+0`, with 34 `_TEXT` bytes. Its OMF carries six FIXUPP thread declarations and one actual fixup (`record_hex` `c4145601`, location 20). The leaf's ASCII-lowercasing purpose is related, but code shape, ABI, and fixup obligations differ, so it is not evidence that the game helper is the library symbol.

Independent startup evidence ties a different resident-root range to the same installed CRT. MZ entry `2203:0678` starts at root position `0x226a8`, saves DOS-provided segment state, sets up `DS`, makes setup calls, then far-jumps to root link address `1000:fed4` (file offset 138964). That target begins `b4 30 cd 21 3c 02 73 05 33 c0 06 50 cb`, matching the first 13 bytes of the large-model `__astart` member `dos\crt0.asm`. The full OMF member begins at archive offset `0x10`, has a 1,230-byte record-bounded extent and SHA-256 `2e9a254b9bd00ea59884089e78e951f0e40343e11d24976f32d9582f4ded259d`; its `_TEXT` is 255 bytes (SHA-256 `19f2be62cae28b23102f07c5c20733d06bed0e6fea50d23bbb44bf94201b57a`). For the first 184 bytes, every byte outside the member's declared FIXUPP fields matches root exactly; 40 bytes lie in declared fixup locations. The first mismatch outside those fixup locations is at member offset 184, where the resident code diverges. This is a strong startup-code match, not a complete match of the 255-byte segment.

The exact no-fixup leaves cluster between root positions `0x212ac` and `0x21feb`, and the startup-code match lies nearby. Together they establish MSC 6.0 large-model CRT code in the resident image. They do not establish a complete contiguous CRT segment boundary: intervening and trailing bytes have not all been attributed, and matching code bytes do not prove which identical library release supplied them. The string `MS Run-Time Library - Copyright (c) 1990, Microsoft Corp.` is in overlay 17, not in this resident range. The `.RTLink(R)/Plus` error marker is in the final resident bytes, but neither string is used as the basis for these code matches. Keep exact matched extents classified as proven runtime; leave the remaining root bytes subject to separate evidence.
