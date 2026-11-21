# Resident MZ relocation order in RTLink and Prince of Persia 2

**Result.** The source object predicts relocation sites and order *within* each
target bucket. The permutation of buckets depends on link context. RTLink
6.10 demonstrably changes this permutation with the same caller, same
provider, same placement of every relocated byte, and identical MZ load
images. The original CHECKNIS 41-record span is consistent with the
within-bucket rule when the same C source is compiled with `/Zd` or `/Zi`.
The original MISC claim union requires finer chunking than `/Zd` supplies in
the tested build. `/Zi` reproduces its within-bucket order.

## Observed RTLink 6.10 rule

1. **Source scope is a linked output segment.** RTLink visits the relocating
   output segments in MAP order in these links. A/B share `SHARED` across two
   MASM objects and `SECOND` gets its own buckets. Reversing A/B reverses their
   contributions inside each shared bucket. The MSC `BOTH`/`BACK` pair does
   the same for `UNIT_TEXT`. Consecutive objects in different output segments
   get separate passes. Individual input contributions can be projected out
   of a public-combined segment without changing their within-bucket order.
2. **Buckets use resolved target identity.** For matching ordinary relative
   frames, the tested identities are external symbol, group, or the target
   input SEGDEF contribution. Two external names can share a `16:16` address
   and still have separate buckets (`PROBE_X` and `ALIAS_X`). `METHODBOTH`
   shows that distinct input SEGDEF targets also stay separate when their
   names and linked addresses agree. Its shared group and external targets
   combine across input objects. MSC F5/T6 and MASM F2/T6 both show the
   external-symbol behavior, with MASM F0/T4 and F1/T5 tested for direct
   segment and group references.
3. **Preserve traversal within each field kind.** Within a bucket, LOC2
   base16 sites precede LOC3 pointer segment words in `METHODS`,
   `METHODDEFS` and `METHODBOTH`. The split holds across object contributions
   in `METHODBOTH`. Each list preserves the input contribution traversal,
   FIXUPP record order, and subrecord order. The emitted MZ site is the LOC2
   field itself or the LOC3 field plus two. For an MSC far-call-only bucket
   this is exactly raw FIXUPP order, descending within each compiler LEDATA
   chunk and visiting the chunks in file order. The MASM ascending case
   survives as well. LEDATA breaks do not restart the target bucket.
4. **Only segment words enter MZ.** `MANY` and `MANYZD` each link 316 LOC3
   references and 105 same-file, self-relative LOC1 calls. Each produces
   exactly 316 MZ entries. `SWITCH` produces 13 MZ entries from 13 LOC3
   calls, alongside 14 non-self-relative LOC1 fixups in CODE that use
   target threads, 13 of them also using frame threads. Thread declarations
   themselves occupy no site. The model uses the reader's resolved fixups.
   Used threads on LOC2/LOC3 were outside these controls, as were mismatched
   frame/target identities, absolute providers and overlay emission rules.

`MANY` has three LEDATA chunks at object offsets `0, 708, 1416`, with
`149, 149, 123` actual fixups. `/Zd` retains the full CODE bytes and starts
nine chunks at `0, 247, 494, 741, 988, 1235, 1482, 1729, 1976`; the same
per-target traversal rule predicts all 316 MZ sites in each output.
All 33 source-only links and 1,224 MZ records pass the site, bucket,
within-bucket and MAP segment-order checks. The pre-existing controls in
`evidence/rtlink-probe/relocation-order/` independently show the far-only
subset.

## Why the full permutation needs link context

`FIXED` links `START, ONE, DEFDATA`. `FIXEDFIRST` links `START, DEFDATA,
ONE`. These are the same three object byte strings. The provider is in a
different output class, so moving it in the input leaves the linked load
image byte-identical, including every linked call. Each MZ record (offset,
segment, site, stored word) exists once in both outputs. The sequences
below show the image offsets in table order.

```text
FIXED       54 39 74 24 59  9 44 29 64 14 49 34 69 19
FIXEDFIRST   9 54 44 14 69 59 34 29 64 24 39 49 74 19
```

