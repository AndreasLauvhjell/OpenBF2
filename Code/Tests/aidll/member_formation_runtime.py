"""[TEMP] Exact member/coordinator bodies; separate chosen/actual allowance variants."""
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
NORMAL = ['table_' + str(x) for x in range(8)] + [
    'capture_formation', 'fresh_leave_index', 'fresh_second_index', 'fresh_join_index',
    'retain_allowance_true', 'retain_allowance_false', 'leave_clear_timing',
    'join_clear_timing', 'no_action_flags', 'getter', 'position_copy', 'goto_borrowed']
REAL = ['dead', 'man_down', 'no_vehicle', 'fresh_player', 'real_capture', 'owner_stop',
        'coord_empty', 'coord_one', 'coord_two', 'coord_append', 'coord_shrink', 'coord_replace',
        'coord_closest_clear', 'coord_closest_restore']


def generate(out):
    methods = [('SquadMember', 'updateFormationStatus'), ('SquadMember', 'isAllowedInFormation'),
               ('SquadCoordinator', 'getSquadFormation'), ('SquadCoordinator', 'update'),
               ('SquadCoordinator', 'unAssignSquad'), ('SquadMember', 'getPosition'),
               ('SquadCoordinator', 'getSquadGotoPos')]
    records = []
    for owner, method in methods:
        path = CODE / 'BF2' / AI / 'AIBot' / (owner + '.cpp')
        source = path.read_text(encoding='utf-8')
        found = re.findall(r'^[^\n{};]+ ' + owner + '::' + method + r'\([^\n]*\)\s*\{[\s\S]*?^}', source, re.M)
        if len(found) != 1:
            raise RuntimeError('Expected unique definition: ' + owner + '::' + method)
        records.append(dict(source=path.as_posix(), owner=owner, method=method, body=found[0],
                            source_sha256=hashlib.sha256(source.encode()).hexdigest(),
                            body_sha256=hashlib.sha256(found[0].encode()).hexdigest()))
    out.mkdir(parents=True, exist_ok=True)
    actual = ['// [TEMP] Exact production definitions, no statement transformations.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    for record in records:
        if record['method'] == 'isAllowedInFormation':
            actual += ['#if defined(REAL_ALLOWANCE)', record['body'], '#endif']
        else:
            actual.append(record['body'])
    actual.append('} } }')
    (out / 'actual.cpp').write_text('\n\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(records, indent=2), encoding='utf-8')
    mock = ['// [TEMP] Typed player observer; all unused virtuals terminate.',
            'namespace dice { namespace hfe { namespace ai {',
            'class ObservedPlayer : public IAIPlayer { public: int identity = 0;']
    bodies = {'getIsAlive': 'return observePlayer(identity, 0);',
              'isManDown': 'return observePlayer(identity, 1);',
              'getCurrentVehicle': 'return observePlayer(identity, 2) ? reinterpret_cast<IAIObject*>(1) : nullptr;'}
    for name, decl in helpers.methods('IAIPlayer', 'AI/AIInterface/BFKnowledge/AIPlayer.h'):
        mock.append(decl + ' override { ' + bodies.get(name, 'unexpected("IAIPlayer::' + name + '");') + ' }')
    mock += ['};', '} } }']
    (out / 'mocks.inl').write_text('\n'.join(mock), encoding='utf-8')
    paths = [Path(__file__).with_suffix(s) for s in ('.h', '.cpp', '.py', '.cmd')]
    paths.append(Path(__file__).with_name('sai_states_runtime.py'))
    (out / 'fixture_sources.json').write_text(json.dumps({p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}, indent=2), encoding='utf-8')


def run(out):
    summary = []
    checks = 0
    stops = 0
    for actual, modes in [(False, NORMAL), (True, REAL)]:
        for mode in modes:
            exe = out / ('real_allowance.exe' if actual else 'member_formation_runtime.exe')
            proc = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
            output = proc.stdout + proc.stderr
            (out / (mode + '.log')).write_text(output, encoding='utf-8')
            # The coord_* cases ended in SquadCoordinator::update (updateSquadPosition) until
            # 2026-09-28; the tail is written now, so they complete and check the full order.
            stop = mode == 'owner_stop'
            if stop:
                name = 'SquadMember::isAllowedInFormation (IAIObject information handle)'
                expected = 'VERIFIED ' + mode + '\n[FRONTIER] dice::hfe::ai::' + name + ' reached\n'
                if proc.returncode != 3 or output != expected:
                    raise RuntimeError((mode, proc.returncode, output))
                result = 'PASS ' + mode + ' exact checked trace/diagnostic/abort 3'
                stops += 1
            else:
                match = re.fullmatch(r'PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
                if proc.returncode != 0 or not match:
                    raise RuntimeError((mode, proc.returncode, output))
                checks += int(match.group(1))
                result = 'PASS ' + mode + ' ' + match.group(1) + ' checks'
            summary.append(result)
            print(result)
    summary.append('PASS member formation: ' + str(len(NORMAL) + len(REAL)) + ' strict x86 cases, ' + str(checks) + ' normal checks, ' + str(stops) + ' exact abort traces')
    (out / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: member_formation_runtime.py generate OUT | run OUT')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
