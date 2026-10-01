"""[TEMP] Exact radius wrappers, grid selector and XZ helper; typed receiving observers."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
KNOWLEDGE = 'AI/AIDLL/AI/AIKnowledge/'


def methods(name, relative):
    source = (CODE/'BF2'/relative).read_text(encoding='utf-8')
    found = re.findall(r'^class '+name+r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', source, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError('Expected one real class: '+name)
    body = re.sub(r'//[^\n]*', '', found[0])
    for original in re.findall(r'virtual\s+([^;]+);', body):
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(original.split()))
        name = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not name.startswith('~'):
            yield name, declaration


def generate(out):
    evidence = []
    specs = [
        (KNOWLEDGE+'BFInformationEnvironment.cpp', r'^void BFInformationEnvironment::getObjectsWithinRadius\(std::vector<unsigned int>& out,\s*const Vec[23]&[\s\S]*?^}', 2),
        (KNOWLEDGE+'BFInformationEnvironment.cpp', r'^IAIInformationGrid\* BFInformationEnvironment::getInformationGrid\([^\n]*\) const\s*\{[\s\S]*?^}', 1),
        ('Math/Vec3.cpp', r'^void vec3ToVec2XZ\([^\n]*\)\s*\{[\s\S]*?^}', 1),
    ]
    for relative, pattern, count in specs:
        path = CODE / 'BF2' / relative
        source = path.read_text(encoding='utf-8')
        bodies = re.findall(pattern, source, re.M)
        if len(bodies) != count or any('BF2_AI_FRONTIER' in body for body in bodies):
            raise RuntimeError(('Expected recovered bodies', relative, len(bodies)))
        for body in bodies:
            evidence.append({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                             'body': body, 'body_sha256': hashlib.sha256(body.encode()).hexdigest()})
    out.mkdir(parents=True, exist_ok=True)
    actual = ['// [TEMP] Exact production bodies; no substitutions.',
              '#include "'+Path(__file__).with_suffix('.h').as_posix()+'"',
              'namespace dice { namespace hfe { namespace ai {']
    actual += [entry['body'] for entry in evidence[:3]]
    actual += ['} // ai', evidence[3]['body'], '} }']
    (out/'radius_wrappers_actual.cpp').write_text('\n\n'.join(actual), encoding='utf-8')
    (out/'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    generated = ['// [TEMP] Untested virtuals abort; actual radius wrappers are excluded.',
                 'namespace dice { namespace hfe { namespace ai {']
    for name, declaration in methods('BFInformationEnvironment', KNOWLEDGE+'BFInformationEnvironment.h'):
        if name == 'getObjectsWithinRadius' and ('const Vec2&' in declaration or 'const Vec3&' in declaration):
            continue
        signature = re.sub(r'\b'+name+r'(?=\s*\()', 'BFInformationEnvironment::'+name, declaration, count=1)
        generated.append(signature+' { unexpected("BFInformationEnvironment::'+name+'"); }')
    generated.append('class ObservedGrid : public IAIInformationGrid { public: int identity; explicit ObservedGrid(int id):identity(id) {}')
    for name, declaration in methods('IAIInformationGrid', KNOWLEDGE+'AIInformationGrid.h'):
        if name == 'getInformationWithinCircle':
            continue
        generated.append(declaration+' override { unexpected("IAIInformationGrid::'+name+'"); }')
    generated += ['void getInformationWithinCircle(std::vector<unsigned>& out,const Vec2& position,float radius,IAIInformationGridPredicate* keep) const override { observeGrid(this,identity,out,position,radius,keep); }',
                  '};', '} } }']
    (out/'radius_wrappers_mocks.inl').write_text('\n'.join(generated), encoding='utf-8')


MODES = ['vec3_delegate', 'vec3_nonfinite', 'world_a', 'world_b', 'side_zero',
         'reload_grid', 'borrowed_position', 'vec3_copy', 'null_predicate']


def run(exe):
    summary = []
    for mode in MODES:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout+result.stderr
        (exe.parent/(mode+'.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or output != 'PASS '+mode+'\n':
            raise RuntimeError((mode, result.returncode, output))
        summary.append(output.strip())
        print(summary[-1])
    summary.append('PASS radius wrappers: '+str(len(MODES))+' strict x86 cases')
    (exe.parent/'runtime.log').write_text('\n'.join(summary)+'\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: radius_wrappers_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
