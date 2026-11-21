"""Locked-image consistency of the site-frame rule (research; prints, writes nothing).

1. Every code-overlay table is monotone in site frame by site position.
2. Relocated far words (stored segment, offset word before it) landing in a code-overlay
   frame extent: does their segment equal that overlay's site frame? A word may aim at
   any overlay holding the address, so a word counts as consistent if one holder agrees.
3. Root MZ records: site segment against the census root frame (from CALL FAR targets).
4. The gate's `_site_frame` over every reviewed code-overlay target.
"""
import bisect
import collections
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
import module_gate
from common import read_json
from oracle import Oracle
from tu_census import census, frame_extents, overlay_vectors, relocation_frames


def main():
    oracle = Oracle.load()
    overlays = {n: s for n, s in oracle.spaces.items() if s.descriptor and s.descriptor['flags'] == 4}
    vectors = overlay_vectors(oracle, overlays)
    extents = {}
    for name, space in overlays.items():
        rows = sorted(space.relocations, key=lambda r: r['image_offset'])
        assert all(a['segment'] <= b['segment'] for a, b in zip(rows, rows[1:])), name
        entries = collections.defaultdict(set)
        for v in vectors.values():
            if v['target_space'] == name:
                entries[v['target_segment']].add(v['target_position'])
        extents[name] = frame_extents(space, relocation_frames(space), entries)
    print('1. monotone site frames in', len(overlays), 'code overlays,',
          sum(len(s.relocations) for s in overlays.values()), 'records')

    def frames_at(name, pos):
        return [f for f, e in extents[name].items()
                if e.get('reviewed_start', e['start_min']) <= pos < e.get('reviewed_end_exclusive',
                                                                          e['end_exclusive_max'])]
    counts = collections.Counter()
    for source in oracle.spaces.values():
        for r in source.relocations:
            at = r['image_offset']
            if at < 2:
                continue
            seg, off = r['stored_word'], struct.unpack_from('<H', source.data, at - 2)[0]
            holders = {n: frames_at(n, (seg - s.link_segment) * 16 + off) for n, s in overlays.items()
                       if 0 <= (seg - s.link_segment) * 16 + off < s.size}
            holders = {n: f for n, f in holders.items() if f}
            if not holders:
                continue
            kind = 'CALL FAR' if at >= 3 and source.data[at - 3] == 0x9a else 'other word'
            scope = 'same overlay' if source.name in holders else 'other space'
            agree = any(seg in f for f in holders.values())
            counts[(kind, scope, 'agrees' if agree else 'agrees with no holder')] += 1
    print('2.', dict(sorted(counts.items())))

    report = census(oracle)
    root = oracle.spaces['root']
    frames = [(f['base'], int(f['frame'], 16)) for f in next(s for s in report['spaces']
                                                            if s['space'] == 'root')['frames']]
    bases = [b for b, _ in frames]
    mz = collections.Counter()
    for r in root.relocations:
        i = bisect.bisect_right(bases, r['image_offset']) - 1
        frame = frames[i][1]
        mz['equal' if r['segment'] == frame else
           'previous file tail in shared paragraph' if r['image_offset'] - frames[i][0] < 16 else
           'segment %04x (no call-target frame)' % r['segment']] += 1
    print('3. root MZ site segment vs census frame:', dict(mz))

    rows = collections.Counter()
    for t in read_json(ROOT / 'evidence/targets.json')['functions']:
        space = oracle.spaces[t['space']]
        if t['space'] not in overlays or t.get('boundary_status') != 'REVIEWED':
            continue
        at = space.position(t['segment'], t['offset'])
        found, refused = module_gate._frame_evidence(t, space, oracle, (at, at + t['size']))
        site = [e for e in found if e['kind'] == 'overlay site frame']
        other = [e for e in found if e['kind'] != 'overlay site frame']
        before = 'witnessed' if other else 'refused' if [u for u in refused if 'kind' not in u] else 'none'
        if site:
            agree = all(e['frame'] == site[0]['frame'] for e in other)
            rows[(before, 'site frame', 'agrees' if agree else 'DISAGREES')] += 1
        else:
            reason = next(u['reason'] for u in refused if u.get('kind') == 'overlay site frame')
            rows[(before, 'refused: ' + ('relocated far word enters with another frame'
                                         if reason.startswith('relocated far word') else reason))] += 1
    print('4. reviewed code-overlay targets:')
    for key, n in sorted(rows.items()):
        print('  ', n, key)


if __name__ == '__main__':
    main()
