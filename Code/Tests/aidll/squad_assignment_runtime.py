"""[TEMP] Exact assignment queries and unassignment, native chosen lifetimes, strict x86."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]


def generate(out):
    path = CODE / 'BF2/AI/AIDLL/AI/AIBot/SquadCoordinator.cpp'
    source = path.read_text(encoding='utf-8')
    evidence = []
    for method in ['isSquadAssignedToAnyArea', 'getSquadStrategicAreaAssignment', 'unAssignSquad']:
        found = re.findall(r'^[^\n{};]+ SquadCoordinator::' + method + r'\(\)\s*\{[\s\S]*?^}', source, re.M)
        if len(found) != 1 or 'FRONTIER' in found[0]:
            raise RuntimeError('Expected one recovered definition: ' + method)
        evidence.append({'source': path.as_posix(), 'body': found[0],
                         'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                         'body_sha256': hashlib.sha256(found[0].encode()).hexdigest()})
    out.mkdir(parents=True, exist_ok=True)
    actual = ['// [TEMP] Verbatim recovered methods, unchanged real headers.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    actual += [item['body'] for item in evidence]
    actual.append('} } }')
    (out / 'squad_assignment_actual.cpp').write_text('\n\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')

    mocks = ['// [TEMP] Unused area virtuals abort; queries never call an area method.',
             'namespace dice { namespace hfe { namespace ai {']
    for owner in ['AIStrategicObject', 'AIStrategicArea']:
        path = CODE / ('BF2/AI/AIDLL/AI/AIStrategicEntity/' + owner + '.h')
        header = path.read_text(encoding='utf-8')
        found = re.findall(r'^class ' + owner + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', header, re.M | re.S)
        if len(found) != 1:
            raise RuntimeError('Expected one real class: ' + owner)
        body = re.sub(r'/\*.*?\*/|//[^\n]*', '', found[0], flags=re.S)
        for original in re.findall(r'virtual\s+([^;]+);', body):
            declaration = ' '.join(original.split())
            if re.search(r'=\s*0$', declaration):
                continue
            name = re.search(r'(~?\w+)\s*\(', declaration).group(1)
            if name.startswith('~'):
                continue
            signature = re.sub(r'\b' + name + r'(?=\s*\()', owner + '::' + name, declaration, count=1)
            mocks.append(signature + ' { unexpected("' + owner + '::' + name + '"); }')
    mocks.append('} } }')
    (out / 'squad_assignment_mocks.inl').write_text('\n'.join(mocks), encoding='utf-8')


def run(exe):
    summary = []
    count = 0
    for mode in ['null', 'identity', 'unassign']:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        match = re.fullmatch(r'PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
        if result.returncode != 0 or not match:
            raise RuntimeError((mode, result.returncode, output))
        count += int(match.group(1))
        summary.append(output.strip())
        print(summary[-1])
    summary.append('PASS squad assignment: 3 strict x86 cases / ' + str(count) + ' checks')
    (exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: squad_assignment_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
