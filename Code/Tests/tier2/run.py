"""Routine x86 vehicle regressions against a completed isolated engine manifest.

All instrumentation is compiled into private copies. No reference content is written.
The /FORCE link limits acceptance to the paths each observer actually exercises.
"""
from pathlib import Path
import argparse
import json
import os
import re
import shutil
import socket
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
CASES = {
    'bodies': ('bodies', 'wake_island_2007', 1, {}),
    'aircraft': ('aircraft', 'wake_island_2007', 901, {}),
    'floating': ('floating', 'wake_island_2007', 901, {}),
    'z8_float': ('vehicle_float', 'wake_island_2007', 181, {'BF2_FLOAT_VEHICLE':'chthe_z8'}),
    'uh60_float': ('vehicle_float', 'wake_island_2007', 181, {'BF2_FLOAT_VEHICLE':'usthe_uh60'}),
    'mi17_float': ('vehicle_float', 'gulf_of_oman', 181, {'BF2_FLOAT_VEHICLE':'the_mi17'}),
    'lav_tow': ('vehicle_float', 'gulf_of_oman', 811, {'BF2_FLOAT_VEHICLE':'USAPC_LAV25'}),
    'parachute_water': ('exit_parachute', 'dalian_plant', 1600, {}),
    'parachute_land': ('exit_parachute', 'dalian_plant', 1600, {'BF2_EXIT_VEHICLE':'chthe_z8'}),
    'moving_exit': ('exit_ground', 'dalian_plant', 150, {'BF2_EXIT_VEHICLE':'jep_paratrooper','BF2_EXIT_GROUND':'1'}),
    'su34_exit': ('aircraft_exit', 'gulf_of_oman', 150, {'BF2_EXIT_VEHICLE':'ruair_su34'}),
    'su34_copilot': ('aircraft_exit', 'gulf_of_oman', 150, {'BF2_EXIT_VEHICLE':'ruair_su34','BF2_EXIT_COPILOT':'1'}),
    'collision': ('collision', 'dalian_plant', 421, {}),
    'commander': ('commander', 'dalian_plant', 2700, {}),
    'sidewinder': ('missile', 'wake_island_2007', 130, {'BF2_MISSILE_PLANE':'1'}),
    'archer': ('missile', 'wake_island_2007', 130, {'BF2_MISSILE_PLANE':'1','BF2_MISSILE_REVERSE':'1'}),
    'friendly': ('missile', 'wake_island_2007', 130, {'BF2_MISSILE_PLANE':'1','BF2_MISSILE_FRIENDLY':'1'}),
}
HOOKS = {
    'bodies': ('bool vehicleBodies();', 'vehicleBodies()', '', 'true'),
    'aircraft': ('bool aircraftCheckBindings(); bool aircraftCheckIdle(); void aircraftSnapshot(int);',
                 'aircraftCheckBindings()', 'if (f == 0 || f == 300 || f == 900) aircraftSnapshot(f);', 'aircraftCheckIdle()'),
    'floating': ('void boatFrame(int); bool boatResults();', 'true', 'boatFrame(f);', 'boatResults()'),
    'vehicle_float': ('void floatFrame(int); bool floatResults();', 'true', 'floatFrame(f);', 'floatResults()'),
    'exit_parachute': ('void exitFrame(int); bool exitResults();', 'true', 'exitFrame(f);', 'exitResults()'),
    'exit_ground': ('void exitFrame(int); bool exitResults();', 'true', 'exitFrame(f);', 'exitResults()'),
    'aircraft_exit': ('void exitFrame(int); bool exitResults();', 'true', 'exitFrame(f);', 'exitResults()'),
    'collision': ('void collisionFrame(int); bool collisionResults();', 'true', 'collisionFrame(f);', 'collisionResults()'),
    'commander': ('bool commanderSetup(); void commanderSnapshot(int); bool commanderResult();',
                  'commanderSetup()', 'commanderSnapshot(f);', 'commanderResult()'),
    'missile': ('void missileFrame(int); bool missileResults();', 'true', 'missileFrame(f);', 'missileResults()'),
}

