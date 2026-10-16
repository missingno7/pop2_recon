"""Synthetic RTLink order contrasts; no original/candidate bytes are link input."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT/'tools'))
from assembler import assemble_asm
from binding import fixes
from common import read_json, write_json, sha, require
from compiler import compile_c, load_lock
from mz import MZ


def run():
    work=ROOT/'build/workers/wave9_farcall_sol/order-probe'
    work.mkdir(parents=True,exist_ok=True)
    sources=Path(__file__).parent
    lock=load_lock(); runner=lock['runner']
    linker=Path('C:/tools/rtlink-plus-6.10/installed/RTLINK.EXE')
    require(sha(linker.read_bytes())=='ea6f57a470a26801086593d54aa99f6bf983dd024fc19b5e13361a07f43e17fb','Linker hash changed')
    objects={}; receipts={}
    for name in ['A','B','DEFS']:
        r=assemble_asm(sources/(name+'.ASM'),'masm510',workdir=work/name)
        require(r.ok,r.log)
        objects[name]=r
        receipts[name]={'source_sha256':r.source_sha256,'staged_source_sha256':r.staged_source_sha256,
                        'object_sha256':r.obj_sha256,'fixups':fixes(r.parsed),
                        'ledata':[{k:f[k] for k in ('data_offset','data_length','segment_index')} for f in r.parsed.fixups]}
    many=('extern void far pascal PROBE_X(void);\nextern void far pascal PROBE_Y(void);\n'
          'extern void far pascal ALIAS_X(void);\nvoid far pascal MANY_CALLS(void)\n{\n'+
          ''.join('    '+name+'();\n' for name in ['PROBE_X','PROBE_Y','ALIAS_X']*100)+'}\n')
    many_path=work/'MANY.C';many_path.write_bytes(many.encode('ascii'))
    r=compile_c(many_path,'msc600',['/c','/AM','/Oe','/Gs','/Zl','/Gc'],workdir=work/'MANY')
    require(r.ok,r.log);objects['MANY']=r
    receipts['MANY']={'source_sha256':r.source_sha256,'staged_source_sha256':r.staged_source_sha256,
                      'object_sha256':r.obj_sha256,'flags':r.flags,'fixups':fixes(r.parsed),
                      'ledata':[{k:f[k] for k in ('data_offset','data_length','segment_index')} for f in r.parsed.fixups]}
    # Startup is research-only: its ordinary MODEND start address is outside
    # the component reader's accepted no-entry OMF subset. Never alter it.
    stage=work/'START';stage.mkdir(exist_ok=True)
    start=(sources/'START.ASM').read_bytes()
    (stage/'START.ASM').write_bytes(start.replace(b'\n',b'\r\n'))
    asm=lock['assemblers']['masm510']
    argv=[runner['path'],*runner['options'],asm['executable'],'START.ASM,START.OBJ,NUL,NUL;']
    env={'PATH':asm['directory'],'MSDOS_PATH':asm['directory'],'TEMP':'.','TMP':'.','MSDOS_TEMP':'.',
         'SYSTEMROOT':os.environ.get('SYSTEMROOT',r'C:\Windows')}
    c=subprocess.run(argv,cwd=stage,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                     timeout=30,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    require(c.returncode==0,c.stdout.decode('latin1'))
    start_obj=(stage/'START.OBJ').read_bytes()
    objects['START']=SimpleNamespace(obj_bytes=start_obj,parsed=None)
    receipts['START']={'source_sha256':sha(start),'object_sha256':sha(start_obj),'argv':argv}
    links={}
    for label,names in [('AB',['START','A','B','DEFS']),('BA',['START','B','A','DEFS']),('MSC',['START','MANY','DEFS'])]:
        stage=work/label;stage.mkdir(exist_ok=True)
        for name in names:(stage/(name+'.OBJ')).write_bytes(objects[name].obj_bytes)
        arf='SYNTAX FREEFORMAT\nVERBOSE\nOUTPUT ORDER\nMAP\n'+''.join('FILE '+n+'\n' for n in names)
        (stage/'ORDER.ARF').write_bytes(arf.replace('\n','\r\n').encode('ascii'))
        output=stage/'ORDER.EXE'
        if output.exists():output.unlink()
        argv=[runner['path'],*runner['options'],str(linker),'@ORDER.ARF']
        env={'PATH':str(linker.parent),'MSDOS_PATH':str(linker.parent),'TEMP':'.','TMP':'.','MSDOS_TEMP':'.',
             'SYSTEMROOT':os.environ.get('SYSTEMROOT',r'C:\Windows')}
        c=subprocess.run(argv,cwd=stage,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                         timeout=60,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        log=c.stdout.decode('latin1');(stage/'link.log').write_bytes(log.encode('latin1'))
        require(c.returncode==0 and output.is_file(),'Link failed: '+log)
        data=output.read_bytes();mz=MZ.parse(data)
        map_text=(stage/'ORDER.MAP').read_text(encoding='latin1')
        publics={m[2]:(int(m[0],16),int(m[1],16)) for m in re.findall(r'([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})\s+([A-Za-z_][A-Za-z_0-9]*)',map_text)}
        labelled={}; module_rows={}
        for name in names:
            obj=objects[name].parsed
            if obj is None:continue
            for seg in obj.segment_defs:
                ownpub=[p for p in obj.publics if p['segment']==seg['name'] and p['offset']==0]
                if not ownpub:continue
                pub=ownpub[0];linkseg,linkoff=publics[pub['name']]
                base=linkseg*16+linkoff
                fields=[]
                for fix in fixes(obj):
                    if fix['segment']!=seg['name']:continue
                    require(fix['loc_type']==3,'Unexpected probe fixup')
                    site=base+fix['segment_offset']+2
                    labelled[site]={'module':name,'segment':seg['name'],'symbol':fix['target']['name'],
                                    'field':fix['segment_offset'],'ledata_chunk':next(f['data_offset'] for f in obj.fixups if fix in f['resolved'])}
                    fields.append(site)
                module_rows[name+':'+seg['name']]=fields
        rows=[{'index':r['index'],'site':r['image_offset'],**labelled[r['image_offset']]} for r in mz.relocations]
        links[label]={'argv':argv,'response':arf,'log':log,'exe_sha256':sha(data),'map':map_text,
                      'relocations':rows,'module_segment_fixup_sites':module_rows}
        print(label,'relocations',len(rows),'\n',rows if label!='MSC' else rows[:12])
    report={'schema':1,'purpose':'Synthetic linker observation only; no acceptance ownership or original-linker identity',
            'runner':runner,'linker':{'path':str(linker),'sha256':sha(linker.read_bytes())},
            'inputs':receipts,'links':links}
    write_json(work/'raw-report.json',report)
    write_json(work/'summary.json',summarize(report))
    print('REPORT',work/'raw-report.json')


def summarize(report):
    """Assert full orders, then keep a compact receipt rather than bulk fixups."""
    summary={'schema':1,'purpose':report['purpose'],'runner':report['runner'],'linker':report['linker'],
             'source_script_sha256':sha(Path(__file__).read_bytes()),'inputs':{},'links':{}}
    for name,spec in report['inputs'].items():
        summary['inputs'][name]={k:spec[k] for k in ('source_sha256','object_sha256')}
        if 'staged_source_sha256' in spec:
            summary['inputs'][name]['staged_source_sha256']=spec['staged_source_sha256']
    for label,link in report['links'].items():
        rows=link['relocations']
        segments=list(dict.fromkeys(r['segment'] for r in rows))
        group_order={}
        for seg in segments:
            names=[r['symbol'] for r in rows if r['segment']==seg]
            runs=[n for i,n in enumerate(names) if i==0 or names[i-1]!=n]
            require(len(runs)==len(set(runs)), 'Probe target buckets interleave')
            group_order[seg]=runs
        orders=[]
        for name,seg,symbol in dict.fromkeys((r['module'],r['segment'],r['symbol']) for r in rows):
            expected=[f['segment_offset'] for f in report['inputs'][name]['fixups']
                      if f['segment']==seg and f['target']['name']==symbol]
            observed=[r for r in rows if (r['module'],r['segment'],r['symbol'])==(name,seg,symbol)]
            require(expected==[r['field'] for r in observed], 'Probe changed within-symbol FIXUPP order')
            orders.append({'input':name+':'+seg+':'+symbol,'count':len(expected),
                           'ordered_offsets_sha256':sha(','.join(map(str,expected)).encode('ascii')),
                           'operand_offsets':expected if len(expected)<=4 else expected[:4]+expected[-4:],
                           'original_indices': [r['index'] for r in observed] if len(expected)<=4 else
                                               [observed[0]['index'],observed[-1]['index']]})
        alias_positions=re.findall(r'([0-9A-Fa-f]{4}:[0-9A-Fa-f]{4})\s+(?:ALIAS_X|PROBE_X)\b',link['map'])
        require(len(set(alias_positions))==1,'Probe aliases do not share an address')
        summary['links'][label]={'response':link['response'],'exe_sha256':link['exe_sha256'],
                                'warning_count':link['log'].lower().count('warning '),
                                'relocation_count':len(rows),'target_group_order_by_output_segment':group_order,
                                'within_symbol_orders_checked':orders,'equal_address_symbols_stay_separate':True}
        require(summary['links'][label]['warning_count']==0,'Probe linker warnings')
    work=ROOT/'build/workers/wave9_farcall_sol/order-probe'
    from omf import OmfReader
    obj=OmfReader().read((work/'MANY/UNIT.OBJ').read_bytes())
    chunks=[]
    for record in obj.fixups:
        offsets=[f['segment_offset'] for f in record['resolved'] if f['kind']=='fixup']
        if not offsets:continue
        require(all(a>b for a,b in zip(offsets,offsets[1:])), 'MSC probe FIXUPPs are not descending per LEDATA')
        chunks.append({'ledata_offset':record['data_offset'],'ledata_length':record['data_length'],
                       'fixup_count':len(offsets),'first_operand_offset':offsets[0],
                       'last_operand_offset':offsets[-1]})
    require(len({c['ledata_offset'] for c in chunks})>1,'MSC probe did not cross LEDATA chunks')
    summary['msc600']={'flags':report['inputs']['MANY']['flags'],'calls':300,'fixupp_chunks':chunks,
                       'within_chunk_order':'descending','across_chunks':'LEDATA traversal, not global site sort'}
    summary['observations']=[
        'SHARED is one combined output CODE segment across A and B; its target buckets contain both modules before the next target. Reversing A/B reverses module contributions inside each bucket.',
        'SECOND is another CODE segment in A and has its own buckets. Group scope in these probes is combined output segment, not object module or LEDATA record.',
        'PROBE_X and ALIAS_X have equal public addresses but distinct target buckets; bucket keys are linker symbol identities, not bound address pairs.',
        'All module/segment/symbol projections preserve complete raw FIXUPP traversal, including MSC descending fields within each LEDATA and the transition to the next chunk.',
        'No general between-target bucket permutation, original linker version, original TU partition or natural output placement follows from these probes.']
    return summary


if __name__=='__main__':run()
