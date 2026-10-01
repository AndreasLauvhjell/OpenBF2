// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: CONTROL-MAP-COPY-FIXTURE]
// Actual io::ControlMap record classes, assignment, reset and destruction. The add* bodies are
// still stubs, so records are inserted through the fixture friend; every body exercised is the
// recovered one and nothing ControlMap owns is shadowed.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../BF2/IO/Input/ControlMap.h"
using namespace dice::hfe;
using namespace dice::hfe::io;
static unsigned checks, failures;
static void check(bool value,const char* label)
{ ++checks; if(!value){++failures;std::printf("FAIL %s\n",label);} }

namespace dice { namespace hfe { namespace io {
struct ControlMapFixture {
    static std::map<int,AxisRemap*>& axes(ControlMap& m){ return m.m_axisMappings; }
    static std::map<int,TriggerRemap*>& triggers(ControlMap& m){ return m.m_triggerMappings; }
    static std::vector<TriggerMapping*>& unassigned(ControlMap& m){ return m.m_unassignedTriggerMappings; }
    static float riseTime(ControlMap& m){ return m.m_buttonRiseTime; }
    static float fallTime(ControlMap& m){ return m.m_buttonFallTime; }
    static float doubleTapTime(ControlMap& m){ return m.m_doubleTapTime; }
};
} } }

static void fillAxis(AxisMapping& m,int seed)
{
    m.device=seed; m.type=seed+1; m.field08=seed+2; m.field0C=seed+3; m.field10=seed+4; m.field14=seed+5;
    m.field18=seed+6; m.field1C=seed+7; m.pov=seed+8; m.flag24=static_cast<unsigned char>(seed+9);
    m.field28=seed+10; m.field2C=seed+11; m.field30=seed+12; m.field34=seed+13; m.field38=seed+14;
    m.field3C=seed+15; m.field40=seed+16; m.value48=seed+0.5; m.invert=static_cast<unsigned char>(seed&1);
}
static bool sameAxis(const AxisMapping& a,const AxisMapping& b)
{
    return a.device==b.device && a.type==b.type && a.field08==b.field08 && a.field0C==b.field0C && a.field10==b.field10
        && a.field14==b.field14 && a.field18==b.field18 && a.field1C==b.field1C && a.pov==b.pov && a.flag24==b.flag24
        && a.field28==b.field28 && a.field2C==b.field2C && a.field30==b.field30 && a.field34==b.field34 && a.field38==b.field38
        && a.field3C==b.field3C && a.field40==b.field40 && a.value48==b.value48 && a.invert==b.invert;
}
static void fillTrigger(TriggerMapping& m,int seed)
{
    m.device=seed; m.type=seed+1; m.button=seed+2; m.key=seed+3; m.axisOptions=seed+4; m.field14=seed+5;
    m.flag18=static_cast<unsigned char>(seed+6); m.value20=seed+0.25; m.float28=seed+0.75f;
}
static bool sameTrigger(const TriggerMapping& a,const TriggerMapping& b)
{
    return a.device==b.device && a.type==b.type && a.button==b.button && a.key==b.key && a.axisOptions==b.axisOptions
        && a.field14==b.field14 && a.flag18==b.flag18 && a.value20==b.value20 && a.float28==b.float28;
}
static bool zeroAxis(const AxisMapping& m)
{
    AxisMapping z; std::memset(&z,0,sizeof z);
    return sameAxis(m,z);
}

