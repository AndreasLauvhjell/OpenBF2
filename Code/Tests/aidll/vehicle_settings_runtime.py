"""[TEMP] Exact AISettings roster methods, whole MaterialManager/Debug/StringUtil, strict x86.

AISettings allocations deliberately survive to child process exit: its production destructor
remains a named frontier. No test replacement destructor or whole-settings teardown claim.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / 'BF2'
AI = 'AI/AIDLL/AI/'
METHODS = ('getNumberOfVehicles', 'getNumberOfVehicleGroups', 'getVehicle',
           'getVehicleClusterMapping', 'getClusterCostType', 'getClusterCostTypes',
           'getVehicleGroup', 'getVehicleGroupName', 'getVehicleMaterialManager',
           'getVehicleMaterialCost', 'isValidMaterial', 'setVehicle',
           'setVehicleClusterMapping', 'createVehicleGroup', 'addVehicleToVehicleGroup',
           'setVehicleMaterialCost', 'setManDownEquipment', 'getManDownEquipmentType',
           'setMaxNumberOfBehaviours', 'getNumberOfBehaviours')
MODES = ('registration', 'stock_roster', 'duplicates', 'clusters', 'groups',
         'group_target_capture', 'material_defaults', 'material_values',
         'material_destruction', 'material_dispatch', 'stock_costs', 'man_down', 'behaviour_count', 'material_parser')
STOPS = {'settings_destructor': '[FRONTIER] dice::hfe::ai::AISettings::~AISettings reached\n',
         'known_vehicle_group': 'DEBUG Vehicle of name Tank is unknown.\n',
         'negative_vehicle': 'DEBUG Error: Illegal vehicle id: -1\n',
         'past_vehicle': 'DEBUG Error: Illegal vehicle id: 1\n',
         'numeric_material': 'DEBUG 1 is not recognised as a proper materialId.\n',
         'empty_material': 'DEBUG  is not recognised as a proper materialId.\n'}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def declarations(path, name):
    source = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(encoding='utf-8'), flags=re.S)
    found = re.findall(r'^class ' + name + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', source, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError('Expected one class ' + name)
    for signature in re.findall(r'virtual\s+([^;{}]+);', found[0]):
        signature = ' '.join(signature.split())
        if re.search(r'=\s*0$', signature):
            continue
        method = re.search(r'(~?\w+)\s*\(', signature).group(1)
        if not method.startswith('~'):
            yield method, signature


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    # Read-only inputs. The fixture calls typed methods, not the script interpreter/bindings.
    stock = Path(r'D:\B2BF Launcher\Battlefield2\mods\bf2\AI\AIBehaviours.ai')
    commands = []
    for number, line in enumerate(stock.read_text(encoding='latin1').splitlines(), 1):
        tokens = line.split()
        if tokens and tokens[0].lower() in ('aisettings.setvehicle', 'aisettings.setvehicleclustermapping',
                                          'aisettings.setvehiclematerialcost'):
            commands.append({'line': number, 'tokens': tokens})
    vehicles = [c['tokens'][1] for c in commands if c['tokens'][0].lower() == 'aisettings.setvehicle']
    mappings = [c['tokens'][1:] for c in commands if c['tokens'][0].lower() == 'aisettings.setvehicleclustermapping']
    costs = [c['tokens'][1:] for c in commands if c['tokens'][0].lower() == 'aisettings.setvehiclematerialcost']
    if (len(vehicles), len(mappings), len(costs)) != (15, 6, 24):
        raise RuntimeError('Unexpected stock fixture inventory')
    # Numeric identities independently pinned by the complete PE/ELF Material.cpp parser audit.
    materials = {'Ground': 1, 'Road': 2, 'Shallows': 3, 'DeepWater': 4}
    stock_lines = ['// [TEMP] Selected stock script arguments, passed directly to typed methods.',
                   'static const char* const stockVehicles[] = {' + ','.join(json.dumps(v) for v in vehicles) + '};',
                   'struct StockMapping { const char* source; const char* target; };',
                   'static const StockMapping stockMappings[] = {']
    stock_lines += ['{' + ','.join(json.dumps(v) for v in values) + '},' for values in mappings]
    stock_lines += ['};', 'struct StockCost { const char* vehicle; unsigned char material; float cost; };',
                    'static const StockCost stockCosts[] = {']
    stock_lines += ['{' + json.dumps(vehicle) + ',' + str(materials[material]) + ',' + str(float(cost)) + 'f},'
                    for vehicle, material, cost in costs]
    stock_lines += ['};']
    (out / 'stock_data.inl').write_text('\n'.join(stock_lines), encoding='utf-8')
    (out / 'stock_input.json').write_text(json.dumps({'source': stock.as_posix(), 'sha256': sha(stock),
        'commands': commands, 'material_ids': materials, 'scope': 'direct method inputs; no console execution'}, indent=2))
    path = BF2 / AI / 'Main/AISettings.cpp'
    source = path.read_text(encoding='utf-8')
    patterns = [r'AISettings::AISettings\(\)\s*:[\s\S]*?^}',
                r'AISettings::~AISettings\(\)\s*\{[\s\S]*?^}']
    patterns += [r'IAISettings(?P<modifier>Modifier)?::(?:IAISettings(?:Modifier)?|~IAISettings(?:Modifier)?)\(\)\s*\{[\s\S]*?^}']
    evidence = []
    for pattern in patterns:
        matches = list(re.finditer('^' + pattern, source, re.M))
        expected = 4 if '?P<modifier>' in pattern else 1
        if len(matches) != expected:
            raise RuntimeError(('Lifecycle extraction count', pattern, len(matches)))
        for match in matches:
            body = match.group(0)
            evidence.append(dict(source=path.as_posix(), source_sha256=sha(path), body=body,
                                 body_sha256=hashlib.sha256(body.encode()).hexdigest()))
    for method in METHODS:
        matches = re.findall(r'^[^\n;{}]*\bAISettings::' + method + r'\([^{};]*?\)(?: const)?\s*\{[\s\S]*?^}', source, re.M)
        expected = 2 if method in ('getVehicle', 'getVehicleGroup', 'getVehicleMaterialCost', 'addVehicleToVehicleGroup') else 1
        if len(matches) != expected:
            raise RuntimeError(('Method extraction count', method, len(matches)))
        for body in matches:
            if 'BF2_AI_FRONTIER' in body:
                raise RuntimeError('Requested roster method is still a frontier: ' + method)
            evidence.append(dict(source=path.as_posix(), source_sha256=sha(path), body=body,
                                 body_sha256=hashlib.sha256(body.encode()).hexdigest()))
    bot_path = BF2 / AI / 'AIBot/BotMain.cpp'
    bot_source = bot_path.read_text(encoding='utf-8')
    for pattern in (r'unsigned char BotMain::mu8NBehaviours\s*=\s*[^;]+;',
                    r'void BotMain::setNBehaviours\([^\n]*\) \{[^\n]+\}',
                    r'unsigned char BotMain::getNBehaviours\(\) \{[^\n]+\}'):
        matches = re.findall('^' + pattern, bot_source, re.M)
        if len(matches) != 1:
            raise RuntimeError(('Bot static extraction count', pattern, len(matches)))
        body = matches[0]
        evidence.append(dict(source=bot_path.as_posix(), source_sha256=sha(bot_path), body=body,
                             body_sha256=hashlib.sha256(body.encode()).hexdigest()))
    (out / 'actual.cpp').write_text('// [TEMP] Verbatim production methods; no statement substitutions.\n#include "'
        + Path(__file__).with_suffix('.h').as_posix() + '"\nnamespace dice { namespace hfe { namespace ai {\n'
        + '\n\n'.join(item['body'] for item in evidence) + '\n} } }\n', encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
    lines = ['// [TEMP] Untested actual-header virtuals fail explicitly.', 'namespace dice { namespace hfe {']
    for name, relative, excluded in [('ClassManager', 'Core/ClassManager.h', {'registerSimpleSingleton', 'unregisterSimpleSingleton'}),
                                     ('AISettings', AI + 'Main/AISettings.h', set(METHODS))]:
        if name == 'AISettings':
            lines.append('namespace ai {')
        for method, declaration in declarations(BF2 / relative, name):
            if method in excluded:
                continue
            declaration = re.sub(r'\b' + method + r'(?=\s*\()', name + '::' + method, declaration, count=1)
            lines.append(declaration + ' { unexpected("' + name + '::' + method + '"); }')
    lines.append('} } }')
    (out / 'unused.inl').write_text('\n'.join(lines), encoding='utf-8')
    whole = [AI + 'AIPathFinding/Material/MaterialManager.cpp', AI + 'AIPathFinding/Material/MaterialManager.h',
             AI + 'AIPathFinding/Material/Material.cpp', AI + 'AIPathFinding/Material/Material.h',
             'Core/Debug.cpp', 'Core/StringUtil.cpp']
    context = [AI + 'Main/AISettings.h', AI + 'Main/AISettingsInterfaces.h', AI + 'Main/CommonSingletonServer.h',
               AI + 'AIBot/BotMain.h']
    (out / 'whole_sources.json').write_text(json.dumps({p: sha(BF2 / p) for p in whole}, indent=2))
    (out / 'header_context.json').write_text(json.dumps({p: sha(BF2 / p) for p in context}, indent=2))
    (out / 'fixture_sources.json').write_text(json.dumps({Path(__file__).with_suffix(s).name:
        sha(Path(__file__).with_suffix(s)) for s in ('.h', '.cpp', '.py', '.cmd')}, indent=2))
    (out / 'cases.json').write_text(json.dumps({'normal': MODES, 'stops': STOPS}, indent=2))


def run(exe):
    summary = []
    checks = 0
    for mode in (*MODES, *STOPS):
        result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=15)
        output = result.stdout + result.stderr
        (exe.parent / (mode + '.log')).write_text(output, encoding='utf-8')
        if mode in STOPS:
            if result.returncode != 3 or output != STOPS[mode]:
                raise RuntimeError((mode, result.returncode, output))
            line = 'PASS ' + mode + ' exact stop'
        else:
            match = re.fullmatch(r'PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
            if result.returncode != 0 or not match:
                raise RuntimeError((mode, result.returncode, output))
            checks += int(match.group(1))
            line = output.strip()
        summary.append(line)
        print(line)
    summary.append(f'PASS vehicle settings: {len(MODES) + len(STOPS)} strict x86 cases / {checks} checks / {len(STOPS)} exact stops')
    (exe.parent / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('generate OUT | run EXE')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