def once(source, old, new):
    if source.count(old) != 1:
        raise RuntimeError('Instrumentation anchor changed: ' + old)
    return source.replace(old, new)

def includes(source, origin):
    return re.sub(r'#include "([^"]+)"', lambda m: '#include "' +
                  (origin / m[1]).resolve().as_posix() + '"', source)

def boot(suite):
    source = includes((ROOT/'Code/Build/bootmain.cpp').read_text(encoding='utf-8'), ROOT/'Code/Build')
    decl, setup, frame, result = HOOKS[suite]
    source = once(source, 'int main(int argc, const char** argv)',
                  '#include "'+(ROOT/'Code/BF2/IO/SettingsManager.h').as_posix()+'"\n'
                  'bool tier2Listener();\n'+decl+'\nint main(int argc, const char** argv)')
    source = once(source, '    dice::hfe::BF2* bf2 = new dice::hfe::BF2();',
                  '    dice::hfe::io::settingsManager->registerUserVar("AIDLL", false, dice::hfe::io::VariableSet(0));\n'
                  '    dice::hfe::BF2* bf2 = new dice::hfe::BF2();')
    anchor = '    printf("[boot] BF2::init returned TRUE\\n"); fflush(stdout);'
    source = once(source, anchor, anchor+'\n    if (!tier2Listener()) return 65;\n'
                  '    bool tier2Pass = '+setup+';')
    source = once(source, '        if (aiAtFrame > 0 && f == aiAtFrame)',
                  '        '+frame+'\n        if (aiAtFrame > 0 && f == aiAtFrame)')
    source = once(source, '    if (GetEnvironmentVariableA("BF2_SHUTDOWN", 0, 0) != 0)',
                  '    tier2Pass = ('+result+') && tier2Pass;\n'
                  '    if (GetEnvironmentVariableA("BF2_SHUTDOWN", 0, 0) != 0)')
    source = once(source, 'addr.sin_addr.s_addr = INADDR_ANY;', 'addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);')
    source = source.replace('rebind probe: bind(0.0.0.0:%d)', 'rebind probe: bind(127.0.0.1:%d)')
    source = once(source, '        closesocket(s);', '        tier2Pass = (r == 0) && tier2Pass;\n        closesocket(s);')
    offset = source.rindex('    return 0;')
    return source[:offset] + '    return tier2Pass ? 0 : 66;' + source[offset+len('    return 0;'):]

def instrumented(suite):
    result = {}
    if suite == 'vehicle_float':
        for name in ('PhysicsNode', 'PointPhysicsNode'):
            original = ROOT/'Code/BF2/Physics'/f'{name}.cpp'
            source = includes(original.read_text(encoding='utf-8'), original.parent)
            source = '#include "'+(HERE/'physics_scope.h').as_posix()+'"\n'+source
            pattern = r'(void '+name+r'::\w+\([^;{}]*\)\s*\{)'
            source, count = re.subn(pattern, lambda m: m[1]+' FloatPhysicsScope scope(this,__FUNCTION__);', source)
            if count == 0: raise RuntimeError('No physics observer sites')
            result[original] = source
        original = ROOT/'Code/BF2/IO/BitStream.cpp'
        source = includes(original.read_text(encoding='utf-8'), original.parent)
        source = '#include <intrin.h>\n'+source
        anchor = 'namespace dice {'
        source = once(source, anchor, 'extern void floatWire(const char*, const dice::hfe::Vec3&, const dice::hfe::Vec3&, float, void*);\n'+anchor)
        for method in ('writeCompressedVector', 'writeHighCompressedVector', 'writeCompressedVector2'):
            pattern = r'(void BitStream::'+method+r'\(const Vec3& v, const Vec3& ref, float scale\)\s*\{)'
            source, count = re.subn(pattern, lambda m: m[1]+'\n    floatWire("'+method+'",v,ref,scale,_ReturnAddress());', source)
            if count != 1: raise RuntimeError('Wire observer anchor changed: '+method)
        result[original] = source
    if suite == 'collision':
        original = ROOT/'Code/BF2/Game/Objects/Base/SimpleObject.cpp'
        source = includes(original.read_text(encoding='utf-8'), original.parent)
        source = '#include "'+(ROOT/'Code/BF2/Game/Objects/IObject.h').as_posix()+'"\n' + \
                 'void collisionContact(dice::hfe::world::IObject*,dice::hfe::world::IObject*,const dice::hfe::Vec3&);\n'+source
        anchor = '    getRootParent(this);\n\n    bool result = true;'
        source = once(source, anchor, '    collisionContact(this,other,first);\n'+anchor)
        result[original] = source
    return result

