"""[TEMP] Chunk 03B phases B/C (2026-09-28): the AI template console surface, end to end.

The binding block of AI/AIInterface/Module/BFConsole.cpp (aiTemplate, aiTemplatePlugIn and kitTemplate)
is extracted VERBATIM into one generated translation unit and linked with the WHOLE, unchanged
console-registry, template, plug-in, parser and math translation units. Chosen collaborators only:
ClassManager publication, the console's working file, and typed observers for the DLL's IAISettings
and IAIPathfindingCourier, generated from the real headers - every slot the templates do not call
aborts by name.

The `stock` case replays every live aiTemplate / aiTemplatePlugIn line of the stock
Objects_server.zip (all 133 ai/Objects.ai, in archive order) through the real console registry.
The generator reproduces the script executor's comment handling - `rem` lines and
`beginrem`..`endrem` blocks are skipped, as OldConsole does - and derives every expected count from
the same filtered text, so nothing is hand-copied from a census.
"""
from pathlib import Path
import collections
import hashlib
import importlib.util
import json
import re
import subprocess
import sys
import zipfile

CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / 'BF2'
ARCHIVE = Path(r'D:\B2BF Launcher\Battlefield2\mods\bf2\Objects_server.zip')

MODES = ['metadata', 'gates', 'templates', 'plugins', 'controls', 'parsers', 'weapons', 'stock']
ABORTING_MODES = {
    'duplicate_template': "DEBUG-ERROR AI/AIInterface/BFKnowledge/AITemplateManager.cpp(38): Template 'TWICE' already exists",
    'duplicate_plugin': "DEBUG-ERROR AI/AIInterface/BFKnowledge/AITemplatePlugInManager.cpp(37): TemplatePlugIn 'twice' already exists"}

# (name, declaration, pure) for each virtual DECLARED in `class owner`.
def declarations(owner, relative):
    text = (BF2 / relative).read_text(encoding='utf-8')
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
    found = re.findall(r'^class ' + owner + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', text, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError('Expected one class ' + owner + ' in ' + relative)
    for original in re.findall(r'virtual\s+([^;{}]+);', found[0]):
        pure = bool(re.search(r'=\s*0\s*$', original))
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(original.split()))
        name = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not name.startswith('~'):
            yield name, declaration, pure


def observer(space, cls, base, relative, bodies):
    """A class overriding every pure virtual of `base`. `bodies` maps the header's declaration (as
    written there, parameter names and all) to the complete override line the fixture wants."""
    lines = ['namespace dice { namespace hfe { namespace ' + space + ' {',
             'class ' + cls + ' : public ' + base + ' { public:']
    used = set()
    for name, declaration, pure in declarations(base, relative):
        if not pure:
            continue
        if declaration in bodies:
            lines.append('    ' + bodies[declaration])
            used.add(declaration)
        else:
            lines.append('    ' + declaration + ' override { unexpected("' + cls + '::' + name + '"); }')
    missing = set(bodies) - used
    if missing:
        raise RuntimeError(('observer bodies name no pure virtual', cls, sorted(missing)))
    lines += ['};', '} } }']
    return lines


