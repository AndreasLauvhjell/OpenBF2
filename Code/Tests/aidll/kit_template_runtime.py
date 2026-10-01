"""[TEMP] Strict whole-TU engine kit-template checks, with actual frontier case."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

if len(sys.argv) != 2:
    raise SystemExit('Usage: kit_template_runtime.py EXE')
exe = Path(sys.argv[1]).resolve()
code = Path(__file__).resolve().parents[2]
sources = []
for suffix in ('.h', '.cpp'):
    path = code / ('BF2/AI/AIInterface/BFKnowledge/KitTemplate' + suffix)
    sources.append({'source': path.as_posix(), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
(exe.parent / 'source.json').write_text(json.dumps(sources, indent=2), encoding='utf-8')
summary = []
counts = {'constructor': 15, 'strengths': 16, 'independent': 4, 'destruction': 3, 'slots': 5}
for mode, count in counts.items():
    child = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
    output = child.stdout + child.stderr
    (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
    expected = f'PASS {mode} checks={count}\n'
    if child.returncode != 0 or output != expected:
        raise RuntimeError((mode, child.returncode, output, expected))
    summary.append(output.strip())
    print(summary[-1])
# 2026-09-29: the init_frontier case retired with its stop - KitTemplate::initFromObject is written
# (AI chunk 07) and has no caller in either image; the retired mode must now be refused.
child = subprocess.run([str(exe), 'init_frontier'], capture_output=True, text=True, timeout=15)
if child.returncode != 2 or child.stdout + child.stderr != '':
    raise RuntimeError(('init_frontier retired', child.returncode, child.stdout + child.stderr))
summary.append('PASS KitTemplate: 5 strict x86 cases (43 checks); initFromObject written, no caller in either image')
(exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
print(summary[-1])
