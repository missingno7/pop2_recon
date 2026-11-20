"""Source-only RTLink control: does each medium-model .C file get its own far-call frame?

Research only. Three small files are compiled with the project's MSC 6.00 flags
under distinct DOS basenames (so each emits its own NAME_TEXT segment), linked
with the pinned research RTLink 6.10, and the MZ relocations of every CALL FAR
are read back. Output stays in build/workers/rtlink_frames/. No executable runs
and no original bytes are linker input.
"""
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'evidence/rtlink-probe/near-public'))
from common import require, sha, write_json
from compiler import compile_c, load_lock
from mz import MZ
from probe import FLAGS, invoke  # same flags and runner environment as the near-public controls

# FILEA: A1 calls A2 in the same file (expected PUSH CS; CALL near).
# FILEB: calls into FILEA (expected CALL FAR); B2 calls B1 in the same file.
# FILEC: calls into FILEB and FILEA.
SOURCES = {
    'FILEA': """
extern int far pascal B1(int);
int far pascal A2(int x) { return x * 3 + 1; }
int far pascal A1(int x) { return A2(x) + B1(x); }
""",
    'FILEB': """
extern int far pascal A1(int);
extern int far pascal A2(int);
int far pascal B1(int x) { return x - 7; }
int far pascal B2(int x) { return B1(x) + A1(x) + A2(x); }
char far pascal B3(char c) { return (char)(c ^ 0x55); }
""",
    'FILEC': """
extern int far pascal B2(int);
extern int far pascal A1(int);
int far pascal C1(int x) { return B2(x) + A1(x); }
""",
}


def run():
    work = ROOT / 'build/workers/rtlink_frames'
    work.mkdir(parents=True, exist_ok=True)
    lock = load_lock()
    runner = lock['runner']
    linker = Path('C:/tools/rtlink-plus-6.10/installed/RTLINK.EXE')
    require(sha(linker.read_bytes()) ==
            'ea6f57a470a26801086593d54aa99f6bf983dd024fc19b5e13361a07f43e17fb',
            'Research linker pin changed')
    objects, inputs = {}, {}
    for name, text in SOURCES.items():
        result = compile_c(text.lstrip('\n'), 'msc600', FLAGS, basename=name, workdir=work / name)
        require(result.ok, result.log)
        module = result.parsed
        code = [s for s in module.segment_defs if s['class'] == 'CODE']
        require(len(code) == 1, 'Expected one CODE segment per file')
        objects[name] = result
        inputs[name] = {'source_sha256': result.source_sha256, 'object_sha256': result.obj_sha256,
                        'segment': code[0]['name'], 'alignment': code[0]['alignment'],
                        'size': code[0]['length'],
                        'publics': {p['name']: p['offset'] for p in module.publics}}
    start = ROOT / 'evidence/rtlink-probe/relocation-order/START.ASM'
    stage = work / 'START'
    stage.mkdir(exist_ok=True)
    raw = start.read_bytes().replace(b'\r\n', b'\n')
    (stage / 'START.ASM').write_bytes(raw.replace(b'\n', b'\r\n'))
    (stage / 'START.OBJ').unlink(missing_ok=True)
    masm = lock['assemblers']['masm510']
    invoke([runner['path'], *runner['options'], masm['executable'], 'START.ASM,START.OBJ,NUL,NUL;'],
           stage, masm['directory'])
    link = work / 'LINK'
    link.mkdir(exist_ok=True)
    (link / 'START.OBJ').write_bytes((stage / 'START.OBJ').read_bytes())
    for name, result in objects.items():
        (link / (name + '.OBJ')).write_bytes(result.obj_bytes)
    response = 'SYNTAX FREEFORMAT\nVERBOSE\nOUTPUT PROBE\nMAP\n' + ''.join(
        'FILE ' + n + '\n' for n in ['START', *SOURCES])
    (link / 'PROBE.ARF').write_bytes(response.replace('\n', '\r\n').encode('ascii'))
    for suffix in ('EXE', 'MAP'):
        (link / ('PROBE.' + suffix)).unlink(missing_ok=True)
    log = invoke([runner['path'], *runner['options'], str(linker), '@PROBE.ARF'], link, linker.parent)
    require('warning' not in log.lower() and 'error' not in log.lower(), log)
    data = (link / 'PROBE.EXE').read_bytes()
    mz = MZ.parse(data)
    image = mz.load_image(data)
    map_text = (link / 'PROBE.MAP').read_text(encoding='latin1')
    publics = {m[2]: (int(m[0], 16), int(m[1], 16)) for m in re.findall(
        r'([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})\s+([A-Za-z_][A-Za-z_0-9]*)', map_text)}
    owner = {p: n for n, spec in inputs.items() for p in spec['publics']}
    calls = []
    for r in mz.relocations:
        at = r['image_offset'] - 3
        if at >= 0 and image[at] == 0x9a:
            off, seg = struct.unpack_from('<HH', image, at + 1)
            callee = next((p for p, a in publics.items() if a == (seg, off)), None)
            calls.append({'site': at, 'frame': seg, 'offset': off, 'callee': callee,
                          'callee_file': owner.get(callee)})
    near = []
    at = image.find(b'\x0e\xe8')
    while at != -1:
        near.append(at)
        at = image.find(b'\x0e\xe8', at + 1)
    frames = {}
    for name, spec in inputs.items():
        addresses = {publics[p] for p in spec['publics']}
        segs = {s for s, _ in addresses}
        require(len(segs) == 1, f'{name} publics span several frames: {addresses}')
        seg = segs.pop()
        # Segment start = linear address of any public minus its offset inside the object.
        p, at = next(iter(spec['publics'].items()))
        frames[name] = {'frame': seg, 'public_addresses': sorted(addresses),
                        'segment_start': publics[p][0] * 16 + publics[p][1] - at}
    distinct = len({f['frame'] for f in frames.values()}) == len(frames)
    gaps = []
    ordered = sorted(inputs, key=lambda n: frames[n]['frame'])
    for a, b in zip(ordered, ordered[1:]):
        end = frames[a]['segment_start'] + inputs[a]['size']
        begin = frames[b]['segment_start']
        require(begin - frames[b]['frame'] * 16 < 16, 'Frame is not the paragraph floor of the file start')
        gaps.append({'after': a, 'before': b, 'end': end, 'next_start': begin,
                     'fill_hex': image[end:begin].hex()})
    report = {'schema': 1,
              'scope': 'Synthetic source-only compiler/linker observation; no original TU or ownership proof',
              'script_sha256': sha(Path(__file__).read_bytes()), 'flags': FLAGS,
              'linker_sha256': sha(linker.read_bytes()), 'inputs': inputs,
              'exe_sha256': sha(data), 'map_sha256': sha(map_text.encode('latin1')),
              'publics': publics, 'frames': frames, 'distinct_frame_per_file': distinct,
              'far_calls': calls, 'near_call_sites': near, 'inter_file_fill': gaps}
    write_json(work / 'receipt.json', report)
    print('distinct frame per file:', distinct)
    for n in ordered:
        print(n, inputs[n]['segment'], f"size={inputs[n]['size']}", 'frame=%04x' % frames[n]['frame'],
              ['%04x:%04x' % a for a in frames[n]['public_addresses']])
    for c in calls:
        print('CALL FAR %04x:%04x -> %s (%s)' % (c['frame'], c['offset'], c['callee'], c['callee_file']))
    print('PUSH CS; CALL near sites:', len(near))
    for g in gaps:
        print('fill', g)


if __name__ == '__main__':
    run()
