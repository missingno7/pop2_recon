# Code-frame census — 2026-11-20

Phase 1 of the [module acceptance plan](tu-acceptance-plan.md). Research only:
no ownership, target or gate changes.

```
python tools/tu_census.py              # writes build/workers/tu_census/census.json
python evidence/rtlink-probe/frames/probe.py
```

## Linker control: one frame per file

[`frames/probe.py`](../evidence/rtlink-probe/frames/probe.py) compiles three
small files with the project flags (`/c /AM /Oe /Gs /Zl /Gc`) under distinct
basenames and links them with the pinned research RTLink 6.10. Its
[receipt](../evidence/rtlink-probe/frames/receipt.json) shows:

- each file emits its own `NAME_TEXT` and gets its own far-call frame (`0000`,
  `0003`, `0008`);
- the frame is the paragraph floor of the file's start, and offsets continue from
  there (FILEB starts at linear `0x38`, so its publics are `0003:0008`, ...);
- same-file calls become `PUSH CS; CALL near`, and cross-file calls stay `CALL FAR`
  with a segment relocation;
- even-length WORD-aligned segments abut with no fill.

The original's first-entry offsets (`02cc:0002`, `0f77:0002`, `1d3a:0004`) match
this shape. A file therefore starts between `frame*16` and `frame*16+15`, and the
exact byte is decided by reviewing the previous file's end. This is a synthetic
observation with RTLink 6.10, not the game's original linker or options.

## Root image

There are 30 frames, taken from unambiguous relocated `CALL FAR` targets.

- **Near calls stay inside frames.** All 1,380 `PUSH CS; CALL near` sites
  (relocation-free, with IP wrapped at 64K in the caller's segment) stay inside
  their own frame; none crosses a frame boundary. This agrees with frame = file.
- **Near-call blocks per frame.** Each file is contiguous, so its same-file call
  intervals must merge into blocks inside it. Most frames form one block covering
  80 to 99% of their span. A gap between blocks is a necessary condition for an
  inner file boundary, but not proof of one.
- **`1101` is one 49 KB connected block.** It has 153 called entries and 632
  internal near calls, and no inner boundary is visible. It is a single large
  file or something unusual, and needs review before it is drafted.
- **`1fe6` holds all 21 pinned runtime members.** These are 1,166 bytes, so this
  frame is treated as runtime, not game code.
- **`2203` and `22e5` look partly like data.** Their leading bytes look like
  tables, so they need classification before any card is written.
- **Small frames with no near calls** are the cheapest complete-module pilots.
  The first two are the ones named in the plan.

  | Frame | Span | Entries | Note |
  | --- | ---: | ---: | --- |
  | `21fe` | 80 | 1 | trailing zeros; may be runtime |
  | `1ec9` | 96 | 1 | |
  | `1ead` | 448 | 8 | 38 bytes owned |
  | `1d3a` | 480 | 1 | |
  | `1d58` | 480 | 2 | |

  `1f61` (304 bytes, 5 entries) has one near call.

The frame `0000` state-step pair and frame `02cc` (14 owners, 532 bytes)
remain the partial-module pilots.

## Overlays: not resolved yet

There are 382 far calls whose segment word lands in more than one overlay, because
overlays 2/3, 4–7/10, 8/9, 11/12 and 13–16 share link segments. The census
does not let those calls create frames, so most overlays show at most one frame.
Overlay module boundaries need RTLink's own call resolution (vectors or descriptor
data, see [rtlink.md](rtlink.md)). Until then, overlay modules stay out of scope,
as the plan's first gate version already assumes.
