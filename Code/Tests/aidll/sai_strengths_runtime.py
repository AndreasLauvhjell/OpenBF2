"""[TEMP] Exact updateStrengths body, actual headers/pool, chosen observations."""
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
    source_path = CODE / "BF2" / AI / "AIStrategic/SAI.cpp"
    source = source_path.read_text(encoding="utf-8")
    bodies = re.findall(r"^void SAI::updateStrengths\(\)\s*\{[\s\S]*?^}", source, re.M)
    if len(bodies) != 1 or 'BF2_AI_FRONTIER("dice::hfe::ai::SAI::updateStrengths")' in bodies[0]:
        raise RuntimeError("Expected one recovered updateStrengths body")
    out.mkdir(parents=True, exist_ok=True)
    (out / "sai_strengths_actual.cpp").write_text(
        '// [TEMP] Verbatim production updateStrengths; no statement substitution.\n#include "'
        + Path(__file__).with_suffix('.h').as_posix() + '"\nnamespace dice { namespace hfe { namespace ai {\n'
        + bodies[0] + '\n} } }\n', encoding='utf-8')
    (out / "sai_strengths_source.json").write_text(json.dumps({
        "source": source_path.as_posix(), "source_sha256": hashlib.sha256(source.encode()).hexdigest(),
        "body_sha256": hashlib.sha256(bodies[0].encode()).hexdigest(), "body": bodies[0]
    }, indent=2), encoding='utf-8')
    lines = ['// [TEMP] Typed observers; all unused calls explicitly abort.',
             'namespace dice { namespace hfe { namespace ai {']
    specs = [
        ('Information', 'ObservedInformation', {'getSecurity', 'getSide'},
         'int identity = 0; ObservedInformation() : Information(nullptr, 0.0) {}',
         'float getSecurity() override { return observeSecurity(identity); }\n'
         'float getSecurity(double) override { unexpected("Information::getSecurity(double)"); }\n'
         'int getSide() const override { return observeSide(identity); }'),
        ('IPIPhysical', 'ObservedPhysical', {'getStrengthType'}, 'int identity = 0;',
         'int getStrengthType() override { return observeCategory(identity); }'),
        ('IAIInformationEnvironment', 'ObservedEnvironment', {'getAllInformation'}, '',
         'void getAllInformation(std::vector<unsigned int>& out, int side, bool which, '
         'IAIInformationGridPredicate* keep, IAIInformationGridPredicate* cull) override '
         '{ observeQuery(out, side, which, keep, cull); }\n'
         'void getAllInformation(std::vector<unsigned int>&, int, bool, IAIInformationGridPredicate*) const override '
         '{ unexpected("one-predicate query"); }')]
    for base, name, excluded, fields, observed in specs:
        relative = AI + 'AIKnowledge/' + ('BFInformationEnvironment' if base == 'IAIInformationEnvironment' else base) + '.h'
        lines.append('class ' + name + ' : public ' + base + ' { public: ' + fields)
        for method, declaration in methods(base, relative):
            if method not in excluded:
                lines.append(declaration + ' override { unexpected("' + base + '::' + method + '"); }')
        lines += [observed, '};']
    for method, declaration in methods('SAI', AI + 'AIStrategic/SAI.h'):
        signature = re.sub(r'\b' + method + r'(?=\s*\()', 'SAI::' + method, declaration, count=1)
        lines.append(signature + ' { unexpected("SAI::' + method + '"); }')
    lines.append('} } }')
    (out / 'sai_strengths_mocks.inl').write_text('\n'.join(lines), encoding='utf-8')


def run(exe):
    modes = ['empty_decay', 'mixed_chain', 'unoccupied', 'wide_product', 'unit_capture',
             'side_after_callback', 'manager_after_secondary', 'repeated_accumulation']
    summary = []
    for mode in modes:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or 'PASS ' + mode not in output or 'UNEXPECTED' in output or '[FRONTIER]' in output:
            raise RuntimeError((mode, result.returncode, output))
        summary.append('PASS ' + mode)
        print(summary[-1])
    summary.append('PASS updateStrengths: ' + str(len(modes)) + ' strict x86 cases, isolated production body')
    (exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: sai_strengths_runtime.py generate OUTPUT_DIR | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
