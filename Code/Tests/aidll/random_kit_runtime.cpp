// [TEMP] Exact GetRandomKit, findSuitableKit and getBoxMaxCorner bodies, whole
// actual InformationManager/pool. Other native lifetimes/typed receivers are chosen;
// vehicleOccupied is a caller collaborator, tested separately in its own suite.
#include "random_kit_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cmath>
#include <limits>
#include <new>
#include <windows.h>
#include <crtdbg.h>
using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "x86 only");
[[noreturn]] static void unexpected(const char* message)
{ std::fprintf(stderr,"UNEXPECTED %s\n",message); std::fflush(stderr); std::abort(); }
static void require(bool value,const char* message) { if(!value) unexpected(message); }
static std::string mode,caseName,calls;
static bool score() { return mode.compare(0,13,"policy_score_")==0; }
static bool expectedFallback;
static int expectedObjectResult=2,expectedScoreDraws;
static unsigned scoreRaw=0xffffffffu;
static bool captureMapAllocations;
static void* mapAllocations[32];
static bool mapFrees[32];
static unsigned mapAllocationCount;
static unsigned expectedMapNodes;
static const void* queryStorage;
static bool queryFreed;
void* operator new(std::size_t size)
{
    void* pointer=std::malloc(size?size:1);
    if(!pointer) throw std::bad_alloc();
    if(captureMapAllocations) {
        if(mapAllocationCount==32) std::abort();
        mapAllocations[mapAllocationCount++]=pointer;
    }
    return pointer;
}
void operator delete(void* pointer) noexcept
{
    if(pointer && pointer==queryStorage) queryFreed=true;
    for(unsigned i=0;i<mapAllocationCount;++i) if(pointer==mapAllocations[i]) mapFrees[i]=true;
    std::free(pointer);
}
void operator delete(void* pointer,std::size_t) noexcept { ::operator delete(pointer); }
static void checkMapDestroyed()
{
    require(mapAllocationCount>=expectedMapNodes,"actual native map copy allocated independent nodes");
    for(unsigned i=0;i<mapAllocationCount;++i) require(mapFrees[i],"copied map destroyed before shared fallback");
}
static void trace(const std::string& event)
{
    if(!calls.empty()) calls+=','; calls+=event;
    if(mode.find("_stop")!=std::string::npos || score()) {
        std::printf("TRACE %s\n",event.c_str()); std::fflush(stdout);
    }
}
static unsigned undoRight(unsigned v,unsigned shift)
{ unsigned r=v; for(int i=0;i<8;++i) r=v^(r>>shift); return r; }
static unsigned undoLeft(unsigned v,unsigned shift,unsigned mask)
{ unsigned r=v; for(int i=0;i<8;++i) r=v^((r<<shift)&mask); return r; }
static unsigned stateFor(unsigned v)
{ return undoRight(undoLeft(undoLeft(undoRight(v,18),15,0xDB8B0000u),7,0x2B5B2500u),11); }
static unsigned raw;
static int* randomIndex;
static unsigned* randomState;
static int* secondRandomIndex;
static unsigned* secondRandomState;
static AIRandom* replacementRandom;
static AIRandom* firstRandom;
static int randomConstructed;
static int refillCalls;
static std::vector<IKitTemplate*> templates;
static std::vector<IKitTemplate*> scoringTemplates;
static IKitTemplate* all[13];
static int types[13],reads[13],managerReads;
static IKitTemplateManager* alternateManager;
static SquadMember *expectedMember, *secondMember;
static SquadManager *managerA, *managerB, *expectedSquadManager;
static SquadCoordinator *squadA, *squadB, *lookupResult;
static IAIPlayer *selectedPlayer, *replacementPlayer;
static int memberReads, lookupReads, leaderReads, scanReads, playerReads, kitReads;
static int inputPlayerId, inputSide, currentKit;
static bool hasMedic;
static bool returnMedic, objectPresent, initialSquad, assigned;
static SquadMember *fallbackMember, *rosterMembers[4];
static IAIPlayer* rosterPlayers[4];
static int rosterKits[4], countReads, indexReads, memberPlayerReads, rosterKitReads, assignmentReads;
static int expectedRosterReads;
static std::vector<int> visitedIndices;
static AIStrategicArea* expectedArea;
static IAIInformationEnvironment* alternateEnvironment;
static Vec2 returnedPosition;
static int positionReads, rectangleReads;
using DeadBots=std::map<int,std::pair<int,int> >;
struct ScanRecord {
    Information* info=nullptr; IPIPhysical** physicalSlot=nullptr;
    int side=2, strength=3, kit=4; float health=1.0f; bool occupied=true;
};
static ScanRecord records[8];
static int constructingInformation,constructingSAI,occupancyReads,sideReads,strengthReads,healthReads,informationKitReads;
static int mainReads,deadReads,currentIdReads,botLookupReads,spawnAreaReads;
static unsigned handles[8];
static InformationManager *firstInformationManager,*secondInformationManager;
static IPIPhysical* replacementPhysical;
static std::vector<unsigned>* queriedHandles;
static SAI *sais[2],*selectedSAI;
static DeadBots* sourceMaps[2];
static IAIMain* replacementMain;
static IBotManager *botManagers[2];
static Bot* plannedBots[2];
static std::vector<unsigned> scanHandles;
static int recordIndex(const Information* info)
{ for(int i=0;i<8;++i) if(records[i].info==info) return i; unexpected("Information receiver"); }
static int observeInformationSide(const Information* info)
{
    const int id=recordIndex(info); trace("side"+std::to_string(id)); ++sideReads;
    require(occupancyReads>0 && mainReads==0,"side only after occupied result and before SAI query");
    if(mode=="policy_object_scan_retained_physical_stop") *records[id].physicalSlot=replacementPhysical;
    return records[id].side;
}
static int observeStrength(int id)
{
    trace("strength"+std::to_string(id)); ++strengthReads;
    require(id<8 && sideReads==strengthReads && mainReads==0,"one strength query follows each accepted side");
    return records[id].strength;
}
static float observeHealth(int id)
{
    trace("health"+std::to_string(id)); ++healthReads;
    require(id<8 && strengthReads>0 && mainReads==0,"health through retained physical before SAI");
    if(mode=="policy_object_scan_main_swap_stop") IAIMain::m_instance=replacementMain;
    return records[id].health;
}
static SAI* observeSAI(int identity,int side)
{
    trace("sai"+std::to_string(identity)); ++mainReads;
    require(side==inputSide && mainReads==1 && rectangleReads==1,"fresh main receives original side after rectangle scan");
    require(identity==(mode=="policy_object_scan_main_swap_stop"?1:0),"fresh main singleton after scan callbacks");
    SAI* result=selectedSAI;
    if(mode=="policy_score_borrowed_vector") {
        std::vector<IKitTemplate*> replacement(13,nullptr); replacement[6]=all[6];
        scoringTemplates.swap(replacement);
        CommonSingletonServer::m_iKitTemplateManager=alternateManager;
    }
    if(mode=="policy_object_scan_sai_capture_stop") { selectedSAI=sais[1]; IAIMain::m_instance=replacementMain; }
    return result;
}
static unsigned observeBotId(int identity)
{
    captureMapAllocations=false;
    trace("botid"+std::to_string(++currentIdReads));
    require(identity==0 && deadReads==1,"original bot queried for each copied map entry");
    if((mode=="policy_object_map_copy_stop" || mode=="policy_score_map_copy_bin") && currentIdReads==1) {
        // Insert without invalidating iterators: a borrowed map would wrongly visit this.
        (*sourceMaps[0])[99]=std::make_pair(-200,13);
        (*sourceMaps[0])[20].second=13;
    }
    if(mode=="policy_object_map_manager_stop" && currentIdReads==1) IBotManager::instance=botManagers[1];
    if(mode=="policy_object_map_dynamic_id_stop") return currentIdReads==1?10u:20u;
    return 10u;
}
static Bot* observeBotLookup(int identity,unsigned id)
{
    trace("botlookup"+std::to_string(identity)+":"+std::to_string(id)); ++botLookupReads;
    require(deadReads==1 && currentIdReads>0,"map lookup after current bot id query");
    const int expected=(mode=="policy_object_map_manager_stop" && botLookupReads==1)?1:0;
    require(identity==expected,"manager reloaded for each planned bot lookup");
    if(id==30u) return nullptr;
    return id==40u?plannedBots[1]:plannedBots[0];
}
static int observeSpawnArea(int identity)
{
    trace("spawnarea"+std::to_string(identity)); ++spawnAreaReads;
    require(identity==1 || identity==2,"retained resolved bot receives target query");
    if(mode=="policy_object_map_manager_stop") IBotManager::instance=botManagers[0];
    if(mode=="policy_score_map_area_capture") expectedArea->m_index=71;
    return identity==1?70:71;
}
static unsigned floatBits(float value) { unsigned bits; std::memcpy(&bits,&value,4); return bits; }
static float fromBits(unsigned bits) { float value; std::memcpy(&value,&bits,4); return value; }
static const Vec2& observePosition(const AIStrategicArea* area,int side)
{
    trace("position"); ++positionReads;
    require(area==expectedArea && side==inputSide,"retained area and original side for position");
    require(objectPresent && !returnMedic && positionReads==1,"position only after medic refusal and nonnull object");
    require(managerReads==2 && *randomIndex==1 && countReads==0,"geometry follows entry draw and precedes roster");
    if(mode=="policy_object_cached_centre_environment_stop") {
        expectedArea->m_centre={1000.0f,-1000.0f};
        AISingletonServer::m_iaiInformationEnvironment=alternateEnvironment;
    }
    if(mode=="policy_object_alias_position_stop") {
        expectedArea->m_centre=returnedPosition;
        return expectedArea->m_centre;
    }
    return returnedPosition;
}
static void observeRectangle(int identity,std::vector<unsigned>& result,const Vec2& lower,const Vec2& upper,
                             int side,bool world,IAIInformationGridPredicate* predicate)
{
    ++rectangleReads;
    require(identity==(mode=="policy_object_cached_centre_environment_stop"?1:0),"environment loaded after position callback");
    require(positionReads==1 && rectangleReads==1 && *randomIndex==1,"one position then one rectangle before second draw");
    require(side==inputSide && world && predicate==nullptr && result.empty(),"fresh result, original side, true world, null filter");
    require(countReads==0 && assignmentReads==0 && managerReads==2,"nonnull geometry skips null-object roster/fallback");
    require(&lower!=&upper && &lower!=&returnedPosition && &upper!=&returnedPosition &&
            &lower!=&expectedArea->m_centre && &upper!=&expectedArea->m_centre,"independent native corner values");
    if(mode=="policy_object_rounding_stop") {
        require(floatBits(lower.x)==0xc0ffffffu && floatBits(lower.y)==0xc1000000u &&
                floatBits(upper.x)==0x41800000u && floatBits(upper.y)==0x41800000u,
                "PE Y-only delta spill witness and actual wide box helper");
    } else if(mode=="policy_object_reversed_stop") {
        require(lower.x==34.0f && lower.y==8.0f && upper.x==-30.0f && upper.y==-24.0f,
                "reversed corners remain unsorted and unclamped");
    } else if(mode=="policy_object_nonfinite_stop") {
        require(std::isnan(lower.x) && std::isnan(upper.x) && std::isinf(lower.y) && std::signbit(lower.y) &&
                std::isinf(upper.y) && !std::signbit(upper.y),"nonfinite corner arithmetic is preserved");
    } else {
        require(lower.x==4.0f && lower.y==5.0f && upper.x==20.0f && upper.y==45.0f,
                "cached centre and copied position form exact rectangle");
    }
    if(mode=="policy_object_populated_stop") { result.push_back(0u); result.push_back(0x12340042u); }
    for(unsigned handle:scanHandles) result.push_back(handle);
    result.reserve(result.size()+4); // [TEMP] append callbacks below must not invalidate iterators.
    queriedHandles=&result;
    queryStorage=result.data(); queryFreed=false;
    trace("rectangle"+std::to_string(identity)+":size"+std::to_string(result.size()));
}
static bool policy() { return mode.compare(0,7,"policy_")==0; }
static SquadMember* observeMember()
{
    trace("member"+std::to_string(++memberReads));
    require(policy() && memberReads<=(returnMedic?2:(initialSquad?3:2)),"caller member query count");
    if(!returnMedic && (!objectPresent || expectedFallback) && memberReads==(initialSquad?3:2)) {
        require(!assigned && *randomIndex==1+expectedScoreDraws,"fallback member follows prior actual draws");
        require(assignmentReads==(initialSquad?1:0),"fallback follows required assignment query");
        if(objectPresent) { checkMapDestroyed(); require(!queryFreed,"query storage survives shared fallback member callback"); }
        templates={all[1],all[2]};
        if(mode=="policy_fallback_templates") {
            CommonSingletonServer::m_iKitTemplateManager=alternateManager;
            templates={all[3],all[4]};
        }
        return fallbackMember;
    }
    if(memberReads==2 && mode=="policy_retained_squad") {
        lookupResult=squadB; SquadManager::m_instance=managerB;
    }
    return memberReads==1 ? expectedMember : secondMember;
}
static int observePlayerId()
{
    trace("player_id");
    require(policy() && managerReads==2 && *randomIndex==1,"player id follows draw and fresh template query");
    if(mode=="policy_manager_capture") SquadManager::m_instance=managerB;
    if(mode=="policy_template_swap") SquadManager::m_instance=managerA;
    return inputPlayerId;
}
static IAIPlayer* observePlayer()
{
    trace("player"); ++playerReads;
    require(policy() && hasMedic && scanReads==1,"player query follows positive medic scan");
    IAIPlayer* result=selectedPlayer;
    if(mode=="policy_retained_player") selectedPlayer=replacementPlayer;
    return result;
}
static int observeKit(int identity)
{
    trace("kit"+std::to_string(identity)); ++kitReads;
    if(identity>=10 && identity<14) {
        const int slot=identity-10; ++rosterKitReads;
        require(countReads==1 && assignmentReads==0,"roster kit between count and assignment");
        if(slot==0 && mode=="policy_cached_count") squadA->m_squadMembers.push_back(rosterMembers[2]);
        if(slot==0 && mode=="policy_live_members") {
            std::vector<SquadMember*> replacement(8,rosterMembers[2]);
            replacement[1]=rosterMembers[3];
            squadA->m_squadMembers.swap(replacement);
        }
        return rosterKits[slot];
    }
    require(identity==0 && playerReads==1,"retained returned player receiver");
    return currentKit;
}
static const std::vector<IKitTemplate*>& observeTemplates(int identity)
{
    trace("templates"+std::to_string(identity)); ++managerReads;
    if(policy() && managerReads==2) {
        require(*randomIndex==1,"entry consumes discarded random result before second manager query");
        if(mode=="policy_template_swap") SquadManager::m_instance=managerB;
        if(objectPresent) return scoringTemplates;
    }
    return templates;
}
static KitType observeType(int identity)
{
    trace("type"+std::to_string(identity)+":"+std::to_string(++reads[identity]));
    if(identity==0 && reads[0]==1) {
        if(mode=="reload") templates[0]=all[1];
        if(mode=="live_end") templates.push_back(all[1]);
        if(mode=="manager_swap" || mode=="policy_template_swap")
            CommonSingletonServer::m_iKitTemplateManager=alternateManager;
    }
    if(mode=="second_type" && identity==0 && reads[0]==2) return static_cast<KitType>(5);
    return static_cast<KitType>(types[identity]);
}
#include "random_kit_mocks.inl"
namespace dice { namespace hfe { namespace ai {
// [TEMP] Chosen lifetimes; the helper itself and AIRandom::next are actual code.
Information::Information(IAIObject* object,double time):m_time(time),m_object(object),m_physical(nullptr),m_unit(nullptr)
{ ScanRecord& record=records[constructingInformation++]; record.info=this; record.physicalSlot=&m_physical; }
Information::~Information() {}
int Information::getInformationSort() { unexpected("Information::getInformationSort"); }
int Information::getKitType() const
{
    int id=recordIndex(this); trace("informationkit"+std::to_string(id)); ++informationKitReads;
    require(records[id].strength==0 && records[id].side!=(inputSide==1)+1 && mainReads==0,"kit only for occupied nonenemy infantry");
    return records[id].kit;
}

int IPIPhysical::getType() { unexpected("IPIPhysical::getType"); }
bool vehicleOccupied(unsigned handle,int side)
{
    Information* info=informationManager->getInformation(handle);
    const int id=recordIndex(info); trace("occupied"+std::to_string(id)); ++occupancyReads;
    require(side==inputSide && rectangleReads==1 && mainReads==0,"occupancy uses original side after query");
    if(mode=="policy_object_scan_retained_physical_stop") *records[id].physicalSlot=replacementPhysical;
    if(mode=="policy_object_scan_manager_swap_stop" && id==0) informationManager=secondInformationManager;
    if(mode=="policy_object_scan_live_end_stop" && id==0) queriedHandles->push_back(handles[1]);
    return records[id].occupied;
}
IAIMain* IAIMain::m_instance=nullptr;
IAIMain::IAIMain() {}
IAIMain::~IAIMain() {}
ISAI::~ISAI() {}
SAI::SAI(int side,int,int,Vec2,Vec2):m_side(side)
{ sais[constructingSAI]=this; sourceMaps[constructingSAI]=&m_map5C; }
SAI::~SAI() {}
const DeadBots& SAI::getDeadBots() const
{
    int identity=this==sais[0]?0:1;
    trace("dead"+std::to_string(identity)+":size"+std::to_string(m_map5C.size())); ++deadReads;
    require(this==sais[0] && mainReads==1 && deadReads==1,"returned SAI retained through callback mutation");
    if(mode=="policy_score_fresh_random") AIRandom::m_instance=replacementRandom;
    if(mode=="policy_score_refill_negative") *randomIndex=-1;
    if(mode=="policy_score_refill_captured_vector") *randomIndex=624;
    captureMapAllocations=!m_map5C.empty();
    expectedMapNodes=static_cast<unsigned>(m_map5C.size());
    return m_map5C;
}
IBotManager* IBotManager::instance=nullptr;
IBotManager::IBotManager() {}
IBotManager::~IBotManager() {}
Bot::~Bot() {}
int Bot::getBotType() const { unexpected("Bot::getBotType"); }
BAPContext* Bot::getScriptContext() { unexpected("Bot::getScriptContext"); }
IAIPlayer::~IAIPlayer() {}
SquadManager* SquadManager::m_instance=nullptr;
SquadManager::SquadManager() {}
SquadCoordinator::SquadCoordinator(int side,int squadIndex):m_side(side),m_squadIndex(squadIndex) {}
SquadMember::SquadMember(SquadCoordinator* squad,IAIPlayer* player,int squadIndex,int side,int memberIndex)
    :m_squad(squad),m_player(player),m_squadIndex(squadIndex),m_side(side),m_memberIndex(memberIndex),m_isSquadLeader(false) {}
SquadMember::~SquadMember() {}
SquadCoordinator* SquadManager::getSquadFromPlayer(int playerId,int side)
{
    trace(this==managerA ? "lookupA" : "lookupB"); ++lookupReads;
    require(this==expectedSquadManager,"platform-specific singleton capture around player callback");
    require(playerId==inputPlayerId && side==inputSide,"unchanged full player id and original side");
    require(memberReads==1 && managerReads==2,"one entry member lookup before squad search");
    return lookupResult;
}
bool SquadMember::isSquadLeader()
{
    trace(this==expectedMember ? "leader1" : "leader2"); ++leaderReads;
    require(this==secondMember && memberReads==2,"fresh second member used for leader query");
    return m_isSquadLeader;
}
bool SquadCoordinator::hasMemberWithKit(KitType type)
{
    trace(this==squadA ? "scanA" : "scanB"); ++scanReads;
    require(this==squadA && type==static_cast<KitType>(5),"retained selected squad and exact medic enum");
    require(leaderReads==1 && !secondMember->m_isSquadLeader,"scan only follows nonleader");
    return hasMedic;
}
int SquadCoordinator::getNumberOfSquadMembers()
{
    trace("count"); ++countReads;
    require(this==squadA && (!objectPresent || expectedFallback) && !returnMedic,"retained squad receives count after scoring refusal");
    require(countReads==1 && memberReads==2 && *randomIndex==1+expectedScoreDraws,"capture count exactly once before fallback draw");
    if(objectPresent) { checkMapDestroyed(); require(!queryFreed,"query storage remains alive through shared roster"); }
    if(mode=="policy_retained_roster_squad") {
        lookupResult=squadB; SquadManager::m_instance=managerB;
    }
    return mode=="policy_negative_count" ? -3 : static_cast<int>(m_squadMembers.size());
}
SquadMember* SquadCoordinator::getSquadMember(int index)
{
    trace("index"+std::to_string(index)); ++indexReads; visitedIndices.push_back(index);
    require(this==squadA && countReads==1 && assignmentReads==0,"retained squad receives fresh indexed lookup");
    require(index>=0 && index<static_cast<int>(m_squadMembers.size()),"fixture index must be live and in range");
    return m_squadMembers[index];
}
IAIPlayer* SquadMember::getPlayer()
{
    int slot=0; while(slot<4 && rosterMembers[slot]!=this) ++slot;
    require(slot<4,"indexed returned member must be retained");
    trace("roster_player"+std::to_string(slot)); ++memberPlayerReads;
    IAIPlayer* result=m_player;
    if(mode=="policy_retained_roster_player") m_player=rosterPlayers[3];
    return result;
}
bool SquadCoordinator::isSquadAssignedToAnyArea()
{
    trace("assigned"); ++assignmentReads;
    require(this==squadA && countReads==1,"assignment uses retained squad after one count");
    require(indexReads==expectedRosterReads && memberPlayerReads==expectedRosterReads && rosterKitReads==expectedRosterReads,
            "signed captured roster count controls complete lookup chain");
    require(*randomIndex==1+expectedScoreDraws,"assignment query precedes fallback draw");
    return assigned;
}
AIStrategicObject::AIStrategicObject(float,float) {}
AIStrategicObject::~AIStrategicObject() {}
AIStrategicArea::AIStrategicArea(const std::string&,unsigned int,float):AIStrategicObject(0.0f,0.0f) {}
AIStrategicArea::~AIStrategicArea() {}
IAIInformationEnvironment::IAIInformationEnvironment() {}
IAIInformationEnvironment::~IAIInformationEnvironment() {}
void* AISingletonServer::m_iaiInformationEnvironment=nullptr;
IKitTemplate::IKitTemplate() {}
IKitTemplate::~IKitTemplate() {}
IKitTemplateManager::IKitTemplateManager() {}
IKitTemplateManager::~IKitTemplateManager() {}
void* CommonSingletonServer::m_iKitTemplateManager = nullptr;
AIRandom* AIRandom::m_instance = nullptr;
AIRandom::AIRandom():m_seed(0),m_index(0)
{
    if(randomConstructed++==0) { randomIndex=&m_index; randomState=m_state; firstRandom=this; }
    else { secondRandomIndex=&m_index; secondRandomState=m_state; replacementRandom=this; }
    for(unsigned& word:m_state) word=0; m_state[0]=stateFor(raw);
}
AIRandom::~AIRandom() {}
unsigned int AIRandom::generateNumbers() const
{
    require(mode=="refill"||mode=="negative_index"||mode=="policy_score_refill_negative"||mode=="policy_score_refill_captured_vector",
            "only explicit fixture modes may refill"); ++refillCalls;
    require(this==firstRandom,"refill uses captured random receiver");
    *randomIndex=0; randomState[0]=stateFor(score()?scoreRaw:raw);
    if(score()) trace("refill");
    if(mode=="policy_score_refill_captured_vector") {
        AIRandom::m_instance=replacementRandom;
        std::vector<IKitTemplate*> replacement(13,nullptr); replacement[3]=all[3]; scoringTemplates.swap(replacement);
    }
    return 0;
}
} } }
int main(int argc,char** argv)
{
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE); _CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    require(argc==2,"mode"); caseName=argv[1]; mode=caseName;
    // Preserve the preceding input scenarios while giving their now-successful public modes accurate names.
    if(mode.compare(0,14,"policy_object_")==0 && mode!="policy_object_medic") mode+="_stop";
    templates.reserve(16); scoringTemplates.resize(13,nullptr);
    ObservedTemplate values[13];
    for(int i=0;i<13;++i) { values[i].identity=i; all[i]=&values[i]; types[i]=i; scoringTemplates[i]=all[i]; }
    ObservedManager manager,alternate; alternate.identity=1; alternateManager=&alternate;
    CommonSingletonServer::m_iKitTemplateManager=&manager;
    SquadManager firstManager, secondManager;
    managerA=&firstManager; managerB=&secondManager; expectedSquadManager=managerA;
    SquadManager::m_instance=managerA;
    SquadCoordinator firstSquad(0,0), secondSquad(1,2);
    squadA=&firstSquad; squadB=&secondSquad; lookupResult=squadA;
    ObservedPlayer player,otherPlayer; otherPlayer.identity=1;
    selectedPlayer=&player; replacementPlayer=&otherPlayer;
    SquadMember firstMember(squadA,&player,0,0,0), freshMember(squadA,&player,0,0,1);
    expectedMember=&firstMember; secondMember=&freshMember;
    SquadMember tailMember(squadA,&player,0,0,2); fallbackMember=&tailMember;
    ObservedPlayer roster[4];
    for(int i=0;i<4;++i) { roster[i].identity=10+i; rosterPlayers[i]=&roster[i]; rosterKits[i]=i; }
    rosterKits[1]=13; rosterKits[2]=12;
    SquadMember roster0(squadA,&roster[0],0,0,0),roster1(squadA,&roster[1],0,0,1),
                roster2(squadA,&roster[2],0,0,2),roster3(squadA,&roster[3],0,0,3);
    rosterMembers[0]=&roster0; rosterMembers[1]=&roster1; rosterMembers[2]=&roster2; rosterMembers[3]=&roster3;
    ObservedObject strategicObject;
    expectedArea=&strategicObject;
    ObservedEnvironment environment,otherEnvironment; otherEnvironment.identity=1;
    alternateEnvironment=&otherEnvironment;
    AISingletonServer::m_iaiInformationEnvironment=&environment;
    strategicObject.m_centre={10.0f,20.0f}; returnedPosition={12.0f,25.0f};
    strategicObject.m_index=70;
    InformationManager informationA,informationB;
    firstInformationManager=&informationA; secondInformationManager=&informationB; informationManager=&informationA;
    ObservedInformation informationValues[8];
    ObservedPhysical physicalValues[8],alternatePhysical; alternatePhysical.identity=99;
    replacementPhysical=&alternatePhysical;
    for(int i=0;i<8;++i) {
        physicalValues[i].identity=i; *records[i].physicalSlot=&physicalValues[i];
        handles[i]=informationA.getPool()->acquire(&informationValues[i]);
        require(informationB.getPool()->acquire(&informationValues[i])==handles[i],"parallel native pool handles");
    }
    const unsigned nullRecord=informationA.getPool()->acquire(nullptr);
    informationB.getPool()->set(handles[1],&informationValues[2]);
    ObservedMain main,otherMain; otherMain.identity=1; replacementMain=&otherMain; IAIMain::m_instance=&main;
    constructingSAI=0; SAI firstSAI(1,0,0,Vec2(),Vec2());
    constructingSAI=1; SAI secondSAI(2,0,0,Vec2(),Vec2()); selectedSAI=&firstSAI;
    ObservedBotManager botManager,otherBotManager; otherBotManager.identity=1;
    botManagers[0]=&botManager; botManagers[1]=&otherBotManager; IBotManager::instance=&botManager;
    ObservedBot plannedBot,otherPlannedBot; plannedBot.identity=1; otherPlannedBot.identity=2;
    plannedBots[0]=&plannedBot; plannedBots[1]=&otherPlannedBot;
    raw= mode=="middle" ? 0x80000000u : (mode=="last"||mode=="live_end"||mode=="duplicates" ? 0xFFFFFFFFu : 0u);
    templates={all[0],all[1],all[2]};
    int expected= mode=="middle" ? 1 : (mode=="last" ? 2 : 0);
    std::string expectedCalls="templates0,type0:1,type0:2,type1:1,type1:2,type2:1,type2:2";
    if(mode=="single"||mode=="refill"||mode=="negative_index"||policy()||mode=="second_type"||mode=="reload"||mode=="live_end") {
        templates={all[0]}; expectedCalls="templates0,type0:1,type0:2";
    }
    if(mode=="excluded") {
        types[0]=5; templates={nullptr,all[0],nullptr,all[1],nullptr}; expected=1;
        expectedCalls="templates0,type0:1,type1:1,type1:2";
    }
    if(mode=="duplicates") { types[1]=0; raw=0x80000000u; expected=0; }
    if(mode=="reload") { expected=1; expectedCalls="templates0,type0:1,type1:1"; }
    if(mode=="second_type") expected=5;
    if(mode=="live_end") { expected=1; expectedCalls="templates0,type0:1,type0:2,type1:1,type1:2"; }
    AIRandom random,otherRandom; AIRandom::m_instance=&random;
    if(mode=="refill") *randomIndex=624;
    if(mode=="negative_index") *randomIndex=-1;
    if(policy()) {
        inputPlayerId=static_cast<int>(0x81234567u); inputSide=2; currentKit=5;
        returnMedic=mode=="policy_missing_medic" || mode=="policy_existing_medic" || mode=="policy_fresh_member" ||
                    mode=="policy_manager_capture" || mode=="policy_template_swap" || mode=="policy_retained_squad" ||
                    mode=="policy_retained_player" || mode=="policy_object_medic";
        objectPresent=mode.compare(0,14,"policy_object_")==0 || score();
        initialSquad=mode!="policy_fallback_no_squad" && mode!="policy_object_no_squad_stop" && mode!="policy_score_shared_no_squad";
        assigned=mode=="policy_assigned_empty_stop" || mode=="policy_assigned_roster_stop" || mode=="policy_score_shared_assigned_stop";
        expectedFallback=objectPresent && !returnMedic;
        if(mode=="policy_object_scan_enemy_types_stop") { expectedObjectResult=6; expectedFallback=false; }
        if(mode=="policy_object_scan_health_values_stop") { expectedObjectResult=4; expectedFallback=false; }
        hasMedic=mode=="policy_existing_medic" || mode=="policy_fallback_other_kit" ||
                 mode=="policy_fallback_wide_kit" || mode=="policy_retained_player";
        if(!initialSquad) lookupResult=nullptr;
        freshMember.m_isSquadLeader=!returnMedic && !hasMedic;
        if(mode=="policy_fresh_member") firstMember.m_isSquadLeader=true;
        if(mode=="policy_fallback_other_kit") currentKit=4;
        if(mode=="policy_fallback_wide_kit") currentKit=0x105;
        if(mode=="policy_template_swap") expectedSquadManager=managerB;
        if(mode=="policy_roster" || mode=="policy_assigned_roster_stop")
            squadA->m_squadMembers={rosterMembers[0],rosterMembers[1],rosterMembers[2]};
        if(mode=="policy_cached_count" || mode=="policy_live_members")
            squadA->m_squadMembers={rosterMembers[0],rosterMembers[1]};
        if(mode=="policy_retained_roster_player" || mode=="policy_retained_roster_squad")
            squadA->m_squadMembers={rosterMembers[0]};
        expectedRosterReads=static_cast<int>(squadA->m_squadMembers.size());
        if(mode=="policy_object_reversed_stop") {
            strategicObject.m_centre={10.0f,-4.0f}; returnedPosition={2.0f,-8.0f};
        }
        if(mode=="policy_object_rounding_stop") {
            strategicObject.m_centre={fromBits(0x3f800001u),fromBits(0x3f800001u)};
            returnedPosition={4.0f,4.0f};
        }
        if(mode=="policy_object_nonfinite_stop") {
            strategicObject.m_centre={0.0f,0.0f};
            returnedPosition={std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()};
        }
        if(mode=="policy_object_side_one_stop") inputSide=1;
        if(mode=="policy_object_scan_invalid_stop")
            scanHandles={0u,0xffff0000u,handles[0]^0x10000u,nullRecord};
        if(mode=="policy_object_scan_zero_manager_stop") { scanHandles={0u,0xffff0000u}; informationManager=nullptr; }
        if(mode=="policy_object_scan_unoccupied_stop") {
            scanHandles={handles[0]}; records[0].occupied=false; *records[0].physicalSlot=nullptr;
        }
        if(mode=="policy_object_scan_enemy_types_stop" || mode=="policy_object_scan_other_types_stop") {
            const int categories[8]={0,1,2,4,3,-1,(std::numeric_limits<int>::max)(),(std::numeric_limits<int>::min)()};
            for(int i=0;i<8;++i) {
                records[i].strength=categories[i];
                records[i].side=mode=="policy_object_scan_enemy_types_stop"?1:0;
                scanHandles.push_back(handles[i]);
            }
        }
        if(mode=="policy_object_scan_kit13_stop") { scanHandles={handles[0]}; records[0].strength=0; records[0].kit=13; }
        if(mode=="policy_object_scan_health_values_stop") {
            const float healths[5]={fromBits(0x3effffffu),0.5f,fromBits(0x3f000001u),
                std::numeric_limits<float>::quiet_NaN(),-std::numeric_limits<float>::infinity()};
            for(int i=0;i<5;++i) { scanHandles.push_back(handles[i]); records[i].strength=1; records[i].health=healths[i]; }
        }
        if(mode=="policy_object_scan_retained_physical_stop" || mode=="policy_object_scan_main_swap_stop") {
            scanHandles={handles[0]}; records[0].strength=0;
        }
        if(mode=="policy_object_scan_live_end_stop") scanHandles={handles[0]};
        if(mode=="policy_object_scan_manager_swap_stop") scanHandles={handles[0],handles[1]};
        if(mode=="policy_object_scan_map_populated_stop") {
            (*sourceMaps[0])[-1]=std::make_pair(-999,4);
            (*sourceMaps[0])[10]=std::make_pair(1,13); // self skipped before lookup
            (*sourceMaps[0])[20]=std::make_pair(2,13); // matching target, sentinel kit
            (*sourceMaps[0])[30]=std::make_pair(3,4); // no corresponding bot
            (*sourceMaps[0])[40]=std::make_pair(4,3); // mismatched area
            (*sourceMaps[0])[(std::numeric_limits<int>::max)()]=std::make_pair(5,12);
        }
        if(mode=="policy_object_map_copy_stop" || mode=="policy_object_map_manager_stop" || mode=="policy_object_map_dynamic_id_stop") {
            (*sourceMaps[0])[10]=std::make_pair(7,4);
            (*sourceMaps[0])[20]=std::make_pair(8,3);
        }
        if(mode=="policy_object_map_manager_stop") (*sourceMaps[0])[30]=std::make_pair(9,4);
        #include "scoring_setup.inl"
        randomState[1]=stateFor(scoreRaw); randomState[2]=stateFor(0xffffffffu);
        secondRandomState[0]=stateFor(0xffffffffu);
        ObservedBot bot;
        const int result=BotSpawner::findSuitableKit(nullptr,nullptr,nullptr,nullptr,
            objectPresent ? &strategicObject : nullptr,inputSide,&bot);
        require(!assigned,"remaining assigned-squad stop returned");
        if(objectPresent && !returnMedic) {
            require(result==expectedObjectResult,"actual scoring or shared fallback result");
            require(positionReads==1 && rectangleReads==1 && mainReads==1 && deadReads==1,"object scan and map query counts");
            require(managerReads==(expectedFallback?3:2),"scoring borrows entry templates; fallback obtains a fresh vector");
            require(countReads==((expectedFallback && initialSquad)?1:0) && assignmentReads==countReads,"shared roster after only result13");
            require(memberReads==(expectedFallback?(initialSquad?3:2):(initialSquad?2:1)),"direct scoring suppresses final member lookup");
            const bool refill=mode=="policy_score_refill_negative"||mode=="policy_score_refill_captured_vector";
            const bool fresh=mode=="policy_score_fresh_random";
            require(*randomIndex==(refill||fresh?1:1+expectedScoreDraws+(expectedFallback?1:0)),"exact first-owner draw index");
            require(*secondRandomIndex==(fresh?1:0) && refillCalls==(refill?1:0),"fresh/captured random owner and refill count");
            checkMapDestroyed(); require(queryFreed,"information vector released at final caller return");
            std::printf("PASS %s\n",caseName.c_str()); return 0;
        }
        const int expectedResult=returnMedic ? 5 : (mode=="policy_fallback_templates" ? 4 : 2);
        require(result==expectedResult,"actual medic or second-random result replaces discarded entry draw zero");
        expectedCalls="member1,templates0,type0:1,type0:2,templates";
        expectedCalls+=mode=="policy_template_swap" ? "1" : "0";
        expectedCalls+=",player_id,";
        expectedCalls+=mode=="policy_template_swap" ? "lookupB" : "lookupA";
        if(initialSquad) {
            expectedCalls+=",member2,leader2";
            if(!freshMember.m_isSquadLeader) expectedCalls+=",scanA";
            if(hasMedic) expectedCalls+=",player,kit0";
        }
        if(!returnMedic) {
            if(initialSquad) {
                expectedCalls+=",count";
                for(int i=0;i<expectedRosterReads;++i) {
                    const int slot=(mode=="policy_live_members" && i==1) ? 3 : i;
                    expectedCalls+=",index"+std::to_string(i)+",roster_player"+std::to_string(slot)+",kit"+std::to_string(10+slot);
                }
                expectedCalls+=",assigned";
            }
            expectedCalls+=initialSquad ? ",member3" : ",member2";
            expectedCalls+=mode=="policy_fallback_templates" ? ",templates1,type3:1,type3:2,type4:1,type4:2" :
                                                               ",templates0,type1:1,type1:2,type2:1,type2:2";
        }
        require(calls==expectedCalls,"policy exact short-circuit call order and receiver identity");
        require(managerReads==(returnMedic?2:3) && lookupReads==1 && leaderReads==(initialSquad?1:0),"policy lookup counts");
        require(scanReads==((initialSquad && !freshMember.m_isSquadLeader)?1:0),"only nonleader scans for medic");
        require(playerReads==(hasMedic?1:0) && kitReads==expectedRosterReads+(hasMedic?1:0),"current and roster kit query counts");
        require(countReads==((!returnMedic && initialSquad)?1:0) && assignmentReads==countReads,"one count then assignment after roster");
        require(*randomIndex==(returnMedic?1:2) && refillCalls==0,"medic consumes one draw, fallback consumes two");
        require(positionReads==0 && rectangleReads==0,"medic and null-object returns perform no rectangle query");
        std::printf("PASS %s\n",caseName.c_str());
        return 0;
    }
    const int result=BotSpawner::GetRandomKit(expectedMember);
    require(result==expected,"selected type including wide upper boundary");
    require(calls==expectedCalls,"callback order/repeated getter/live end");
    require(managerReads==1,"borrow one vector without refreshing its manager");
    require(*randomIndex==1 && refillCalls==((mode=="refill"||mode=="negative_index") ? 1 : 0),"one draw and unsigned refill boundary");
    std::printf("PASS %s\n",caseName.c_str());
}
