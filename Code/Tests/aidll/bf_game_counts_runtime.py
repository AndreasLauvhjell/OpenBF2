"""[TEMP] Exact adapter/getTeam extraction, native chosen lifetime collaborators."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / 'BF2'


def virtuals(name, relative):
    text = (BF2 / relative).read_text(encoding='utf-8')
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
    classes = re.findall(r'^class ' + name + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', text, re.M | re.S)
    if len(classes) != 1:
        raise RuntimeError('Expected exactly one class ' + name)
    for declaration in re.findall(r'virtual\s+([^;{}]+);', classes[0]):
        pure = bool(re.search(r'=\s*0\s*$', declaration))
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(declaration.split()))
        method = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not method.startswith('~'):
            yield method, declaration, pure


def extract(relative, expression):
    path = BF2 / relative
    source = path.read_text(encoding='utf-8')
    found = re.findall(expression, source, re.M | re.S)
    if len(found) != 1 or 'FRONTIER' in found[0]:
        raise RuntimeError('Expected one recovered production method: ' + expression)
    body = found[0]
    return body, {'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(), 'body': body, 'body_sha256': hashlib.sha256(body.encode()).hexdigest()}


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    actual = ['// [TEMP] Exact production method text, compiled against real headers.', '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"', 'namespace dice { namespace hfe { namespace ai {']
    evidence = []
    for name in ('getTickets', 'getNumberOfControlPoints'):
        body, record = extract('AI/AIInterface/BFKnowledge/BFGame.cpp', r'^int BFGame::' + name + r'\([^\n]*\n\{.*?^}')
        actual.append(body)
        evidence.append(record)
    actual.append('} namespace world {')
    body, record = extract('Game/Objects/Game/ControlPoint.cpp', r'^int\s+ControlPoint::getTeam\(\)\s*\{[^}]+\}')
    actual.extend([body, '} } }'])
    evidence.append(record)
    (out / 'counts_actual.cpp').write_text('\n'.join(actual), encoding='utf-8')
    (out / 'counts_source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')

    generated = ['// [TEMP] All unused virtuals abort; tested production methods excluded.', 'namespace dice { namespace hfe { namespace ai {']
    for name, declaration, pure in virtuals('BFGame', 'AI/AIInterface/BFKnowledge/BFGame.h'):
        if name in ('getTickets', 'getNumberOfControlPoints'):
            continue
        signature = re.sub(r'\b' + name + r'(?=\s*\()', 'BFGame::' + name, declaration, count=1)
        generated.append(signature + ' { unexpected("BFGame::' + name + '"); }')
    generated.append('} namespace world {')
    for owner, path in (('IObject', 'Game/Objects/IObject.h'), ('Object', 'Game/Objects/Object.h'), ('SimpleObject', 'Game/Objects/Base/SimpleObject.h'), ('Bundle', 'Game/Objects/Base/Bundle.h'), ('ControlPoint', 'Game/Objects/Game/ControlPoint.h')):
        for name, declaration, pure in virtuals(owner, path):
            if pure or (owner == 'ControlPoint' and name == 'getTeam'):
                continue
            signature = re.sub(r'\b' + name + r'(?=\s*\()', owner + '::' + name, declaration, count=1)
            generated.append(signature + ' { unexpected("' + owner + '::' + name + '"); }')
    generated.append('class ScoreObserver : public IScoreManager { public:')
    generated.append('TeamScore* selected = nullptr; IScoreManager* replacement = nullptr; std::vector<unsigned int> sides; TeamScore* getTeamScore(unsigned int team) override { sides.push_back(team); if(replacement) scoreManager = replacement; return selected; }')
    for owner, path in (('IBase', 'Core/IBase.h'), ('IScoreManager', 'Game/GameLogic/ScoreSystem/ScoreManager.h')):
        for name, declaration, pure in virtuals(owner, path):
            if name != 'getTeamScore':
                generated.append(declaration + ' override { unexpected("IScoreManager::' + name + '"); }')
    generated.extend(['};', '}'])
    generated.append('class LogicObserver : public IGameLogic { public: std::map<unsigned int, world::IObject*> points; mutable int calls = 0; std::map<unsigned int, world::IObject*>* getControlPointMap() const override { ++calls; return const_cast<std::map<unsigned int, world::IObject*>*>(&points); }')
    for name, declaration, pure in virtuals('IGameLogic', 'Game/IGameLogic.h'):
        if name != 'getControlPointMap':
            generated.append(declaration + ' override { unexpected("IGameLogic::' + name + '"); }')
    generated.extend(['};', '} }'])
    (out / 'counts_collaborators.inl').write_text('\n'.join(generated), encoding='utf-8')


def run(exe):
    modes = ['tickets', 'null_logic', 'empty', 'teams', 'live_map', 'repeated_object']
    for mode in modes:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or output != 'PASS ' + mode + '\n':
            raise RuntimeError((mode, result.returncode, output))
        print('PASS', mode)
    print('PASS BFGame counts: 6 strict x86 cases')


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: bf_game_counts_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
