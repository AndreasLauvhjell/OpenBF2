"""[TEMP] Three exact query bodies; native chosen lifetime fixture, strict x86."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]


def generate(out):
    specs = [('SquadMember', 'getSquad'), ('SquadCoordinator', 'getSquadPosition'),
             ('SquadCoordinator', 'getSquadLeader')]
    evidence = []
    for owner, method in specs:
        path = CODE / ('BF2/AI/AIDLL/AI/AIBot/' + owner + '.cpp')
        source = path.read_text(encoding='utf-8')
        found = re.findall(r'^[^\n{};]+ ' + owner + '::' + method + r'\([^\n]*\n\{.*?^}', source, re.M | re.S)
        if len(found) != 1 or 'FRONTIER' in found[0]:
            raise RuntimeError('Expected one recovered body: ' + owner + '::' + method)
        body = found[0]
        evidence.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                         'body': body, 'body_sha256': hashlib.sha256(body.encode()).hexdigest()})
    out.mkdir(parents=True, exist_ok=True)
    actual = ['// [TEMP] Verbatim production methods, compiled against unchanged real headers.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    actual.extend(item['body'] for item in evidence)
    actual.append('} } }')
    (out / 'squad_queries_actual.cpp').write_text('\n\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')


def run(exe):
    summary = []
    for mode in ['squad_pointer', 'position', 'leader', 'chain']:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or output != 'PASS ' + mode + '\n':
            raise RuntimeError((mode, result.returncode, output))
        summary.append(output.strip())
        print(summary[-1])
    result = subprocess.run([str(exe), 'layout'], capture_output=True, text=True, timeout=15)
    if result.returncode != 0 or result.stderr:
        raise RuntimeError(('layout', result.returncode, result.stdout, result.stderr))
    layout = json.loads(result.stdout)
    if layout['pointer_bytes'] != 4 or layout['member_squad_offset'] != 24 or layout['vec3_bytes'] != 12:
        raise RuntimeError(('native layout failed', layout))
    (exe.parent / 'native_layout.json').write_text(json.dumps(layout, indent=2), encoding='utf-8')
    summary.append('PASS squad queries: 4 strict x86 cases; native layout recorded')
    (exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: squad_queries_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
