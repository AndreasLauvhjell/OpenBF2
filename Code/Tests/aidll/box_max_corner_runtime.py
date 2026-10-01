"""[TEMP] Whole production Vec3 TU; strict x86 helper fixture without substitutions."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
MODES = ['basic', 'aliases', 'rounding', 'extremes', 'nonfinite', 'x87_matrix']


def record(out):
    out.mkdir(parents=True, exist_ok=True)
    evidence = []
    for suffix in ['cpp', 'h']:
        path = CODE / ('BF2/Math/Vec3.' + suffix)
        source = path.read_text(encoding='utf-8')
        item = {'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest()}
        if suffix == 'cpp':
            found = re.findall(r'^Vec2& getBoxMaxCorner\([^\n]*\)\s*\{[\s\S]*?^}', source, re.M)
            if len(found) != 1:
                raise RuntimeError('Expected one recovered helper definition')
            item.update(body=found[0], body_sha256=hashlib.sha256(found[0].encode()).hexdigest())
        evidence.append(item)
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')


def run(exe):
    summary = []
    count = 0
    for mode in MODES:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        match = re.fullmatch(r'PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
        if result.returncode != 0 or not match:
            raise RuntimeError((mode, result.returncode, output))
        count += int(match.group(1))
        summary.append(output.strip())
        print(summary[-1])
    summary.append('PASS box max corner: ' + str(len(MODES)) + ' strict x86 cases / ' + str(count) + ' checks')
    (exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('record', 'run'):
        raise SystemExit('Usage: box_max_corner_runtime.py record OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    record(target) if sys.argv[1] == 'record' else run(target)
