"""Stock tier3 content test: real BF2 startup, loopback, no bots or clients.

Uses tier2's established private boot-copy builder; it adds observers only.
The engine diagnostic link uses /FORCE, so acceptance covers exercised paths.
"""
from pathlib import Path
from zipfile import ZipFile
import argparse
import importlib.util
import json
import os
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('tier2_runner',HERE.parent/'tier2/run.py')
runner=importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
runner.HERE=HERE
runner.HOOKS['content']=('bool contentSetup(); void contentFrame(int); bool contentResult();',
                         'contentSetup()','contentFrame(f);','contentResult()')
runner.HOOKS['managers']=('bool managersSetup(); void managersFrame(int); bool managersResult(); bool managersShutdown();',
                         'managersSetup()','managersFrame(f);','managersResult()')
original_boot=runner.boot
def boot(suite):
    source=original_boot(suite)
    if suite=='managers':
        source=runner.once(source,'        delete bf2;',
            '        delete bf2;\n        tier2Pass = managersShutdown() && tier2Pass;')
    return source
runner.boot=boot

def instrumented(suite):
    if suite=='managers':
        original=ROOT/'Code/BF2/Game/Objects/Game/AmbientEffectArea.cpp'
        source=runner.includes(original.read_text(encoding='utf-8'),original.parent)
        source=runner.once(source,'namespace dice {',
            'void managersObserve(const char*,dice::hfe::world::AmbientEffectAreaManager*,dice::hfe::world::AmbientEffectArea*,int,unsigned,unsigned);\nnamespace dice {')
        for anchor,extra in (
            ('        ambientEffectAreaManager = this;',
             '\n    managersObserve("construct",this,0,ambientEffectAreaNextId,unsigned(m_areas.size()),unsigned(m_areasById.size()));'),
            ('        ambientEffectAreaManager = 0;',
             '\n    managersObserve("destruct",this,0,ambientEffectAreaNextId,unsigned(m_areas.size()),unsigned(m_areasById.size()));'),
            ('    ambientEffectAreaNextId = 0;',
             '\n    managersObserve("clear",this,0,ambientEffectAreaNextId,unsigned(m_areas.size()),unsigned(m_areasById.size()));'),
            ('    m_areasById[area->m_id] = area;',
             '\n    managersObserve("add",this,area,area->m_id,unsigned(m_areas.size()),unsigned(m_areasById.size()));'),
            ('        m_areasById.erase(*it);',
             '\n    managersObserve("remove",this,area,area->m_id,unsigned(m_areas.size()),unsigned(m_areasById.size()));')):
            source=runner.once(source,anchor,anchor+extra)
        observed={original:source}
        original=ROOT/'Code/BF2/Game/Objects/Game/Trigger.cpp'
        source=runner.includes(original.read_text(encoding='utf-8'),original.parent)
        source=runner.once(source,'namespace dice {',
            'void triggersObserve(const char*,dice::hfe::world::Trigger*,unsigned);\nnamespace dice {')
        for anchor,extra in (
            ('        triggerManager->addTrigger(this);', '\n    triggersObserve("add",this,0);'),
            ('Trigger::~Trigger()\n{', '\n    triggersObserve("remove",this,0);'),
            ('bool Trigger::checkTriggerArea()\n{', '\n    triggersObserve("check",this,0);'),
            ('void Trigger::updateOnEvent(TriggerType type, IBase* source)\n{',
             '\n    triggersObserve("event",this,unsigned(type));'),
            ('void Trigger::executeTrigger(IBase* source, const Vec3& position, int effectorIndex)\n{',
             '\n    triggersObserve("execute",this,0);')):
            source=runner.once(source,anchor,anchor+extra)
        observed[original]=source
        original=ROOT/'Code/BF2/Game/GameServer/GameServer.cpp'
        source=runner.includes(original.read_text(encoding='utf-8'),original.parent)
        source=runner.once(source,'namespace dice {',
            'void ambientEventObserve(dice::hfe::GameEvent*);\nnamespace dice {')
        source=runner.once(source,
            'void GameServer::sendGameEventToAllConnected(GameEvent* event) { _sendGameEventToAll(event, true, false); }',
            'void GameServer::sendGameEventToAllConnected(GameEvent* event) { ambientEventObserve(event); _sendGameEventToAll(event, true, false); }')
        observed[original]=source
        return observed
    original=ROOT/'Code/BF2/Game/Objects/Game/DestroyableObject.cpp'
    source=runner.includes(original.read_text(encoding='utf-8'),original.parent)
    marker='namespace dice {'
    source=runner.once(source,marker,
        'void contentPriority(dice::hfe::world::IObject*, dice::hfe::io::INetworkable*);\n'+marker)
    source=runner.once(source,'    io::INetworkable* networkable = getNetworkable();',
        '    io::INetworkable* networkable = getNetworkable();\n    contentPriority(this,networkable);')
    return {original:source}
