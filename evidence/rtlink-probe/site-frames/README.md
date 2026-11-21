# Overlay relocation site frames (RTLink 6.10)

Question: is an overlay relocation record's `segment` the CS frame of the code
around its site, so that it witnesses the frame of a claim that no unambiguous
far call reaches?

## Controlled link

`python evidence/rtlink-probe/site-frames/probe.py` compiles six MSC 6.00 files
(project flags `/c /AM /Oe /Gs /Zl /Gc`) and assembles `OVPTR.ASM` (a `dd`
table of two overlay entries). The pinned research RTLink 6.10 links START and
ROOTC resident, and sections ONE (O1A, O1Z, O1B, OVPTR) and TWO (O2A, O2B) in
one `BEGINAREA`, so they share link segment 0127. O1A has 42 far calls with
sites up to offset 0x1e7, O1Z has no far reference, and O2B calls O1A1 in the
other section. Nothing is executed, and no original bytes are linker input.
[`receipt.json`](receipt.json) is the compact copy of
`build/workers/site_frames/receipt.json`, and the script rechecks the headline
results on every run.

- **Site segment = segment frame.** All 94 records (41 MZ, 47 in ONE, 4 in
  TWO, 2 in the preloaded data section) store the paragraph floor of the start
  of the segment that holds the site, with the offset measured from it, for
  example O1B at `0148:0025..003e` and O1A at `0127:0021..01e7`.
- **Far words store the same frame.** All 56 relocated far words naming a probe
  public store the callee segment's frame. That covers direct same-overlay calls,
  overlay-to-root calls, the MASM `dd` words and the vector jump words.
- **Monotone, with gaps.** Frames increase with position. O1Z gets frame 0146
  but no record, because it has no far reference. Its frame is named only by
  O1B's call `0146:0014`. Records of frames 0127 and 0148 bracket it, so a census
  extent ending at the next record frame would wrongly include it.
- **Groups differ.** `$$OVLVEC` belongs to group `$$OVLMGR`. Its records name its
  own floor `00c4`, while the root calls into it use the group frame `0007`. The
  rule therefore holds only for a code segment that is in no group.
- Root calls, the root data pointers and O2B's call into section ONE all enter
  through `$$OVLVEC` vectors. The manager's own `0007:b8xx..bbxx` data pointers
  have the same shape as the 15 unmatched `2203:b8xx` words in the locked image.

## Locked image

`python evidence/rtlink-probe/site-frames/image_check.py` checks the locked
image. It prints the results and writes nothing.

- All 15 code-overlay tables (2,526 records) are monotone in site frame.
- 447 relocated CALL FARs land in a code-overlay frame extent: 61 from the same
  overlay and 386 from other spaces. Each agrees with the site frame of at least
  one overlay that holds the address. Of the other two-word shapes, 147 agree.
  The 15 that agree with no holder are RTLink manager data pointers
  (`2203:b8xx`, `1fbf/1fa0:ba00`).
- Root MZ records give the same result. Of 2,029 records, 1,988 equal the census
  frame taken from call targets. Five are the previous file's last far call
  inside the shared paragraph. The other 36 lie in segments that no call
  targets: 35 in `22ce`, the floor of the vector segment, which the calls enter
  as group frame `2203`, and one in `2202`.
- Gate rule over the 64 reviewed code-overlay targets: 47 get a site frame. In
  the 11 that already had a vector or same-overlay witness, both agree. 11
  previously refused targets and 25 targets with no candidate are newly
  witnessed. The rule refuses 8 without a bracket and 9 because another frame's
  relocated word lands inside them. Those 9 are aliases from root calls such as
  `2a4e:0000` into overlay 4/6/10 and `29cf:xxxx` into overlay 9, plus one
  manager pointer.

Scope: synthetic RTLink 6.10 observation and a consistency check of the
original image. This does not identify the original linker version or options.
