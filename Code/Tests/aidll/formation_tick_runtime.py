"""[TEMP] Exact formation definitions; separately scoped leaf/scheduler observers."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess
import sys

CODE=Path(__file__).resolve().parents[2]
BASE=CODE/'BF2/AI/AIDLL/AI/AIBot'
_spec=importlib.util.spec_from_file_location('typed_declarations',Path(__file__).with_name('sai_states_runtime.py'))
helpers=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(helpers)
NORMAL=['unoccupied','zero_count_storage','leader_zero','hidden_nonzero','hidden_only',
        'hidden_last','hidden_holes','hidden_duplicate','match_bot','match_human','match_same',
        'match_none','match_skip','match_first','near','equal','far','nan','force_equal',
        'force_nan','invalid_position','empty_anchor','duplicate_leader',
        'range_x','range_y','range_z','range_equal','range_nan','range_sum','range_stack',
        'wait_zero','wait_unoccupied','wait_skip_zero','wait_occupied_stop',
        'scheduler_slot_stop','scheduler_speed_stop',
        'hidden_reset_stop','match_reset_stop','near_reset_stop']
# 2026-09-28: the scheduler is complete. The unoccupied tick now returns, so its old
# `scheduler_unoccupied_stop` is `scheduler_unoccupied`; the tail helpers get their own cases.
NORMAL+=['scheduler_unoccupied','cycle_plain','cycle_select','cycle_negative','cycle_wrap_line',
         'forward_unoccupied','forward_occupied_stop','cycle_wedge_stop','cycle_echelon_stop',
         'trail_unset_anchor','trail_nan_anchor','trail_seed','trail_unoccupied','trail_equal',
         'trail_far','trail_one_slot','slots_empty','slots_unclaimed','slots_sorted','slots_tie',
         'slots_zero_member','slots_single','slots_invalid_position','accessors','join_leader',
         'witness_trail_set','witness_crumb','witness_member_wide','witness_member_allspill']
OBSERVERS=['entry_capture','near_reload','near_coordinator','near_index','near_flag',
           'hidden_pointers','hidden_indices','match_live','match_retained','match_clear_order']
SCHEDULER=['scheduler_order','scheduler_select','scheduler_plain','scheduler_mutation']
STOPS={
 'wait_occupied_stop':'SquadFormation::updateWaitForSquadMembers (occupied slot timing)',
 'scheduler_slot_stop':'SquadFormation::updateSlotZeroPos (getSlotWorldPos)',
 'scheduler_speed_stop':'SquadFormation::updateFormationMaxSpeed - the per-member IPIMobile::getMaxSpeed lookup',
 'hidden_reset_stop':'SquadFormation::updateSlotZeroPos (getSlotWorldPos)',
 'match_reset_stop':'SquadFormation::updateSlotZeroPos (getSlotWorldPos)',
 'near_reset_stop':'SquadFormation::updateSlotZeroPos (getSlotWorldPos)',
 'forward_occupied_stop':'SquadFormation::updateFormationForwardVector (IAIObject information query)',
 'cycle_wedge_stop':'SquadFormation::setWedgeFormation',
 'cycle_echelon_stop':'SquadFormation::setEchelonFormation'}
LEAVES={'getSquadLeader','getSquadPosition','getSquadMember','squadHasPosition',
        'getMemberIndex','isBot','getPosition','setHavePath','resetTrailingPositions'}
SCHEDULE_HELPERS={'updateSlotZeroPos','updateMovementFlag','updateFormationMaxSpeed',
                  'updateFormation','updateWaitForSquadMembers','updateFormationForwardVector',
                  'updateTrailingPositions','updateTrailingSlots','selectFormation','setFormation'}
FREE={'formationLeaderWithinRange','trailPointIsSet','trailCrumbDistanceSq','trailMemberDistanceSq'}

def generate(out):
    out.mkdir(parents=True,exist_ok=True)
    specs={'SquadFormation':['formationLeaderWithinRange','trailPointIsSet','trailCrumbDistanceSq',
                            'trailMemberDistanceSq','update','updateFormation','updateWaitForSquadMembers',
                            'updateSlotZeroPos','updateMovementFlag','updateFormationMaxSpeed','resetTrailingPositions',
                            'updateFormationForwardVector','updateTrailingPositions','updateTrailingSlots',
                            'selectFormation','setFormation','setLineFormation','setWedgeFormation',
                            'setEchelonFormation','getSquadMemberIndexAtSlot','shouldLeaveSquadLeader','joinSquadLeader'],
           'SquadCoordinator':['getSquadLeader','getSquadPosition','getSquadMember','squadHasPosition'],
           'SquadMember':['getMemberIndex','isBot','getPosition','setHavePath','getPlayer']}
    records=[]
    for owner,names in specs.items():
        path=BASE/(owner+'.cpp');source=path.read_text(encoding='utf-8')
        for name in names:
            qualified=name if name in FREE else owner+'::'+name
            args='int index' if name=='getSquadMember' else r'[^\n]*'
            found=re.findall(r'^[^\n;{}]+ '+qualified+r'\('+args+r'\)\s*(?:\{[^{}]*\}|\{[\s\S]*?^})',source,re.M)
            if len(found)!=1:raise RuntimeError(('unique body',qualified,len(found)))
            records.append(dict(source=path.as_posix(),method=name,owner=owner,body=found[0],
                                source_sha256=hashlib.sha256(source.encode()).hexdigest(),body_sha256=hashlib.sha256(found[0].encode()).hexdigest()))
    # The attested anonymous-namespace sort key, copied verbatim as one block.
    source=(BASE/'SquadFormation.cpp').read_text(encoding='utf-8')
    anonymous=re.findall(r'^namespace \{$[\s\S]*?^\} // anonymous namespace$',source,re.M)
    if len(anonymous)!=1:raise RuntimeError(('unique anonymous namespace',len(anonymous)))
    records.insert(0,dict(source=(BASE/'SquadFormation.cpp').as_posix(),method='(anonymous namespace)',owner='SquadFormation',
                          body=anonymous[0],source_sha256=hashlib.sha256(source.encode()).hexdigest(),
                          body_sha256=hashlib.sha256(anonymous[0].encode()).hexdigest()))
    actual=['// [TEMP] Exact untouched production definitions, variant exclusions explicit.',
            '#include "'+Path(__file__).with_suffix('.h').as_posix()+'"',
            'namespace dice { namespace hfe { namespace ai {']
    for record in records:
        conditions=[]
        if record['method'] in LEAVES:conditions.append('!defined(LEAF_OBSERVERS)')
        if record['method'] in SCHEDULE_HELPERS:conditions.append('!defined(SCHEDULER_OBSERVERS)')
        if conditions:actual.append('#if '+' && '.join(conditions))
        actual.append(record['body'])
        if conditions:actual.append('#endif')
    actual+=['// [TEMP] Transparent entries to the exact internal numerical helpers.',
             'bool fixtureFormationRange(const Vec3& a,const Vec3& b) { return formationLeaderWithinRange(a,b); }',
             'bool fixtureTrailPointIsSet(const Vec3& v) { return trailPointIsSet(v); }',
             'float fixtureTrailCrumbDistanceSq(const Vec3& a,const Vec3& c) { return trailCrumbDistanceSq(a,c); }',
             'float fixtureTrailMemberDistanceSq(const Vec3& p,const Vec3& a) { return trailMemberDistanceSq(p,a); }',
             '} } }']
    (out/'actual.cpp').write_text('\n\n'.join(actual),encoding='utf-8')
    (out/'source.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
    mock=['// [TEMP] Typed player observer for the forward-vector stop; every other virtual terminates.',
          'namespace dice { namespace hfe { namespace ai {',
          'class ObservedPlayer : public IAIPlayer { public:']
    for name,decl in helpers.methods('IAIPlayer','AI/AIInterface/BFKnowledge/AIPlayer.h'):
        body='return observeVehicle(this);' if name=='getCurrentVehicle' else 'unexpected("IAIPlayer::'+name+'");'
        mock.append(decl+' override { '+body+' }')
    mock+=['};','} } }']
    (out/'mocks.inl').write_text('\n'.join(mock),encoding='utf-8')
    files=[Path(__file__).with_suffix(s) for s in ['.h','.cpp','.py','.cmd']]
    files.append(Path(__file__).with_name('sai_states_runtime.py'))
    (out/'fixture_sources.json').write_text(json.dumps({p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in files},indent=2),encoding='utf-8')
    headers=[BASE/(n+'.h') for n in ['SquadFormation','SquadCoordinator','SquadMember']]
    headers += [CODE/'BF2/AI/AIInterface/BFKnowledge/BFTimer.h',CODE/'BF2/Math/Vec3.h',
                CODE/'BF2/AI/AIInterface/BFKnowledge/AIPlayer.h']
    (out/'headers.json').write_text(json.dumps({p.as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in headers},indent=2),encoding='utf-8')

def run(out):
    results=[];checks=0;stops=0
    for exe,modes in [('formation_tick_runtime.exe',NORMAL),('leaf_observers.exe',OBSERVERS),('scheduler_observers.exe',SCHEDULER)]:
        for mode in modes:
            p=subprocess.run([str(out/exe),mode],capture_output=True,text=True,timeout=15)
            output=p.stdout+p.stderr;(out/(mode+'.log')).write_text(output,encoding='utf-8')
            if mode in STOPS:
                expected='VERIFIED '+mode+'\n[FRONTIER] dice::hfe::ai::'+STOPS[mode]+' reached\n'
                if p.returncode!=3 or output!=expected:raise RuntimeError((mode,p.returncode,output))
                result='PASS '+mode+' exact checked state/diagnostic/abort 3';stops+=1
            else:
                m=re.fullmatch('PASS '+mode+r' ([1-9][0-9]*) checks\n',output)
                if p.returncode!=0 or not m:raise RuntimeError((mode,p.returncode,output))
                checks+=int(m.group(1));result=output.strip()
            results.append(result);print(result)
    results.append(f'PASS formation tick: {len(results)} strict x86 cases, {checks} normal checks, {stops} exact abort traces')
    (out/'runtime.log').write_text('\n'.join(results)+'\n',encoding='utf-8');print(results[-1])

if __name__=='__main__':
    if len(sys.argv)!=3 or sys.argv[1] not in ['generate','run']:raise SystemExit('generate OUT | run OUT')
    out=Path(sys.argv[2]).resolve();generate(out) if sys.argv[1]=='generate' else run(out)
