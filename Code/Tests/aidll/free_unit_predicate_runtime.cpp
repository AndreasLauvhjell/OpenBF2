// [TEMP]
// Tests the real predicate and pool with chosen Information/IPIUnit lifetimes and typed occupancy observers.
#include "free_unit_predicate_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>
#include <string>
#include <windows.h>
#include <crtdbg.h>
using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*)==4,"MSVC x86 fixture");
[[noreturn]] static void unexpected(const char* text)
{ std::fprintf(stderr,"UNEXPECTED %s\n",text); std::fflush(stderr); std::abort(); }
static void require(bool condition,const char* text) { if(!condition) unexpected(text); }
static std::string mode,calls;
static void trace(const std::string& event) { if(!calls.empty()) calls+=','; calls+=event; }
static std::string n(int value) { return std::to_string(value); }
static unsigned rawFloat(float value) { unsigned bits; std::memcpy(&bits,&value,4); return bits; }
static float fromBits(unsigned bits) { float value; std::memcpy(&value,&bits,4); return value; }
struct Record {
    Information* info=nullptr; IPIUnit* unit=nullptr; IPIUnit** unitSlot=nullptr;
    void** mobileSlot=nullptr; unsigned* flags=nullptr; float* strength=nullptr;
    bool occupied=false;
};
static Record records[4];
static int constructing,secondaryCalls,*predicateSide;
static float* predicateStrength;
static unsigned children[4],childCount;
static InformationManager* alternateManager;
static int mobileToken;
static int identity(const Information* info)
{ for(int i=0;i<4;++i) if(records[i].info==info) return i; unexpected("Information identity"); }

#include "predicate_observer.inl"
namespace dice { namespace hfe { namespace ai {
// [TEMP] Typed members only. The preserved partial Information prefix is native
// storage, not a claim that its PE object layout has been fully recovered.
Information::Information(IAIObject* object,double time):m_objectTypeFlags(0),m_strength(0),
    m_time(time),m_object(object),m_mobile(reinterpret_cast<IPIMobile*>(&mobileToken)),m_physical(nullptr),m_unit(nullptr)
{
    Record& r=records[constructing]; r.info=this; r.unitSlot=&m_unit;
    r.mobileSlot=reinterpret_cast<void**>(&m_mobile); r.flags=&m_objectTypeFlags; r.strength=&m_strength;
}
Information::~Information() {}
int Information::getInformationSort() { unexpected("Information::getInformationSort"); }
int Information::getFirstSecondaryObjectIterator() const
{
    require(identity(this)==0,"iterator starts on original root"); trace("first0");
    if(mode=="first_unit_reload") *records[0].unitSlot=records[1].unit;
    return 41;
}
unsigned int Information::getSecondaryObject(int& iterator,int side)
{
    require(identity(this)==0,"secondary iteration receiver stays original root");
    require(iterator==41+secondaryCalls,"single iterator passed by reference");
    require(side==*predicateSide,"side is read again for each callback");
    trace("secondary0:"+n(side)); ++iterator;
    const unsigned index=static_cast<unsigned>(secondaryCalls++);
    if(mode=="manager_swap" && index==0) informationManager=alternateManager;
    if(mode=="zero_manager") informationManager=nullptr;
    if(mode=="side_accumulator") { *predicateSide=9; *predicateStrength=200; }
    return index<childCount ? children[index] : 0;
}

IPIUnit::IPIUnit(Information* info):m_battleStrengths(nullptr),m_strategicStrengths(nullptr),
    m_strengthMod(0),m_ammo(0),m_ammoTime(0),m_ownsPointer24(false),m_pointer24(nullptr)
{ m_information=info; records[constructing].unit=this; }
IPIUnit::~IPIUnit() {}
int IPIUnit::getType() { unexpected("IPIUnit::getType"); }
void IPIUnit::setTime() { unexpected("IPIUnit::setTime"); }
void IPIUnit::setTime(double) { unexpected("IPIUnit::setTime(double)"); }
bool IPIUnit::isOccupied()
{
    int id=-1; for(int i=0;i<4;++i) if(records[i].unit==this) id=i;
    require(id>=0,"real unit identity"); trace("occupied"+n(id));
    if(mode=="value_reload") *records[0].strength=7;
    if(mode=="first_unit_reload") *records[0].strength=9;
    if(mode=="side_accumulator") { *predicateSide=4; *predicateStrength=100; }
    return records[id].occupied;
}
} } }

