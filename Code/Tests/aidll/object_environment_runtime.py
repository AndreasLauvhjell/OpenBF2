"""[TEMP] Exact BFObjectEnvironment bodies (2026-09-28) against typed observers from the real headers.

BFObjectEnvironment's constructor, destructor, getWorldPlayerOccupiedObjectsWithIds and
damageSeverelyDamagedVehicles, and PlayerManager::getPlayers(), are extracted verbatim into one
generated translation unit compiled against the real headers. Players, the objects they ride, object
templates, armor, armor templates, the timer, the game logic and the filter are typed observers; every
virtual nobody expects terminates by name. Chosen constructors skip all registry, template and world
work - they are fixture scaffolding, not recovery.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
BF2 = CODE / 'BF2'
ENV = 'AI/AIInterface/BFKnowledge/BFObjectEnvironment.cpp'

NORMAL = ['wo_empty', 'wo_filters', 'wo_predicate', 'wo_ladder', 'wo_ladder_empty', 'wo_ladder_no_ai',
          'wo_parachute', 'wo_zipline', 'wo_rope', 'wo_other', 'wo_container_predicate',
          'wo_reread_id', 'wo_reread_class', 'wo_live_list',
          'wo_parachute_no_occupant', 'wo_parachute_no_interface',
          'dv_fresh_ctor', 'dv_interval', 'dv_backwards', 'dv_nan', 'dv_skips', 'dv_parent_walk',
          'dv_damage_args', 'dv_zero_max', 'dv_live_map', 'dv_no_pco']


def declarations(owner, relative):
    """(name, declaration, pure) for each virtual declared WITHOUT a body in `class owner`."""
    text = (BF2 / relative).read_text(encoding='utf-8')
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
    found = re.findall(r'^class ' + owner + r'(?:\s*:\s*[^\{]+)?\s*\{(.*?)^};', text, re.M | re.S)
    if len(found) != 1:
        raise RuntimeError('Expected one class ' + owner + ' in ' + relative)
    body = re.sub(r'^[ \t]+(?:class|struct) \w+[^{;]*\{.*?^[ \t]+\};', '', found[0], flags=re.M | re.S)
    for original in re.findall(r'virtual\s+([^;{}]+);', body):
        pure = bool(re.search(r'=\s*0\s*$', original))
        declaration = re.sub(r'\s*=\s*0$', '', ' '.join(original.split()))
        name = re.search(r'(~?\w+)\s*\(', declaration).group(1)
        if not name.startswith('~'):
            yield name, declaration, pure


def out_of_line(owner, declaration, name):
    # Trailing return type: class-scope lookup then applies to nested return types (ComponentMap).
    at = re.search(r'\b' + name + r'\s*\(', declaration)
    returns = declaration[:at.start()].strip()
    rest = re.sub(r'\s*=\s*[^,()]+(?=[,)])', '', declaration[at.start() + len(name):])   # no defaults
    return 'auto ' + owner + '::' + name + rest + ' -> ' + returns


def extract(relative, expression):
    path = BF2 / relative
    source = path.read_text(encoding='utf-8')
    found = re.findall(expression, source, re.M | re.S)
    if len(found) != 1 or 'FRONTIER' in found[0]:
        raise RuntimeError(('one recovered production body', relative, expression, len(found)))
    return found[0], dict(source=path.as_posix(), source_sha256=hashlib.sha256(source.encode()).hexdigest(),
                          body=found[0], body_sha256=hashlib.sha256(found[0].encode()).hexdigest())


# Out-of-line definitions whose vtables the chosen lifetimes emit. Every one terminates by name.
ABORTING = [
    ('ai', 'BFObjectEnvironment', ENV.replace('.cpp', '.h'),
     {'getWorldPlayerOccupiedObjectsWithIds', 'damageSeverelyDamagedVehicles'}),
    ('world', 'PlayerManager', 'Scene/Object/PlayerManager.h', set()),
    ('world', 'IPlayer', 'Game/Objects/IPlayer.h', set()),
    ('world', 'IObject', 'Game/Objects/IObject.h', set()),
    ('world', 'Object', 'Game/Objects/Object.h', set()),
    ('world', 'SimpleObject', 'Game/Objects/Base/SimpleObject.h', set()),
    ('world', 'Bundle', 'Game/Objects/Base/Bundle.h', set()),
    ('world', 'PlayerControlObject', 'Game/Objects/Game/PlayerControlObject.h', set()),
    ('world', 'IPlayerControlObject', 'Scene/Object/ObjectTypes/Game/IPlayerControlObject.h', set()),
    ('world', 'IObjectTemplate', 'Game/Objects/IObjectTemplate.h', set()),
    ('world', 'ArmorGLCompTemplate', 'Game/Objects/GameLogicComp/ArmorGLCompTemplate.h', set()),
    ('world', 'IObjectComponent', 'Game/Objects/IObjectComponent.h', set()),
]
# PlayerManager::getPlayers() is the REAL body (extracted); its (int team) overload is not.
KEEP = {('PlayerManager', 'const std::list<IPlayer*>& getPlayers()')}

# Observers: every pure virtual overridden; the named ones forward to fixture hooks, the rest abort.
OBSERVED = [
    ('ai', 'ObservedTimer', [('IBFTimer', 'AI/AIInterface/BFKnowledge/BFTimer.h')],
     {'getGameTime': 'double getGameTime() const override { return timerGameTime(); }'}, ''),
    # IGameLogic declares its own IBase quartet (it has no base class), so it is listed alone.
    ('hfe', 'ObservedLogic', [('IGameLogic', 'Game/IGameLogic.h')],
     {'getPcoMap': 'std::map<unsigned int, world::IObject*>* getPcoMap() const override { return logicPcoMap(); }',
      'giveDamage': 'bool giveDamage(world::IObject* object, float damage, world::IPlayer* player, '
                    'std::list<WeakPtr<world::IPlayer> >* assists, world::IObject* source, int team, '
                    'world::IObject* weapon, world::IObject* vehicle, Vec3 position, Vec3 direction, '
                    'int material, bool a13, bool canTK, bool noFriendlyFireMod) override { return '
                    'logicGiveDamage(object, damage, player, assists, source, team, weapon, vehicle, '
                    'position, direction, material, a13, canTK, noFriendlyFireMod); }'}, ''),
    ('world', 'ObservedPlayer', [('IBase', 'Core/IBase.h'), ('IPlayer', 'Game/Objects/IPlayer.h')],
     {'getIsAlive': 'bool getIsAlive() const override { return playerAlive(identity); }',
      'getIsManDown': 'bool getIsManDown() override { return playerDown(identity); }',
      'getVehicle': 'IObject* getVehicle() const override { return playerVehicle(identity); }',
      'getId': 'int getId() const override { return playerId(identity); }'}, ''),
    ('world', 'ObservedTemplate', [('IBase', 'Core/IBase.h'), ('IObjectTemplate', 'Game/Objects/IObjectTemplate.h')],
     {'getClassID': 'unsigned int getClassID() const override { return templateClass(identity); }',
      'getComponentFromClassID': 'IObjectTemplateComponent* getComponentFromClassID(unsigned int classId) '
                                 'const override { return templateComponent(identity, classId); }'}, ''),
    ('world', 'ObservedArmor', [('IBase', 'Core/IBase.h'), ('IObjectComponent', 'Game/Objects/IObjectComponent.h'),
                                ('IArmorGLComp', 'Game/Objects/GameLogicComp/IArmorGLComp.h')],
     {'getHitPoints': 'float getHitPoints() override { return armorHitPoints(identity); }',
      'getMaxHitPoints': 'float getMaxHitPoints() override { return armorMaxHitPoints(identity); }'}, ''),
]


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    header = Path(__file__).with_suffix('.h').as_posix()
    records = []
    ai_bodies = []
    for expression in (r'^BFObjectEnvironment::BFObjectEnvironment\(\)\n\{.*?^}',
                       r'^BFObjectEnvironment::~BFObjectEnvironment\(\)\n\{.*?^}',
                       r'^void BFObjectEnvironment::getWorldPlayerOccupiedObjectsWithIds\([^\n]*\n\{.*?^}',
                       r'^void BFObjectEnvironment::damageSeverelyDamagedVehicles\(\)\n\{.*?^}'):
        body, record = extract(ENV, expression)
        ai_bodies.append(body)
        records.append(record)
    players, record = extract('Scene/Object/PlayerManager.cpp',
                              r'^const std::list<IPlayer\*>& PlayerManager::getPlayers\(\)\n\{.*?^}')
    records.append(record)
    # PlayerControlObject's SprintState member: its real constructor and the reset() it tail-calls.
    sprint = []
    for expression in (r'^SprintState::SprintState\(\)[^\n]*\n\{.*?^}',
                       r'^void SprintState::reset\(\)[^\n]*\n\{.*?^}'):
        body, record = extract('Game/Objects/Game/PlayerControlObject.cpp', expression)
        sprint.append(body)
        records.append(record)
    actual = ['// [TEMP] Exact untouched production definitions.', '#include "' + header + '"',
              'namespace dice { namespace hfe { namespace ai {'] + ai_bodies + \
             ['}', 'namespace world {', players, '}'] + sprint + ['} }']
    (out / 'actual.cpp').write_text('\n\n'.join(actual) + '\n', encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(records, indent=2), encoding='utf-8')

    lines = ['// [TEMP] Generated from the real headers. Unexpected calls terminate by name.']
    for space, owner, relative, skip in ABORTING:
        lines.append('namespace dice { namespace hfe { namespace ' + space + ' {')
        for name, declaration, pure in declarations(owner, relative):
            if pure or name in skip or (owner, declaration) in KEEP:
                continue
            lines.append(out_of_line(owner, declaration, name) + ' { unexpected("' + owner + '::' + name + '"); }')
        lines.append('} } }')
    for space, observer, bases, bodies, extra in OBSERVED:
        opening = 'namespace dice { namespace hfe {' + ('' if space == 'hfe' else ' namespace ' + space + ' {')
        closing = '} }' + ('' if space == 'hfe' else ' }')
        lines += [opening, 'class ' + observer + ' : public ' + bases[-1][0] + ' { public: int identity = 0; ' + extra]
        seen = set()
        for base, relative in bases:
            for name, declaration, pure in declarations(base, relative):
                if not pure:
                    continue
                if name in bodies:
                    if name not in seen:
                        lines.append('    ' + bodies[name])
                    seen.add(name)
                    continue
                lines.append('    ' + declaration + ' override { unexpected("' + observer + '::' + name + '"); }')
        missing = set(bodies) - seen
        if missing:
            raise RuntimeError(('observed methods are not pure in the bases', observer, sorted(missing)))
        lines += ['};', closing]
    (out / 'collaborators.inl').write_text('\n'.join(lines) + '\n', encoding='utf-8')

    fixture = [Path(__file__).with_suffix(s) for s in ('.h', '.cpp', '.py', '.cmd')]
    (out / 'fixture_sources.json').write_text(
        json.dumps({p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in fixture}, indent=2),
        encoding='utf-8')


def run(out):
    results, checks = [], 0
    for mode in NORMAL:
        proc = subprocess.run([str(out / 'object_environment_runtime.exe'), mode], capture_output=True,
                              text=True, timeout=15)
        output = proc.stdout + proc.stderr
        (out / (mode + '.log')).write_text(output, encoding='utf-8')
        match = re.fullmatch(r'PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
        if proc.returncode != 0 or not match:
            raise RuntimeError((mode, proc.returncode, output))
        checks += int(match.group(1))
        results.append(output.strip())
        print(results[-1])
    results.append('PASS BFObjectEnvironment bodies: ' + str(len(NORMAL)) + ' strict x86 cases, ' +
                   str(checks) + ' checks')
    (out / 'runtime.log').write_text('\n'.join(results) + '\n', encoding='utf-8')
    print(results[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: object_environment_runtime.py generate OUT | run OUT')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