def build(suite, manifest, out, vcvars):
    folder = out/suite
    folder.mkdir(exist_ok=True)
    # Only outputs owned by this exact suite; a failed link must not pass via a stale image.
    for name in ('runtime.exe', 'runtime.pdb', 'runtime.ilk', 'runtime.lib', 'runtime.exp'):
        (folder/name).unlink(missing_ok=True)
    (folder/'bootmain.cpp').write_text(boot(suite), encoding='utf-8')
    sources = [folder/'bootmain.cpp', HERE/(suite+'.cpp'), HERE/'lifecycle.cpp']
    entries = manifest.read_text().splitlines()
    for original, source in instrumented(suite).items():
        generated = folder/('observed_'+original.name)
        generated.write_text(source, encoding='utf-8')
        sources.append(generated)
        matches = [row for row in entries if re.search(r'[/\\]\d+_'+re.escape(original.stem)+r'\.obj"?$', row)]
        if len(matches)>1:
            # World and animation Trigger/TriggerManager share filenames. Match the
            # compiler's source-path record in the object, retaining the other image member.
            paths=(str(original).lower().encode(),original.as_posix().lower().encode())
            matches=[row for row in matches if any(path in Path(row.strip('"')).read_bytes().lower()
                                                   for path in paths)]
        if len(matches) != 1: raise RuntimeError('Ambiguous engine object: '+str(original))
        entries.remove(matches[0])
    (folder/'engine_objects.rsp').write_text('\n'.join(entries)+'\n')
    flags = '/nologo /c /MD /Z7 /std:c++14 /permissive /EHsc /DDEDICATED=1 /FI"'+str(ROOT/'Code/BF2/Core/BuildConfig.h')+'"'
    # Observed GameServer includes PythonHost, just as the regular engine build does.
    for include in ('Include','PC'):
        flags += ' /I"'+str(ROOT/'Code/BF2/External/python-embedded/py-src-2.3.4'/include)+'"'
    commands = ['@echo off', 'setlocal', 'call "'+str(vcvars)+'" -arch=x86 -no_logo >nul 2>&1']
    objects = []
    for i, source in enumerate(sources):
        obj = folder/f'test_{i}.obj'
        objects.append('"'+str(obj)+'"')
        commands += ['cl '+flags+' /Fo"'+str(obj)+'" "'+str(source)+'"', 'if errorlevel 1 exit /b 1']
    pyroot = ROOT/'Code/BF2/External/python-embedded/OBJ/python/release'
    commands += ['link /nologo /FORCE /DEBUG /INCREMENTAL:NO /PDBPAGESIZE:16384 /NODEFAULTLIB:python23.lib /subsystem:console '
                 '/OUT:"'+str(folder/'runtime.exe')+'" /PDB:"'+str(folder/'runtime.pdb')+'" '
                 '/MAP:"'+str(folder/'runtime.map')+'" /IMPLIB:"'+str(folder/'runtime.lib')+'" '+
                 ' '.join(objects)+' @"'+str(folder/'engine_objects.rsp')+'" "'+str(pyroot/'dice_py.lib')+'" '
                 'advapi32.lib user32.lib shell32.lib Ws2_32.lib dbghelp.lib iphlpapi.lib',
                 'if not exist "'+str(folder/'runtime.exe')+'" exit /b 1', 'exit /b 0']
    script = folder/'build.cmd'
    script.write_text('\n'.join(commands)+'\n')
    with (folder/'build.log').open('w') as log:
        subprocess.run(['cmd','/c',str(script)],stdout=log,stderr=subprocess.STDOUT,check=True)
    shutil.copy2(pyroot/'dice_py.dll',folder/'dice_py.dll')
    return folder/'runtime.exe'

