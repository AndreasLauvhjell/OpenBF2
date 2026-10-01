"""[TEMP] Exact four-adapter extraction; native chosen lifetime collaborators."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / 'BF2'
GAME = 'AI/AIInterface/BFKnowledge/BFGame.h'
PLAYER = 'AI/AIInterface/BFKnowledge/AIPlayer.h'
MANAGER = 'Game/GameLogic/SpawnSystem/SpawnManager.h'
GROUP = 'Game/GameLogic/SpawnSystem/SpawnGroup.h'
WORLD_PLAYER = 'Game/Objects/IPlayer.h'
TESTED = {'BFGame': {'getSpawnPointList', 'getSpawnPointPosition', 'isEligibleSpawnpoint'},
          'AIPlayer': {'getSpawnGroup'}}


def virtuals(name, relative):
    source = (BF2 / relative).read_text(encoding='utf-8')
    source = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
    classes = re.findall(r'^class ' + name + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', source, re.M | re.S)
    if len(classes) != 1:
        raise RuntimeError('Expected exactly one class ' + name)
    for declaration in re.findall(r'virtual\s+([^;{}]+);', classes[0]):
        pure = bool(re.search(r'=\s*0\s*$', declaration))
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(declaration.split()))
        method = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not method.startswith('~'):
            yield method, declaration, pure


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    actual = ['// [TEMP] Verbatim production methods, unchanged real headers.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    evidence = []
    for owner, header in (('BFGame', GAME), ('AIPlayer', PLAYER)):
        path = (BF2 / header).with_suffix('.cpp')
        source = path.read_text(encoding='utf-8')
        for name in sorted(TESTED[owner]):
            found = re.findall(r'^[^\n{};]+ ' + owner + '::' + name + r'\([^\n]*\n\{.*?^}', source, re.M | re.S)
            if len(found) != 1 or 'FRONTIER' in found[0]:
                raise RuntimeError('Expected one recovered body: ' + owner + '::' + name)
            body = found[0]
            actual.append(body)
            evidence.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                             'body': body, 'body_sha256': hashlib.sha256(body.encode()).hexdigest()})
    actual.append('} } }')
    (out / 'queries_actual.cpp').write_text('\n'.join(actual), encoding='utf-8')
    (out / 'queries_source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')

    generated = ['// [TEMP] Untested virtual bodies abort. Tested bodies excluded.', 'namespace dice { namespace hfe { namespace ai {']
    for owner, header in (('BFGame', GAME), ('AIPlayer', PLAYER)):
        for name, declaration, pure in virtuals(owner, header):
            if name in TESTED[owner]:
                continue
            signature = re.sub(r'\b' + name + r'(?=\s*\()', owner + '::' + name, declaration, count=1)
            generated.append(signature + ' { unexpected("' + owner + '::' + name + '"); }')
    generated.append('}')
    for name, declaration, pure in virtuals('SpawnManager', MANAGER):
        signature = re.sub(r'\b' + name + r'(?=\s*\()', 'SpawnManager::' + name, declaration, count=1)
        generated.append(signature + ' { unexpected("SpawnManager::' + name + '"); }')
    generated.append('class GroupObserver : public ISpawnGroup { public:')
    generated.extend(['int team = 1; bool active = true; bool human = false; Vec3 position; mutable std::string trace;',
        'mutable void (*afterTeam)(const GroupObserver&) = nullptr; mutable void (*afterActive)(const GroupObserver&) = nullptr; mutable void (*afterHuman)(const GroupObserver&) = nullptr;',
        'int getTeam() const override { trace += "team,"; int value = team; if (afterTeam) afterTeam(*this); return value; }',
        'bool getActive(bool ai) const override { require(ai, "isAI argument"); trace += "active,"; bool value = active; if (afterActive) afterActive(*this); return value; }',
        'bool getOnlyForHuman() const override { trace += "human,"; bool value = human; if (afterHuman) afterHuman(*this); return value; }',
        'const Vec3& getPos() override { trace += "pos,"; return position; }'])
    for owner, header in (('IBase', 'Core/IBase.h'), ('ISpawnGroup', GROUP)):
        for name, declaration, pure in virtuals(owner, header):
            if name not in ('getTeam', 'getActive', 'getOnlyForHuman', 'getPos'):
                generated.append(declaration + ' override { unexpected("ISpawnGroup::' + name + '"); }')
    generated.extend(['};', 'namespace world {'])
    for name, declaration, pure in virtuals('IPlayer', WORLD_PLAYER):
        if not pure:
            signature = re.sub(r'\b' + name + r'(?=\s*\()', 'IPlayer::' + name, declaration, count=1)
            generated.append(signature + ' { unexpected("IPlayer::' + name + '"); }')
    generated.append('class PlayerObserver : public IPlayer { public: int group = 0; mutable int calls = 0; int getSpawnGroup() const override { ++calls; return group; }')
    for owner, header in (('IBase', 'Core/IBase.h'), ('IPlayer', WORLD_PLAYER)):
        for name, declaration, pure in virtuals(owner, header):
            if name != 'getSpawnGroup':
                generated.append(declaration + ' override { unexpected("IPlayer::' + name + '"); }')
    generated.extend(['};', '} } }'])
    (out / 'queries_collaborators.inl').write_text('\n'.join(generated), encoding='utf-8')


def run(exe):
    modes = ['player', 'list', 'position', 'zero', 'null', 'mismatch', 'inactive', 'human', 'active', 'captured']
    for mode in modes:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or output != 'PASS ' + mode + '\n':
            raise RuntimeError((mode, result.returncode, output))
        print('PASS', mode)
    print('PASS spawn queries: 10 strict x86 cases')


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: spawn_queries_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
