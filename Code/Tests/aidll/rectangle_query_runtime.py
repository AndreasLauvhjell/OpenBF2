"""[TEMP] Exact rectangle wrapper/selector, whole grid/pool and typed native fixtures."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE=Path(__file__).resolve().parents[2]
KNOWLEDGE=CODE/'BF2/AI/AIDLL/AI/AIKnowledge'
MODES=('world_a','world_b','side_zero','reload_grid','borrowed_corners','null_predicate','nonfinite',
       'real_empty','real_bounds','real_filter_stale')


def methods(owner, filename):
    text=(KNOWLEDGE/filename).read_text(encoding='utf-8')
    found=re.findall(r'^class '+owner+r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};',text,re.M|re.S)
    if len(found)!=1: raise RuntimeError('Expected actual class '+owner)
    body=re.sub(r'/\*.*?\*/|//[^\n]*','',found[0],flags=re.S)
    for declaration in re.findall(r'virtual\s+([^;]+);',body):
        declaration=re.sub(r'\s*=\s*0$','',' '.join(declaration.split()))
        name=re.search(r'(~?\w+)\s*\(',declaration).group(1)
        if not name.startswith('~'): yield name,declaration


def generate(out):
    path=KNOWLEDGE/'BFInformationEnvironment.cpp'
    source=path.read_text(encoding='utf-8')
    evidence=[]
    for name in ('getObjectsWithinRectangle','getInformationGrid'):
        found=re.findall(r'^[^\n{};]+ BFInformationEnvironment::'+name+r'\([\s\S]*?^}',source,re.M)
        if len(found)!=1 or 'BF2_AI_FRONTIER' in found[0]: raise RuntimeError('Expected recovered '+name)
        evidence.append({'source':path.as_posix(),'source_sha256':hashlib.sha256(source.encode()).hexdigest(),
                         'body':found[0],'body_sha256':hashlib.sha256(found[0].encode()).hexdigest()})
    out.mkdir(parents=True,exist_ok=True)
    actual=['// [TEMP] Exact current production bodies; no substitutions.',
            '#include "'+Path(__file__).with_suffix('.h').as_posix()+'"',
            'namespace dice { namespace hfe { namespace ai {']
    actual += [entry['body'] for entry in evidence]+['} } }']
    (out/'rectangle_query_actual.cpp').write_text('\n\n'.join(actual),encoding='utf-8')
    (out/'source.json').write_text(json.dumps(evidence,indent=2),encoding='utf-8')
    whole={name:hashlib.sha256((KNOWLEDGE/name).read_bytes()).hexdigest()
           for name in ('AIInformationGrid.cpp','InformationManager.cpp')}
    (out/'whole_tus.json').write_text(json.dumps(whole,indent=2),encoding='utf-8')
    mocks=['// [TEMP] Untested virtuals abort; exact tested wrapper excluded.',
           'namespace dice { namespace hfe { namespace ai {']
    for name,declaration in methods('BFInformationEnvironment','BFInformationEnvironment.h'):
        if name=='getObjectsWithinRectangle': continue
        signature=re.sub(r'\b'+name+r'(?=\s*\()','BFInformationEnvironment::'+name,declaration,count=1)
        mocks.append(signature+' { unexpected("BFInformationEnvironment::'+name+'"); }')
    mocks.append('class ObservedGrid : public IAIInformationGrid { public: int identity; explicit ObservedGrid(int id):identity(id) {}')
    for name,declaration in methods('IAIInformationGrid','AIInformationGrid.h'):
        if name=='getInformationWithinRectangle': continue
        mocks.append(declaration+' override { unexpected("IAIInformationGrid::'+name+'"); }')
    mocks.append('void getInformationWithinRectangle(std::vector<unsigned>& out,const Vec2& min,const Vec2& max,IAIInformationGridPredicate* keep) const override { observeGrid(identity,out,min,max,keep); } };')
    mocks.append('class ObservedInformation : public Information { public: int identity; Vec3 position; ObservedInformation(int id,Vec3 pos):Information(nullptr,0.0),identity(id),position(pos) {}')
    for name,declaration in methods('Information','Information.h'):
        body='return position;' if name=='getPosition' else 'unexpected("Information::'+name+'");'
        mocks.append(declaration+' override { '+body+' }')
    mocks += ['};','} } }']
    (out/'rectangle_query_mocks.inl').write_text('\n'.join(mocks),encoding='utf-8')


def run(exe):
    summary=[]
    checks=0
    for mode in MODES:
        result=subprocess.run([str(exe),mode],capture_output=True,text=True,timeout=15)
        output=result.stdout+result.stderr
        (exe.parent/(mode+'.log')).write_text(output,encoding='utf-8')
        match=re.fullmatch(r'PASS '+mode+r' ([1-9][0-9]*) checks\n',output)
        if result.returncode!=0 or not match: raise RuntimeError((mode,result.returncode,output))
        checks+=int(match.group(1));summary.append(output.strip())
    summary.append('PASS rectangle query: '+str(len(MODES))+' strict x86 cases / '+str(checks)+' checks')
    print('\n'.join(summary))


if __name__=='__main__':
    if len(sys.argv)!=3 or sys.argv[1] not in ('generate','run'):
        raise SystemExit('Usage: rectangle_query_runtime.py generate OUTPUT | run EXE')
    target=Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1]=='generate' else run(target)
