"""[TEMP] Whole production manager/template TUs; typed aborting collaborators only."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / 'BF2'

def class_body(path, name):
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(encoding='utf-8'), flags=re.S)
    match = re.search(r'^class ' + name + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', text, re.M | re.S)
    if not match:
        raise RuntimeError('Missing class ' + name)
    return match.group(1)

def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    lines = ['// [TEMP] Untested actual-header collaborator virtuals abort. No tested class definitions.',
             'namespace dice { namespace hfe {']
    specs = [('ClassManager', 'Core/ClassManager.h', {'registerSimpleSingleton', 'unregisterSimpleSingleton'}),
             ('Console', 'IO/Console/Console.h', set()),
             ('OldConsole', 'IO/Console/Console.h', {'getWorkingFile'})]
    for name, relative, excluded in specs:
        if name == 'Console':
            lines.append('namespace io {')
        body = class_body(BF2 / relative, name)
        # Every OldConsole override is explicit. Console/ClassManager ordinary virtuals
        # carry the virtual keyword; in-class definitions are excluded by braces.
        pattern = r'([^;{}]+\boverride)\s*;' if name == 'OldConsole' else r'virtual\s+([^;{}]+);'
        for declaration in re.findall(pattern, body):
            if re.search(r'=\s*0\s*$', declaration):
                continue
            declaration = re.sub(r'\b(?:virtual|override)\b', '', declaration)
            declaration = ' '.join(declaration.split())
            method = re.search(r'(~?\w+)\s*\(', declaration).group(1)
            if method.startswith('~') or method in excluded:
                continue
            declaration = re.sub(r'\b' + method + r'(?=\s*\()', name + '::' + method, declaration, count=1)
            lines.append(declaration + ' { unexpected("' + name + '::' + method + '"); }')
    lines += ['} } }']
    (out / 'kit_collaborators.inl').write_text('\n'.join(lines), encoding='utf-8')
    files = ['AI/AIInterface/BFKnowledge/KitTemplateManager.h', 'AI/AIInterface/BFKnowledge/KitTemplateManager.cpp',
             'AI/AIInterface/BFKnowledge/KitTemplate.h', 'AI/AIInterface/BFKnowledge/KitTemplate.cpp']
    (out / 'source.json').write_text(json.dumps([{'source': (BF2 / p).as_posix(),
        'sha256': hashlib.sha256((BF2 / p).read_bytes()).hexdigest(), 'compilation': 'whole unchanged file'}
        for p in files], indent=2), encoding='utf-8')

def run(exe):
    for mode in ['lifecycle', 'population', 'metadata', 'mutation', 'reset']:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=20)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or not re.fullmatch(r'PASS ' + mode + r' \d+ checks\n', output):
            raise RuntimeError((mode, result.returncode, output))
        print(output, end='')
    print('PASS kit manager: 5 strict x86 cases')

if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: kit_manager_runtime.py generate OUTPUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