The common load image SHA-256 is
`a5d8176187fbf2524a48ddc3c13eb258793f7dfec8b5f311d2f15bf9b9c778c0`.
This counterexample rules out a total-order function of one object, even
with placement and resolved addresses. `ONE` and `REVCALL` change target
order with reversed calls and the provider last. They have the same
permutation of first-FIXUPP target ordinals. Reversing EXTDEF enumeration
(`REVDECL`) or the assembly definition order (`REVDEFS`) leaves the `ONE`
order intact. With the provider first, reversing calls leaves the
provider-first order intact. The `COUNT1`–`COUNT13` and 100-name `LARGE`
controls preserve the `ONE` permutation restricted to their matching
first-FIXUPP ordinals. The changes suggest that encounter order and prior
symbol state affect the order in which RTLink enumerates buckets. The
global enumeration algorithm and historical symbol state remain unknown.

## Original observations

The locked `assets/DOS/PRINCE.EXE` MZ table agrees record-for-record with the
existing Oracle JSON. All 2,029 root records form 31 distinct runs of the
raw relocation `segment` field, with nonoverlapping increasing linear site
ranges across runs. Near CHECKNIS these are `0000` (indices 0–133), `0169`
(134–238), `0284` (239–279), `02cc` (280–382) and `04a6` (383–426).
This agrees with the output-segment evidence. Original object boundaries
and historical CS require independent evidence. All 41 sites in frame `0284` are the
segment words of original `9A` instructions.

### CHECKNIS.C at `0284:000e`

The worker source with MSC 6.00 and `/c /AM /Oe /Gs /Zl /Gc` reproduces the
worker's object hash `1b896a1971f7c26877f8ffb3714966f1958bfc5a9bdf901284e67c8f6f1355a5`.
The plain, `/Zd` and `/Zi` builds emit the same 1,140 candidate CODE bytes
and the same 41 segment-word sites. The plain build starts LEDATA at
object offsets `0, 945`. `/Zd` starts at `0, 464, 839`, or frame offsets
`000e, 01de, 0355`. `/Zi` uses 17 CODE LEDATA records and preserves the
relevant split points. Original MZ index sequences below are zero-based.

| Symbol | Original sites within frame | Original indices | Plain FIXUPP indices | `/Zd` and `/Zi` |
|---|---|---|---|---|
| `FIND_OPTION` | `01a4,0202` | 245,246 | 246,245 | 245,246 |
| `_atoi` | `01b1,020f` | 269,270 | 270,269 | 269,270 |
| `START_MUSIC` | `01c4,01e5,0364` | 264,265,266 | 266,265,264 | 264,265,266 |
| `PREPARE_MUSIC` | `01f1,03d9` | 243,244 | 243,244 | 243,244 |
| `ERASE_SCREEN` at `0226` | `0306,0290` | 254,255 | 254,255 | 254,255 |
| `DRAW_ROOM_PASS` at `0326` | `03c3,03ba` | 260,261 | 261,260 | 260,261 |
| `RELEASE_IMAGE` at `03f0` | `047a,0472` | 251,252 | 251,252 | 251,252 |

The first `/Zd` split corrects the first two ascending pairs and separates
the first two `START_MUSIC` fields. The second split places the third
`START_MUSIC` field in a later chunk and puts both `DRAW_ROOM_PASS` calls in
the same chunk. The plain `03bf` frame split falls between those two calls.
This accounts for the reversal without a new within-symbol linker rule.
The fresh FAR projection of the cited plain object already agrees for
`ERASE_SCREEN` and `RELEASE_IMAGE`, superseding the earlier ordering notes
for this object. The four early entries have zero original MZ sites. The entries
`00e6`, `0226`, `0326`, `03f0` have 8, 12, 14, 7 respectively. Every
symbol group is contiguous in the whole 41-site projection, with 33 total
groups. This constrains the historical chunk boundaries. The specific
historical debug flags remain unknown.

### Owners and same-source claims

The manifest snapshot used for the check has SHA-256
`4c3264b54b51323d1ef3cf44cad5359a3f917a2c8e63fb61fd6dac3599d71040`.
All 19 owners carrying `relocation_indices` were checked against freshly
compiled, object-hash-verified recipe sources. The 25 original sites agree
exactly as sets, and every owner passes the within-bucket check. Every case
and the complete original ordering appear in `check-results.json`.

