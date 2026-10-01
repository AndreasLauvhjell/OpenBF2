"""[TEMP] Exact SAI map getter and existing setter; native x86 fixture."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
MODES = ('identity', 'contents', 'borrow_and_copy')


def generate(out):
    path = CODE / 'BF2/AI/AIDLL/AI/AIStrategic/SAI.cpp'
    source = path.read_text(encoding='utf-8')
    evidence = []
    for name in ('getDeadBots', 'setNewBotKit'):
        found = re.findall(r'^[^\n{};]+ SAI::' + name + r'\([^\n]*\)\s*(?:const\s*)?\{[\s\S]*?^}', source, re.M)
        if len(found) != 1 or 'BF2_AI_FRONTIER' in found[0]:
            raise RuntimeError('Expected exact recovered definition: ' + name)
        evidence.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                         'body': found[0], 'body_sha256': hashlib.sha256(found[0].encode()).hexdigest()})
    out.mkdir(parents=True, exist_ok=True)
    actual = ['// [TEMP] Verbatim production bodies, no substitutions.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    actual += [item['body'] for item in evidence]
    actual.append('} } }')
    (out / 'dead_bots_actual.cpp').write_text('\n\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    header = path.with_suffix('.h').read_text(encoding='utf-8')
    found = re.findall(r'^class SAI\s*:\s*public ISAI\s*\{(.*?)^};', header, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError('Expected actual concrete SAI declaration')
    body = re.sub(r'/\*.*?\*/|//[^\n]*', '', found[0], flags=re.S)
    mocks = ['// [TEMP] Every untested virtual aborts; tested methods excluded.',
             'namespace dice { namespace hfe { namespace ai {']
    excluded = set()
    for declaration in re.findall(r'virtual\s+([^;]+);', body):
        declaration = ' '.join(declaration.split())
        name = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if name.startswith('~'):
            continue
        if name in ('getDeadBots', 'setNewBotKit'):
            excluded.add(name)
            continue
        signature = re.sub(r'\b' + name + r'(?=\s*\()', 'SAI::' + name, declaration, count=1)
        mocks.append(signature + ' { unexpected("SAI::' + name + '"); }')
    if excluded != {'getDeadBots', 'setNewBotKit'}:
        raise RuntimeError('Missing real declarations')
    mocks.append('} } }')
    (out / 'dead_bots_mocks.inl').write_text('\n'.join(mocks), encoding='utf-8')


def run(exe):
    summary = []
    total = 0
    for mode in MODES:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        match = re.fullmatch(r'PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
        if result.returncode != 0 or not match:
            raise RuntimeError((mode, result.returncode, output))
        total += int(match.group(1))
        summary.append(output.strip())
    summary.append('PASS dead-bot getter: ' + str(len(MODES)) + ' strict x86 cases / ' + str(total) + ' checks')
    print('\n'.join(summary))


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: dead_bots_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
