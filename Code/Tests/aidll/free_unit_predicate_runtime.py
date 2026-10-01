"""[TEMP] Exact FreeUnitPred. (The first-secondary stop variant retired 2026-09-29.)"""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess
import sys

CODE=Path(__file__).resolve().parents[2]
AI='AI/AIDLL/AI/'
_spec=importlib.util.spec_from_file_location('fixture_helpers',Path(__file__).with_name('bot_spawner_runtime.py'))
_helpers=importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_helpers)


def generate(out):
    out.mkdir(parents=True,exist_ok=True)
    evidence=[]
    for relative,pattern in [
        (AI+'AIStrategicMap/BotSpawner.cpp',r'^namespace \{\n[\s\S]*?class FreeUnitPred[^\n]*\n\{[\s\S]*?^};\n}')]:
        path=CODE/'BF2'/relative; source=path.read_text(encoding='utf-8')
        found=re.findall(pattern,source,re.M)
        if len(found)!=1: raise RuntimeError(('exact body',relative,len(found)))
        evidence.append({'source':path.as_posix(),'source_sha256':hashlib.sha256(source.encode()).hexdigest(),
                         'body':found[0],'body_sha256':hashlib.sha256(found[0].encode()).hexdigest()})
    if 'BF2_AI_FRONTIER' in evidence[0]['body']: raise RuntimeError('Predicate is still a frontier')
    include='#include "'+Path(__file__).with_suffix('.h').as_posix()+'"\nnamespace dice { namespace hfe { namespace ai {\n'
    bridge='''
// [TEMP] Genuine concrete lifetime and typed fields; tested class unchanged above.
IAIInformationGridPredicate* makeFixturePredicate(int side,int*& sideSlot,float*& strengthSlot)
{
    FreeUnitPred* predicate=new FreeUnitPred(side);
    sideSlot=&predicate->m_side; strengthSlot=&predicate->m_strength;
    return predicate;
}
void destroyFixturePredicate(IAIInformationGridPredicate* predicate)
{ delete static_cast<FreeUnitPred*>(predicate); }
'''
    (out/'predicate_actual.cpp').write_text(include+evidence[0]['body']+bridge+'\n} } }',encoding='utf-8')
    (out/'source.json').write_text(json.dumps(evidence,indent=2),encoding='utf-8')
    lines=['// [TEMP] Typed chosen Information subclass. Untested virtual calls abort.',
           'namespace dice { namespace hfe { namespace ai {',
           'class ObservedInformation : public Information { public: ObservedInformation():Information(nullptr,0) {}']
    for method,declaration,pure in _helpers.methods('Information',AI+'AIKnowledge/Information.h'):
        lines.append(declaration+' override { unexpected("Information::'+method+'"); }')
    lines+=['};','} } }']
    (out/'predicate_observer.inl').write_text('\n'.join(lines),encoding='utf-8')


MODES=['unit_null','mobile_null','flag80','flag200000','all_occupied','root_free',
       'secondary_rootguards','float_steps','value_reload','first_unit_reload','manager_swap',
       'side_accumulator','zero_manager','nan_strength','zero_commit','invalid_handle']


def run(exe):
    summary=[]
    for mode in MODES:
        p=subprocess.run([str(exe),mode],capture_output=True,text=True,timeout=15)
        output=p.stdout+p.stderr
        (exe.parent/(mode+'.log')).write_text(output,encoding='utf-8')
        expected='PASS '+mode+'\n'
        if p.returncode!=0 or output!=expected:
            raise RuntimeError((mode,p.returncode,output))
        summary.append('PASS '+mode); print(summary[-1])
    summary.append('PASS FreeUnitPred: 16 strict x86 cases')
    (exe.parent/'runtime.log').write_text('\n'.join(summary)+'\n',encoding='utf-8')
    print(summary[-1])


if __name__=='__main__':
    if len(sys.argv)!=3 or sys.argv[1] not in ('generate','run'):
        raise SystemExit('Usage: free_unit_predicate_runtime.py generate OUTPUT | run EXE')
    p=Path(sys.argv[2]).resolve()
    generate(p) if sys.argv[1]=='generate' else run(p)
