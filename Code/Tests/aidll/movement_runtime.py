"""[TEMP] Exact bounded member movement; actual and explicitly observed getter scopes."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
BASE = Path(__file__).with_suffix('')
spec = importlib.util.spec_from_file_location('typed_declarations', Path(__file__).with_name('sai_states_runtime.py'))
helpers = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helpers)
NORMAL = ['human', 'dead', 'dead_mutation', 'mobile_no_vehicle', 'mobile_dead',
          'mobile_down', 'mobile_fresh_players', 'mobile_null_mutation']
STOPS = ['formation_actual', 'alive_null', 'alive_vehicle', 'fresh_player', 'mobile_owner']
OBSERVERS = ['formation_order', 'formation_invalid', 'independent_order', 'independent_mutation']


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def generate(out):
    records = []
    for owner, name in [('SquadMember', 'moveWithFormation'), ('SquadMember', 'moveWithoutFormation'),
                        ('SquadMember', 'isMobile'),
                        ('SquadMember', 'getPosition'), ('SquadCoordinator', 'getSquadFormation'),
                        ('SquadCoordinator', 'getSquadGotoPos')]:
        path = CODE / 'BF2/AI/AIDLL/AI/AIBot' / (owner + '.cpp')
        source = path.read_text(encoding='utf-8')
        found = re.findall(r'^[^\n{};]+ ' + owner + '::' + name + r'\([^\n]*\)\s*\{[\s\S]*?^}', source, re.M)
        if len(found) != 1:
            raise RuntimeError('Expected unique definition: ' + owner + '::' + name)
        records.append(dict(source=path.as_posix(), owner=owner, method=name, body=found[0],
                            source_sha256=sha(source), body_sha256=sha(found[0])))
    out.mkdir(parents=True, exist_ok=True)
    for variant in ['actual', 'observer']:
        lines = ['// [TEMP] Verbatim production bodies; unchanged real headers.',
                 '#include "' + BASE.with_suffix('.h').as_posix() + '"',
                 'namespace dice { namespace hfe { namespace ai {']
        lines += [r['body'] for r in records if variant == 'actual' or r['method'].startswith('move') or r['method'] == 'isMobile']
        lines += ['} } }']
        (out / (variant + '.cpp')).write_text('\n\n'.join(lines), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(records, indent=2), encoding='utf-8')
    mock = ['// [TEMP] Typed player observer. Every unused virtual aborts.',
            'namespace dice { namespace hfe { namespace ai {',
            'class ObservedPlayer : public IAIPlayer { public: int identity = 0;']
    bodies = {'getIsAlive': 'return observeAlive(identity);', 'isManDown': 'return observeDown(identity);',
              'getCurrentVehicle': 'return observeVehicle(identity);'}
    for name, decl in helpers.methods('IAIPlayer', 'AI/AIInterface/BFKnowledge/AIPlayer.h'):
        mock.append(decl + ' override { ' + bodies.get(name, 'unexpected("IAIPlayer::' + name + '");') + ' }')
    mock += ['};', '} } }']
    (out / 'mocks.inl').write_text('\n'.join(mock), encoding='utf-8')
    paths = [BASE.with_suffix(s) for s in ['.h', '.cpp', '.py', '.cmd']]
    paths.append(Path(__file__).with_name('sai_states_runtime.py'))
    (out / 'fixture_sources.json').write_text(json.dumps({p.as_posix(): sha(p.read_text(encoding='utf-8')) for p in paths}, indent=2), encoding='utf-8')


def run(out):
    summary, checks, stops = [], 0, 0
    for mode in NORMAL + STOPS + OBSERVERS:
        proc = subprocess.run([str(out / ('observer.exe' if mode in OBSERVERS else 'actual.exe')), mode],
                              capture_output=True, text=True, timeout=15)
        output = proc.stdout + proc.stderr
        (out / (mode + '.log')).write_text(output, encoding='utf-8')
        if mode in NORMAL:
            match = re.fullmatch(r'PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
            if proc.returncode != 0 or not match:
                raise RuntimeError((mode, proc.returncode, output))
            checks += int(match[1])
            line = output.strip()
        else:
            method = ('moveWithFormation (SquadFormation::getSlotWorldPos)' if mode.startswith('formation_')
                      else 'isMobile (IAIObject information handle)' if mode.startswith('mobile_')
                      else 'moveWithoutFormation (IAIObject information handle)')
            expected = 'VERIFIED ' + mode + '\n[FRONTIER] dice::hfe::ai::SquadMember::' + method + ' reached\n'
            if proc.returncode != 3 or output != expected:
                raise RuntimeError((mode, proc.returncode, output))
            stops += 1
            line = 'PASS ' + mode + ' exact checked trace/diagnostic/abort 3'
        summary.append(line)
        print(line)
    line = f'PASS movement: {len(summary)} strict x86 cases, {checks} normal checks, {stops} exact abort traces'
    print(line)
    summary.append(line)
    (out / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ['generate', 'run']:
        raise SystemExit('Usage: movement_runtime.py generate|run OUTPUT')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
