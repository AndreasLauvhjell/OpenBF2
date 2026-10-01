"""[TEMP] Exact manager tick/helpers with declared native typed collaborators."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess
import sys

CODE=Path(__file__).resolve().parents[2]
AI='AI/AIDLL/AI/'
spec=importlib.util.spec_from_file_location('decls',Path(__file__).with_name('sai_states_runtime.py'))
decls=importlib.util.module_from_spec(spec)
spec.loader.exec_module(decls)
MODES='ctor_poison ctor_firstwins tick_cycle tick_side_nonzero tick_callback_side0 tick_callback_side1 tick_callback_side2 tick_callback_cursor tick_high_cursor tick_size_grow tick_size_shrink tick_zero_size tick_artillery_order art_inactive art_equal art_future art_first art_second art_both art_time_nan art_start_nan art_duration_nan art_enable_second art_disable_second art_duration_change art_grow art_shrink art_replace art_rearm_first'.split()
MODES += 'tick_message_mutation tick_human_mutation art_just_after'.split()

def generate(out):
    out.mkdir(parents=True,exist_ok=True)
    path=CODE/'BF2'/AI/'AIBot/SquadManager.cpp'
    source=path.read_text(encoding='utf-8')
    signatures=[r'SquadManager::SquadManager\(\)',r'void SquadManager::update\([^\n]+\)',
                r'void SquadManager::updateArtilleryStatus\([^\n]+\)',
                r'void SquadManager::clearCommanderMessages\(\)',
                r'void SquadManager::clearPlayerValidPositions\(\)']
    evidence=[]
    for signature in signatures:
        matches=re.findall('^'+signature+r'\s*(?::[\s\S]*?)?\{[\s\S]*?^}',source,re.M)
        if len(matches)!=1: raise RuntimeError('Expected one complete method: '+signature)
        evidence.append(dict(source=path.as_posix(),source_sha256=hashlib.sha256(source.encode()).hexdigest(),
                             body=matches[0],body_sha256=hashlib.sha256(matches[0].encode()).hexdigest()))
    (out/'actual.cpp').write_text('// [TEMP] Complete verbatim production definitions.\n#include "'
        +Path(__file__).with_suffix('.h').as_posix()+'"\nnamespace dice { namespace hfe { namespace ai {\n'
        +'\n\n'.join(e['body'] for e in evidence)+'\n} } }\n',encoding='utf-8')
    (out/'source.json').write_text(json.dumps(evidence,indent=2),encoding='utf-8')
    lines=['// [TEMP] Typed observers; unused virtuals abort.','namespace dice { namespace hfe { namespace ai {']
    interfaces=[
        ('IAIGame','BFGame.h','ObservedGame',{'getMaxNumberOfSquads'},'int getMaxNumberOfSquads() const override { return gameSquads(); }')]
    for base,header,name,excluded,custom in interfaces:
        lines.append('class '+name+' : public '+base+' { public:')
        for method,declaration in decls.methods(base,'AI/AIInterface/BFKnowledge/'+header):
            if method not in excluded: lines.append(declaration+' override { unexpected("'+base+'::'+method+'"); }')
        lines += [custom,'};']
    lines.append('} } }')
    (out/'mocks.inl').write_text('\n'.join(lines),encoding='utf-8')
    (out/'cases.json').write_text(json.dumps(MODES,indent=2))
    (out/'fixture_sources.json').write_text(json.dumps({Path(__file__).with_suffix(s).name:hashlib.sha256(Path(__file__).with_suffix(s).read_bytes()).hexdigest() for s in ('.h','.cpp','.py','.cmd')},indent=2))

def run(exe):
    summary=[]
    for mode in MODES:
        result=subprocess.run([str(exe),mode],capture_output=True,text=True,timeout=15)
        output=result.stdout+result.stderr
        (exe.parent/(mode+'.log')).write_text(output,encoding='utf-8')
        if result.returncode!=0 or output!='PASS '+mode+'\n': raise RuntimeError((mode,result.returncode,output))
        summary.append('PASS '+mode); print(summary[-1])
    summary.append('PASS squad manager tick: '+str(len(MODES))+' strict x86 cases')
    (exe.parent/'runtime.log').write_text('\n'.join(summary)+'\n',encoding='utf-8'); print(summary[-1])

if __name__=='__main__':
    if len(sys.argv)!=3 or sys.argv[1] not in ('generate','run'): raise SystemExit('generate OUT | run EXE')
    path=Path(sys.argv[2]).resolve()
    generate(path) if sys.argv[1]=='generate' else run(path)
