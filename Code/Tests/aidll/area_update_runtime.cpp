// [TEMP] Exact area caller/real numerical helpers; all external observations are typed.
#include "area_update_runtime.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <windows.h>
#include <crtdbg.h>
using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "MSVC x86 only");
[[noreturn]] static void unexpected(const char* s) { std::fprintf(stderr,"UNEXPECTED %s\n",s); std::fflush(stderr); std::abort(); }
static void require(bool v,const char* s) { if(!v) unexpected(s); }
static std::string mode,calls;
static void trace(const std::string& s) { if(!calls.empty()) calls+=','; calls+=s; }
static bool same(float a,float b) { return std::isnan(b)?std::isnan(a):std::memcmp(&a,&b,4)==0; }
static void checkFloat(float a,float b,const char* s) { if(!same(a,b)) { std::fprintf(stderr,"FLOAT %s got %.9g expected %.9g\n",s,a,b); unexpected(s); } }
static AIStrategicArea* area;
static int ownSide=1,queryCalls,ownerValue=1,constructing,sideReads[6];
static float counterValue=1.0f;
static Vec2 wantedUpper{8.0f,12.0f};
static IAIGame* alternateGame;
static IAIInformationEnvironment* alternateEnvironment;
static InformationManager* alternateManager;
static std::vector<unsigned>* queryVector;
static void* queryAllocation;
static bool queryFreed;
static int* friendlyMask;
static int* enemyMask;
void* operator new(std::size_t n) { if(void* p=std::malloc(n?n:1)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { if(p && p==queryAllocation) { queryFreed=true; queryAllocation=nullptr; } std::free(p); }
void operator delete(void* p,std::size_t) noexcept { ::operator delete(p); }
struct Record {
    Information* information=nullptr; IPIUnit* unit=nullptr; IPIPhysical* physical=nullptr;
    IPIUnit** unitSlot=nullptr; IPIPhysical** physicalSlot=nullptr; float* strengthSlot=nullptr;
    unsigned* flagSlot=nullptr; float** battleSlot=nullptr;
    float security=1.0f, strength=2.0f; float battle[6]={1,2,3,4,5,6}; float strategic[2]={2,4};
    bool occupied=true; int side=1,category=2; unsigned children[2]={}; int childCount=0;
};
static Record records[6];
static unsigned handles[6], roots[4][3]; static int rootCounts[4];
static int recordOf(const Information* p) { for(int i=0;i<6;++i) if(records[i].information==p) return i; unexpected("record identity"); }
static float security(int id) {
    trace("sec"+std::to_string(id));
    if(mode=="unit_reload" && id==0) *records[0].unitSlot=records[1].unit;
    if(mode=="live_end" && id==0) queryVector->push_back(handles[1]);
    return records[id].security;
}
static int informationSide(int id) {
    trace("side"+std::to_string(id));
    if(mode=="repeated_side" && id==0) return sideReads[id]++==0?0:2;
    return records[id].side;
}
static int category(int id) {
    trace("type"+std::to_string(id));
    if(mode=="battle_pointer_capture" && id==0) *records[0].battleSlot=records[1].battle;
    return records[id].category;
}
static bool relation(int side,bool friendly) { trace(std::string(friendly?"friend":"enemy")+std::to_string(side)); return friendly?side==ownSide:(side!=0 && side!=ownSide); }
static int owner(int id,unsigned cp) {
    trace("owner"+std::to_string(id)); require(id==0 && cp==73,"cached game/current control point");
    require(queryFreed,"query storage freed before owner callback");
    if(mode=="owner_changes_id") area->m_controlPointId=unsigned(-1);
    return ownerValue;
}
static float takeover() { trace("counter"); require(!area->m_enemyTakeover[ownSide-1],"takeover flag cleared before counter"); return counterValue; }
static bool objectFlag(AIStrategicObject::StrategicObjectType type) {
    trace("flag"); require(type==AIStrategicObject::ControlPoint,"actual control point flag forwarded");
    if(mode=="counter_fresh") { CommonSingletonServer::m_iaiGame=alternateGame; area->m_controlPointId=0xfedcba98u; }
    return mode!="counter_non_control";
}
static float gameCounter(int id,unsigned cp) {
    trace("gameCounter"+std::to_string(id));
    require(id==(mode=="counter_fresh"?1:0) && cp==(mode=="counter_fresh"?0xfedcba98u:73u),"post-flag game/id read");
    return -0.375f;
}
static void query(int id,std::vector<unsigned>& out,const Vec2& first,const Vec2& last,int side,bool world,IAIInformationGridPredicate* predicate) {
    trace("q"+std::to_string(queryCalls)); require(id==0 && queryCalls<4,"same captured environment four times");
    require(side==(queryCalls<2?0:ownSide) && world==(queryCalls%2==0) && !predicate,"ordered side/world/predicate");
    require(&first==&area->m_centre,"borrowed centre reference");
    checkFloat(last.x,wantedUpper.x,"upper x"); checkFloat(last.y,wantedUpper.y,"upper y");
    require(area->m_resourceNeed[ownSide-1]==0 && area->m_assignedSquadStrength[ownSide-1]==17,"counts reset and squad result stored before query");
    if(queryCalls==0) { require(out.empty(),"new query vector"); out.reserve(12); queryVector=&out; queryAllocation=out.data(); }
    else require(&out==queryVector,"shared query vector");
    if(mode=="queries") require(out.size()==static_cast<unsigned>(queryCalls),"all queries append without clear");
    for(int i=0;i<rootCounts[queryCalls];++i) out.push_back(roots[queryCalls][i]);
    if(mode=="cached_dependencies" && queryCalls==0) { area->m_centre={99,101}; AISingletonServer::m_iaiInformationEnvironment=alternateEnvironment; }
    if(mode=="cached_dependencies" && queryCalls>0) require(first.x==99 && first.y==101,"borrowed centre mutation");
    ++queryCalls;
}
#include "mocks.inl"
namespace dice { namespace hfe { namespace ai {
// [CHOSEN] These constructors initialize genuine native objects, not retail lifetimes.
ISAI::~ISAI() {}
SAI::SAI(int side,int,int,Vec2,Vec2):m_sideMask(1<<side),m_enemyMask(side==1?4:2) { friendlyMask=&m_sideMask; enemyMask=&m_enemyMask; }
SAI::~SAI() {}
IAIGame::IAIGame() {} IAIGame::~IAIGame() {}
IAIInformationEnvironment::IAIInformationEnvironment() {} IAIInformationEnvironment::~IAIInformationEnvironment() {}
void* CommonSingletonServer::m_iaiGame=nullptr;
void* AISingletonServer::m_iaiInformationEnvironment=nullptr;
AIStrategicObject::AIStrategicObject(float,float) {
    for(int i=0;i<4;++i) m_statistics[i]=new EnemyStatistics(i<2?6:2);
    for(int i=0;i<4;++i) m_status[i]=StatusMap::Neutral;
    m_position[0]={5,8}; m_position[1]={5,8}; m_size[0]=3.0f;
    m_index=41;
    for(int i=0;i<2;++i) { m_attack[i]=true; m_needCurveInput[i]=0; m_resourceNeed[i]=99; m_assignedSquadStrength[i]=99; }
}
AIStrategicObject::~AIStrategicObject() { for(int i=0;i<4;++i) delete m_statistics[i]; }
AIStrategicArea::AIStrategicArea(const std::string&,unsigned,float):AIStrategicObject(0,0) {
    m_centre={2,4}; m_controlPointId=73; m_sideEnabled[0]=m_sideEnabled[1]=true;
    m_statusChanged=true; m_enemyTakeover[0]=m_enemyTakeover[1]=true;
}
AIStrategicArea::~AIStrategicArea() {}
SquadManager* SquadManager::m_instance=nullptr;
SquadManager::SquadManager() {}
int SquadManager::getSquadStrengths(int side,int index,StrategicStrengthType type) {
    trace("squads"); require(side==ownSide && index==41,"squad side/index");
    require(static_cast<int>(type)==(area->m_attack[side-1]?0:1),"attack/defence strategic selector");
    require(area->m_assignedSquadStrength[side-1]==0,"assigned counter cleared before callback");
    if(mode=="cached_dependencies") CommonSingletonServer::m_iaiGame=alternateGame;
    return 17;
}
Information::Information(IAIObject* object,double time):m_time(time),m_object(object),m_physical(nullptr),m_unit(nullptr) {
    Record& r=records[constructing]; r.unitSlot=&m_unit; r.physicalSlot=&m_physical;
    r.strengthSlot=&m_strength; r.flagSlot=&m_objectTypeFlags; m_strength=2; m_objectTypeFlags=0;
}
Information::~Information() {}
int Information::getInformationSort() { unexpected("information sort"); }
unsigned Information::getFirstSecondaryObject(int& it,int side) {
    const int id=recordOf(this); trace("first"+std::to_string(id)); require(side==ownSide,"first secondary side");
    it=1;
    if(mode=="manager_carry" && id==0) informationManager=alternateManager;
    return records[id].childCount?records[id].children[0]:0;
}
unsigned Information::getSecondaryObject(int& it,int side) {
    const int id=recordOf(this); trace("next"+std::to_string(id)); require(side==ownSide && it>=1 && it<=2,"secondary root/iterator/side");
    return it<records[id].childCount?records[id].children[it++]:0;
}

IPIUnit::IPIUnit(Information* information):m_battleStrengths(records[constructing].battle),m_strategicStrengths(records[constructing].strategic) {
    m_information=information; records[constructing].battleSlot=&m_battleStrengths;
}
IPIUnit::~IPIUnit() {}
int IPIUnit::getType() { unexpected("unit type"); }
void IPIUnit::setTime() { unexpected("unit time"); } void IPIUnit::setTime(double) { unexpected("unit time value"); }
bool IPIUnit::isOccupied() { for(int i=0;i<6;++i) if(records[i].unit==this) { trace("occupied"+std::to_string(i)); return records[i].occupied; } unexpected("unit identity"); }

int IPIPhysical::getType() { unexpected("physical type"); }
} } }

static std::string recordTrace(int id,bool unit,bool occupied,int kind,bool secondary=false) {
    const std::string s=std::to_string(id); std::string t="sec"+s;
    if(unit) { t+=",occupied"+s; if(occupied) { t+=",side"+s+",friend"+std::to_string(records[id].side); if(kind!=1) t+=",side"+s+",enemy"+std::to_string(records[id].side); if(kind==2) t+=",type"+s; } }
    if(!secondary) t+=",first"+s;
    return t;
}
int main(int argc,char** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX); _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE); _CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    require(argc==2,"mode required"); mode=argv[1];
    if(mode=="helper_zero") {
        SampleHelper helper(0); int count[4]={4,5,6,7};
        helper.clear(nullptr,nullptr); helper.addSample(nullptr,2,nullptr,count);
        require(helper.m_size==0 && count[0]==4 && count[1]==5 && count[2]==7 && count[3]==7,"zero-size skips arrays but increments indexed count");
        std::printf("PASS %s\n",mode.c_str()); return 0;
    }
    if(mode=="helper_index") {
        SampleHelper helper(2); float strength[3]={99,99,123},input[2]={1.5f,-2.5f}; int count[4]={99,99,456,8};
        helper.clear(strength,count); require(strength[0]==0 && strength[1]==0 && strength[2]==123 && count[0]==0 && count[1]==0 && count[2]==456,"size-limited clear");
        helper.addSample(input,3,strength,count); helper.addSample(input,3,strength,count);
        require(strength[0]==3 && strength[1]==-5 && strength[2]==123 && count[0]==0 && count[1]==0 && count[2]==456 && count[3]==10,"independent count index and repeated sample");
        std::printf("PASS %s\n",mode.c_str()); return 0;
    }
    if(mode=="mask_one" || mode=="mask_two" || mode=="mask_wrap") {
        const int side=mode=="mask_two"?2:1; SAI native(side,0,0,Vec2{},Vec2{}); ISAI* relationInterface=&native;
        if(mode=="mask_wrap") {
            *friendlyMask=static_cast<int>(0x80000001u); *enemyMask=8;
            require(relationInterface->isFriend(-1) && relationInterface->isFriend(31) && relationInterface->isFriend(32) && relationInterface->isFriend(64) && !relationInterface->isFriend(33),"x86 friendly shift masking");
            require(relationInterface->isEnemy(3) && relationInterface->isEnemy(35) && relationInterface->isEnemy(-29) && !relationInterface->isEnemy(2),"independent enemy mask with wrapped shifts");
        } else {
            require(relationInterface->isFriend(side) && !relationInterface->isEnemy(side) && relationInterface->isEnemy(3-side) && !relationInterface->isFriend(3-side) && !relationInterface->isFriend(0) && !relationInterface->isEnemy(0),"actual native relation leaves");
        }
        std::printf("PASS %s\n",mode.c_str()); return 0;
    }
    SCurve curve;
    ObservedArea concrete; area=&concrete;
    ObservedSAI sai; SquadManager squads; SquadManager::m_instance=&squads;
    ObservedGame game,otherGame; otherGame.identity=1; alternateGame=&otherGame; CommonSingletonServer::m_iaiGame=&game;
    if(mode=="counter_non_control" || mode=="counter_control" || mode=="counter_fresh") {
        if(mode=="counter_non_control") CommonSingletonServer::m_iaiGame=nullptr;
        checkFloat(concrete.AIStrategicArea::getControlPointTakeOverCounter(),mode=="counter_non_control"?1.0f:-0.375f,"actual Area counter result");
        require(calls==(mode=="counter_non_control"?"flag":mode=="counter_fresh"?"flag,gameCounter1":"flag,gameCounter0"),"actual counter complete callback trace");
        std::printf("PASS %s\n",mode.c_str()); return 0;
    }
    ObservedEnvironment environment,otherEnvironment; otherEnvironment.identity=1; alternateEnvironment=&otherEnvironment; AISingletonServer::m_iaiInformationEnvironment=&environment;
    InformationManager manager,replacementManager; informationManager=&manager; alternateManager=&replacementManager;
    ObservedInformation* info[6]; ObservedPhysical physical[6]; IPIUnit* units[6];
    for(int i=0;i<6;++i) { constructing=i; info[i]=new ObservedInformation; records[i].information=info[i];
        physical[i].identity=i; records[i].physical=&physical[i]; *records[i].physicalSlot=&physical[i];
        units[i]=new IPIUnit(info[i]); records[i].unit=units[i]; *records[i].unitSlot=units[i]; handles[i]=manager.getPool()->acquire(info[i]); }
    // Explicit expected outcomes; these do not implement the decision tree under test.
    StatusMap::Status expectedStatus=StatusMap::Owned; bool expectedChanged=true,expectedTakeover=false;
    float expectedTotal=3,expectedFree=3,expectedFriendly=0,expectedEnemy=0,expectedRatio=0;
    int expectedNeed=2; float expectedBattle[6]={}; int expectedBattleCount[6]={};
    float expectedStrategic[2]={}; int expectedStrategicCount[2]={};
    std::string middle,tail="owner0,counter";
    if(mode=="cp_hostile") { ownerValue=2; expectedStatus=StatusMap::Hostile; expectedChanged=false; }
    else if(mode=="cp_neutral") { ownerValue=0; expectedStatus=StatusMap::Neutral; expectedChanged=false; }
    else if(mode=="cp_lost_neutral") { area->m_status[0]=StatusMap::Owned; ownerValue=0; expectedStatus=StatusMap::Neutral; expectedTakeover=true; tail="owner0"; }
    else if(mode=="cp_takeover" || mode=="cp_takeover_equal" || mode=="cp_takeover_nan") {
        area->m_status[0]=StatusMap::Owned;
        counterValue=mode=="cp_takeover"?0.5f:(mode=="cp_takeover_equal"?0.9f:std::numeric_limits<float>::quiet_NaN());
        expectedChanged=mode=="cp_takeover"; expectedTakeover=expectedChanged; expectedStatus=expectedChanged?StatusMap::Neutral:StatusMap::Owned;
    }
    else if(mode=="owner_changes_id") { expectedChanged=false; expectedTakeover=true; tail="owner0"; }
    else if(mode=="free_empty") { area->m_controlPointId=unsigned(-1); expectedStatus=StatusMap::Neutral; expectedChanged=false; expectedTakeover=true; tail=""; }
    else if(mode=="defence") area->m_attack[0]=false;
    else if(mode=="side_two") { ownSide=ownerValue=2; }
    else if(mode=="curve_half") { area->m_needCurveInput[0]=0.5f; expectedNeed=1; }
    else if(mode=="curve_high") { area->m_needCurveInput[0]=1; expectedNeed=1; }
    else if(mode=="geometry_rounding") { area->m_position[0]={5,1.0f}; area->m_centre={2,-16777216.0f}; wantedUpper.y=16777216.0f; }
    else if(mode=="cached_dependencies" || mode=="cp_owned") {}
    else {
        roots[0][0]=handles[0]; rootCounts[0]=1;
        expectedTotal=5;
        if(mode=="invalid") { roots[0][0]=0; expectedTotal=3; }
        else if(mode=="no_unit") { *records[0].unitSlot=nullptr; expectedFree=5; middle=recordTrace(0,false,false,0); }
        else if(mode=="unoccupied") { records[0].occupied=false; middle=recordTrace(0,true,false,0); }
        else if(mode=="neither") { records[0].side=0; middle=recordTrace(0,true,true,0); }
        else if(mode=="friend" || mode=="flagged_friend" || mode=="unit_reload") {
            expectedFriendly=21; expectedRatio=1; middle=recordTrace(0,true,true,1);
            if(mode=="flagged_friend") *records[0].flagSlot=16;
            if(mode=="unit_reload") middle="sec0,occupied1,side0,friend1,first0";
        }
        else if(mode=="enemy" || mode=="flagged_enemy" || mode=="battle_pointer_capture" || mode=="repeated_side" || mode=="fractional_need") {
            records[0].side=2; expectedEnemy=21; expectedNeed=4;
            for(int i=0;i<6;++i) expectedBattle[i]=records[0].battle[i]; expectedBattleCount[2]=1;
            expectedStrategic[0]=2; expectedStrategic[1]=4; expectedStrategicCount[1]=1;
            middle=recordTrace(0,true,true,2);
            if(mode=="flagged_enemy") *records[0].flagSlot=16;
            if(mode=="battle_pointer_capture") for(int i=0;i<6;++i) records[1].battle[i]=100;
            if(mode=="repeated_side") middle="sec0,occupied0,side0,friend0,side0,enemy2,type0,first0";
            if(mode=="fractional_need") records[0].strategic[0]=expectedStrategic[0]=2.75f;
        }
        else if(mode=="secondary_enemy" || mode=="secondary_flagged_friend") {
            *records[0].unitSlot=nullptr; records[0].childCount=1; records[0].children[0]=handles[1];
            *records[1].flagSlot=16; expectedTotal=7; expectedFree=5;
            middle=recordTrace(0,false,false,0)+",";
            if(mode=="secondary_enemy") { records[1].side=2; expectedEnemy=21; expectedNeed=4;
                for(int i=0;i<6;++i) expectedBattle[i]=records[1].battle[i]; expectedBattleCount[2]=1;
                expectedStrategic[0]=2; expectedStrategic[1]=4; expectedStrategicCount[1]=1;
                middle+=recordTrace(1,true,true,2,true); }
            else { expectedFriendly=21; expectedRatio=1; middle+=recordTrace(1,true,true,1,true); }
            middle+=",next0";
        }
        else if(mode=="wide_sum") { records[0].battle[0]=16777216.0f; records[0].battle[1]=1; records[0].battle[2]=-16777216.0f; for(int i=3;i<6;++i) records[0].battle[i]=0;
            expectedFriendly=1; expectedRatio=1; middle=recordTrace(0,true,true,1); }
        else if(mode=="negative_ratio" || mode=="nan_ratio") {
            for(int i=0;i<6;++i) records[0].battle[i]=0;
            records[0].battle[0]=mode=="negative_ratio"?-3.0f:std::numeric_limits<float>::quiet_NaN();
            expectedFriendly=records[0].battle[0]; middle=recordTrace(0,true,true,1);
        }
        else if(mode=="weighted_product") { records[0].security=0.5f; *records[0].strengthSlot=3; expectedTotal=4.5f; expectedFriendly=10.5f; expectedRatio=1; middle=recordTrace(0,true,true,1); }
        else if(mode.compare(0,5,"free_")==0) {
            area->m_controlPointId=unsigned(-1); expectedTakeover=true; tail="";
            bool hostile=mode=="free_enemy" || mode=="free_flagged_enemy" || mode=="free_enemy_disabled" || mode=="free_secondary_enemy";
            const bool secondary=mode=="free_secondary_friend" || mode=="free_secondary_enemy";
            const int id=secondary?1:0;
            area->m_status[0]=hostile?StatusMap::Owned:StatusMap::Hostile;
            records[id].side=hostile?2:1;
            if(hostile) { expectedEnemy=21; expectedNeed=10; expectedStatus=StatusMap::Hostile;
                for(int i=0;i<6;++i) expectedBattle[i]=records[id].battle[i]; expectedBattleCount[2]=1;
                expectedStrategic[0]=2; expectedStrategic[1]=4; expectedStrategicCount[0]=1;
            } else { expectedFriendly=21; expectedRatio=1; }
            if(secondary) { *records[0].unitSlot=nullptr; records[0].children[0]=handles[1]; records[0].childCount=1; *records[1].flagSlot=16;
                expectedTotal=7; expectedFree=5; middle=recordTrace(0,false,false,0)+","+recordTrace(1,true,true,hostile?2:1,true)+",next0";
            } else middle=recordTrace(0,true,true,hostile?2:1);
            if(mode=="free_flagged_friend" || mode=="free_flagged_enemy" || mode=="free_friend_disabled" || mode=="free_enemy_disabled") {
                if(mode=="free_flagged_friend" || mode=="free_flagged_enemy") *records[0].flagSlot=16;
                else area->m_sideEnabled[hostile?1:0]=false;
                expectedChanged=false; expectedStatus=hostile?StatusMap::Owned:StatusMap::Hostile; expectedNeed=hostile?8:1;
            }
            if(mode=="free_contested") { area->m_status[0]=StatusMap::Unknown; records[1].side=2; roots[0][1]=handles[1]; rootCounts[0]=2;
                expectedStatus=StatusMap::Unknown; expectedChanged=false; expectedTotal=7; expectedFriendly=expectedEnemy=21; expectedRatio=0.5f; expectedNeed=3;
                for(int i=0;i<6;++i) expectedBattle[i]=records[1].battle[i]; expectedBattleCount[2]=1;
                expectedStrategic[0]=2; expectedStrategic[1]=4; expectedStrategicCount[1]=1;
                middle=recordTrace(0,true,true,1)+","+recordTrace(1,true,true,2);
            }
        }
        else if(mode=="live_end") { expectedTotal=7; expectedFriendly=42; expectedRatio=1; middle=recordTrace(0,true,true,1)+","+recordTrace(1,true,true,1); }
        else if(mode=="queries") { for(int q=0;q<4;++q) { roots[q][0]=handles[q]; rootCounts[q]=1; if(q) middle+=","; middle+=recordTrace(q,true,true,1); }
            expectedTotal=11; expectedFriendly=84; expectedRatio=1; }
        else if(mode=="manager_carry") { *records[0].unitSlot=nullptr; *records[2].unitSlot=nullptr;
            replacementManager.getPool()->acquire(info[0]); unsigned replacementHandle=replacementManager.getPool()->acquire(info[2]);
            roots[0][1]=replacementHandle; rootCounts[0]=2; expectedTotal=7; expectedFree=7;
            middle=recordTrace(0,false,false,0)+","+recordTrace(2,false,false,0); }
        else unexpected("unknown mode");
    }
    bool changed=true;
    concrete.update(ownSide,&sai,changed);
    std::string wanted="squads,q0,q1,q2,q3"; if(!middle.empty()) wanted+=","+middle; if(!tail.empty()) wanted+=","+tail;
    if(calls!=wanted) { std::fprintf(stderr,"TRACE got=%s expected=%s\n",calls.c_str(),wanted.c_str()); unexpected("exact callbacks"); }
    const int s=ownSide-1;
    require(changed==expectedChanged && area->m_statusChanged==expectedChanged,"changed output overwritten/published");
    require(area->m_status[s]==expectedStatus && area->m_enemyTakeover[s]==expectedTakeover,"status and takeover result");
    checkFloat(area->m_f164[s],expectedTotal,"total + radius"); checkFloat(area->m_f164[s+2],expectedFree,"no-unit + radius");
    checkFloat(area->m_friendlyBattleStrength[s],expectedFriendly,"friendly battle strength"); checkFloat(area->m_enemyBattleStrength[s],expectedEnemy,"enemy battle strength");
    checkFloat(area->m_friendlyRatio[s],expectedRatio,"friendly ratio");
    require(area->m_resourceNeed[s]==expectedNeed && area->m_assignedSquadStrength[s]==17,"shaped need and assigned strength");
    EnemyStatistics* battle=area->m_statistics[s]; EnemyStatistics* strategic=area->m_statistics[s+2];
    require(battle->m_samples==1 && strategic->m_samples==1,"one real sample each");
    for(int i=0;i<6;++i) { checkFloat(battle->m_strength[i],expectedBattle[i],"battle sample"); checkFloat(battle->m_count[i],static_cast<float>(expectedBattleCount[i]),"battle sample counts"); }
    for(int i=0;i<2;++i) { checkFloat(strategic->m_strength[i],expectedStrategic[i],"strategic sample"); checkFloat(strategic->m_count[i],static_cast<float>(expectedStrategicCount[i]),"strategic sample counts"); }
    require(queryCalls==4 && queryFreed,"queries and storage lifetime");
    for(int i=0;i<6;++i) { delete units[i]; delete info[i]; }
    informationManager=nullptr; CommonSingletonServer::m_iaiGame=nullptr; AISingletonServer::m_iaiInformationEnvironment=nullptr; SquadManager::m_instance=nullptr;
    std::printf("PASS %s\n",mode.c_str());
}
