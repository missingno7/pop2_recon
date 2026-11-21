"""RTLink 6.10 control: what segment does an overlay relocation record carry?

Research only, never an acceptance path. Seven source files (MSC 6.00 with the
project flags, plus one MASM pointer table) are linked by the pinned research
RTLink 6.10: START and ROOTC resident, and two sections ONE and TWO in one
BEGINAREA, so they share a load region. ONE holds four code segments, one of
them (O1Z) relocation-free. Output stays in build/workers/site_frames/. No
executable runs and no original bytes are linker input.

Questions:
1. Is each overlay relocation record's `segment` the frame (paragraph floor of
   the start) of the code segment containing the site, with `offset` measured
   from that frame?
2. Does every relocated far pointer word (CALL FAR operand or `dd`) store the
   callee segment's frame, the same value as (1) for that segment?
3. Are frames monotone in position, and does a relocation-free segment get its
   own frame that no record names?
4. Contrast: for a segment inside a group (RTLink's own $$OVLVEC in $$OVLMGR),
   do the records still name the segment floor while calls use the group frame?

Run `python evidence/rtlink-probe/site-frames/probe.py`; the checked
receipt.json is the compact copy of build/workers/site_frames/receipt.json.
"""
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'evidence/rtlink-probe/near-public'))
from assembler import assemble_asm, verify_assembler
from common import require, sha, write_json
from compiler import compile_c, load_lock
from mz import MZ
from probe import FLAGS, invoke
from probe_rtlink import parse_overlays

CALLS = ''.join('    x = ROOTF(x) ^ 0x%04x;\n' % (0x1111 * (i % 15 + 1)) for i in range(40))
SOURCES = {
    'ROOTC': """
extern int far pascal O1A1(int);
extern int far pascal O1B1(int);
extern int far pascal O2A1(int);
extern int far pascal O2B2(int);
int (far pascal *root_ptrs[2])(int) = { O1B1, O2A1 };
int far pascal ROOTF(int x) { return x + 5; }
int far pascal ROOT1(int x) { return O1A1(x) + O1B1(x) + O2A1(x) + O2B2(x) + root_ptrs[x & 1](x); }
""",
    'O1A': """
extern int far pascal O1B1(int);
extern int far pascal O1B2(int);
extern int far pascal ROOTF(int);
int far pascal O1A2(int x) { return x * 3 + 1; }
int far pascal O1A1(int x)
{
""" + CALLS + """    return O1A2(x) + O1B1(x) + O1B2(x);
}
""",
    'O1Z': """
int far pascal O1Z1(int x) { return x ^ 0x5a5a; }
int far pascal O1Z2(int x) { return O1Z1(x) + 3; }
""",
    'O1B': """
extern int far pascal O1A2(int);
extern int far pascal ROOTF(int);
extern int far pascal O1Z2(int);
int far pascal O1B1(int x) { return x - 7; }
int far pascal O1B2(int x) { return O1B1(x) + O1A2(x) + ROOTF(x) + O1Z2(x); }
""",
    'O2A': """
extern int far pascal O2B1(int);
extern int far pascal ROOTF(int);
int far pascal O2A1(int x) { return O2B1(x) + ROOTF(x) + 9; }
""",
    'O2B': """
extern int far pascal O2A1(int);
extern int far pascal O1A1(int);
int far pascal O2B1(int x) { return x * 5; }
int far pascal O2B2(int x) { return O2A1(x) + O2B1(x) + O1A1(x); }
""",
}
LAYOUT = {'root': ['START', 'ROOTC'], 'ONE': ['O1A', 'O1Z', 'O1B', 'OVPTR'], 'TWO': ['O2A', 'O2B']}


def parse_map(text):
    """Segment rows per section and publics (frame, offset, residency)."""
    sections, current = {}, None
    for line in text.splitlines():
        head = re.match(r'^ (Resident|Overlay ([0-9A-F]+)H)\s*$', line)
        if head:
            current = 1 if head.group(1) == 'Resident' else int(head.group(2), 16)
            sections[current] = []
            continue
        row = re.match(r'^ ([0-9A-F]{5})H ([0-9A-F]{5})H ([0-9A-F]{5})H (\S+)\s+(\S+)', line)
        if row and current is not None:
            sections[current].append({'start': int(row[1], 16), 'length': int(row[3], 16),
                                      'name': row[4], 'class': row[5]})
        elif not line.strip():
            current = None
    publics = {m[3]: {'frame': int(m[0], 16), 'offset': int(m[1], 16), 'kind': m[2],
                      'section': int(m[4], 16) if m[4] else 1}
               for m in re.findall(r'([0-9A-F]{4}):([0-9A-F]{4})\s+(Res|Ovl|Abs)\s+(\S+)(?: \(([0-9A-F]+)H\))?',
                                   text)}
    return sections, publics


