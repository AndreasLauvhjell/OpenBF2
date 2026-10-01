"""[TEMP] Exact selector, its real local predicate and bounded actual getters."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess
import sys
_spec = importlib.util.spec_from_file_location('primary_fixture_helpers', Path(__file__).with_name('bot_spawner_runtime.py'))
_helpers = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_helpers)
methods = _helpers.methods

CODE = Path(__file__).resolve().parents[2]
AI = 'AI/AIDLL/AI/'


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    specs = [
        (AI+'AIStrategicMap/BotSpawner.cpp', r'int BotSpawner::findSuitableSpawnGroup\(int[^\n]*\)', False),
        (AI+'AIStrategicMap/BotSpawner.cpp', r'namespace \{\n[\s\S]*?class FreeUnitPred[^\n]*', True),
        (AI+'AIBot/SquadMember.cpp', r'IAIPlayer\* SquadMember::getPlayer\(\)', False),
        (AI+'AIBot/SquadMember.cpp', r'SquadCoordinator\* SquadMember::getSquad\(\)', False),
        (AI+'AIBot/SquadMember.cpp', r'bool SquadMember::isSquadLeader\(\)', False),
        (AI+'AIBot/SquadCoordinator.cpp', r'void SquadCoordinator::getSquadPosition\([^\n]*\)', False),
        (AI+'AIBot/SquadCoordinator.cpp', r'SquadMember\* SquadCoordinator::getSquadLeader\(\)', False),
        (AI+'AIStrategicEntity/AIStrategicObjectManager.cpp', r'AIStrategicObjectManager\* AIStrategicObjectManager::getInstance\(\)', False)]
    evidence = []
    for relative, signature, predicate in specs:
        path = CODE / 'BF2' / relative
        source = path.read_text(encoding='utf-8')
        pattern = '^'+signature + (r'\n\{[\s\S]*?^};\n}' if predicate else r'\s*\{[^\n]*}|^'+signature+r'\s*\{[\s\S]*?^}')
        found = re.findall(pattern, source, re.M)
        if len(found) != 1 or (not predicate and 'BF2_AI_FRONTIER' in found[0]):
            raise RuntimeError(('exact body', signature, len(found)))
        evidence.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                         'body': found[0], 'body_sha256': hashlib.sha256(found[0].encode()).hexdigest()})
    # Namespace-local predicate must precede the caller. Both blocks are verbatim.
    actual = ['// [TEMP] Exact statements and real headers; no replacements.',
              '#include "'+Path(__file__).with_suffix('.h').as_posix()+'"',
              'namespace dice { namespace hfe { namespace ai {',
              'std::vector<SpawnChoice> BotSpawner::m_pointWeightVector;',
              evidence[1]['body'], evidence[0]['body']]
    actual += [e['body'] for e in evidence[2:]]
    actual.append('} } }')
    (out/'bot_selector_actual.cpp').write_text('\n\n'.join(actual), encoding='utf-8')
    (out/'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    specs = [
        ('Bot',AI+'AIBot/BotMain.h','ObservedBot',{'getSquadMember'},'',
         'SquadMember* getSquadMember() override { return observeMember(); }'),
        ('IAIPlayer','AI/AIInterface/BFKnowledge/AIPlayer.h','ObservedPlayer',{'getIsAIPlayer','getIsAlive','isManDown'},'int identity=0;',
         'bool getIsAIPlayer() override { return observePlayer(identity,0); }\n'
         'bool getIsAlive() const override { return observePlayer(identity,1); }\n'
         'bool isManDown() const override { return observePlayer(identity,2); }'),
        ('IAIGame','AI/AIInterface/BFKnowledge/BFGame.h','ObservedGame',{'getSpawnPointList','getSpawnPointPosition','isEligibleSpawnpoint','getControlPointId'},'int identity=0;',
         'const std::list<int>& getSpawnPointList(int side) const override { return observeList(identity,side); }\n'
         'const Vec3& getSpawnPointPosition(int group) const override { return observeSpawnPosition(identity,group); }\n'
         'bool isEligibleSpawnpoint(int side,int group) const override { return observeEligible(identity,side,group); }\n'
         'int getControlPointId(int group) const override { return observeControl(identity,group); }'),
        ('IAIMain',AI+'Main/AIMain.h','ObservedMain',{'getSAI'},'int identity=0;',
         'SAI* getSAI(int side) override { return observeSAI(identity,side); }'),
        ('IAIPathfinding',AI+'Main/IAIPathfinding.h','ObservedPath',{'getTraceMapIndex','isValidPosition'},'int identity=0;',
         'int getTraceMapIndex() const override { return observeTraceMap(identity); }\n'
         'bool isValidPosition(int map,int type,const Vec3& position,bool flag) const override { return observeValid(identity,map,type,position,flag); }\n'
         'bool isValidPosition(int,int,const Vec3&,unsigned,bool) const override { unexpected("five-arg path query"); }\n'
         'bool isValidPosition(int,int,const Vec3&,unsigned,unsigned,bool) const override { unexpected("six-arg path query"); }'),
        ('IAIInformationEnvironment',AI+'AIKnowledge/BFInformationEnvironment.h','ObservedEnvironment',{'getObjectsWithinRadius'},'int identity=0;',
         'void getObjectsWithinRadius(std::vector<unsigned>& out,const Vec3& position,float radius,int side,bool flag,IAIInformationGridPredicate* predicate) const override { observeQuery(identity,out,position,radius,side,flag,predicate); }\n'
         'void getObjectsWithinRadius(std::vector<unsigned>&,float,const Bot&,bool,IAIInformationGridPredicate*) const override { unexpected("Bot radius query"); }\n'
         'void getObjectsWithinRadius(std::vector<unsigned>&,const Vec2&,float,int,bool,IAIInformationGridPredicate*) const override { unexpected("Vec2 radius query"); }'),
        ('AIStrategicObject',AI+'AIStrategicEntity/AIStrategicObject.h','ObservedObject',{'getPosition'},'int identity=0; ObservedObject():AIStrategicObject(0,0) {}',
         'const Vec2& getPosition(int side) const override { return observeObjectPosition(identity,side); }'),
        ('Information',AI+'AIKnowledge/Information.h','RejectedInformation',set(),
         'RejectedInformation():Information(nullptr,0) { m_objectTypeFlags=0; m_strength=0; }','')]
    lines = ['// [TEMP] Typed observers; all untested virtual calls abort.', 'namespace dice { namespace hfe { namespace ai {']
    for base,relative,name,excluded,fields,custom in specs:
        lines.append('class '+name+' : public '+base+' { public: '+fields)
        for method,declaration,pure in methods(base,relative):
            if method not in excluded:
                lines.append(declaration+' override { unexpected("'+base+'::'+method+'"); }')
        lines += [custom,'};']
    lines.append('class ObservedSAI : public SAI { public: ObservedSAI():SAI(2,0,0,Vec2(),Vec2()) {} void addSpawnToPoint(int group) override { observeAdd(group); } };')
    for base,relative in [('SAI',AI+'AIStrategic/SAI.h'),('AIStrategicObject',AI+'AIStrategicEntity/AIStrategicObject.h'),('AIStrategicObjectManager',AI+'AIStrategicEntity/AIStrategicObjectManager.h')]:
        for method,declaration,pure in methods(base,relative):
            if not pure:
                signature=re.sub(r'\b'+method+r'(?=\s*\()',base+'::'+method,declaration,count=1)
                lines.append(signature+' { unexpected("'+base+'::'+method+'"); }')
    lines.append('} } }')
    (out/'bot_selector_mocks.inl').write_text('\n'.join(lines),encoding='utf-8')


MODES = ['empty','none','control_minus_one','closest_null','weighted_first','weighted_last',
         'weighted_clamp','weighted_nan','weighted_infinity','weighted_boundary','weighted_refill',
         'weighted_wide','weighted_mutation','cached_context','live_list','leader_nearest','leader_tie',
         'leader_invalid','leader_nan','leader_rounding','leader_reload','leader_ai','leader_dead',
         'leader_down','leader_self','random_equal','random_above','selected_reload','leader_fallback','predicate_refusal']


def run(exe):
    summary=[]
    for mode in MODES:
        p=subprocess.run([str(exe),mode],capture_output=True,text=True,timeout=15)
        output=p.stdout+p.stderr
        (exe.parent/(mode+'.log')).write_text(output,encoding='utf-8')
        if p.returncode != 0 or output != 'PASS '+mode+'\n':
            raise RuntimeError((mode,p.returncode,output))
        summary.append('PASS '+mode); print(summary[-1])
    summary.append('PASS selector: '+str(len(MODES))+' strict x86 cases')
    (exe.parent/'runtime.log').write_text('\n'.join(summary)+'\n',encoding='utf-8')
    print(summary[-1])


if __name__=='__main__':
    if len(sys.argv)!=3 or sys.argv[1] not in ('generate','run'):
        raise SystemExit('Usage: bot_selector_runtime.py generate OUTPUT | run EXE')
    p=Path(sys.argv[2]).resolve()
    generate(p) if sys.argv[1]=='generate' else run(p)
