"""[TEMP] Strict x86 fixture for AIPathfinding's seeded seven-argument polygon query.

The large AIPathfinding TU cannot be linked whole in isolation because its vtable retains unrelated
named-frontier subsystems.  As in map_polygon_runtime, this driver extracts the exact target and
the small production bodies it calls, hashes those bodies and their source, and links all lower
navmesh/material translation units whole.  No replacement success body is supplied.
"""

from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / 'BF2'
AI = 'AI/AIDLL/AI/'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def declarations(class_name: str, indent: str = ''):
    path = BF2 / AI / 'Main/AIPathfinding.h'
    source = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(encoding='utf-8'), flags=re.S)
    found = re.findall(r'^' + indent + r'class ' + class_name + r'[^\{]*\{(.*?)^'
                       + indent + r'};', source, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError(('class', class_name, len(found)))
    body = re.sub(r'^    class \w+[^\{]*\{.*?^    };', '', found[0], flags=re.M | re.S)
    for signature in re.findall(r'virtual\s+([^;{}]+);', body):
        signature = re.sub(r'\s*=\s*0$', '', ' '.join(signature.split()))
        method = re.search(r'(~?\w+)\s*\(', signature).group(1)
        if not method.startswith('~'):
            yield method, signature


def write_navmesh(out: Path, name: str, material: int) -> None:
    data = out / 'data' / 'AIPathFinding'
    data.mkdir(parents=True, exist_ok=True)
    # One leaf, one horizontal quad, one island. The format is the real v3 .qtr/v5 .clb surface.
    node = struct.pack('<4IBBHI', 0, 0, 0, 0, 1, 1, 0, 0)
    polygon = struct.pack('<BBHIfff', 4, material, 0, 0, 0.0, 1.0, 0.0)
    order = struct.pack('<I', 0)
    vertices = b''.join(struct.pack('<fff', *point) for point in
                        ((-3, 0, -3), (-3, 0, 3), (3, 0, 3), (3, 0, -3)))
    indices = struct.pack('<4I', 0, 1, 2, 3)
    neighbours = struct.pack('<4I', *([0xFFFFFFFF] * 4))
    arrays = ((node, 24), (polygon, 20), (order, 4), (vertices, 12),
              (indices, 4), (neighbours, 4))
    raw = struct.pack('<iifffff', 1, 3, -4.0, -4.0, 4.0, 4.0, 8.0)
    for value, stride in arrays:
        raw += struct.pack('<i', len(value) // stride) + value
    (data / (name + '.qtr')).write_bytes(raw)
    (data / (name + '.clb')).write_bytes(struct.pack('<5i', 5, 0, 0, 0, 0))


def exact_body(source: str, pattern: str, label: str) -> str:
    found = re.findall(pattern, source, re.M)
    if len(found) != 1:
        raise RuntimeError(('exact extraction', label, len(found)))
    return found[0]


def generate(out: Path) -> None:
    out.mkdir(parents=True, exist_ok=True)
    write_navmesh(out, 'walkable', 1)
    write_navmesh(out, 'blocked', 0)
    cases = ['bounds', 'hint_reuse', 'hint_fallback', 'material_choice',
             'null_material_hint', 'failure_outputs', 'alias_outputs', 'callback_order']
    (out / 'cases.json').write_text(json.dumps(cases, indent=2) + '\n', encoding='utf-8')
    (out / 'crash_cases.json').write_text(json.dumps(['crash_null_settings'], indent=2) + '\n',
                                          encoding='utf-8')

    source_path = BF2 / AI / 'Main/AIPathfinding.cpp'
    source = source_path.read_text(encoding='utf-8')
    patterns = [
        (r'^AIPathfinding::MapContainer::MapContainer\([^\n]+\)[\s\S]*?^}', 'MapContainer ctor'),
        (r'^AIPathfinding::MapContainer::~MapContainer\(\)\s*\{[\s\S]*?^}', 'MapContainer dtor'),
        (r'^bool AIPathfinding::MapContainer::getPolygon\(const Vec3&[^,]*,\s*unsigned int&[^,]*,'
         r'\s*unsigned int&[^,]*,\s*unsigned int[^,&]*,\s*unsigned int[^,&]*,'
         r'\s*MaterialManager\s*\*[^)]*\) const\s*\{[\s\S]*?^}', 'seeded MapContainer query'),
        (r'^int AIPathfinding::getNumberOfMaps\(\) const\s*\{[\s\S]*?^}', 'map count'),
        (r'^bool AIPathfinding::getPolygon\(const Vec3&[^,]*,\s*int[^,]*,'
         r'\s*unsigned int&[^,]*,\s*unsigned int&[^,]*,\s*unsigned int[^,&]*,'
         r'\s*unsigned int[^,&]*,\s*int[^)]*\) const\s*\{[\s\S]*?^}',
         'seeded outer query'),
    ]
    records = []
    bodies = []
    for pattern, label in patterns:
        body = exact_body(source, pattern, label)
        bodies.append(body)
        records.append({'label': label, 'source': source_path.as_posix(),
                        'source_sha256': sha(source_path),
                        'body_sha256': hashlib.sha256(body.encode()).hexdigest(), 'body': body})
    (out / 'actual.cpp').write_text(
        '// [TEMP] Exact production extracts; no behavior substitutions.\n'
        '#include "' + (Path(__file__).with_name('map_polygon_runtime.h')).as_posix() + '"\n'
        'namespace dice { namespace hfe { namespace ai {\n' + '\n\n'.join(bodies)
        + '\n} } }\n', encoding='utf-8')

    target_frontier = 'BF2_AI_FRONTIER' in bodies[-1]
    evidence = {'exact_bodies': records, 'fixtures': {}}
    fixture_paths = [Path(__file__).resolve(),
                     Path(__file__).with_name('seeded_polygon_runtime.cpp').resolve(),
                     Path(__file__).with_name('seeded_polygon_runtime.cmd').resolve(),
                     Path(__file__).with_name('map_polygon_runtime.h').resolve(),
                     Path(__file__).with_name('navmesh_loader_runtime.cpp').resolve()]
    for path in fixture_paths:
        evidence['fixtures'][path.as_posix()] = sha(path)
    (out / 'source.json').write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')

    target_signature = ('bool getPolygon(const Vec3&, int, unsigned int&, unsigned int&, '
                        'unsigned int, unsigned int, int) const')
    mocks = ['// [TEMP] Unexercised virtuals abort by their typed names.',
             'namespace dice { namespace hfe { namespace ai {']
    for owner, name, indent in (('AIPathfinding', 'AIPathfinding', ''),
                                ('AIPathfinding::MapContainer', 'MapContainer', '    ')):
        for method, signature in declarations(name, indent):
            if owner == 'AIPathfinding' and (method == 'getNumberOfMaps'
                                              or signature == target_signature):
                continue
            definition = re.sub(r'\b' + method + r'(?=\s*\()', owner + '::' + method,
                                signature, count=1)
            mocks.append(definition + ' { unexpected("unused ' + owner + '::' + method + '"); }')
    mocks.append('} } }')
    (out / 'unused.inl').write_text('\n'.join(mocks) + '\n', encoding='utf-8')

    settings_path = BF2 / AI / 'Main/AISettingsInterfaces.h'
    settings_source = re.sub(r'/\*.*?\*/|//[^\n]*', '',
                             settings_path.read_text(encoding='utf-8'), flags=re.S)
    settings_body = re.search(r'^class IAISettings\s*\{(.*?)^};', settings_source,
                              re.M | re.S).group(1)
    observer = [
        '// [TEMP] Typed settings observer. Every slot except material lookup aborts.',
        'class QuerySettings : public dice::hfe::ai::IAISettings { public:',
        '    mutable int calls = 0; mutable int equipment = -999;',
        '    MaterialManager* result = nullptr;',
        '    AIPathfinding::MapContainer* replacement = nullptr; AIPathfinding* path = nullptr;',
        '    MaterialManager* getVehicleMaterialManager(int id) const override;',
    ]
    for signature in re.findall(r'virtual\s+([^;{}]+);', settings_body):
        signature = re.sub(r'\s*=\s*0$', '', ' '.join(signature.split()))
        method = re.search(r'(~?\w+)\s*\(', signature).group(1)
        if method.startswith('~') or method == 'getVehicleMaterialManager':
            continue
        observer.append('    ' + signature + ' override { unexpected("unused settings '
                        + method + '"); }')
    observer.append('};')
    (out / 'settings.inl').write_text('\n'.join(observer) + '\n', encoding='utf-8')

    if target_frontier:
        state = ('Expected temporary runtime failure: the exact extracted outer seeded query still '
                 'contains its named production frontier.')
    else:
        state = 'The extracted production target contains no named frontier.'
    (out / 'README.md').write_text(f"""# Seeded polygon strict x86 validation

This generated fixture tests the seven-argument seeded `AIPathfinding::getPolygon` forwarder over
real serialized v3 navmeshes, the real `MapContainer`/`QuadTreeManager` query chain, and a typed
`IAISettings` observer. The five AIPathfinding bodies in `actual.cpp` are exact source extracts;
their source/body hashes are in `source.json`. Lower navmesh/material TUs link whole.

{state}

The final matrix covers signed/upper map bounds, settings and material-table selection (including
the valid-hint null-material path), known polygon/quad-tree hints, hint fallback, success/failure
output mutation, aliased outputs, the settings-before-map-receiver ordering, and the accepted-map
null-settings omission. `runtime.log` is written only after every case passes.
""", encoding='utf-8')


def run(exe: Path, out: Path) -> None:
    cases = json.loads((out / 'cases.json').read_text(encoding='utf-8'))
    summary = []
    checks = 0
    for mode in cases:
        result = subprocess.run([str(exe), mode, str(out)], capture_output=True,
                                text=True, timeout=30)
        output = result.stdout + result.stderr
        (out / (mode + '.log')).write_text(output, encoding='utf-8')
        match = re.search(r'^PASS ' + re.escape(mode) + r' ([1-9][0-9]*) checks\n\Z',
                          output, re.M)
        if (result.returncode != 0 or not match or 'FAIL' in output
                or 'UNEXPECTED' in output or '[FRONTIER]' in output):
            raise RuntimeError((mode, result.returncode, output))
        checks += int(match.group(1))
        summary.append(match.group(0).strip())
        print(summary[-1])
    crash_cases = json.loads((out / 'crash_cases.json').read_text(encoding='utf-8'))
    for mode in crash_cases:
        result = subprocess.run([str(exe), mode, str(out)], capture_output=True,
                                text=True, timeout=30)
        output = result.stdout + result.stderr
        (out / (mode + '.log')).write_text(output, encoding='utf-8')
        if (result.returncode == 0 or 'REACHED-AFTER' in output or '[FRONTIER]' in output
                or 'UNEXPECTED' in output or 'FAIL' in output):
            raise RuntimeError((mode, result.returncode, output))
        line = 'PASS ' + mode + ' preserved missing settings guard'
        summary.append(line)
        print(line)
    summary.append(f'PASS seeded polygon: {len(cases)} strict x86 cases / {checks} checks, '
                   f'{len(crash_cases)} guard crash')
    (out / 'runtime.log').write_text('\n'.join(summary) + '\n', encoding='utf-8')
    with (out / 'README.md').open('a', encoding='utf-8') as readme:
        readme.write('\nValidation complete: ' + summary[-1] + '.\n')
    print(summary[-1])


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == 'generate':
        generate(Path(sys.argv[2]).resolve())
    elif len(sys.argv) == 4 and sys.argv[1] == 'run':
        run(Path(sys.argv[2]).resolve(), Path(sys.argv[3]).resolve())
    else:
        raise SystemExit('generate OUT | run EXE OUT')