def run():
    work = ROOT / 'build/workers/site_frames'
    work.mkdir(parents=True, exist_ok=True)
    lock = load_lock()
    runner = lock['runner']
    linker = Path('C:/tools/rtlink-plus-6.10/installed/RTLINK.EXE')
    require(sha(linker.read_bytes()) ==
            'ea6f57a470a26801086593d54aa99f6bf983dd024fc19b5e13361a07f43e17fb', 'Research linker pin changed')
    link = work / 'LINK'
    link.mkdir(exist_ok=True)
    inputs = {}
    for name, text in SOURCES.items():
        result = compile_c(text.lstrip('\n'), 'msc600', FLAGS, basename=name, workdir=work / name)
        require(result.ok, result.log)
        (link / (name + '.OBJ')).write_bytes(result.obj_bytes)
        inputs[name] = {'source_sha256': result.source_sha256, 'object_sha256': result.obj_sha256}
    result = assemble_asm(Path(__file__).parent / 'OVPTR.ASM', 'masm510', workdir=work / 'OVPTR')
    require(result.ok, result.log)
    (link / 'OVPTR.OBJ').write_bytes(result.obj_bytes)
    inputs['OVPTR'] = {'source_sha256': result.source_sha256, 'object_sha256': result.obj_sha256}
    masm = verify_assembler('masm510')
    stage = work / 'START'
    stage.mkdir(exist_ok=True)
    raw = (ROOT / 'evidence/rtlink-probe/relocation-order/START.ASM').read_bytes().replace(b'\r\n', b'\n')
    (stage / 'START.ASM').write_bytes(raw.replace(b'\n', b'\r\n'))
    (stage / 'START.OBJ').unlink(missing_ok=True)
    invoke([runner['path'], *runner['options'], masm['executable'], 'START.ASM,START.OBJ,NUL,NUL;'],
           stage, masm['directory'])
    (link / 'START.OBJ').write_bytes((stage / 'START.OBJ').read_bytes())
    inputs['START'] = {'source_sha256': sha(raw), 'object_sha256': sha((stage / 'START.OBJ').read_bytes())}

    response = ('SYNTAX FREEFORMAT\nVERBOSE\nOUTPUT PROBE\nMAP\n' +
                ''.join('FILE %s\n' % n for n in LAYOUT['root']) + 'BEGINAREA\n' +
                ''.join('  SECTION = "%s"\n' % s + ''.join('    FILE %s\n' % n for n in LAYOUT[s])
                        for s in ('ONE', 'TWO')) + 'ENDAREA\n')
    (link / 'PROBE.ARF').write_bytes(response.replace('\n', '\r\n').encode('ascii'))
    for suffix in ('EXE', 'MAP'):
        (link / ('PROBE.' + suffix)).unlink(missing_ok=True)
    log = invoke([runner['path'], *runner['options'], str(linker), '@PROBE.ARF'], link, linker.parent)
    require('warning' not in log.lower() and 'error' not in log.lower(), log)
    data = (link / 'PROBE.EXE').read_bytes()
    map_text = (link / 'PROBE.MAP').read_text(encoding='latin1')
    sections, publics = parse_map(map_text)
    mz = MZ.parse(data)
    image = mz.load_image(data)

    # Code segments (one per input file) with their map start and frame.
    segs = {}
    for section, rows in sections.items():
        for row in rows:
            if row['class'] == 'CODE' and row['name'].endswith('_TEXT') and row['length']:
                file = row['name'][:-5]
                if file in SOURCES or file in ('OVPTR', 'ENTRY'):
                    segs[file] = dict(row, section=section, frame=row['start'] // 16)
    owner = {}
    for file, seg in segs.items():
        for name, pub in publics.items():
            linear = pub['frame'] * 16 + pub['offset']
            if pub['kind'] in ('Res', 'Ovl') and pub['section'] == seg['section'] and seg['start'] <= linear < seg['start'] + seg['length'] \
                    and not name.startswith('$$'):
                require(pub['frame'] == seg['frame'], f'{name} public frame is not its segment floor')
                owner[name] = file

    # Overlay descriptors: located by scanning the resident image for the validated layout.
    overlay_ids = sorted(s for s in sections if s != 1)
    table = next(off for off in range(mz.header_size, mz.file_image_size - 18 * len(overlay_ids))
                 if _parses(data, off, len(overlay_ids)))
    overlays = parse_overlays(data, table, len(overlay_ids), allowed_final_short=15)

    rows_all = [dict(r, section=s) for s, rows in sections.items() for r in rows if r['length']]

    def segment_at(section, linear):
        """The map segment holding the site: a probe code file, else (name, frame floor)."""
        hits = [r for r in rows_all if r['section'] == section and r['start'] <= linear < r['start'] + r['length']]
        require(len(hits) == 1, f'site {linear:#x} not in exactly one map segment')
        name = hits[0]['name']
        file = name[:-5] if name.endswith('_TEXT') and name[:-5] in segs else name
        return file, (segs[file]['frame'] if file in segs else hits[0]['start'] // 16), hits[0]['start']

    records, words = [], []
    spaces = {1: (image, list(mz.relocations), 0)}
    for ov in overlays:
        rows = []
        for j in range(ov.relocation_count):
            off, seg = struct.unpack_from('<HH', data, ov.relocation_offset + 4 * j)
            rows.append({'index': j, 'segment': seg, 'offset': off})
        code = data[ov.code_offset:ov.code_offset + ov.size_paragraphs * 16]
        spaces[ov.overlay_id] = (code, rows, ov.link_segment)
    for section, (code, rows, link_segment) in spaces.items():
        for r in rows:
            linear = r['segment'] * 16 + r['offset']
            pos = linear - link_segment * 16
            file, frame, start = segment_at(section, linear)
            stored = struct.unpack_from('<H', code, pos)[0]
            records.append({'section': section, 'index': r['index'], 'site_segment': r['segment'],
                            'site_offset': r['offset'], 'site_linear': linear, 'site_file': file,
                            'file_frame': frame, 'file_start': start, 'code_file': file in segs,
                            'site_segment_is_file_frame': r['segment'] == frame})
            target_off = struct.unpack_from('<H', code, pos - 2)[0]
            callee = [n for n, p in publics.items() if p['kind'] in ('Res', 'Ovl') and
                      (p['frame'], p['offset']) == (stored, target_off) and n in owner]
            # Equal link addresses in sections sharing the area: name every candidate.
            callee = '|'.join(sorted(callee)) or None
            is_call = pos >= 3 and code[pos - 3] == 0x9a
            words.append({'section': section, 'site_file': file, 'site_linear': linear,
                          'kind': 'CALL FAR' if is_call else 'far pointer', 'segment': stored,
                          'offset': target_off, 'callee': callee,
                          'callee_file': '|'.join(owner[c] for c in callee.split('|')) if callee else None,
                          'callee_file_frame': segs[owner[callee.split('|')[0]]]['frame'] if callee else None})
    frames_by_file = {f: s['frame'] for f, s in segs.items()}
    named = {(r['section'], r['site_segment']) for r in records}
    summary = {
        'records': len(records),
        'site_segment_equals_containing_file_frame': sum(r['site_segment_is_file_frame'] for r in records),
        'overlay_records': sum(r['section'] != 1 for r in records),
        'far_words': len(words),
        'far_words_to_named_public': sum(w['callee'] is not None for w in words),
        'far_word_segment_equals_callee_file_frame':
            sum(w['callee'] is not None and w['segment'] == w['callee_file_frame'] for w in words),
        'files_without_records': sorted(f for f, s in segs.items() if (s['section'], s['frame']) not in named),
        'frames_monotone_in_position': all(
            a['frame'] <= b['frame'] for sec in sections for a, b in zip(
                *(lambda l: (l, l[1:]))(sorted((s for s in segs.values() if s['section'] == sec),
                                               key=lambda s: s['start'])))),
    }
    unresolved = [w for w in words if w['callee'] is None]
    report = {
        'schema': 1,
        'scope': 'Synthetic source-only RTLink 6.10 observation; no original TU, linker identity or ownership',
        'script_sha256': sha(Path(__file__).read_bytes()), 'flags': FLAGS,
        'linker_sha256': sha(linker.read_bytes()), 'runner_sha256': runner['sha256'],
        'inputs': inputs, 'response': response, 'exe_sha256': sha(data),
        'map_sha256': sha(map_text.encode('latin1')), 'descriptor_table_file_offset': table,
        'overlays': [{'overlay_id': o.overlay_id, 'link_segment': o.link_segment,
                      'relocation_count': o.relocation_count, 'size_paragraphs': o.size_paragraphs}
                     for o in overlays],
        'code_segments': {f: {k: s[k] for k in ('section', 'start', 'length', 'frame')} for f, s in segs.items()},
        'summary': summary, 'records': records, 'far_words': words, 'unresolved_far_words': unresolved,
    }
    write_json(work / 'raw-report.json', report)
    # Headline observations; a rerun that changes any of them fails here.
    require(summary['site_segment_equals_containing_file_frame'] == summary['records'], 'Site segment != frame')
    require(summary['far_word_segment_equals_callee_file_frame'] == summary['far_words_to_named_public'],
            'Far word segment != callee frame')
    require(summary['files_without_records'] == ['O1Z'] and summary['frames_monotone_in_position'],
            'Relocation-free control changed')
    per_file = {}
    for r in records:
        row = per_file.setdefault('%d:%s' % (r['section'], r['site_file']), {
            'frame': '%04x' % r['file_frame'], 'start': r['file_start'], 'records': 0, 'site_offsets': []})
        row['records'] += 1
        row['site_offsets'].append(r['site_offset'])
    for row in per_file.values():
        row['site_offsets'] = [min(row['site_offsets']), max(row['site_offsets'])]
    edges = {}
    for w in words:
        if w['callee']:
            key = '%d:%s -> %s %s %04x:%04x' % (w['section'], w['site_file'], w['kind'], w['callee'],
                                                w['segment'], w['offset'])
            edges[key] = edges.get(key, 0) + 1
    receipt = {k: report[k] for k in ('schema', 'scope', 'script_sha256', 'flags', 'linker_sha256',
                                      'runner_sha256', 'inputs', 'response', 'exe_sha256', 'map_sha256',
                                      'descriptor_table_file_offset', 'overlays', 'code_segments', 'summary')}
    # Contrast: $$OVLVEC is a member of group $$OVLMGR (origin 0007:0 in the map). Its records
    # name the segment floor, but the root calls entering its vectors use the group frame.
    vec = next(r for r in sections[1] if r['name'] == '$$OVLVEC')
    origin = int(re.search(r'([0-9A-F]{4}):0\s+\$\$OVLMGR', map_text)[1], 16)
    vec_records = {r['site_segment'] for r in records if r['site_file'] == '$$OVLVEC'}
    vec_calls = {w['segment'] for w in words if w['site_file'] == 'ROOTC' and w['kind'] == 'CALL FAR'
                 and vec['start'] <= w['segment'] * 16 + w['offset'] < vec['start'] + vec['length']}
    require(vec_records == {vec['start'] // 16} and vec_calls == {origin} and origin != vec['start'] // 16,
            'Grouped-segment contrast changed')
    receipt.update(grouped_segment_contrast={
        'segment': '$$OVLVEC', 'group': '$$OVLMGR', 'segment_floor': '%04x' % (vec['start'] // 16),
        'group_frame': '%04x' % origin, 'record_site_segments': ['%04x' % s for s in sorted(vec_records)],
        'call_word_segments': ['%04x' % s for s in sorted(vec_calls)]})
    receipt.update(records_by_segment=per_file, far_words_to_probe_publics=edges,
                   root_calls_into_overlays=sorted({'%04x:%04x' % (w['segment'], w['offset']) for w in words
                                                    if w['site_file'] == 'ROOTC' and w['callee'] is None}),
                   observations=[
                       'Every relocation record (root MZ and both overlay tables) stores the frame of the '
                       'segment holding its site, i.e. the paragraph floor of that segment start, with the '
                       'offset measured from it (O1A sites up to 0127:01e7, O1B at 0148:0025..003e).',
                       'Every relocated far word naming a probe public stores the callee segment frame: '
                       'direct same-overlay calls, overlay-to-root calls, the MASM dd table and the vector '
                       'jump words alike.',
                       'O1Z has no far reference and therefore no record; its frame 0146 is named only by '
                       'the O1B call word. Records of frames 0127 and 0148 bracket it.',
                       'Root calls and the root DGROUP pointers enter overlays through $$OVLVEC records; '
                       'O2B calls O1A1 in the other section of the area through a vector as well.',
                       'Sections ONE and TWO share link segment 0127: O1A2 and O2A1 both are 0127:0000.',
                       'A grouped segment differs: $$OVLVEC records name its floor 00c4 while root calls '
                       'into it use the $$OVLMGR group frame 0007. Site frame = CS frame only for a code '
                       'segment outside any group (MSC medium-model NAME_TEXT).'])
    write_json(work / 'receipt.json', receipt)
    print(json.dumps(summary, indent=1))
    for f, s in sorted(segs.items(), key=lambda kv: (kv[1]['section'], kv[1]['start'])):
        print('section %d %-6s start %05x len %04x frame %04x' % (s['section'], f, s['start'], s['length'],
                                                                     s['frame']))
    for r in records:
        print('rec sec %d #%d %04x:%04x in %s frame %04x %s' % (
            r['section'], r['index'], r['site_segment'], r['site_offset'], r['site_file'], r['file_frame'],
            'OK' if r['site_segment_is_file_frame'] else 'DIFF'))
    for w in words:
        print('word sec %d %s %s %04x:%04x -> %s (%s frame %s)' % (
            w['section'], w['site_file'], w['kind'], w['segment'], w['offset'], w['callee'], w['callee_file'],
            '%04x' % w['callee_file_frame'] if w['callee_file_frame'] is not None else None))
    return report


def _parses(data, off, count):
    try:
        parse_overlays(data, off, count, allowed_final_short=15)
        return True
    except (ValueError, struct.error):
        return False


import json  # noqa: E402

if __name__ == '__main__':
    run()
