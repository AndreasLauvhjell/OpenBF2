"""[TEMP] Exact coordinator prefix/path bodies, real headers, chosen native observers."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
AI = 'AI/AIDLL/AI/'
spec = importlib.util.spec_from_file_location('typed_declarations', Path(__file__).with_name('sai_states_runtime.py'))
helpers = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helpers)
MODES = ['empty', 'timestamp_bits', 'commander', 'null_assignment', 'other_kind',
         'owned_attack', 'takeable', 'own_all', 'not_own_all', 'defend_side_one',
         'defend_side_two', 'defend_not_owned', 'defend_refusal', 'commander_mutation',
         'fresh_main_side', 'captured_enemy', 'cleared_members', 'paths',
         'nonempty_tail', 'duplicate_members_tail', 'members_append_tail', 'members_shrink_tail',
         'members_replace_tail', 'takeable_stop', 'own_all_stop', 'have_any_stop']
MODES += ['ticks_append_tail', 'ticks_shrink_tail', 'ticks_replace_tail']
OBSERVER_MODES = ['paths_append', 'paths_shrink', 'paths_replace']
# The eight member-loop cases ran into the updateSquadPosition stop until 2026-09-28; the
# coordinator tail is now written, so they complete and main() checks the whole ordered trace.
STOPS = {
    'takeable_stop': ('m0:1,c0,a', 'AIStrategicArea::isAreaTakeable'),
    'own_all_stop': ('m0:1,c0,a,m0:1,o0:1', 'SAI::ownAllTakeableCps'),
    'have_any_stop': ('m0:1,c0,m0:1,h0:2', 'SAI::haveAnyOwnedAndTakeableCps')}


def generate(out):
    methods = [('AIBot/SquadCoordinator.cpp', 'SquadCoordinator', 'update'),
               ('AIBot/SquadCoordinator.cpp', 'SquadCoordinator', 'updateSquadPaths'),
               ('AIBot/SquadCoordinator.cpp', 'SquadCoordinator', 'unAssignSquad'),
               ('AIBot/SquadMember.cpp', 'SquadMember', 'setHavePath'),
               ('AIStrategic/SAI.cpp', 'SAI', 'isPlayerCommander'),
               ('AIStrategic/SAI.cpp', 'SAI', 'ownAllTakeableCps'),
               ('AIStrategic/SAI.cpp', 'SAI', 'haveAnyOwnedAndTakeableCps'),
               ('AIStrategicEntity/AIStrategicArea.cpp', 'AIStrategicArea', 'isAreaTakeable')]
    evidence = []
    for relative, owner, method in methods:
        path = CODE / 'BF2' / (AI + relative)
        source = path.read_text(encoding='utf-8')
        found = re.findall(r'^[^\n{};]+ ' + owner + '::' + method + r'\([^\n]*\)\s*\{[\s\S]*?^}', source, re.M)
        if len(found) != 1:
            raise RuntimeError('Expected unique exact body: ' + owner + '::' + method)
        evidence.append(dict(source=path.as_posix(), owner=owner, method=method, body=found[0],
                             source_sha256=hashlib.sha256(source.encode()).hexdigest(),
                             body_sha256=hashlib.sha256(found[0].encode()).hexdigest()))
    out.mkdir(parents=True, exist_ok=True)
    actual = ['// [TEMP] Exact definitions. Setter-only observer variant is explicitly separate.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    for item in evidence:
        if item['method'] == 'setHavePath':
            actual += ['#if !defined(PATH_OBSERVERS)', item['body'], '#endif']
        else:
            actual.append(item['body'])
    actual.append('} } }')
    (out / 'actual.cpp').write_text('\n\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    lines = ['// [TEMP] Untested virtuals abort; chosen typed observers never shadow tested definitions.',
             'namespace dice { namespace hfe { namespace ai {']
    for base, relative in [('SAI', 'AIStrategic/SAI.h'), ('AIStrategicObject', 'AIStrategicEntity/AIStrategicObject.h'),
                           ('AIStrategicArea', 'AIStrategicEntity/AIStrategicArea.h')]:
        source = re.sub(r'//[^\n]*', '', (CODE / 'BF2' / (AI + relative)).read_text(encoding='utf-8'))
        for name, decl in helpers.methods(base, AI + relative):
            if any(e['owner'] == base and e['method'] == name for e in evidence):
                continue
            original = re.search(r'virtual\s+[^;]*\b' + name + r'\s*\([^;]*;', source)
            if base == 'AIStrategicObject' and original and re.search(r'=\s*0\s*;', original.group(0)):
                continue
            signature = re.sub(r'\b' + name + r'(?=\s*\()', base + '::' + name, decl, count=1)
            lines.append(signature + ' { unexpected("' + base + '::' + name + '"); }')
    lines += ['class ObservedMain : public IAIMain { public: int identity = 0;']
    for name, decl in helpers.methods('IAIMain', AI + 'Main/AIMain.h'):
        if name != 'getSAI':
            lines.append(decl + ' override { unexpected("IAIMain::' + name + '"); }')
    lines += ['SAI* getSAI(int side) override { return observeMain(identity, side); }', '};', '} } }']
    (out / 'mocks.inl').write_text('\n'.join(lines), encoding='utf-8')
    paths = [Path(__file__).with_suffix(s) for s in ('.h', '.cpp', '.py', '.cmd')]
    paths.append(Path(__file__).with_name('sai_states_runtime.py'))
    (out / 'fixture_sources.json').write_text(json.dumps({p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}, indent=2), encoding='utf-8')


def run(out):
    summary = []
    checks = 0
    for observer, modes in [(False, MODES), (True, OBSERVER_MODES)]:
        exe = out / ('path_observers.exe' if observer else 'coordinator_tick_runtime.exe')
        for mode in modes:
            proc = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
            output = proc.stdout + proc.stderr
            (out / (mode + '.log')).write_text(output, encoding='utf-8')
            if mode in STOPS:
                trace, name = STOPS[mode]
                expected = 'TRACE ' + trace + '\n[FRONTIER] dice::hfe::ai::' + name + ' reached\n'
                if proc.returncode != 3 or output != expected:
                    raise RuntimeError((mode, proc.returncode, output))
                result = 'PASS ' + mode + ' exact trace/diagnostic/abort 3'
            else:
                match = re.fullmatch(r'PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
                if proc.returncode != 0 or not match:
                    raise RuntimeError((mode, proc.returncode, output))
                checks += int(match.group(1))
                result = 'PASS ' + mode + ' ' + match.group(1) + ' checks'
            summary.append(result)
            print(result)
    summary.append('PASS coordinator tick: ' + str(len(MODES) + len(OBSERVER_MODES)) + ' strict x86 cases, ' + str(checks) + ' normal checks and ' + str(len(STOPS)) + ' exact abort traces')
    (out / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: coordinator_tick_runtime.py generate OUT | run OUT')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