def free_port(port, address):
    if port == 16567: raise ValueError('Port 16567 belongs to the owner')
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as sock:
        sock.setsockopt(socket.SOL_SOCKET,socket.SO_EXCLUSIVEADDRUSE,1)
        sock.bind((address,port))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-out',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--game-dir',type=Path,default=Path(r'D:\B2BF Launcher\Battlefield2'))
    parser.add_argument('--vcvars',type=Path,default=Path(r'D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat'))
    parser.add_argument('--case',action='append',choices=list(CASES))
    parser.add_argument('--port',type=int,default=16986)
    parser.add_argument('--query-port',type=int,default=29886)
    args=parser.parse_args()
    args.out=args.out.resolve()
    args.out.mkdir(parents=True,exist_ok=True)
    manifest=args.build_out.resolve()/'engine_objects.rsp'
    if not manifest.is_file(): raise ValueError('Completed isolated build manifest required')
    if args.out == args.build_out.resolve(): raise ValueError('Use a separate regression output directory')
    config = args.out/'ServerSettings.con'
    config.write_text('sv.internet 0\nsv.ranked 0\nsv.punkBuster 0\nsv.voipEnabled 0\n'
                      'sv.serverIP "127.0.0.1"\nsv.serverPort '+str(args.port)+'\n'
                      'sv.interfaceIP "127.0.0.2"\nsv.gameSpyPort '+str(args.query_port)+'\n'
                      'sv.allowNATNegotiation 0\nsv.autoRecord 0\nsv.useGlobalRank 0\n'
                      'sv.maxPlayers 64\nsv.numPlayersNeededToStart 2\nsv.startDelay 15\nsv.autoBalanceTeam 0\n'
                      'SettingsManager.stringSet AIDLL ""\n')
    binaries={}
    results=[]
    for name in args.case or CASES:
        suite,level,frames,flags=CASES[name]
        if suite not in binaries:
            print('BUILD',suite,flush=True)
            binaries[suite]=build(suite,manifest,args.out,args.vcvars)
        free_port(args.port,'0.0.0.0')
        free_port(args.query_port,'0.0.0.0')
        env={k:v for k,v in os.environ.items() if not k.upper().startswith('BF2_')}
        env.update(BF2_FRAMES=str(frames),BF2_QUIET='1',BF2_SHUTDOWN='1',BF2_PORT=str(args.port),BF2_SERVER_SCREEN='0')
        env.update(flags)
        env['BF2_TIER2_LEVEL']=level
        if suite in ('aircraft','floating'):env['BF2_TIER2_EXPECTATIONS']=str(HERE/(suite+'.tsv'))
        command=[str(binaries[suite]),'+modPath','mods/bf2','+loadLevel',level,'+gameMode','gpm_cq',
                 '+maxPlayers','64','+port',str(args.port),'+noStatusMonitor','1','+config',str(config)]
        logpath=args.out/(name+'.log')
        print('RUN',name,'frames',frames,'port',args.port,flush=True)
        with logpath.open('w') as log:
            try:
                run=subprocess.run(command,cwd=args.game_dir,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=300)
                code=run.returncode
            except subprocess.TimeoutExpired:
                code=124
        output=logpath.read_text(errors='replace')
        passed=code==0 and '[tier2-listener] PASS=1' in output and 'PORT FREE' in output
        free_port(args.port,'127.0.0.1')
        results.append(dict(case=name,exit=code,passed=passed,frames=frames,port=args.port,log=str(logpath),env=flags))
        (args.out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
        print('PASS' if passed else 'FAIL',name,'exit',code,flush=True)
    return 0 if all(r['passed'] for r in results) else 1

if __name__=='__main__':raise SystemExit(main())