int main()
{
    check(sizeof(void*)==4,"x86 fixture");
    check(sizeof(AxisMapping)==88 && sizeof(TriggerMapping)==48 && sizeof(AxisRemap)==184 && sizeof(TriggerRemap)==104,
        "record sizes match the PE allocations (B8/68 remaps, 58/30 mappings)");
    // the record constructors and resets
    {
        AxisMapping a; check(zeroAxis(a),"AxisMapping constructs zeroed through reset()");
        fillAxis(a,3); a.reset(); check(zeroAxis(a),"AxisMapping::reset zeroes every field");
        TriggerMapping t;
        check(t.device==0 && t.type==0 && t.button==0 && t.key==0 && t.axisOptions==0 && t.field14==0 && t.flag18==0 && t.value20==0.0 && t.float28==10000.0f,
            "TriggerMapping constructs zeroed with the 10000.0f float at +28");
        fillTrigger(t,5); t.reset(); check(t.float28==10000.0f && t.device==0 && t.value20==0.0,"TriggerMapping::reset restores the 10000.0f float");
        AxisMapping b; fillAxis(b,9); a=b; check(sameAxis(a,b),"AxisMapping assignment copies every field");
        TriggerMapping u; fillTrigger(u,2); t=u; check(sameTrigger(t,u),"TriggerMapping assignment copies every field");
    }
    // a populated map and its copy
    ControlMap source; source.setName(std::string("live")); source.setID(7);
    AxisRemap* axis=new AxisRemap; fillAxis(axis->primary,10); fillAxis(axis->alternate,20); axis->isAnalogue=1; axis->value=0.625f;
    ControlMapFixture::axes(source)[3]=axis;
    TriggerRemap* trigger=new TriggerRemap; fillTrigger(trigger->primary,30); fillTrigger(trigger->alternate,40); trigger->state=2;
    ControlMapFixture::triggers(source)[8]=trigger;
    TriggerMapping* loose=new TriggerMapping; fillTrigger(*loose,50);
    ControlMapFixture::unassigned(source).push_back(loose);
    source.setButtonRiseTime(0.011f); source.setButtonFallTime(0.012f); source.setDoubleTapTime(0.3f);
    source.setInvertMouse(true); source.setMouseSensitivity(2.5f); source.setKeyboardSensitivity(0.4f);
    source.setYawFactor(1.5f); source.setPitchFactor(0.75f);
    for(int i=0;i<IDA_COUNT;++i) source.setAxisScale(static_cast<InputDeviceAxes>(i),0.1f*(i+1));
    check(source.getAxisValue(3)==0.625f && source.isAxisValueAnalogue(3) && source.getTriggerState(8)==2,"the three accessor hit paths read the records");
    check(source.getAxisValue(4)==0.0f && !source.isAxisValueAnalogue(4) && source.getTriggerState(9)==0,"the miss paths stay");

    ControlMap copy; copy.setName(std::string("_live")); copy.setID(99);
    AxisRemap* stale=new AxisRemap; ControlMapFixture::axes(copy)[1]=stale;   // reset() must free this one
    copy=source;
    check(ControlMapFixture::axes(copy).size()==1 && ControlMapFixture::axes(copy).count(3)==1,"the axis map is reset, then copied by key");
    AxisRemap* copiedAxis=ControlMapFixture::axes(copy)[3];
    check(copiedAxis!=axis && sameAxis(copiedAxis->primary,axis->primary) && sameAxis(copiedAxis->alternate,axis->alternate)
        && copiedAxis->isAnalogue==1 && copiedAxis->value==0.625f,"axis remaps are deep copies: both mappings, the flag and the value");
    check(ControlMapFixture::triggers(copy).size()==1 && ControlMapFixture::triggers(copy)[8]!=trigger
        && sameTrigger(ControlMapFixture::triggers(copy)[8]->primary,trigger->primary)
        && sameTrigger(ControlMapFixture::triggers(copy)[8]->alternate,trigger->alternate)
        && ControlMapFixture::triggers(copy)[8]->state==2,"trigger remaps are deep copies: both mappings and the state byte");
    check(ControlMapFixture::unassigned(copy).size()==1 && ControlMapFixture::unassigned(copy)[0]!=loose
        && sameTrigger(*ControlMapFixture::unassigned(copy)[0],*loose),"unassigned trigger mappings are cloned");
    check(ControlMapFixture::riseTime(copy)==0.011f && ControlMapFixture::fallTime(copy)==0.012f && ControlMapFixture::doubleTapTime(copy)==0.3f,
        "the three times are copied");
    check(copy.getInvertMouse() && copy.getMouseSensitivity()==2.5f && copy.getKeyboardSensitivity()==0.4f
        && copy.getYawFactor()==1.5f && copy.getPitchFactor()==0.75f,"the feel scalars are copied");
    bool scales=true; for(int i=0;i<IDA_COUNT;++i) scales=scales && copy.getAxisScale(static_cast<InputDeviceAxes>(i))==0.1f*(i+1);
    check(scales,"the four axis scales are copied");
    check(copy.getName()=="_live" && copy.getID()==99,"name and id are not copied");
    check(copy.getAxisValue(3)==0.625f && copy.isAxisValueAnalogue(3) && copy.getTriggerState(8)==2,"the copy answers through its own records");
    copy=source;
    check(ControlMapFixture::axes(copy).size()==1 && ControlMapFixture::triggers(copy).size()==1 && ControlMapFixture::unassigned(copy).size()==1,
        "a second assignment resets first, so nothing accumulates");
    // reset
    source.reset();
    check(ControlMapFixture::axes(source).empty() && ControlMapFixture::triggers(source).empty() && ControlMapFixture::unassigned(source).empty(),
        "reset frees and empties both maps and the vector");
    bool restored=true; for(int i=0;i<IDA_COUNT;++i) restored=restored && source.getAxisScale(static_cast<InputDeviceAxes>(i))==0.03f;
    check(restored && !source.getInvertMouse() && source.getMouseSensitivity()==1.0f && source.getKeyboardSensitivity()==0.1f
        && source.getYawFactor()==1.0f && source.getPitchFactor()==1.0f,"reset restores the six feel scalars");
    check(ControlMapFixture::riseTime(source)==0.011f && ControlMapFixture::fallTime(source)==0.012f && ControlMapFixture::doubleTapTime(source)==0.3f,
        "reset leaves the three times alone");
    check(source.getName()=="live" && source.getID()==7,"reset leaves name and id alone");
    std::printf("ControlMap copy: %u/%u passed; ControlMap=%zu AxisMapping=%zu TriggerMapping=%zu AxisRemap=%zu TriggerRemap=%zu\n",
        checks-failures,checks,sizeof(ControlMap),sizeof(AxisMapping),sizeof(TriggerMapping),sizeof(AxisRemap),sizeof(TriggerRemap));
    return failures?1:0;
}