runner.instrumented=instrumented

def expectations(game_dir):
    result={}
    with ZipFile(game_dir/'mods/bf2/Objects_server.zip') as archive:
        for path in archive.namelist():
            if not path.lower().endswith(('.con','.tweak')):continue
            active=None
            for line in archive.read(path).decode('latin1').splitlines():
                tokens=line.strip().split(None,2)
                if not tokens:continue
                verb=tokens[0].lower()
                if verb in ('objecttemplate.create','objecttemplate.activesafe') and len(tokens)==3:
                    active=tokens[2].strip('"')
                if verb=='objecttemplate.active' and len(tokens)>=2:active=tokens[1].strip('"')
                if active and len(tokens)>=2:
                    for kind in ('ik','strategicObject'):
                        prefix='objecttemplate.'+kind.lower()+'.'
                        if verb.startswith(prefix):
                            prop=tokens[0][len(prefix):]
                            value=line.strip()[len(tokens[0]):].strip().strip('"')
                            result[(kind,active,prop)]=value
    return ''.join('\t'.join(key)+'\t'+value+'\n' for key,value in sorted(result.items()))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-out',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--game-dir',type=Path,default=Path(r'D:\B2BF Launcher\Battlefield2'))
    parser.add_argument('--port',type=int,default=16987)
    parser.add_argument('--query-port',type=int,default=29887)
    parser.add_argument('--reload',action='store_true')
    parser.add_argument('--suite',choices=('content','managers'),default='content')
    args=parser.parse_args()
    args.out=args.out.resolve(); args.out.mkdir(parents=True,exist_ok=True)
    runner.free_port(args.port,'0.0.0.0'); runner.free_port(args.query_port,'0.0.0.0')
    expected=args.out/'expectations.tsv'; expected.write_text(expectations(args.game_dir))
    vcvars=Path(r'D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat')
    binary=runner.build(args.suite,args.build_out.resolve()/'engine_objects.rsp',args.out,vcvars)
    config=args.out/'ServerSettings.con'
    config.write_text('sv.internet 0\nsv.ranked 0\nsv.punkBuster 0\nsv.voipEnabled 0\n'
        'sv.serverIP "127.0.0.1"\nsv.serverPort '+str(args.port)+'\n'
        'sv.interfaceIP "127.0.0.2"\nsv.gameSpyPort '+str(args.query_port)+'\n'
        'sv.allowNATNegotiation 0\nsv.autoRecord 0\nsv.useGlobalRank 0\n'
        'sv.maxPlayers 64\nsv.numPlayersNeededToStart 2\nsv.autoBalanceTeam 0\n'
        'sv.startDelay 0\nsv.endOfRoundDelay 1\nsv.endDelay 1\n'
        'SettingsManager.stringSet AIDLL ""\n')
    env={k:v for k,v in os.environ.items() if not k.upper().startswith('BF2_')}
    env.update(BF2_FRAMES='1800' if args.reload else '360',BF2_QUIET='1',BF2_SHUTDOWN='1',
        BF2_PORT=str(args.port),BF2_SERVER_SCREEN='0',BF2_TIER2_LEVEL='dalian_plant',
        BF2_TIER3_EXPECTATIONS=str(expected))
    if args.reload:env.update(BF2_RELOAD_AT='360',BF2_TIER3_RELOAD='1')
    command=[str(binary),'+modPath','mods/bf2','+loadLevel','dalian_plant','+gameMode','gpm_cq',
        '+maxPlayers','64','+port',str(args.port),'+noStatusMonitor','1','+config',str(config)]
    with (args.out/'stock.log').open('w') as log:
        run=subprocess.run(command,cwd=args.game_dir,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=240)
    output=(args.out/'stock.log').read_text(errors='replace')
    passed=run.returncode==0 and '[tier2-listener] PASS=1' in output and 'PORT FREE' in output
    runner.free_port(args.port,'127.0.0.1')
    result=dict(passed=passed,exit=run.returncode,command=command,
        env={k:v for k,v in env.items() if k.upper().startswith('BF2_')})
    (args.out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2),flush=True)
    return 0 if passed else 1

if __name__=='__main__':raise SystemExit(main())
