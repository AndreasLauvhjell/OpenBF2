"""[TEMP] Exact recovered squad helper bodies, native types and typed observations."""
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
MODES='message_empty message_purge0 message_purge1 message_purge_both message_expiry message_time_nan message_fresh_main message_fresh_commander message_reload_payload message_append message_clock_mutation setter_insert setter_erase setter_nan setter_full_id human_empty human_null human_ai human_dead human_down human_vehicle_null human_repeat_ai human_copy human_prune human_fresh_factory human_live_map human_vehicle_stop'.split()

def generate(out):
    out.mkdir(parents=True,exist_ok=True)
    path=CODE/'BF2'/AI/'AIBot/SquadManager.cpp'
    source=path.read_text(encoding='utf-8')
    evidence=[]
    for method in ('updateCommanderMessages','updateHumanPlayers','setPlayerOnValidPosition'):
        found=re.findall(r'^void SquadManager::'+method+r'\([^\n]*\)\s*\{[\s\S]*?^}',source,re.M)
        if len(found)!=1: raise RuntimeError('Expected one complete body: '+method)
        evidence.append(dict(source=path.as_posix(),source_sha256=hashlib.sha256(source.encode()).hexdigest(),body=found[0],body_sha256=hashlib.sha256(found[0].encode()).hexdigest()))
    (out/'actual.cpp').write_text('// [TEMP] Verbatim complete production definitions.\n#include "'+Path(__file__).with_suffix('.h').as_posix()+'"\nnamespace dice { namespace hfe { namespace ai {\n'+'\n\n'.join(e['body'] for e in evidence)+'\n} } }\n',encoding='utf-8')
    (out/'source.json').write_text(json.dumps(evidence,indent=2),encoding='utf-8')
    lines=['// [TEMP] Typed collaborators only; every unused virtual aborts.','namespace dice { namespace hfe { namespace ai {']
    specs=[
      ('IAIMain',AI+'Main/AIMain.h','ObservedMain',{'getSAI'},'int identity=0;','SAI* getSAI(int side) override { return observeSAI(identity,side); }'),
      ('IBFTimer','AI/AIInterface/BFKnowledge/BFTimer.h','ObservedTimer',{'getGameTime'},'','double getGameTime() const override { return observeTime(); }'),
      ('IBFCommander','AI/AIInterface/BFKnowledge/BFCommander.h','ObservedCommander',{'sendAttackMessage','sendDefendMessage'},'int identity=0;','void sendAttackMessage(int a,int b) override { observeCommand(identity,0,a,b); }\nvoid sendDefendMessage(int a,int b) override { observeCommand(identity,1,a,b); }'),
      ('IAIPlayerFactory','AI/AIInterface/BFKnowledge/AIPlayerFactory.h','ObservedFactory',{'getAllPlayers','getPlayerFromId'},'int identity=0;','const std::vector<IAIPlayer*>& getAllPlayers() override { return observeRoster(identity); }\nIAIPlayer* getPlayerFromId(int id) override { return observeLookup(identity,id); }'),
      ('IAIPlayer','AI/AIInterface/BFKnowledge/AIPlayer.h','ObservedPlayer',{'getId','getIsAIPlayer','getIsAlive','isManDown','getCurrentVehicle','getSide'},'int identity=0;',
       'int getId() const override { return observeId(identity); }\nbool getIsAIPlayer() override { return observeAI(identity); }\nbool getIsAlive() const override { return observeAlive(identity); }\nbool isManDown() const override { return observeDown(identity); }\nIAIObject* getCurrentVehicle() override { return observeVehicle(identity); }\nint getSide() override { return observeSide(identity); }')]
    for base,header,name,excluded,fields,custom in specs:
        lines.append('class '+name+' : public '+base+' { public: '+fields)
        for method,decl in decls.methods(base,header):
            if method not in excluded: lines.append(decl+' override { unexpected("'+base+'::'+method+'"); }')
        lines += [custom,'};']
    for method,decl in decls.methods('SAI',AI+'AIStrategic/SAI.h'):
        if method!='isPlayerCommander':
            signature=re.sub(r'\b'+method+r'(?=\s*\()', 'SAI::'+method,decl,count=1)
            lines.append(signature+' { unexpected("SAI::'+method+'"); }')
    lines.append('} } }')
    (out/'mocks.inl').write_text('\n'.join(lines),encoding='utf-8')
    (out/'cases.json').write_text(json.dumps(MODES,indent=2))
    (out/'fixture_sources.json').write_text(json.dumps({Path(__file__).with_suffix(s).name:hashlib.sha256(Path(__file__).with_suffix(s).read_bytes()).hexdigest() for s in ('.h','.cpp','.py','.cmd')},indent=2))

def run(exe):
    summary=[]
    count=0
    for mode in MODES:
        result=subprocess.run([str(exe),mode],capture_output=True,text=True,timeout=15)
        output=result.stdout+result.stderr
        (exe.parent/(mode+'.log')).write_text(output,encoding='utf-8')
        if mode=='human_vehicle_stop':
            if result.returncode!=3 or output!='[FRONTIER] dice::hfe::ai::SquadManager::updateHumanPlayers (IAIObject::getInformation) reached\n': raise RuntimeError((mode,result.returncode,output))
            summary.append('PASS '+mode+' exact exit3 frontier')
        else:
            match=re.fullmatch(r'PASS '+mode+r' ([1-9][0-9]*) checks\n',output)
            if result.returncode!=0 or not match: raise RuntimeError((mode,result.returncode,output))
            count+=int(match.group(1)); summary.append(output.strip())
        print(summary[-1])
    summary.append('PASS squad helpers: '+str(len(MODES))+' strict x86 cases / '+str(count)+' checks / 1 frontier')
    (exe.parent/'runtime.log').write_text('\n'.join(summary)+'\n',encoding='utf-8'); print(summary[-1])

if __name__=='__main__':
    if len(sys.argv)!=3 or sys.argv[1] not in ('generate','run'): raise SystemExit('generate OUT | run EXE')
    path=Path(sys.argv[2]).resolve()
    generate(path) if sys.argv[1]=='generate' else run(path)
