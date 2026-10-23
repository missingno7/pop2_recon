"""Fresh source-only RTLink controls; research, never an acceptance path.

The linker receives independently compiled/assembled source, not original bytes.
Output is confined to build/workers/near_public. No executable is run.
"""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
from assembler import assemble_asm, verify_assembler
from binding import declarations, fixes
from common import read_json, require, sha, write_json
from compiler import compile_c, load_lock
from mz import MZ

FLAGS = ['/c', '/AM', '/Oe', '/Gs', '/Zl', '/Gc']


def invoke(argv, directory, path):
    env = {'PATH': str(path), 'MSDOS_PATH': str(path), 'TEMP': '.', 'TMP': '.',
           'MSDOS_TEMP': '.', 'SYSTEMROOT': os.environ.get('SYSTEMROOT', r'C:\Windows')}
    result = subprocess.run(argv, cwd=directory, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=60,
                            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    log = result.stdout.decode('latin1')
    (directory / 'run.log').write_bytes(result.stdout)
    require(result.returncode == 0, log)
    return log


def run():
    work = ROOT / 'build/workers/near_public'
    work.mkdir(parents=True, exist_ok=True)
    sources = Path(__file__).parent
    old = read_json(ROOT / 'evidence/tu/state-step.json')
    lock = load_lock()
    runner = lock['runner']
    linker = Path('C:/tools/rtlink-plus-6.10/installed/RTLINK.EXE')
    require(sha(linker.read_bytes()) ==
            'ea6f57a470a26801086593d54aa99f6bf983dd024fc19b5e13361a07f43e17fb',
            'Research linker pin changed')
    caller = old['controls']['caller_only']['source_text']
    group = old['compiler']['source_text']
    callee = (ROOT / old['controls']['callee_only']['source']).read_text(encoding='ascii')
    marker = '\nchar far pascal UPDATE_STEP_DELTA(void)\n{'
    declarations_end = caller.index('\nchar far pascal INCREMENT_STATE_AND_UPDATE(void)\n{')
    backward = caller[:declarations_end] + group[group.index(marker):] + caller[declarations_end:]
    objects, inputs = {}, {}
    for name, source in [('GROUP', group), ('CALLER', caller), ('CALLEE', callee), ('BACK', backward)]:
        path = work / (name + '.C')
        path.write_bytes(source.encode('ascii'))
        result = compile_c(path, 'msc600', FLAGS, workdir=work / name)
        require(result.ok, result.log)
        objects[name] = result
        module = result.parsed
        code = next(s for s in module.segment_defs if s['class'] == 'CODE')
        raw = module.segments[code['name']]
        inputs[name] = {'source_text': source, 'source_sha256': result.source_sha256,
                        'object_sha256': result.obj_sha256, 'code_hex': raw.hex(),
                        'code_sha256': sha(raw), 'size': len(raw),
                        'declarations': declarations(module), 'fixups': fixes(module),
                        'fixupp_records': module.fixups}
    require(inputs['GROUP']['object_sha256'] == old['compiler']['object_sha256'],
            'Combined control differs from the prior complete object')
    require(inputs['CALLEE']['object_sha256'] == old['controls']['callee_only']['object_sha256'],
            'Accepted isolated callee object changed')
    for name in ['DEFS', 'PREFIX']:
        result = assemble_asm(sources / (name + '.ASM'), 'masm510', workdir=work / name)
        require(result.ok, result.log)
        objects[name] = result
        inputs[name] = {'source_sha256': result.source_sha256, 'object_sha256': result.obj_sha256}

    # The synthetic startup has a MODEND entry, outside the component reader's
    # subset. Assemble the existing source unchanged; never edit its object.
    startup = sources.parent / 'relocation-order/START.ASM'
    asm = verify_assembler('masm510')
    stage = work / 'START'
    stage.mkdir(exist_ok=True)
    raw = startup.read_bytes().replace(b'\r\n', b'\n')
    (stage / 'START.ASM').write_bytes(raw.replace(b'\n', b'\r\n'))
    (stage / 'START.OBJ').unlink(missing_ok=True)
    argv = [runner['path'], *runner['options'], asm['executable'], 'START.ASM,START.OBJ,NUL,NUL;']
    invoke(argv, stage, asm['directory'])
    start_object = (stage / 'START.OBJ').read_bytes()
    inputs['START'] = {'source_sha256': sha(raw), 'object_sha256': sha(start_object), 'argv': argv}

    links = {}
    for label, names in [('GROUP', ['GROUP']), ('SHIFT', ['PREFIX', 'GROUP']),
                         ('SPLIT', ['CALLER', 'CALLEE']), ('BACK', ['BACK'])]:
        stage = work / ('LINK_' + label)
        stage.mkdir(exist_ok=True)
        (stage / 'START.OBJ').write_bytes(start_object)
        for name in names + ['DEFS']:
            (stage / (name + '.OBJ')).write_bytes(objects[name].obj_bytes)
        response = 'SYNTAX FREEFORMAT\nVERBOSE\nOUTPUT PROBE\nMAP\n' + ''.join(
            'FILE ' + name + '\n' for name in ['START'] + names + ['DEFS'])
        (stage / 'PROBE.ARF').write_bytes(response.replace('\n', '\r\n').encode('ascii'))
        for suffix in ['EXE', 'MAP']:
            (stage / ('PROBE.' + suffix)).unlink(missing_ok=True)
        argv = [runner['path'], *runner['options'], str(linker), '@PROBE.ARF']
        log = invoke(argv, stage, linker.parent)
        require('warning' not in log.lower() and 'error' not in log.lower(), log)
        data = (stage / 'PROBE.EXE').read_bytes()
        mz = MZ.parse(data)
        image = mz.load_image(data)
        map_text = (stage / 'PROBE.MAP').read_text(encoding='latin1')
        publics = {m[2]: (int(m[0], 16), int(m[1], 16)) for m in re.findall(
            r'([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})\s+([A-Za-z_][A-Za-z_0-9]*)', map_text)}
        contributions, expected_relocations = {}, []
        for name in names:
            if name == 'PREFIX':
                continue
            module = objects[name].parsed
            code = next(s for s in module.segment_defs if s['class'] == 'CODE')
            raw = module.segments[code['name']]
            entry = next(p for p in module.publics if p['segment'] == code['name'] and p['offset'] == 0)
            seg, off = publics[entry['name']]
            base = seg * 16 + off
            linked = image[base:base+len(raw)]
            require(len(linked) == len(raw), 'Truncated linked contribution')
            occupied, fields = set(), []
            for fix in fixes(module):
                at = fix['segment_offset']
                target = fix['target']
                require(target['kind'] == 'external' and target['method'] == 6 and
                        fix['frame'] == {'kind': 'target_frame', 'target': target} and
                        fix['displacement'] is None, 'Unexpected probe fixup')
                target_seg, target_off = publics[target['name']]
                width = fix['field_width']
                require(not any(raw[at:at+width]), 'Unexpected encoded addend')
                if fix['self_relative']:
                    require(fix['loc_type'] == 1 and width == 2 and seg == target_seg,
                            'Near field changed CS or shape')
                    value = (target_off - (off + at + 2)) & 0xffff
                    observed = struct.unpack_from('<H', linked, at)[0]
                    kind = 'same-CS relative16'
                elif fix['loc_type'] == 3:
                    require(width == 4, 'Unexpected far width')
                    value, observed = [target_off, target_seg], list(struct.unpack_from('<HH', linked, at))
                    kind = 'far16:16'
                    expected_relocations.append(base + at + 2)
                else:
                    require(fix['loc_type'] == 1 and width == 2, 'Unexpected data field')
                    value, observed = target_off, struct.unpack_from('<H', linked, at)[0]
                    kind = 'DGROUP offset16'
                require(value == observed, 'Linker field differs from the predicted equation')
                fields.append({'offset': at, 'target': target['name'], 'kind': kind,
                               'predicted': value, 'observed': observed})
                occupied.update(range(at, at+width))
            literal_diffs = [i for i in range(len(raw)) if i not in occupied and raw[i] != linked[i]]
            require(not literal_diffs, 'Linker changed ordinary contribution bytes')
            contributions[name] = {'public_address': [seg, off], 'size': len(raw),
                                   'linked_code_hex': linked.hex(), 'fields': fields,
                                   'literal_differences': literal_diffs,
                                   'ordinary_bytes_compared': len(raw)-len(occupied)}
        require([r['image_offset'] for r in mz.relocations] == expected_relocations,
                'Unexpected MZ relocation sites in the source-only control')
        links[label] = {'response': response, 'argv': argv, 'exe_sha256': sha(data),
                        'map_sha256': sha(map_text.encode('latin1')), 'publics': publics,
                        'relocations': list(mz.relocations), 'contributions': contributions,
                        'warning_count': 0}
        print(label, {n: {'size': c['size'], 'literal_differences': c['literal_differences'],
                         'fields': [f for f in c['fields'] if 'relative' in f['kind'] or 'far' in f['kind']]}
                      for n, c in contributions.items()})
    require(links['SHIFT']['contributions']['GROUP']['public_address'][1] ==
            links['GROUP']['contributions']['GROUP']['public_address'][1] + 4,
            'Prefix did not move the whole grouped contribution by four bytes')
    report = {'schema': 1,
        'scope': 'Synthetic source-only compiler/linker observation; no original TU or ownership proof',
        'script_sha256': sha(Path(__file__).read_bytes()), 'flags': FLAGS,
        'toolchain_lock_sha256': sha((ROOT / 'layout/toolchain.lock.json').read_bytes()),
        'runner': runner, 'linker': {'path': str(linker), 'sha256': sha(linker.read_bytes())},
        'inputs': inputs, 'links': links}
    write_json(work / 'raw-report.json', report)
    report['inputs'] = {name: {key: value for key, value in spec.items()
                              if key in ('source_sha256', 'object_sha256', 'size', 'code_sha256', 'code_hex')}
                        for name, spec in inputs.items()}
    report['source_material'] = {
        'GROUP': 'evidence/tu/state-step.json: compiler.source_text',
        'CALLER': 'evidence/tu/state-step.json: controls.caller_only.source_text',
        'CALLEE': 'src/update_step_delta.c',
        'BACK': 'Same declarations and bodies, with UPDATE_STEP_DELTA before its caller',
        'START': 'evidence/rtlink-probe/relocation-order/START.ASM',
        'DEFS': 'evidence/rtlink-probe/near-public/DEFS.ASM',
        'PREFIX': 'evidence/rtlink-probe/near-public/PREFIX.ASM'}
    for spec in report['links'].values():
        spec.pop('argv')
    write_json(work / 'receipt.json', report)


if __name__ == '__main__':
    run()
