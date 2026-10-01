"""[TEMP] Exact strategic-object caller/dependencies, actual headers, chosen lifetimes."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
AI = 'AI/AIDLL/AI/'
helper_spec = importlib.util.spec_from_file_location('typed_declarations', Path(__file__).with_name('sai_states_runtime.py'))
helpers = importlib.util.module_from_spec(helper_spec)
helper_spec.loader.exec_module(helpers)

MODES = ['single_owned', 'single_hostile', 'single_neutral', 'side_two', 'resume',
         'restart', 'changed_output', 'sticky_change', 'callback_flag', 'cleared_flag',
         'entry_replace', 'side_change', 'cursor_move', 'live_end', 'manager_swap',
         'dirty_replace_side', 'dirty_live_end', 'destination_swap', 'float_accumulation',
         'commander', 'release_main_swap', 'release_returns', 'release_refusal', 'release_late_index']

def generate(out):
    specs = [
        (AI+'AIStrategic/SAI.cpp', r'bool SAI::updateStrategicObjects\(\)[^\n]*'),
        (AI+'AIStrategic/SAI.cpp', r'EnemyStatistics::EnemyStatistics\(unsigned int size\)'),
        (AI+'AIStrategic/SAI.cpp', r'EnemyStatistics::~EnemyStatistics\(\)'),
        (AI+'AIStrategic/SAI.cpp', r'void EnemyStatistics::clear\(\)'),
        (AI+'AIStrategic/SAI.cpp', r'void EnemyStatistics::add\(EnemyStatistics& other\)'),
        (AI+'AIStrategic/SAI.cpp', r'bool SAI::isPlayerCommander\(\)'),
        (AI+'AIStrategicEntity/AIStrategicObjectManager.cpp', r'AIStrategicObjectManager\* AIStrategicObjectManager::getInstance\(\)'),
        (AI+'AIStrategicEntity/AIStrategicObjectManager.cpp', r'const std::vector<AIStrategicObject\*>& AIStrategicObjectManager::getStrategicObjects\(\) const'),
        (AI+'AIStrategicEntity/AIStrategicObject.cpp', r'[^\n]+AIStrategicObject::getBattleStatistics\(int side\) const'),
        (AI+'AIStrategicEntity/AIStrategicObject.cpp', r'int AIStrategicObject::releaseAssignedResources\(int side\)')]
    evidence=[]
    for relative, signature in specs:
        path=CODE/'BF2'/relative
        source=path.read_text(encoding='utf-8')
        found=re.findall('^'+signature+r'\s*(?::[\s\S]*?)?\{[\s\S]*?^}',source,re.M)
        if len(found)!=1 or 'BF2_AI_FRONTIER' in found[0]:
            raise RuntimeError('Expected recovered exact body: '+signature)
        body=found[0]
        evidence.append(dict(source=path.as_posix(),source_sha256=hashlib.sha256(source.encode()).hexdigest(),
                             body_sha256=hashlib.sha256(body.encode()).hexdigest(),body=body))
    out.mkdir(parents=True,exist_ok=True)
    (out/'sai_objects_actual.cpp').write_text('// [TEMP] Verbatim complete definitions; no statement changes.\n#include "'
        +Path(__file__).with_suffix('.h').as_posix()+'"\nnamespace dice { namespace hfe { namespace ai {\n'
        +'\n\n'.join(e['body'] for e in evidence)+'\n} } }\n',encoding='utf-8')
    (out/'source.json').write_text(json.dumps(evidence,indent=2),encoding='utf-8')
    lines=['// [TEMP] Typed observers; unused real declarations abort.', 'namespace dice { namespace hfe { namespace ai {']
    lines.append('class ObservedObject : public AIStrategicObject { public: int identity = 0; '
                 'ObservedObject() : AIStrategicObject(0.0f, 0.0f) {}')
    object_header=AI+'AIStrategicEntity/AIStrategicObject.h'
    for method,declaration in helpers.methods('AIStrategicObject',object_header):
        if method not in ('update','setAsDirty'):
            lines.append(declaration+' override { unexpected("AIStrategicObject::'+method+'"); }')
    lines += ['void update(int side, ISAI* sai, bool& changed) override { observeUpdate(identity, side, sai, changed); }',
              'void setAsDirty(int side) override { observeDirty(identity, side); }','};']
    lines.append('class ObservedMain : public IAIMain { public: int identity = 0;')
    for method,declaration in helpers.methods('IAIMain',AI+'Main/AIMain.h'):
        if method!='getSAI':
            lines.append(declaration+' override { unexpected("IAIMain::'+method+'"); }')
    lines += ['SAI* getSAI(int side) override { return observeMain(identity, side); }','};']
    for base,relative in [('SAI',AI+'AIStrategic/SAI.h'),
                          ('AIStrategicObjectManager',AI+'AIStrategicEntity/AIStrategicObjectManager.h')]:
        for method,declaration in helpers.methods(base,relative):
            if base=='SAI' and method in ('isPlayerCommander','releaseSquadsFromObject'):
                continue
            signature=re.sub(r'\b'+method+r'(?=\s*\()',base+'::'+method,declaration,count=1)
            lines.append(signature+' { unexpected("'+base+'::'+method+'"); }')
    object_source=re.sub(r'//[^\n]*','',(CODE/'BF2'/object_header).read_text(encoding='utf-8'))
    for method,declaration in helpers.methods('AIStrategicObject',object_header):
        original=re.search(r'virtual\s+[^;]*\b'+method+r'\s*\([^;]*;',object_source)
        if original and re.search(r'=\s*0\s*;',original.group(0)):
            continue
        signature=re.sub(r'\b'+method+r'(?=\s*\()','AIStrategicObject::'+method,declaration,count=1)
        lines.append(signature+' { unexpected("AIStrategicObject::'+method+'"); }')
    lines.append('} } }')
    (out/'sai_objects_mocks.inl').write_text('\n'.join(lines),encoding='utf-8')
    fixture_hashes={Path(__file__).with_suffix(s).name:hashlib.sha256(Path(__file__).with_suffix(s).read_bytes()).hexdigest()
                    for s in ('.h','.cpp','.py','.cmd')}
    (out/'fixture_sources.json').write_text(json.dumps(fixture_hashes,indent=2),encoding='utf-8')

def run(exe):
    summary=[]
    for mode in MODES:
        result=subprocess.run([str(exe),mode],capture_output=True,text=True,timeout=15)
        output=result.stdout+result.stderr
        (exe.parent/(mode+'.log')).write_text(output,encoding='utf-8')
        if result.returncode!=0 or output!='PASS '+mode+'\n':
            raise RuntimeError((mode,result.returncode,output))
        summary.append('PASS '+mode)
        print(summary[-1])
    summary.append('PASS updateStrategicObjects: '+str(len(MODES))+' strict x86 cases')
    (exe.parent/'runtime.log').write_text('\n'.join(summary)+'\n',encoding='utf-8')
    print(summary[-1])

if __name__=='__main__':
    if len(sys.argv)!=3 or sys.argv[1] not in ('generate','run'):
        raise SystemExit('Usage: sai_objects_runtime.py generate OUT | run EXE')
    target=Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1]=='generate' else run(target)
