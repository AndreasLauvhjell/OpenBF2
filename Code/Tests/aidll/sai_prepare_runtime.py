"""[TEMP] Exact prepareUpdate/clearSpawns/BotInfo ctor, real headers and chosen observers."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
AI = 'AI/AIDLL/AI/'


def methods(name, relative):
    source = (CODE / 'BF2' / relative).read_text(encoding='utf-8')
    found = re.findall(r'^class ' + name + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', source, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError('Expected one real declaration: ' + name)
    body = re.sub(r'//[^\n]*', '', found[0])
    for original in re.findall(r'virtual\s+([^;]+);', body):
        pure = bool(re.search(r'=\s*0\s*$', original))
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(original.split()))
        method = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not method.startswith('~'):
            yield method, declaration, pure


def generate(out):
    path = CODE / 'BF2' / AI / 'AIStrategic/SAI.cpp'
    source = path.read_text(encoding='utf-8')
    evidence = []
    for signature in (r'void SAI::prepareUpdate\(\)', r'void SAI::clearSpawnsToPoints\(\)',
                      r'SAI::BotInfo::BotInfo\(Bot\* bot\)'):
        matches = re.findall('^' + signature + r'[^\n]*\n[\s\S]*?^}', source, re.M)
        if len(matches) != 1 or 'BF2_AI_FRONTIER' in matches[0]:
            raise RuntimeError('Expected exact recovered body: ' + signature)
        evidence.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                         'body_sha256': hashlib.sha256(matches[0].encode()).hexdigest(), 'body': matches[0]})
    out.mkdir(parents=True, exist_ok=True)
    (out / 'sai_prepare_actual.cpp').write_text(
        '// [TEMP] Exact production statements; no body substitutions.\n#include "'
        + Path(__file__).with_suffix('.h').as_posix() + '"\nnamespace dice { namespace hfe { namespace ai {\n'
        + '\n\n'.join(item['body'] for item in evidence) + '\n} } }\n', encoding='utf-8')
    (out / 'sai_prepare_source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    lines = ['// [TEMP] Chosen typed receivers, unused virtuals abort.',
             'namespace dice { namespace hfe { namespace ai {']
    specs = [
        ('Bot', AI + 'AIBot/BotMain.h', 'ObservedBot',
         {'getBotId', 'isDead', 'getPlayer', 'setIsInTransitionPhase', 'getCurrentEquipment'}, 'int identity = 0;',
         'unsigned int getBotId() const override { return observeId(identity); }\n'
         'bool isDead() const override { return observeDead(identity); }\n'
         'IAIPlayer* getPlayer() const override { return observePlayer(identity); }\n'
         'void setIsInTransitionPhase(bool value) override { observeTransition(identity, value); }\n'
         'unsigned int getCurrentEquipment() const override { return observeEquipment(identity); }'),
        ('IAIPlayer', 'AI/AIInterface/BFKnowledge/AIPlayer.h', 'ObservedPlayer', {'setSpawnGroup'}, 'int identity = 0;',
         'void setSpawnGroup(int group) override { observeSpawnGroup(identity, group); }'),
        ('IAISettings', AI + 'Main/AISettingsInterfaces.h', 'ObservedSettings', {'isRespawnAllowed'}, 'int identity = 0;',
         'bool isRespawnAllowed() const override { return observeRespawn(identity); }'),
        ('Information', AI + 'AIKnowledge/Information.h', 'ObservedInformation', set(),
         'ObservedInformation() : Information(nullptr, 0.0) {}', ''),
        ('IPIPhysical', AI + 'AIKnowledge/IPIPhysical.h', 'ObservedPhysical', {'getStrengthType'}, 'int identity = 0;',
         'int getStrengthType() override { return observeCategory(identity); }')]
    for base, relative, name, excluded, fields, custom in specs:
        lines.append('class ' + name + ' : public ' + base + ' { public: ' + fields)
        for method, declaration, pure in methods(base, relative):
            if method not in excluded:
                lines.append(declaration + ' override { unexpected("' + base + '::' + method + '"); }')
        lines += [custom, '};']
    for method, declaration, pure in methods('SAI', AI + 'AIStrategic/SAI.h'):
        if method != 'setBotIsAlive':
            signature = re.sub(r'\b' + method + r'(?=\s*\()', 'SAI::' + method, declaration, count=1)
            lines.append(signature + ' { unexpected("SAI::' + method + '"); }')
    lines.append('} } }')
    (out / 'sai_prepare_mocks.inl').write_text('\n'.join(lines), encoding='utf-8')


def run(exe):
    modes = ['empty', 'pending_nulls', 'pending_dead', 'pending_alive', 'pending_live_end',
             'teleport_live_end', 'strength_filters', 'strength_overwrite', 'strength_reload',
             'spawn_nulls', 'spawn_disabled', 'spawn_mutation', 'removal_absent',
             'removal_reload', 'removal_live_end']
    summary = []
    for mode in modes:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or output != 'PASS ' + mode + '\n':
            raise RuntimeError((mode, result.returncode, output))
        summary.append(output.strip()); print(summary[-1])
    summary.append('PASS prepareUpdate: ' + str(len(modes)) + ' strict x86 cases')
    (exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: sai_prepare_runtime.py generate OUTPUT_DIR | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)

