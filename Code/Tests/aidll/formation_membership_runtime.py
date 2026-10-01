"""[TEMP] Verbatim formation removal and real bounded dependencies, strict native x86."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
AI = 'BF2/AI/AIDLL/AI/'
spec = importlib.util.spec_from_file_location('decls', Path(__file__).with_name('sai_states_runtime.py'))
decls = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decls)
MODES = ['membership', 'reset', 'not_found', 'last', 'middle', 'next_empty', 'duplicate',
         'only_zero', 'zero_two', 'nearest_filters', 'nearest_default', 'nearest_tie',
         'nearest_nan', 'nearest_infinity', 'dx_spill', 'dy_wide', 'before_float_store',
         'minimum_underflow', 'timer_mutation', 'trailing_stop']


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    methods = {'SquadFormation': ['nearerFormationMember', 'isInFormation', 'resetSlot', 'leaveFormation',
                                  'resetTrailingPositions', 'updateSlotZeroPos', 'updateMovementFlag'],
               'SquadCoordinator': ['getSquadMember', 'getSquadGotoPos', 'squadHasPosition'],
               'SquadMember': ['getPosition', 'isSquadLeader', 'isBot', 'setHavePath']}
    evidence = []
    for owner, names in methods.items():
        path = CODE / (AI + 'AIBot/' + owner + '.cpp')
        source = path.read_text(encoding='utf-8')
        for name in names:
            qualified = name if name == 'nearerFormationMember' else owner + '::' + name
            arguments = r'int index' if name == 'getSquadMember' else r'[^\n]*'
            pattern = r'^[^\n;{}]+ ' + qualified + r'\(' + arguments + r'\)\s*(?:\{[^{}]*\}|\{[\s\S]*?^})'
            found = re.findall(pattern, source, re.M)
            if len(found) != 1:
                raise RuntimeError('Expected one exact body: ' + qualified)
            evidence.append(dict(source=path.as_posix(), method=qualified, body=found[0],
                                 source_sha256=hashlib.sha256(source.encode()).hexdigest(),
                                 body_sha256=hashlib.sha256(found[0].encode()).hexdigest()))
    (out / 'actual.cpp').write_text('// [TEMP] Exact complete production definitions.\n#include "'
        + Path(__file__).with_suffix('.h').as_posix() + '"\nnamespace dice { namespace hfe { namespace ai {\n'
        + '\n\n'.join(e['body'] for e in evidence) + '\n} } }\n', encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    lines = ['// [TEMP] Typed timer observer; unused virtuals abort.', 'class ObservedTimer : public IBFTimer { public:']
    for method, declaration in decls.methods('IBFTimer', 'AI/AIInterface/BFKnowledge/BFTimer.h'):
        if method != 'getGameTime':
            lines.append(declaration + ' override { unexpected("IBFTimer::' + method + '"); }')
    lines += ['double getGameTime() const override { return observeTime(); }', '};']
    (out / 'mocks.inl').write_text('\n'.join(lines), encoding='utf-8')
    paths = [Path(__file__).with_suffix(s) for s in ('.cpp', '.h', '.py', '.cmd')]
    paths.append(Path(__file__).with_name('sai_states_runtime.py'))
    (out / 'fixture_sources.json').write_text(json.dumps({p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}, indent=2), encoding='utf-8')


def run(out):
    summary = []
    checks = 0
    for mode in MODES:
        proc = subprocess.run([str(out / 'formation_membership_runtime.exe'), mode], capture_output=True, text=True, timeout=15)
        output = proc.stdout + proc.stderr
        (out / (mode + '.log')).write_text(output, encoding='utf-8')
        if mode == 'trailing_stop':
            expected = '[FRONTIER] dice::hfe::ai::SquadFormation::updateSlotZeroPos (getSlotWorldPos) reached\n'
            if proc.returncode != 3 or output != expected:
                raise RuntimeError((mode, proc.returncode, output))
            result = 'PASS trailing_stop exact diagnostic/abort 3'
        else:
            match = re.fullmatch('PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
            if proc.returncode != 0 or not match:
                raise RuntimeError((mode, proc.returncode, output))
            checks += int(match.group(1)); result = output.strip()
        summary.append(result); print(result)
    summary.append('PASS formation membership: ' + str(len(MODES)) + ' strict x86 cases / ' + str(checks) + ' checks / 1 actual frontier')
    (out / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('generate OUT | run OUT')
    out = Path(sys.argv[2]).resolve()
    generate(out) if sys.argv[1] == 'generate' else run(out)
