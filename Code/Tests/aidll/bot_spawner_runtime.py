"""[TEMP] Exact primary chooser and small accessors; real headers, chosen collaborators."""
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
        raise RuntimeError('Expected one real class: ' + name)
    body = re.sub(r'/\*.*?\*/|//[^\n]*', '', found[0], flags=re.S)
    for original in re.findall(r'virtual\s+([^;]+);', body):
        pure = bool(re.search(r'=\s*0\s*$', original))
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(original.split()))
        method = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not method.startswith('~'):
            yield method, declaration, pure


def generate(out):
    specifications = [
        (AI + 'AIStrategicMap/BotSpawner.cpp', r'bool BotSpawner::findSuitableSpawnGroup\(float\*[^\{]*', 'ai'),
        (AI + 'AIStrategic/SAI.cpp', r'void SAI::addSpawnToPoint\([^\n]*\)', 'ai'),
        (AI + 'AIStrategicEntity/AIStrategicObjectManager.cpp', r'AIStrategicObjectManager\* AIStrategicObjectManager::getInstance\(\)', 'ai'),
        (AI + 'AIStrategicEntity/AIStrategicObjectManager.cpp', r'AIStrategicObject\* AIStrategicObjectManager::getStrategicObject\(int[^\n]*\) const', 'ai'),
        (AI + 'AIStrategicEntity/AIStrategicObjectManager.cpp', r'AIStrategicObject\* AIStrategicObjectManager::getClosestObject\(\s*const Vec3&[^,]*, int[^,\n]*\) const', 'ai')]
    evidence = []
    for relative, signature, namespace in specifications:
        path = CODE / 'BF2' / relative
        source = path.read_text(encoding='utf-8')
        found = re.findall('^' + signature + r'\s*\{[\s\S]*?^}', source, re.M)
        if len(found) != 1 or 'BF2_AI_FRONTIER' in found[0]:
            raise RuntimeError('Expected recovered body: ' + signature)
        evidence.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                         'body_sha256': hashlib.sha256(found[0].encode()).hexdigest(), 'body': found[0],
                         'namespace': namespace})
    out.mkdir(parents=True, exist_ok=True)
    actual = ['// [TEMP] Exact production statements, no substitutions.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    actual += [item['body'] for item in evidence if item['namespace'] == 'ai']
    actual += ['}', *[item['body'] for item in evidence if item['namespace'] == 'hfe'], '} }']
    (out / 'bot_spawner_actual.cpp').write_text('\n\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    lines = ['// [TEMP] Typed chosen observations; all other virtual calls abort.',
             'namespace dice { namespace hfe { namespace ai {']
    specs = [
        ('Bot', AI + 'AIBot/BotMain.h', 'ObservedBot', {'getSide', 'getPlayer', 'getSpawnPoint', 'setSpawnTarget'}, '',
         'int getSide() const override { return observeSide(); }\n'
         'IAIPlayer* getPlayer() const override { return observePlayer(); }\n'
         'int getSpawnPoint() const override { return observePriorPoint(); }\n'
         'void setSpawnTarget(int group, int kit) override { observeTarget(-1, group, kit, 2); }\n'
         'void setSpawnTarget(int object, int group, int kit) override { observeTarget(object, group, kit, 3); }'),
        ('IAIPlayer', 'AI/AIInterface/BFKnowledge/AIPlayer.h', 'ObservedPlayer', {'getSpawnGroup'}, '',
         'int getSpawnGroup() const override { return observePlayerGroup(); }'),
        ('IAIGame', 'AI/AIInterface/BFKnowledge/BFGame.h', 'ObservedGame',
         {'getSpawnPointList', 'getSpawnPointPosition', 'isEligibleSpawnpoint'}, 'int identity = 0;',
         'const std::list<int>& getSpawnPointList(int side) const override { return observeList(identity, side); }\n'
         'const Vec3& getSpawnPointPosition(int group) const override { return observeSpawnPosition(identity, group); }\n'
         'bool isEligibleSpawnpoint(int side, int group) const override { return observeEligible(identity, side, group); }'),
        ('IKitTemplateManager', 'AI/AIInterface/BFKnowledge/KitTemplateManager.h', 'ObservedKitManager', {'getAllTemplates'}, '',
         'const std::vector<IKitTemplate*>& getAllTemplates() const override { return observeTemplates(); }'),
        ('IAIMain', AI + 'Main/AIMain.h', 'ObservedMain', {'getSAI'}, 'int identity = 0;',
         'SAI* getSAI(int side) override { return observeSAI(identity, side); }'),
        ('AIStrategicObject', AI + 'AIStrategicEntity/AIStrategicObject.h', 'ObservedObject', {'getPosition', 'gravity'},
         'int identity = 0; ObservedObject() : AIStrategicObject(0.0f, 0.0f) {}',
         'const Vec2& getPosition(int side) const override { return observeObjectPosition(identity, side, m_position[side - 1]); }\n'
         'float gravity(const Vec3& position, int side) override { return observeGravity(identity, position, side); }\n'
         'float gravity(const Vec2&, int) override { unexpected("gravity(Vec2)"); }')]
    for base, relative, name, excluded, fields, custom in specs:
        lines.append('class ' + name + ' : public ' + base + ' { public: ' + fields)
        for method, declaration, pure in methods(base, relative):
            if method not in excluded:
                lines.append(declaration + ' override { unexpected("' + base + '::' + method + '"); }')
        lines += [custom, '};']
    lines.append('class ObservedSAI : public SAI { public: int identity = 0; ObservedSAI() : SAI(1,0,0,Vec2(),Vec2()) {} void addSpawnToPoint(int point) override { observeAdd(identity, point); SAI::addSpawnToPoint(point); observeAfterAdd(identity, point); } };')
    for base, relative, excluded in [('SAI', AI + 'AIStrategic/SAI.h', {'addSpawnToPoint'}),
                                     ('AIStrategicObjectManager', AI + 'AIStrategicEntity/AIStrategicObjectManager.h', set()),
                                     ('AIStrategicObject', AI + 'AIStrategicEntity/AIStrategicObject.h', set())]:
        for method, declaration, pure in methods(base, relative):
            if pure or method in excluded:
                continue
            signature = re.sub(r'\b' + method + r'(?=\s*\()', base + '::' + method, declaration, count=1)
            lines.append(signature + ' { unexpected("' + base + '::' + method + '"); }')
    lines.append('} } }')
    (out / 'bot_spawner_mocks.inl').write_text('\n'.join(lines), encoding='utf-8')


def run(exe):
    modes = ['positive_group', 'zero_empty', 'negative_empty', 'random_equal', 'random_above',
             'prior_eligible', 'fallback_null', 'fallback_object', 'prior_empty', 'no_eligible',
             'nearest', 'tie', 'nan', 'initial_limit', 'wide_distance', 'cached_context',
             'live_list', 'target_reference', 'second_position', 'kit_index_mutation',
             'closest_thresholds', 'closest_mutation', 'counter_insert']
    summary = []
    for mode in modes:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or output != 'PASS ' + mode + '\n':
            raise RuntimeError((mode, result.returncode, output))
        summary.append(output.strip()); print(summary[-1])
    summary.append('PASS primary BotSpawner: ' + str(len(modes)) + ' strict x86 cases')
    (exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: bot_spawner_runtime.py generate OUTPUT_DIR | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