def stock_lines():
    """Every live aiTemplate/aiTemplatePlugIn command, in archive order, with its file."""
    commands = []
    with zipfile.ZipFile(ARCHIVE) as archive:
        for info in archive.infolist():
            if not info.filename.lower().endswith('.ai'):
                continue
            skipping = False
            for raw in archive.read(info).decode('latin-1').splitlines():
                line = raw.strip()
                low = line.lower()
                if low.startswith('beginrem'):
                    skipping = True
                    continue
                if low.startswith('endrem'):
                    skipping = False
                    continue
                if skipping or not line or low.startswith('rem'):
                    continue
                if re.match(r'(aitemplate|aitemplateplugin|weapontemplate)\.', low):
                    commands.append((info.filename, line))
    return commands


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    spec = importlib.util.spec_from_file_location('manager_fixture', Path(__file__).with_name('kit_manager_runtime.py'))
    manager = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(manager)
    manager.generate(out)  # kit_collaborators.inl: the typed aborting ClassManager/Console/OldConsole virtuals.

    source_path = BF2 / 'AI/AIInterface/Module/BFConsole.cpp'
    source = source_path.read_text(encoding='utf-8')
    start = source.index('enum CombatStrengthType : int;')
    end = source.index('// ds:0x8B2ECC0.', start)
    body = source[start:end].rstrip()
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
    (out / 'template_console_actual.cpp').write_text(actual, encoding='utf-8')

    # The Unit plug-in's two array sizes are the retail AISettings constants (6 battle, 2 strategic -
    # the DLL tree's AISettings returns the same); names become ids in first-seen order.
    lines = ['// [TEMP] Generated from the real DLL interface headers. Unexpected calls terminate by name.']
    lines += observer('ai', 'ObservedSettings', 'IAISettings', 'AI/AIDLL/AI/Main/AISettingsInterfaces.h', {
        'int getNBattleStrengths()': 'int getNBattleStrengths() override { ++settingsCalls; return 6; }',
        'int getNStrategicStrengths()': 'int getNStrategicStrengths() override { ++settingsCalls; return 2; }',
        'int getVehicle(const std::string&) const':
            'int getVehicle(const std::string& name) const override { return vehicleId(name); }',
        'const std::string& getVehicle(int) const':
            'const std::string& getVehicle(int id) const override { return vehicleName(id); }'})
    lines += observer('ai', 'ObservedCourier', 'IAIPathfindingCourier', 'AI/AIDLL/AI/Main/BFPathfindingCourier.h', {
        'int getVehicleNumberFromName(const std::string&) const':
            'int getVehicleNumberFromName(const std::string& name) const override { return mapId(name); }'})
    (out / 'template_observers.inl').write_text('\n'.join(lines) + '\n', encoding='utf-8')

    commands = stock_lines()
    templates = [l.split()[1] for f, l in commands if l.lower().startswith('aitemplate.create')]
    plugins = [(l.split()[1], l.split()[2]) for f, l in commands if l.lower().startswith('aitemplateplugin.create')]
    added = [l.split()[1] for f, l in commands if l.lower().startswith('aitemplate.addplugin')]
    weapons = [l.split()[1] for f, l in commands if l.lower().startswith('weapontemplate.create')]
    kinds = collections.Counter(kind for kind, name in plugins)
    if len(set(t.lower() for t in templates)) != len(templates) or len(set(n for k, n in plugins)) != len(plugins):
        raise RuntimeError('stock content now repeats a name - the duplicate path is fatal in retail')
    weapon_duplicates = len(weapons) - len(set(w.lower() for w in weapons))   # a message, not fatal
    wreck_owners = set()
    current = None
    for f, l in commands:
        parts = l.split()
        if parts[0].lower() == 'aitemplate.create':
            current = parts[1]
        elif parts[0].lower() == 'aitemplate.addtype' and len(parts) > 1 and parts[1] == 'ITHasWreck':
            wreck_owners.add(current)
    expected = {'templates': len(templates), 'plugins': len(plugins), 'addPlugIn': len(added),
                'armament': kinds['Armament'], 'controlInfo': kinds['ControlInfo'], 'cover': kinds['Cover'],
                'mobile': kinds['Mobile'], 'physical': kinds['Physical'], 'unit': kinds['Unit'],
                'wrecks': len(wreck_owners), 'commands': len(commands),
                'weapons': len(weapons) - weapon_duplicates, 'weaponDuplicates': weapon_duplicates}
    inl = ['static const char* const stockCommands[] = {']
    inl += ['    ' + json.dumps(l) + ',' for f, l in commands]
    inl += ['};', 'static const unsigned expectedCounts[] = {' + ', '.join(
        str(expected[k]) for k in ('templates', 'plugins', 'addPlugIn', 'armament', 'controlInfo', 'cover',
                                   'mobile', 'physical', 'unit', 'wrecks', 'commands', 'weapons',
                                   'weaponDuplicates')) + '};']
    (out / 'stock_template_commands.inl').write_text('\n'.join(inl) + '\n', encoding='utf-8')

    with zipfile.ZipFile(ARCHIVE) as archive:
        members = sorted(i.filename for i in archive.infolist() if i.filename.lower().endswith('.ai'))
    evidence = {'block': {'source': source_path.as_posix(),
                          'source_sha256': hashlib.sha256(source.encode()).hexdigest(),
                          'body_sha256': hashlib.sha256(body.encode()).hexdigest()},
                'archive': str(ARCHIVE), 'ai_files': len(members), 'expected': expected,
                'fixture': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                            for p in (Path(__file__).with_suffix(s) for s in ('.py', '.cpp', '.cmd'))}}
    (out / 'template_source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    print('generated:', json.dumps(expected))


def run(exe):
    total = 0
    for mode in MODES:
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=60)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        match = re.search(r'^PASS ' + mode + r' (\d+) checks$', output, re.M)
        if result.returncode != 0 or not match:
            raise RuntimeError((mode, result.returncode, output[-3000:]))
        total += int(match.group(1))
        print(output.strip())
    for mode, text in ABORTING_MODES.items():
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=60)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        # Retail's Debug(Error) on a duplicate name is FATAL: the message reaches the installed Error
        # callback (the real setDebugCallback), the callback returns, and ~Debug aborts.
        if result.returncode == 0 or 'PASS' in output or text not in output or 'REACHED-AFTER' in output:
            raise RuntimeError((mode, result.returncode, output[-3000:]))
        print('PASS ' + mode + ': fatal Debug(Error) "' + text + '", exit ' + str(result.returncode))
    print('PASS template console: %d strict x86 cases, %d checks, plus %d fatal-duplicate cases'
          % (len(MODES), total, len(ABORTING_MODES)))


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: template_console_runtime.py generate OUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
