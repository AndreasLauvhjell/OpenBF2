"""[TEMP] One verbatim production getter; real-header typed observers, strict x86."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / 'BF2'
AI = 'AI/AIInterface/BFKnowledge/AIPlayer.h'
PLAYER = 'Game/Objects/IPlayer.h'

def virtuals(owner, relative):
    source = (BF2 / relative).read_text(encoding='utf-8')
    source = re.sub(r'/\*.*?\*/|//[^\n]*', '', source, flags=re.S)
    found = re.findall(r'^class ' + owner + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', source, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError('Expected one class: ' + owner)
    for original in re.findall(r'virtual\s+([^;{}]+);', found[0]):
        pure = bool(re.search(r'=\s*0\s*$', original))
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(original.split()))
        name = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not name.startswith('~'):
            yield name, declaration, pure

def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    path = (BF2 / AI).with_suffix('.cpp')
    source = path.read_text(encoding='utf-8')
    bodies = re.findall(r'^int AIPlayer::getSpawnKit\(\) const\n\{.*?^}', source, re.M | re.S)
    if len(bodies) != 1 or 'BF2_AI_FRONTIER' in bodies[0]:
        raise RuntimeError('Expected recovered getSpawnKit body')
    body = bodies[0]
    header = Path(__file__).with_suffix('.h').as_posix()
    actual = '// [TEMP] Exact production body, no substitutions.\n#include "' + header + '"\n'
    actual += 'namespace dice { namespace hfe { namespace ai {\n' + body + '\n} } }\n'
    (out / 'player_kit_actual.cpp').write_text(actual, encoding='utf-8')
    (out / 'source.json').write_text(json.dumps({'source':path.as_posix(),
        'source_sha256':hashlib.sha256(source.encode()).hexdigest(), 'body':body,
        'body_sha256':hashlib.sha256(body.encode()).hexdigest()}, indent=2), encoding='utf-8')
    lines = ['// [TEMP] Untested virtuals abort; actual getSpawnKit is excluded.',
             'namespace dice { namespace hfe { namespace ai {']
    for name, declaration, pure in virtuals('AIPlayer', AI):
        if name != 'getSpawnKit':
            signature = re.sub(r'\b' + name + r'(?=\s*\()', 'AIPlayer::' + name, declaration, count=1)
            lines.append(signature + ' { unexpected("AIPlayer::' + name + '"); }')
    lines += ['}', 'namespace world {']
    for name, declaration, pure in virtuals('IPlayer', PLAYER):
        if not pure:
            signature = re.sub(r'\b' + name + r'(?=\s*\()', 'IPlayer::' + name, declaration, count=1)
            lines.append(signature + ' { unexpected("IPlayer::' + name + '"); }')
    lines += ['class PlayerObserver : public IPlayer { public:',
              'int value = 0; mutable int calls = 0; IPlayer* replacement = nullptr;',
              'int getKit() const override { ++calls; if (replacement) replacePlayer(replacement); return value; }']
    for owner, relative in [('IBase', 'Core/IBase.h'), ('IPlayer', PLAYER)]:
        for name, declaration, pure in virtuals(owner, relative):
            if name != 'getKit':
                lines.append(declaration + ' override { unexpected("PlayerObserver::' + name + '"); }')
    lines += ['};', '} } }']
    (out / 'player_kit_collaborators.inl').write_text('\n'.join(lines), encoding='utf-8')

def run(exe):
    for mode in ['values', 'replacement']:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or not re.fullmatch(r'PASS ' + mode + r' \d+ checks\n', output):
            raise RuntimeError((mode, result.returncode, output))
        print(output, end='')
    print('PASS player kit: 2 strict x86 cases')

if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: player_kit_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
