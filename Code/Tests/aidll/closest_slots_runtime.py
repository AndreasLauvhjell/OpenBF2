"""[TEMP] Actual three closest-slot bodies; two strict x86 executable scopes."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
BASE = Path(__file__).with_suffix('')
NORMAL = ['swap', 'swap_same', 'swap_allocation_mutation', 'max_zero', 'max_one', 'all_free']
OBSERVERS = ['refuse', 'same', 'zero', 'cross', 'shrink', 'grow', 'replace',
             'clear_next', 'replace_current']
STOPS = ['selector_frontier', 'outer_frontier']


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    source_path = CODE / 'BF2/AI/AIDLL/AI/AIBot/SquadFormation.cpp'
    source = source_path.read_text(encoding='utf-8')
    evidence = []
    for method in ['updateClosestSlots', 'getClosestSlotIndex', 'swapSlots']:
        found = re.findall(r'^[^\n{};]+ SquadFormation::' + method +
                           r'\([^\n]*\n\{.*?^}', source, re.M | re.S)
        if len(found) != 1:
            raise RuntimeError('Expected one exact method: ' + method)
        evidence.append(dict(method=method, source=source_path.as_posix(),
                             source_sha256=sha(source), body=found[0], body_sha256=sha(found[0])))
    header_path = CODE / 'BF2/AI/AIDLL/AI/AIBot/SquadFormation.h'
    evidence.append(dict(source=header_path.as_posix(), source_sha256=sha(header_path.read_text(encoding='utf-8'))))
    for variant in ['actual', 'observer']:
        actual = ['// [TEMP] Verbatim production definitions; unchanged real header.',
                  '#include "' + BASE.with_suffix('.h').as_posix() + '"',
                  'namespace dice { namespace hfe { namespace ai {']
        actual += [e['body'] for e in evidence if 'body' in e and
                   (variant == 'actual' or e['method'] != 'getClosestSlotIndex')]
        actual += ['} } }']
        (out / ('closest_' + variant + '.cpp')).write_text('\n\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    fixtures = [BASE.with_suffix(ext) for ext in ['.h', '.cpp', '.py', '.cmd']]
    (out / 'fixtures.json').write_text(json.dumps([dict(path=p.as_posix(), sha256=sha(p.read_text(encoding='utf-8'))) for p in fixtures], indent=2), encoding='utf-8')


def run(out):
    summary = []
    total = 0
    for mode in NORMAL + OBSERVERS + STOPS:
        exe = out / ('closest_observer.exe' if mode in OBSERVERS else 'closest_actual.exe')
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (out / (mode + '.log')).write_text(output, encoding='utf-8')
        if mode in STOPS:
            expected = '[FRONTIER] dice::hfe::ai::SquadFormation::getClosestSlotIndex (getSlotWorldPos) reached\n'
            if result.returncode != 3 or output != expected:
                raise RuntimeError((mode, result.returncode, output))
            line = 'PASS ' + mode + ' exact diagnostic and abort'
        else:
            match = re.fullmatch(r'PASS ' + mode + r' (\d+)\n', output)
            if result.returncode != 0 or not match:
                raise RuntimeError((mode, result.returncode, output))
            total += int(match[1])
            line = output.strip()
        print(line)
        summary.append(line)
    line = f'PASS closest slots: {len(summary)} cases, {total} checks, {len(STOPS)} exact frontier traces'
    print(line)
    summary.append(line)
    (out / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ['generate', 'run']:
        raise SystemExit('Usage: closest_slots_runtime.py generate|run OUTPUT')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
