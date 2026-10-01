"""[TEMP] Exact coordinator-tail bodies (2026-09-28) with typed observers at every collaborator."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess
import sys

CODE = Path(__file__).resolve().parents[2]
AI = 'AI/AIDLL/AI/'
spec = importlib.util.spec_from_file_location('typed_methods', Path(__file__).with_name('spawn_metadata_runtime.py'))
helpers = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helpers)

NORMAL = ['position_anchor', 'position_leader', 'position_first_alive', 'position_nobody',
          'position_invalid_copy', 'shots_empty', 'shots_invalid', 'shots_expired', 'shots_equal_time',
          'shots_moved', 'shots_edge', 'shots_mixed', 'repair_idle', 'repair_expired', 'repair_equal',
          'repair_nan', 'orders_not_commander', 'orders_taking_flag', 'orders_order_state',
          'orders_null_leader', 'orders_human_leader', 'orders_moving', 'orders_dead', 'orders_man_down',
          'orders_far_hostile', 'orders_far_unassigned', 'orders_waiting', 'orders_invalid_sentinel',
          'incoming_bot_leader', 'incoming_null_leader', 'incoming_not_leaving', 'incoming_no_bot',
          'incoming_empty', 'incoming_other_sender', 'taking_no_area', 'taking_owned',
          'taking_no_control_point', 'taking_inside', 'taking_edge', 'taking_excluded',
          'control_point_query', 'witness_moved_wide', 'witness_moved_naive', 'witness_goto_wide',
          'witness_inside_wide', 'witness_inside_naive']
STOPS = {'orders_send_stop': 'SquadCoordinator::updateNeedOrdersMessage (CommunicationAgent::enqueueRadioMessage)',
         'orders_owned_area_stop': 'SquadCoordinator::updateNeedOrdersMessage (CommunicationAgent::enqueueRadioMessage)',
         'incoming_rejoin_stop': 'SquadCoordinator::updateIncomingMessages (CommunicationAgent::enqueueRadioMessage)'}
FREE = {'shooterMovedAway', 'leaderGotoDistanceSq', 'insideControlPointRadius'}


def extract(relative, owner, names):
    path = CODE / 'BF2' / (AI + relative)
    source = path.read_text(encoding='utf-8')
    records = []
    for name in names:
        qualified = name if name in FREE else owner + '::' + name
        # File-local helpers are anchored on `static` so a call site cannot match.
        prefix = r'^static [^\n;{}]+ ' if name in FREE else r'^[^\n;{}]+ '
        found = re.findall(prefix + qualified + r'\([^\n]*\)\s*(?:\{[^{}\n]*\}|\{[\s\S]*?^})', source, re.M)
        if len(found) != 1:
            raise RuntimeError(('unique body', qualified, len(found)))
        records.append(dict(source=path.as_posix(), owner=owner, method=name, body=found[0],
                            source_sha256=hashlib.sha256(source.encode()).hexdigest(),
                            body_sha256=hashlib.sha256(found[0].encode()).hexdigest()))
    return records


def generate(out):
    out.mkdir(parents=True, exist_ok=True)
    records = []
    records += extract('AIBot/SquadCoordinator.cpp', 'SquadCoordinator',
                       ['shooterMovedAway', 'leaderGotoDistanceSq', 'insideControlPointRadius',
                        'updateSquadPosition', 'updateShotsFired', 'updateRepairRequest',
                        'updateNeedOrdersMessage', 'updateIncomingMessages', 'isSquadTakingFlag',
                        'getSquadLeader', 'resetNeedOrderMessageTimer', 'updateSquadPaths'])
    records += extract('AIBot/SquadFormation.cpp', 'SquadFormation',
                       ['getSquadMemberIndexAtSlot', 'shouldLeaveSquadLeader', 'joinSquadLeader', 'updateMovementFlag'])
    records += extract('AIBot/SquadMember.cpp', 'SquadMember',
                       ['getPlayer', 'getPosition', 'isBot', 'getBot', 'isMoving', 'getPlayerId', 'setHavePath'])
    records += extract('AIStrategicEntity/AIStrategicArea.cpp', 'AIStrategicArea', ['getControlPointPosAndRadius'])
    records += extract('AIStrategic/SAI.cpp', 'SAI', ['isPlayerCommander'])
    actual = ['// [TEMP] Exact untouched production definitions.',
              '#include "' + Path(__file__).with_suffix('.h').as_posix() + '"',
              'namespace dice { namespace hfe { namespace ai {']
    actual += [r['body'] for r in records]
    actual += ['// [TEMP] Transparent entries to the exact internal numerical helpers.',
               'bool fixtureShooterMovedAway(const Vec3& f,const Vec3& p) { return shooterMovedAway(f,p); }',
               'float fixtureLeaderGotoDistanceSq(const Vec3& p,const Vec3& g) { return leaderGotoDistanceSq(p,g); }',
               'bool fixtureInsideControlPointRadius(const Vec3& c,const Vec3& p,float r) { return insideControlPointRadius(c,p,r); }',
               '} } }']
    (out / 'actual.cpp').write_text('\n\n'.join(actual), encoding='utf-8')
    (out / 'source.json').write_text(json.dumps(records, indent=2), encoding='utf-8')

    tested = {(r['owner'], r['method']) for r in records}
    mocks = ['// [TEMP] Only named collaborators observe; every other virtual terminates.',
             'namespace dice { namespace hfe { namespace ai {']
    # Out-of-line virtual definitions the chosen lifetimes need, all terminating.
    for base, relative in [('SAI', 'AIStrategic/SAI.h'), ('AIStrategicObject', 'AIStrategicEntity/AIStrategicObject.h'),
                           ('AIStrategicArea', 'AIStrategicEntity/AIStrategicArea.h'), ('Bot', 'AIBot/BotMain.h')]:
        for name, decl, pure in helpers.methods(base, AI + relative):
            if pure or (base, name) in tested:
                continue
            signature = re.sub(r'\b' + name + r'(?=\s*\()', base + '::' + name, decl, count=1)
            mocks.append(signature + ' { unexpected("' + base + '::' + name + '"); }')
    observed = [
        ('IAIMain', AI + 'Main/AIMain.h', 'ObservedMain', {'getSAI': 'return observeSai(side);'}),
        ('IAIPlayer', 'AI/AIInterface/BFKnowledge/AIPlayer.h', 'ObservedPlayer',
         {'getIsAlive': 'return observeAlive(identity);', 'isManDown': 'return observeDown(identity);'}),
        ('Bot', AI + 'AIBot/BotMain.h', 'ObservedBot', {'getCommunicationAgent': 'return observeRadio(identity);'}),
        ('IBFSquad', 'AI/AIInterface/BFKnowledge/BFSquad.h', 'ObservedSquads',
         {'requestNewCommanderOrder': 'return observeRequest(squad, player);'}),
        ('IAIGame', 'AI/AIInterface/BFKnowledge/BFGame.h', 'ObservedGame',
         {'getControlPointInfo': 'return observeControlPoint(index, position, radius, active);'})]
    for base, header, owner, bodies in observed:
        mocks.append('class ' + owner + ' : public ' + base + ' { public: int identity = 0;')
        for name, decl, pure in helpers.methods(base, header):
            if not pure:
                continue
            mocks.append(decl + ' override { ' + bodies.get(name, 'unexpected("' + base + '::' + name + '");') + ' }')
        mocks.append('};')
    mocks.append('} } }')
    (out / 'mocks.inl').write_text('\n'.join(mocks), encoding='utf-8')
    paths = [Path(__file__).with_suffix(s) for s in ('.h', '.cpp', '.py', '.cmd')]
    paths.append(Path(__file__).with_name('spawn_metadata_runtime.py'))
    (out / 'fixture_sources.json').write_text(json.dumps({p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}, indent=2), encoding='utf-8')


def run(out):
    results, checks, stops = [], 0, 0
    for mode in NORMAL + list(STOPS):
        proc = subprocess.run([str(out / 'coordinator_tail_runtime.exe'), mode], capture_output=True, text=True, timeout=15)
        output = proc.stdout + proc.stderr
        (out / (mode + '.log')).write_text(output, encoding='utf-8')
        if mode in STOPS:
            expected = 'VERIFIED ' + mode + '\n[FRONTIER] dice::hfe::ai::' + STOPS[mode] + ' reached\n'
            if proc.returncode != 3 or output != expected:
                raise RuntimeError((mode, proc.returncode, output))
            result = 'PASS ' + mode + ' exact checked state/diagnostic/abort 3'
            stops += 1
        else:
            match = re.fullmatch(r'PASS ' + mode + r' ([1-9][0-9]*) checks\n', output)
            if proc.returncode != 0 or not match:
                raise RuntimeError((mode, proc.returncode, output))
            checks += int(match.group(1))
            result = output.strip()
        results.append(result)
        print(result)
    results.append('PASS coordinator tail: ' + str(len(results)) + ' strict x86 cases, ' + str(checks) +
                   ' normal checks, ' + str(stops) + ' exact abort traces')
    (out / 'runtime.log').write_text('\n'.join(results) + '\n', encoding='utf-8')
    print(results[-1])


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[1] not in ('generate', 'run'):
        raise SystemExit('Usage: coordinator_tail_runtime.py generate OUT | run OUT')
    target = Path(sys.argv[2]).resolve()
    generate(target) if sys.argv[1] == 'generate' else run(target)
