"""[TEMP] Exact recovered float-counter extraction and strict child-run checks."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

BF2 = Path(__file__).resolve().parents[2] / 'BF2'
CASES = {'values': 12, 'unsigned_ids': 5, 'map_mutation': 5, 'receiver_capture': 5}


def virtuals(name, relative):
    source = (BF2 / relative).read_text(encoding='utf-8')
    source = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
    classes = re.findall(r'^class ' + name + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', source, re.M | re.S)
    if len(classes) != 1:
        raise RuntimeError('Expected one class ' + name)
    for declaration in re.findall(r'virtual\s+([^;{}]+);', classes[0]):
        pure = bool(re.search(r'=\s*0\s*$', declaration))
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(declaration.split()))
        method = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not method.startswith('~'):
            yield method, declaration, pure


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    actual = ['// [TEMP] Exact production definitions, untouched real headers.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    records = []
    for relative, expression, tail in [
        ('AI/AIInterface/BFKnowledge/BFGame.cpp', r'^float BFGame::getControlPointTakeOverCounter\([^\n]*\n\{.*?^}', '} namespace world {'),
        ('Game/Objects/Game/ControlPoint.cpp', r'^float\s+ControlPoint::getTakeOverCounter\(\)\s*\{[^}]+\}', '} } }')]:
        path = BF2 / relative
        source = path.read_text(encoding='utf-8')
        bodies = re.findall(expression, source, re.M | re.S)
        if len(bodies) != 1 or 'FRONTIER' in bodies[0]:
            raise RuntimeError('Expected one recovered method ' + relative)
        body = bodies[0]
        actual.extend([body, tail])
        records.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                        'body': body, 'body_sha256': hashlib.sha256(body.encode()).hexdigest()})
    (out / 'counter_actual.cpp').write_text('\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(records, indent=2), encoding='utf-8')
    mocks = ['// [TEMP] Unused real virtuals abort; both tested bodies excluded.',
             'namespace dice { namespace hfe { namespace ai {']
    for name, declaration, _ in virtuals('BFGame', 'AI/AIInterface/BFKnowledge/BFGame.h'):
        if name != 'getControlPointTakeOverCounter':
            signature = re.sub(r'\b' + name + r'(?=\s*\()', 'BFGame::' + name, declaration, count=1)
            mocks.append(signature + ' { unexpected("BFGame::' + name + '"); }')
    mocks.append('} namespace world {')
    for owner, relative in [('IObject', 'Game/Objects/IObject.h'), ('Object', 'Game/Objects/Object.h'),
                            ('SimpleObject', 'Game/Objects/Base/SimpleObject.h'), ('Bundle', 'Game/Objects/Base/Bundle.h'),
                            ('ControlPoint', 'Game/Objects/Game/ControlPoint.h')]:
        for name, declaration, pure in virtuals(owner, relative):
            if pure or (owner == 'ControlPoint' and name == 'getTakeOverCounter'):
                continue
            signature = re.sub(r'\b' + name + r'(?=\s*\()', owner + '::' + name, declaration, count=1)
            mocks.append(signature + ' { unexpected("' + owner + '::' + name + '"); }')
    mocks.append('} class LogicObserver : public IGameLogic { public: mutable std::map<unsigned int, world::IObject*> points; mutable int calls = 0; void (*onMap)(const LogicObserver&) = nullptr; std::map<unsigned int, world::IObject*>* getControlPointMap() const override { ++calls; if(onMap) onMap(*this); return &points; }')
    for name, declaration, _ in virtuals('IGameLogic', 'Game/IGameLogic.h'):
        if name != 'getControlPointMap':
            mocks.append(declaration + ' override { unexpected("IGameLogic::' + name + '"); }')
    mocks.append('}; } }')
    (out / 'counter_collaborators.inl').write_text('\n'.join(mocks), encoding='utf-8')
    (out / 'fixture_hashes.json').write_text(json.dumps({Path(__file__).with_suffix(s).name:
        hashlib.sha256(Path(__file__).with_suffix(s).read_bytes()).hexdigest()
        for s in ('.h', '.cpp', '.py', '.cmd')}, indent=2), encoding='utf-8')


def run(exe):
    summary = []
    for mode, checks in CASES.items():
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or output != f'PASS {mode} checks={checks}\n':
            raise RuntimeError((mode, result.returncode, output))
        summary.append(output.rstrip())
    summary.append(f'PASS BFGame counter: {len(CASES)} strict x86 cases / {sum(CASES.values())} checks')
    (exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print('\n'.join(summary))


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: bf_game_counter_runtime.py generate OUTPUT | run EXE')
    path = Path(sys.argv[2]).resolve()
    generate(path) if sys.argv[1] == 'generate' else run(path)
