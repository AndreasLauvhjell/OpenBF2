"""[TEMP] Two exact adapter definitions, independent from prior passing suites."""
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
TESTED = {'BFGame': {'getControlPointId'}, 'AIPlayer': {'getIsAIPlayer'}}

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
    actual = ['// [TEMP] Verbatim actual methods, untouched real headers.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    evidence = []
    for owner, header in (('BFGame', GAME), ('AIPlayer', PLAYER)):
        path = (BF2 / header).with_suffix('.cpp')
        source = path.read_text(encoding='utf-8')
        for name in TESTED[owner]:
            found = re.findall(r'^[^\n{};]+ ' + owner + '::' + name + r'\([^\n]*\n\{.*?^}', source, re.M | re.S)
            if len(found) != 1 or 'BF2_AI_FRONTIER' in found[0]:
                raise RuntimeError('Expected recovered body: ' + owner + '::' + name)
            body = found[0]
            actual.append(body)
            evidence.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                             'body': body, 'body_sha256': hashlib.sha256(body.encode()).hexdigest()})
    actual.append('} } }')
    (out / 'selector_queries_actual.cpp').write_text('\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    lines = ['// [TEMP] Tested bodies excluded; untested virtuals abort.', 'namespace dice { namespace hfe { namespace ai {']
    for owner, header in (('BFGame', GAME), ('AIPlayer', PLAYER)):
        for name, declaration, pure in virtuals(owner, header):
            if name not in TESTED[owner]:
                signature = re.sub(r'\b' + name + r'(?=\s*\()', owner + '::' + name, declaration, count=1)
                lines.append(signature + ' { unexpected("' + owner + '::' + name + '"); }')
    lines.append('}')
    for name, declaration, pure in virtuals('SpawnManager', MANAGER):
        signature = re.sub(r'\b' + name + r'(?=\s*\()', 'SpawnManager::' + name, declaration, count=1)
        lines.append(signature + ' { unexpected("SpawnManager::' + name + '"); }')
    lines += ['class GroupObserver : public ISpawnGroup { public:',
              'unsigned int control = 0; int calls = 0;',
              'unsigned int getControlPointId() override { ++calls; return control; }']
    for owner, header in (('IBase', 'Core/IBase.h'), ('ISpawnGroup', GROUP)):
        for name, declaration, pure in virtuals(owner, header):
            if name != 'getControlPointId':
                lines.append(declaration + ' override { unexpected("ISpawnGroup::' + name + '"); }')
    lines += ['};', 'namespace world {']
    for name, declaration, pure in virtuals('IPlayer', WORLD_PLAYER):
        if not pure:
            signature = re.sub(r'\b' + name + r'(?=\s*\()', 'IPlayer::' + name, declaration, count=1)
            lines.append(signature + ' { unexpected("IPlayer::' + name + '"); }')
    lines += ['class PlayerObserver : public IPlayer { public: bool ai = false; mutable int calls = 0;',
              'bool getIsAIPlayer() const override { ++calls; return ai; }']
    for owner, header in (('IBase', 'Core/IBase.h'), ('IPlayer', WORLD_PLAYER)):
        for name, declaration, pure in virtuals(owner, header):
            if name != 'getIsAIPlayer':
                lines.append(declaration + ' override { unexpected("IPlayer::' + name + '"); }')
    lines += ['};', '} } }']
    (out / 'selector_queries_collaborators.inl').write_text('\n'.join(lines), encoding='utf-8')

def run(exe):
    for mode in ['player_values', 'player_replacement', 'control_bits', 'manager_replacement']:
        process = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = process.stdout + process.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if process.returncode != 0 or output != 'PASS ' + mode + '\n':
            raise RuntimeError((mode, process.returncode, output))
        print('PASS', mode)
    print('PASS selector queries: 4 strict x86 cases')

if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: selector_queries_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
