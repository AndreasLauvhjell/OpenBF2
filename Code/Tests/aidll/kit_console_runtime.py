"""[TEMP] Exact binding/parser block, whole real registry/base/manager/template TUs."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess
import sys
import zipfile

CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / 'BF2'

def generate(out):
    spec = importlib.util.spec_from_file_location('manager_fixture', Path(__file__).with_name('kit_manager_runtime.py'))
    manager = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(manager)
    manager.generate(out)  # Typed unused collaborator definitions only.
    source_path = BF2 / 'AI/AIInterface/Module/BFConsole.cpp'
    source = source_path.read_text(encoding='utf-8')
    start = source.index('enum CombatStrengthType : int;')
    end = source.index('// ds:0x8B2ECC0.', start)
    body = source[start:end].rstrip()
    # 2026-09-28: the same block now also carries the aiTemplate and aiTemplatePlugIn bindings
    # (chunk 03B phases B/C), so it needs their headers; template_console_runtime tests those rows.
    includes = ['Core/ConsoleObjectMacro.h', 'Core/StringUtil.h', 'Math/Vec3.h', 'IO/Input/Input.h',
                'AI/AIInterface/BFKnowledge/KitTemplate.h', 'AI/AIInterface/BFKnowledge/KitTemplateManager.h',
                'AI/AIInterface/BFKnowledge/AITemplate.h', 'AI/AIInterface/BFKnowledge/AITemplateManager.h',
                'AI/AIInterface/BFKnowledge/AITemplatePlugInManager.h',
                'AI/AIInterface/BFKnowledge/AITemplateArmament.h', 'AI/AIInterface/BFKnowledge/AITemplateControlInfo.h',
                'AI/AIInterface/BFKnowledge/AITemplateCover.h', 'AI/AIInterface/BFKnowledge/AITemplateMobile.h',
                'AI/AIInterface/BFKnowledge/AITemplatePhysical.h', 'AI/AIInterface/BFKnowledge/AITemplateUnit.h',
                'AI/AIInterface/BFKnowledge/InformationType.h', 'AI/AIInterface/BFKnowledge/SoldierPose.h',
                'AI/AIInterface/BFKnowledge/WeaponTemplate.h', 'AI/AIInterface/BFKnowledge/WeaponTemplateManager.h']
    actual = '\n'.join('#include "' + (BF2 / p).as_posix() + '"' for p in includes)
    actual += '\nnamespace dice { namespace hfe { namespace ai {\n' + body + '\n} } }\n'
    (out / 'kit_console_actual.cpp').write_text(actual, encoding='utf-8')
    evidence = json.loads((out / 'source.json').read_text())
    evidence.append({'source': source_path.as_posix(), 'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                     'body_sha256': hashlib.sha256(body.encode()).hexdigest(), 'body': body})
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    archive = Path(r'D:\B2BF Launcher\Battlefield2\mods\bf2\Objects_server.zip')
    member = 'Kits/ai/Objects.ai'
    with zipfile.ZipFile(archive) as zip_file:
        raw = zip_file.read(member)
    lines = [line.strip() for line in raw.decode('latin1').splitlines()
             if line.strip().lower().startswith('kittemplate.')]
    if len(lines) != 63:
        raise RuntimeError('Unexpected stock kit command count')
    (out / 'stock_commands.inl').write_text('static const char* stockCommands[] = {\n' +
        '\n'.join(json.dumps(line) + ',' for line in lines) + '\n};\n', encoding='utf-8')
    (out / 'stock_source.json').write_text(json.dumps({'archive': str(archive), 'member': member,
        'member_sha256': hashlib.sha256(raw).hexdigest(), 'commands': lines}, indent=2), encoding='utf-8')

def run(exe):
    modes = ['metadata', 'availability', 'arity', 'creation', 'strengths', 'kit_parser', 'combat_parser', 'stock']
    for mode in modes:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=20)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if result.returncode != 0 or not re.fullmatch('PASS ' + mode + r' \d+ checks\n', output):
            raise RuntimeError((mode, result.returncode, output))
        print(output, end='')
    print('PASS kit console: 8 strict x86 cases')

if __name__ == '__main__':
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
