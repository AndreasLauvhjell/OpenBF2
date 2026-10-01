"""[TEMP] Exact squad kit queries; strict native x86, chosen collaborators."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
MODES = ['manager_empty', 'manager_order', 'manager_miss', 'manager_fresh',
         'manager_append', 'manager_shrink', 'kit_empty_null', 'kit_order',
         'kit_integer', 'kit_replace', 'kit_append', 'kit_shrink', 'kit_match_mutation']


def generate(out):
    evidence = []
    for owner, method in [('SquadManager', 'getSquadFromPlayer'),
                          ('SquadCoordinator', 'hasMemberWithKit'), ('SquadMember', 'getPlayer')]:
        path = CODE / ('BF2/AI/AIDLL/AI/AIBot/' + owner + '.cpp')
        source = path.read_text(encoding='utf-8')
        pattern = (r'^[^\n{};]+ ' + owner + '::' + method +
                   r'\([^\n]*\)\s*\{[^{}]*\}' if owner == 'SquadMember' else
                   r'^[^\n{};]+ ' + owner + '::' + method + r'\([^\n]*\)\s*\{[\s\S]*?^}')
        found = re.findall(pattern, source, re.M)
        if len(found) != 1 or 'FRONTIER' in found[0]:
            raise RuntimeError('Expected one recovered method: ' + owner + '::' + method)
        evidence.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                         'body': found[0], 'body_sha256': hashlib.sha256(found[0].encode()).hexdigest()})
    out.mkdir(parents=True, exist_ok=True)
    actual = ['// [TEMP] Verbatim production methods against unchanged real headers.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    actual += [item['body'] for item in evidence]
    actual.append('} } }')
    (out / 'squad_kit_actual.cpp').write_text('\n\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')

    path = CODE / 'BF2/AI/AIInterface/BFKnowledge/AIPlayer.h'
    source = path.read_text(encoding='utf-8')
    found = re.findall(r'^class IAIPlayer\s*\{(.*?)^};', source, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError('Expected real IAIPlayer declaration')
    body = re.sub(r'/\*.*?\*/|//[^\n]*', '', found[0], flags=re.S)
    mocks = ['// [TEMP] Typed observer. Every unexercised virtual aborts.',
             'class ObservedPlayer : public IAIPlayer { public:',
             'int identity; int kit;',
             'ObservedPlayer(int id, int value) : identity(id), kit(value) {}']
    count = 0
    for original in re.findall(r'virtual\s+([^;]+);', body):
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(original.split()))
        method = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if method.startswith('~'):
            continue
        count += 1
        if method == 'getSpawnKit':
            mocks.append(declaration + ' override { return observeKit(identity, kit); }')
        else:
            mocks.append(declaration + ' override { unexpected("IAIPlayer::' + method + '"); }')
    if count != 35:
        raise RuntimeError(('Unexpected interface count', count))
    mocks.append('};')
    (out / 'squad_kit_mocks.inl').write_text('\n'.join(mocks), encoding='utf-8')


def run(exe):
    summary = []
    checks = 0
    for mode in MODES:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        match = re.fullmatch(r'PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
        if result.returncode != 0 or not match:
            raise RuntimeError((mode, result.returncode, output))
        checks += int(match.group(1))
        summary.append(output.strip())
        print(summary[-1])
    summary.append('PASS squad kit queries: ' + str(len(MODES)) + ' strict x86 cases / ' + str(checks) + ' checks')
    (exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: squad_kit_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
