"""[TEMP] Four verbatim metadata methods; no production header transformations."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / 'BF2'
BOT = 'AI/AIDLL/AI/AIBot/BotMain.h'
SAI = 'AI/AIDLL/AI/AIStrategic/SAI.h'
MAIN = 'AI/AIDLL/AI/Main/AIMain.h'
PLAYER = 'AI/AIInterface/BFKnowledge/AIPlayer.h'
KIT = 'AI/AIInterface/BFKnowledge/KitTemplateManager.h'

def methods(name, relative):
    text = (BF2 / relative).read_text(encoding='utf-8')
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
    classes = re.findall(r'^class ' + name + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', text, re.M | re.S)
    if len(classes) != 1:
        raise RuntimeError('Expected one class ' + name)
    for original in re.findall(r'virtual\s+([^;{}]+);', classes[0]):
        pure = bool(re.search(r'=\s*0\s*$', original))
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(original.split()))
        method = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not method.startswith('~'):
            yield method, declaration, pure

def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    specs = [(BOT, 'BotMain', 'getSpawnPoint', 1),
             (BOT, 'BotMain', 'setSpawnTarget', 2),
             (KIT, 'KitTemplateManager', 'getAllTemplates', 1)]
    actual = ['// [TEMP] Exact production definitions, no substitutions.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    evidence = []
    for relative, owner, name, count in specs:
        path = (BF2 / relative).with_suffix('.cpp')
        source = path.read_text(encoding='utf-8')
        found = re.findall(r'^[^\n{};]+ ' + owner + '::' + name + r'\([^\n]*\n\{.*?^}', source, re.M | re.S)
        if len(found) != count or any('BF2_AI_FRONTIER' in body for body in found):
            raise RuntimeError('Expected recovered definitions: ' + owner + '::' + name)
        for body in found:
            actual.append(body)
            evidence.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                             'body': body, 'body_sha256': hashlib.sha256(body.encode()).hexdigest()})
    actual.append('} } }')
    (out / 'metadata_actual.cpp').write_text('\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    lines = ['// [TEMP] Untested virtuals abort; all tested definitions excluded.',
             'namespace dice { namespace hfe { namespace ai {']
    for owner, relative, excluded in [('BotMain', BOT, {'getSpawnPoint', 'setSpawnTarget'}),
                                       ('Bot', BOT, set()), ('SAI', SAI, {'setNewBotKit'}),
                                       ('KitTemplateManager', KIT, {'getAllTemplates'})]:
        for name, declaration, pure in methods(owner, relative):
            if pure or name in excluded:
                continue
            qualified = re.sub(r'\b' + name + r'(?=\s*\()', owner + '::' + name, declaration, count=1)
            lines.append(qualified + ' { unexpected("' + owner + '::' + name + '"); }')
    specs = [
        ('IAIPlayer', PLAYER, 'PlayerObserver', {'getSpawnGroup', 'setSpawnGroup', 'setSpawnKit'},
         'int identity = 0; int group = 0; mutable int getCalls = 0;',
         'int getSpawnGroup() const override { ++getCalls; return group; }\n'
         'void setSpawnGroup(int value) override { observeGroup(identity, value); }\n'
         'void setSpawnKit(int value) override { observeKit(identity, value); }'),
        ('IAIMain', MAIN, 'MainObserver', {'getSAI'}, 'int identity = 0;',
         'SAI* getSAI(int side) override { return observeSAI(identity, side); }')]
    for base, relative, owner, excluded, fields, overrides in specs:
        lines.append('class ' + owner + ' : public ' + base + ' { public: ' + fields)
        for name, declaration, pure in methods(base, relative):
            if name not in excluded:
                lines.append(declaration + ' override { unexpected("' + owner + '::' + name + '"); }')
        lines += [overrides, '};']
    lines.append('} } }')
    (out / 'metadata_collaborators.inl').write_text('\n'.join(lines), encoding='utf-8')

def run(exe):
    modes = ['kit_reference', 'spawn_point', 'two', 'three', 'two_mutation', 'three_mutation']
    for mode in modes:
        process = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = process.stdout + process.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if process.returncode != 0 or output != 'PASS ' + mode + '\n':
            raise RuntimeError((mode, process.returncode, output))
        print('PASS', mode)
    print('PASS spawn metadata: 6 strict x86 cases')

if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: spawn_metadata_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