| Original claim | MZ indices | Result |
|---|---|---|
| `0000:12de`, `0000:1306`, `02cc:00f0`, `0da4:0244` | 69 / 70 / 306 / 1097 | Exact single-bucket order |
| `0f77:0108` | 1329 | Exact single-bucket order |
| `0f77:0164` | 1330,1331 | Both buckets pass |
| `0f77:04ec`, `057a`, `060e`, `06b0`, `072e` | 1333 / 1334 / 1335 / 1336 / 1337 | Exact single-bucket order per claim |
| `1000:62fe` | 1453,1654 | Both buckets pass |
| `1000:943c` | 1588,1611 | Both buckets pass |
| `1000:a82a`, `1000:d922`, `1000:ec2a` | 1460 / 1696 / 1786 | Exact single-bucket order |
| `101f:0490` | 1350,1356,1357 | All three buckets pass |
| `1e23:04c6` | 1764 | Exact single-bucket order |
| `1ead:0118` | 1782,1787 | Both buckets pass |

Combining the two relocation-bearing LGRAF claims preserves `_memcmp`
order `[1786,1787]` from one LEDATA. The MISC claim union shows a stronger
test than its individually checked claims. With its MSC 6.00A `/Oeg`
recipe, three plain LEDATA chunks and six `/Zd` chunks both miss two
repeated-symbol sequences. All three builds emit the same complete 2,712
candidate CODE bytes.

| MISC symbol | Original | Plain | `/Zd` | `/Zi` |
|---|---|---|---|---|
| `INTERSECT_RECT` | 1329,1330 | 1330,1329 | 1330,1329 | 1329,1330 |
| `OFFSET_RECT` | 1331 | 1331 | 1331 | 1331 |
| `ROW_TO_CELL` | 1333,1334,1335,1336,1337 | 1336,1335,1334,1333,1337 | 1334,1333,1337,1336,1335 | 1333,1334,1335,1336,1337 |

The 37 `/Zi` CODE LEDATA chunks put those ascending repeats in separate
chunks. In an earlier MSC 6.00 MISC source snapshot, a `/Zi` probe also
gave ascending symbol order, but emitted 2,710 bytes and lacked the
expected claim-relative site for 1329. The MSC 6.00A recipe snapshot
above supplies the site and complete CODE equality across the three
objects. `OmfReader.read` refuses both inspected `/Zi` objects because
`$$SYMBOLS` data records overlap. The worker diagnostic walks the raw
CODE records and verifies inline F5/T6 far sites, EXTDEF identity, original
claim-relative offsets and CODE extents. The emitted objects stay intact.
`/Zd` creates LINNUM records outside the strict gate's supported record set.
These observations establish ordering for the stated scopes. Separate
code-byte and binding obligations remain in the CHECKNIS drafts.

## Reproduction and predictor interface

Run sequentially from the repository root. The scripts write inside the
worker directory and link only synthetic programs, which are never run.
The research linker is RTLink/Plus 6.10 with SHA-256
`ea6f57a470a26801086593d54aa99f6bf983dd024fc19b5e13361a07f43e17fb`.

```powershell
python -B build/workers/reloc_order/probe.py
python -B build/workers/reloc_order/check_original.py
python -B build/workers/reloc_order/check_predict.py
```

`predict_order(module, "UNIT_TEXT")` returns ordered segment-relative
sites for each `(target_kind, target_name)` bucket. A single bucket gives
an exact local sequence. For several buckets, `bucket_order=` accepts an
independently supplied permutation and returns the flattened sequence.
The checks exercise this mode against four independent synthetic links.
They check all multi-bucket original cases without supplying the observed
permutation to the predictor. The combined control checker repeats the
same rule across consecutive objects with MAP placement. Raw link, map and
original-comparison receipts are in `controls/report.json` and
`original/report.json`. `check-results.json` has the compact conclusions
for 40 original projections plus the two raw `/Zi` diagnostics.
