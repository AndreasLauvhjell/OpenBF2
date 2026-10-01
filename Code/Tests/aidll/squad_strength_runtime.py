"""[TEMP] Verbatim squad-strength bodies, native lifetimes, strict x86; two declared scopes."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
MODES = ['leaves', 'empty', 'filter', 'order', 'retained_player', 'append', 'shrink',
         'outer_append', 'outer_shrink', 'frontier', 'manager_frontier']
OBSERVER_MODES = ['sum_filter', 'reload', 'append', 'shrink']


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    evidence = []
    methods = [('SquadManager', 'getSquadStrengths'), ('SquadCoordinator', 'getMissionStatus'),
               ('SquadCoordinator', 'isSquadAssignedToArea'), ('SquadCoordinator', 'getStrategicStrength'),
               ('SquadMember', 'getPlayer')]
    for owner, method in methods:
        path = CODE / ('BF2/AI/AIDLL/AI/AIBot/' + owner + '.cpp')
        source = path.read_text(encoding='utf-8')
        suffix = r'\s*\{[^{}]*\}' if owner == 'SquadMember' else r'\s*\{[\s\S]*?^}'
        found = re.findall(r'^[^\n{};]+ ' + owner + '::' + method + r'\([^\n]*\)' + suffix, source, re.M)
        if len(found) != 1:
            raise RuntimeError('Expected one exact body: ' + owner + '::' + method)
        evidence.append({'owner': owner, 'method': method, 'source': path.as_posix(), 'body': found[0],
                         'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                         'body_sha256': hashlib.sha256(found[0].encode()).hexdigest()})
    actual = ['// [TEMP] Verbatim selected bodies. Observer variant tests manager alone.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    actual.append(evidence[0]['body'])
    actual.append('#if !defined(SQUAD_STRENGTH_OBSERVERS)')
    actual.extend(x['body'] for x in evidence[1:])
    actual += ['#endif', '} } }']
    (out / 'squad_strength_actual.cpp').write_text('\n\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')

    generated = ['// [TEMP] All unused real area virtuals abort.',
                 'namespace dice { namespace hfe { namespace ai {']
    for owner in ['AIStrategicObject', 'AIStrategicArea']:
        path = CODE / ('BF2/AI/AIDLL/AI/AIStrategicEntity/' + owner + '.h')
        header = path.read_text(encoding='utf-8')
        body = re.findall(r'^class ' + owner + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', header, re.M | re.S)
        if len(body) != 1:
            raise RuntimeError('Expected class ' + owner)
        body = re.sub(r'/\*.*?\*/|//[^\n]*', '', body[0], flags=re.S)
        for original in re.findall(r'virtual\s+([^;]+);', body):
            declaration = ' '.join(original.split())
            if re.search(r'=\s*0$', declaration):
                continue
            name = re.search(r'(~?\w+)\s*\(', declaration).group(1)
            if name.startswith('~'):
                continue
            signature = re.sub(r'\b' + name + r'(?=\s*\()', owner + '::' + name, declaration, count=1)
            generated.append(signature + ' { unexpected("' + owner + '::' + name + '"); }')
    generated += ['} } }', 'class ObservedPlayer : public IAIPlayer { public:',
                  'int id; bool alive, down; IAIObject* vehicle;',
                  'ObservedPlayer(int i) : id(i), alive(false), down(false), vehicle(nullptr) {}']
    header = (CODE / 'BF2/AI/AIInterface/BFKnowledge/AIPlayer.h').read_text(encoding='utf-8')
    body = re.findall(r'^class IAIPlayer\s*\{(.*?)^};', header, re.M | re.S)
    if len(body) != 1:
        raise RuntimeError('Expected IAIPlayer')
    body = re.sub(r'/\*.*?\*/|//[^\n]*', '', body[0], flags=re.S)
    for original in re.findall(r'virtual\s+([^;]+);', body):
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(original.split()))
        name = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if name.startswith('~'):
            continue
        callback = {'getIsAlive': 'record(id, 1); return alive;',
                    'isManDown': 'record(id, 2); return down;',
                    'getCurrentVehicle': 'record(id, 3); return vehicle;'}
        generated.append(declaration + ' override { ' + callback.get(name, 'unexpected("IAIPlayer::' + name + '");') + ' }')
    generated.append('};')
    (out / 'squad_strength_mocks.inl').write_text('\n'.join(generated), encoding='utf-8')


def run(out):
    summary = []
    checks = 0
    for observers, modes in [(False, MODES), (True, OBSERVER_MODES)]:
        exe = out / ('squad_strength_observers.exe' if observers else 'squad_strength_runtime.exe')
        for mode in modes:
            result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
            output = result.stdout + result.stderr
            label = ('observer_' if observers else '') + mode
            (out / (label + '.log')).write_text(output, encoding='utf-8')
            if mode.endswith('frontier'):
                expected = 'TRACE 11 12 13\n[FRONTIER] dice::hfe::ai::SquadCoordinator::getStrategicStrength (IAIObject information query) reached\n'
                if result.returncode != 3 or output != expected:
                    raise RuntimeError((label, result.returncode, output))
                summary.append('PASS ' + label + ' exact trace, diagnostic and abort 3')
            else:
                match = re.fullmatch(r'PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
                if result.returncode != 0 or not match:
                    raise RuntimeError((label, result.returncode, output))
                checks += int(match.group(1))
                summary.append('PASS ' + label + ' ' + match.group(1) + ' checks')
            print(summary[-1])
    summary.append('PASS squad strengths: 15 strict x86 cases, ' + str(checks) + ' checks and 2 exact abort traces')
    (out / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: squad_strength_runtime.py generate OUTPUT | run OUTPUT')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
