"""[TEMP] Exact wrappers/selector, whole real grid/pool, chosen native collaborators."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
AI = 'AI/AIDLL/AI/AIKnowledge/'


def methods(name, relative):
    text = (CODE / 'BF2' / relative).read_text(encoding='utf-8')
    classes = re.findall(r'^class ' + name + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', text, re.M | re.S)
    if len(classes) != 1:
        raise RuntimeError('Expected one class ' + name)
    body = re.sub(r'//[^\n]*', '', classes[0])
    for declaration in re.findall(r'virtual\s+([^;]+);', body):
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(declaration.split()))
        method = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not method.startswith('~'):
            yield method, declaration


def generate(out):
    path = CODE / 'BF2' / AI / 'BFInformationEnvironment.cpp'
    source = path.read_text(encoding='utf-8')
    bodies = re.findall(r'^(?:void|IAIInformationGrid\*) BFInformationEnvironment::(?:getAllInformation|getInformationGrid)\([\s\S]*?^}', source, re.M)
    if len(bodies) != 3 or any('BF2_AI_FRONTIER' in body for body in bodies):
        raise RuntimeError('Expected both recovered wrappers and selector')
    out.mkdir(parents=True, exist_ok=True)
    header = Path(__file__).with_suffix('.h').as_posix()
    (out / 'query_actual.cpp').write_text('// [TEMP] Exact production bodies, no substitution.\n#include "' + header + '"\nnamespace dice { namespace hfe { namespace ai {\n' + '\n'.join(bodies) + '\n} } }\n', encoding='utf-8')
    (out / 'query_source.json').write_text(json.dumps({'source': path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(), 'bodies': [{'body': body, 'sha256': hashlib.sha256(body.encode()).hexdigest()} for body in bodies]}, indent=2), encoding='utf-8')
    generated = ['// [TEMP] Unused virtuals abort; tested wrappers are not defined here.', 'namespace dice { namespace hfe { namespace ai {']
    for name, declaration in methods('BFInformationEnvironment', AI + 'BFInformationEnvironment.h'):
        if name == 'getAllInformation':
            continue
        signature = re.sub(r'\b' + name + r'(?=\s*\()', 'BFInformationEnvironment::' + name, declaration, count=1)
        generated.append(signature + ' { unexpected("BFInformationEnvironment::' + name + '"); }')
    generated.append('class ObservedInformation : public Information { public: int identity; Vec3 position; ObservedInformation(int id) : Information(nullptr,0.0), identity(id), position() {}')
    for name, declaration in methods('Information', AI + 'Information.h'):
        body = 'return position;' if name == 'getPosition' else 'unexpected("Information::' + name + '");'
        generated.append(declaration + ' override { ' + body + ' }')
    generated.extend(['};', '} } }'])
    (out / 'query_mocks.inl').write_text('\n'.join(generated), encoding='utf-8')


def run(exe):
    modes = ['empty', 'selection_append', 'filter_cull', 'null_fast_path', 'pe_slot_sweep', 'pe_slot_const']
    for mode in modes:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or 'PASS ' + mode + '\n' not in output or 'UNEXPECTED' in output or '[FRONTIER]' in output:
            raise RuntimeError((mode, result.returncode, output))
        print('PASS', mode)
    print('PASS information query: 6 strict x86 cases, actual grid/pool and exact wrappers')


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: information_query_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
