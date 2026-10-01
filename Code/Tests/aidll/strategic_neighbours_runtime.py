"""[TEMP] Exact production neighbour scans; real headers and chosen native lifetimes."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
ENTITY = CODE / 'BF2/AI/AIDLL/AI/AIStrategicEntity'
TESTED = {'hasNeighbourWithStatus', 'hasNeighbouringObjectWithStatus'}


def declarations(name):
    source = (ENTITY / (name + '.h')).read_text(encoding='utf-8')
    found = re.findall(r'^class ' + name + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', source, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError('Expected one real class: ' + name)
    body = re.sub(r'//[^\n]*', '', found[0])
    for original in re.findall(r'virtual\s+([^;]+);', body):
        pure = bool(re.search(r'=\s*0\s*$', original))
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(original.split()))
        method = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not method.startswith('~'):
            yield method, declaration, pure


def generate(out):
    path = ENTITY / 'AIStrategicArea.cpp'
    source = path.read_text(encoding='utf-8')
    evidence = []
    for method in sorted(TESTED):
        matches = re.findall(r'^bool AIStrategicArea::' + method + r'\([^)]*\) const\s*\{[\s\S]*?^}', source, re.M)
        if len(matches) != 1 or 'BF2_AI_FRONTIER' in matches[0]:
            raise RuntimeError('Expected one recovered body: ' + method)
        evidence.append({'source': path.as_posix(), 'method': method,
                         'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                         'body_sha256': hashlib.sha256(matches[0].encode()).hexdigest(),
                         'body': matches[0]})
    out.mkdir(parents=True, exist_ok=True)
    (out / 'strategic_neighbours_actual.cpp').write_text(
        '// [TEMP] Exact production bodies, unchanged statements.\n#include "'
        + Path(__file__).with_suffix('.h').as_posix() + '"\nnamespace dice { namespace hfe { namespace ai {\n'
        + '\n\n'.join(item['body'] for item in evidence) + '\n} } }\n', encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    lines = ['// [TEMP] Unexercised virtual bodies abort; tested methods are never substituted.',
             'namespace dice { namespace hfe { namespace ai {']
    for name in ('AIStrategicObject', 'AIStrategicArea'):
        for method, declaration, pure in declarations(name):
            if pure or (name == 'AIStrategicArea' and method in TESTED):
                continue
            signature = re.sub(r'\b' + method + r'(?=\s*\()', name + '::' + method, declaration, count=1)
            lines.append(signature + ' { unexpected("' + name + '::' + method + '"); }')
    lines.append('} } }')
    (out / 'strategic_neighbours_mocks.inl').write_text('\n'.join(lines), encoding='utf-8')


def run(exe):
    modes = ['empty', 'side_selection', 'first', 'later', 'miss', 'distinct_vectors',
             'unknown', 'early_first', 'early_later']
    summary = []
    for mode in modes:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or 'PASS ' + mode + ' checks=' not in output or 'UNEXPECTED' in output or 'FAIL' in output:
            raise RuntimeError((mode, result.returncode, output))
        summary.append(output.strip())
        print(summary[-1])
    summary.append('PASS neighbour scans: 9 strict x86 cases, two exact production bodies')
    (exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: strategic_neighbours_runtime.py generate OUTPUT_DIR | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