static void expectTrace(const std::string& expected)
{
    if(calls!=expected) { std::fprintf(stderr,"FAIL trace\nexpected: %s\nactual:   %s\n",expected.c_str(),calls.c_str()); std::exit(2); }
}
int main(int argc,char** argv)
{
    require(argc==2,"mode"); mode=argv[1];
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    InformationManager first,second; alternateManager=&second; informationManager=&first;
    ObservedInformation* infos[4]; IPIUnit* units[4];
    for(int i=0;i<4;++i) {
        constructing=i; infos[i]=new ObservedInformation; units[i]=new IPIUnit(infos[i]);
        *records[i].unitSlot=units[i]; *records[i].strength=static_cast<float>(i+2);
    }
    unsigned handles[4];
    for(int i=0;i<4;++i) handles[i]=first.getPool()->acquire(infos[i]);
    require(second.getPool()->acquire(infos[0])==handles[0],"matching first pool slot");
    require(second.getPool()->acquire(infos[2])==handles[1],"matching handle resolves distinct payload after callback");
    IAIInformationGridPredicate* predicate=makeFixturePredicate(2,predicateSide,predicateStrength);
    require(*predicateStrength==0 && *predicateSide==2,"actual predicate constructor");
    *predicateStrength=5;
    bool want=true; float wanted=7; std::string expected="first0,occupied0,secondary0:2";
    if(mode=="unit_null" || mode=="mobile_null" || mode=="flag80" || mode=="flag200000") {
        want=false; wanted=fromBits(0x7fc54321u); *predicateStrength=wanted; expected="";
        if(mode=="unit_null") *records[0].unitSlot=nullptr;
        if(mode=="mobile_null") *records[0].mobileSlot=nullptr;
        if(mode=="flag80") *records[0].flags=0x80;
        if(mode=="flag200000") *records[0].flags=0x200000;
    } else if(mode=="all_occupied") {
        records[0].occupied=true; want=false; wanted=fromBits(0x80000000u); *predicateStrength=wanted;
    } else if(mode=="secondary_rootguards") {
        records[0].occupied=true; records[2].occupied=true;
        children[0]=handles[1]; children[1]=handles[2]; children[2]=handles[3]; childCount=3;
        for(int i=1;i<4;++i) { *records[i].mobileSlot=nullptr; *records[i].flags=0x200080; }
        wanted=13; expected+=",occupied1,secondary0:2,occupied2,secondary0:2,occupied3,secondary0:2";
    } else if(mode=="float_steps") {
        *predicateStrength=8; *records[0].strength=16777216; *records[1].strength=1; *records[2].strength=-16777216;
        children[0]=handles[1]; children[1]=handles[2]; childCount=2;
        wanted=8; expected+=",occupied1,secondary0:2,occupied2,secondary0:2";
    } else if(mode=="value_reload") wanted=12;
    else if(mode=="first_unit_reload") { wanted=14; expected="first0,occupied1,secondary0:2"; }
    else if(mode=="manager_swap") {
        children[0]=handles[1]; childCount=1; wanted=11;
        expected+=",occupied2,secondary0:2";
    } else if(mode=="side_accumulator") { wanted=202; expected="first0,occupied0,secondary0:4"; }
    else if(mode=="zero_manager") { children[0]=0xffff0000u; childCount=1; }
    else if(mode=="nan_strength") { *records[0].strength=std::numeric_limits<float>::quiet_NaN(); wanted=*records[0].strength; }
    else if(mode=="zero_commit") { *records[0].strength=0; *predicateStrength=-0.0f; wanted=0.0f; }
    else if(mode=="invalid_handle") {
        children[0]=handles[1]^0x10000u; children[1]=handles[2]; childCount=2;
    } else require(mode=="root_free","known mode");
    const bool result=predicate->includeInformation(infos[0]);
    require(mode!="first_stop","actual forwarding stop returned");
    require(result==want,"predicate bool means at least one unoccupied unit");
    if(mode=="nan_strength") require(std::isnan(*predicateStrength),"NaN value accumulated");
    else require(rawFloat(*predicateStrength)==rawFloat(wanted),"final accumulator exact bits");
    expectTrace(expected);
    if(mode=="invalid_handle") require(secondaryCalls==1,"invalid lookup terminates rather than skip to next handle");
    destroyFixturePredicate(predicate);
    for(int i=0;i<4;++i) { delete units[i]; delete infos[i]; }
    informationManager=nullptr;
    std::printf("PASS %s\n",mode.c_str()); return 0;
}
