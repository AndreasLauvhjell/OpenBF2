"""[TEMP] Verbatim strategic-state caller and accessors, real headers, chosen observers."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
AI = "AI/AIDLL/AI/"


def methods(name, relative):
    source = (CODE / "BF2" / relative).read_text(encoding="utf-8")
    found = re.findall(r"^class " + name + r"(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};", source, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError("Expected one declaration of " + name)
    body = re.sub(r"//[^\n]*", "", found[0])
    for declaration in re.findall(r"virtual\s+([^;]+);", body):
        declaration = re.sub(r"\s*=\s*0$", "", " ".join(declaration.split()))
        name = re.search(r"(~?\w+)\s*\(", declaration).group(1)
        if not name.startswith("~"):
            yield name, declaration


def generate(out):
    specifications = [
        (AI + 'AIStrategic/SAI.cpp', r'void SAI::updateStates\([^\n]*\)'),
        (AI + 'AIStrategicEntity/AIStrategicObject.cpp', r'int AIStrategicObject::getNumberOfAssignedUnits\([^\n]*\) const'),
        (AI + 'AIStrategicEntity/AIStrategicObject.cpp', r'bool AIStrategicObject::isAttack\([^\n]*\) const'),
        (AI + 'AIStrategicEntity/AIStrategicObjectManager.cpp', r'AIStrategicObjectManager\* AIStrategicObjectManager::getInstance\(\)'),
        (AI + 'AIStrategicEntity/AIStrategicObjectManager.cpp', r'const std::vector<AIStrategicObject\*>& AIStrategicObjectManager::getStrategicObjects\(\) const')]
    evidence = []
    for relative, signature in specifications:
        path = CODE / 'BF2' / relative
        source = path.read_text(encoding='utf-8')
        found = re.findall('^' + signature + r'\s*\{[\s\S]*?^}', source, re.M)
        if len(found) != 1 or 'BF2_AI_FRONTIER' in found[0]:
            raise RuntimeError('Expected recovered body: ' + signature)
        evidence.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                         'body_sha256': hashlib.sha256(found[0].encode()).hexdigest(), 'body': found[0]})
    out.mkdir(parents=True, exist_ok=True)
    (out / 'sai_states_actual.cpp').write_text(
        '// [TEMP] Exact production bodies, unchanged statements.\n#include "'
        + Path(__file__).with_suffix('.h').as_posix() + '"\nnamespace dice { namespace hfe { namespace ai {\n'
        + '\n\n'.join(item['body'] for item in evidence) + '\n} } }\n', encoding='utf-8')
    (out / 'sai_states_source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    lines = ['// [TEMP] Actual typed declarations with chosen observations; unused methods abort.',
             'namespace dice { namespace hfe { namespace ai {']
    specs = [
        ('IAIGame', 'AI/AIInterface/BFKnowledge/BFGame.h', 'ObservedGame', {'getTickets', 'getNumberOfControlPoints'},
         'int identity = 0;',
         'int getTickets(int side) const override { return observeTickets(identity, side); }\n'
         'int getNumberOfControlPoints(int side) const override { return observeControlPoints(identity, side); }'),
        ('IBFTimer', 'AI/AIInterface/BFKnowledge/BFTimer.h', 'ObservedTimer', {'getGameTime'}, 'int identity = 0;',
         'double getGameTime() const override { return observeTime(identity); }'),
        ('AIStrategicObject', AI + 'AIStrategicEntity/AIStrategicObject.h', 'ObservedObject',
         {'getType', 'hasNeighbourWithStatus', 'hasNeighbouringObjectWithStatus', 'hasObjectTypeFlag'},
         'int identity = 0; ObservedObject() : AIStrategicObject(0.0f, 0.0f) {}',
         'int getType() const override { return observeType(identity); }\n'
         'bool hasNeighbourWithStatus(StatusMap::Status status, int side) const override { return observeNeighbour(identity, status, side); }\n'
         'bool hasNeighbouringObjectWithStatus(StatusMap::Status status, int side) const override { return observeNeighbourObject(identity, status, side); }\n'
         'bool hasObjectTypeFlag(StrategicObjectType flag) const override { return observeFlag(identity, flag); }')]
    for base, relative, name, excluded, fields, custom in specs:
        lines.append('class ' + name + ' : public ' + base + ' { public: ' + fields)
        for method, declaration in methods(base, relative):
            if method not in excluded:
                lines.append(declaration + ' override { unexpected("' + base + '::' + method + '"); }')
        lines += [custom, '};']
    for base, relative in [('SAI', AI + 'AIStrategic/SAI.h'),
                           ('AIStrategicObjectManager', AI + 'AIStrategicEntity/AIStrategicObjectManager.h')]:
        for method, declaration in methods(base, relative):
            signature = re.sub(r'\b' + method + r'(?=\s*\()', base + '::' + method, declaration, count=1)
            lines.append(signature + ' { unexpected("' + base + '::' + method + '"); }')
    # Definitions needed by the concrete base vtable during its chosen lifetime.
    for method, declaration in methods('AIStrategicObject', AI + 'AIStrategicEntity/AIStrategicObject.h'):
        source = (CODE / 'BF2' / AI / 'AIStrategicEntity/AIStrategicObject.h').read_text(encoding='utf-8')
        original = re.search(r'virtual\s+[^;]*\b' + method + r'\s*\([^;]*;', re.sub(r'//[^\n]*', '', source))
        if original and re.search(r'=\s*0\s*;', original.group(0)):
            continue
        signature = re.sub(r'\b' + method + r'(?=\s*\()', 'AIStrategicObject::' + method, declaration, count=1)
        lines.append(signature + ' { unexpected("AIStrategicObject::' + method + '"); }')
    lines.append('} } }')
    (out / 'sai_states_mocks.inl').write_text('\n'.join(lines), encoding='utf-8')


def run(exe):
    modes = ['empty', 'active_time', 'cached_game', 'owned_hostile', 'neutral',
             'retained_selection', 'assignment_sets', 'repeated_flags', 'alias_outputs',
             'live_end', 'entry_reload']
    summary = []
    for mode in modes:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or 'PASS ' + mode not in output or 'UNEXPECTED' in output or '[FRONTIER]' in output:
            raise RuntimeError((mode, result.returncode, output))
        summary.append('PASS ' + mode); print(summary[-1])
    summary.append('PASS updateStates: ' + str(len(modes)) + ' strict x86 cases, isolated production bodies')
    (exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: sai_states_runtime.py generate OUTPUT_DIR | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)

