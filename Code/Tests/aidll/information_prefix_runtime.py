"""[TEMP] Run native prefix/accessor cases; all tested methods come from the real header."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys

if len(sys.argv) != 2:
    raise SystemExit('Usage: information_prefix_runtime.py EXE')
exe = Path(sys.argv[1]).resolve()
code = Path(__file__).resolve().parents[2]
header = code / 'BF2/AI/AIDLL/AI/AIKnowledge/Information.h'
source = header.read_text(encoding='utf-8')
(exe.parent / 'source.json').write_text(json.dumps({
    'source': header.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
    'scope': 'Real header inline accessors. Chosen native lifetime; baseline is previous native declaration, not retail PE.'
}, indent=2), encoding='utf-8')
summary = []
for mode in ('native_layout', 'flags_strength', 'mobile_presence'):
    result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
    output = result.stdout + result.stderr
    (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
    if result.returncode or 'PASS ' + mode + ' checks=' not in output or 'FAIL' in output or 'UNEXPECTED' in output:
        raise RuntimeError((mode, result.returncode, output))
    summary.append(output.strip())
    print(summary[-1])
summary.append('PASS Information prefix: 3 strict x86 cases')
(exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
print(summary[-1])
