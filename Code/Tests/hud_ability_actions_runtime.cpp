// [TEMP] [CHOSEN, not recovered] Native HUD ability actions and vehicle query.
// No recovered action/validator is shadowed. launchArtillery is an explicitly
// observed unresolved dependency of the artillery wrapper. Map is never built:
// the presence token below is only compared to null, never dereferenced.
#pragma push_macro("main")
#undef main
#define main unusedPrimaryActionsRegression
#include "hud_primary_actions_runtime.cpp"
#undef main
#pragma pop_macro("main")
#include <array>
#include <cmath>
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Game/Objects/Object.h"
#include "../BF2/Game/Objects/ObjectTemplate.h"
#include "../BF2/Game/Objects/Game/SupplyDropManager.h"
#include "../BF2/Scene/Object/ObjectManager.h"
#include "../BF2/Scene/Object/ObjectPredicator.h"
#include "../BF2/Scene/Object/BinTreeCuller.h"
#include "../BF2/Scene/Object/QuadTreeCuller.h"
#include "../BF2/Game/GameEvents/UAVEvent.h"
#include "../BF2/Game/GameEvents/DropVehicleEvent.h"

namespace dice { namespace hfe {
ClassManager* classManager = nullptr;
System* g_system = nullptr;
namespace world {
ObjectManager* objectManager = nullptr;
SupplyDropManager* supplyDropManager = nullptr;
}
} }

struct ActionClock : System {
    double exact = 100;
    double getExactTime() override {return exact;}
};
struct AbilityActionHud : CommandHud {
    bool launch = false;
    unsigned launches = 0, positionsRead = 0;
    std::deque<Vec2> positions;
    std::vector<int> help;
    bool launchArtillery(bool silent) override {check(!silent,"artillery wrapper is nonsilent");++launches;return launch;}
    Vec2 getCommanderPosition(bool relative) override {
        check(!relative,"ability selects absolute commander position");++positionsRead;
        if(positions.empty())actionUnsupported("missing commander position reply");
        Vec2 result=positions.front();positions.pop_front();return result;
    }
    void enableVOHelpMessage(int id) override {help.push_back(id);}
    void disableVOHelpMessage(int id) override {help.push_back(-id);}
};
struct AbilityActionGame : CommandGame {
    std::vector<GameEvent*> events;
    void* queryInterface(unsigned iid) const override {check(iid==IID_IGameServer,"supply checks server interface");return nullptr;}
    void sendGameEvent(unsigned id,GameEvent* event) override {
        check(id==0xffffffffu,"ability client event destination");events.push_back(event);trace.push_back("send");
    }
    void clearEvents() {for(auto* event:events)delete event;events.clear();}
};
struct AbilityActionLogic : Logic {
    bool server = false;
    unsigned invalidReads = 0;
    std::list<std::pair<std::string,bool>> invalid;
    std::function<void()> onInvalid;
    bool isServer() const override {return server;}
    std::list<std::pair<std::string,bool>>* GetInvalidVehicleDropObjectList() override {
        ++invalidReads;if(onInvalid)onInvalid();return &invalid;
    }
};
struct AbilityActionEvents : EventManager {
    std::vector<GameEvent*> posted;
    unsigned postEventNextUpdate(EventCategory category,unsigned id,void* data) override {
        check(category==ECGameLogic&&id==15,"server ability event category and id");
        posted.push_back(static_cast<GameEvent*>(data));trace.push_back("post");return 0;
    }
    void clearEvents(){for(auto* event:posted)delete event;posted.clear();}
};
struct AbilityActionSquads : CommandSquads {
    struct Entry {int team,squad,order,kind;};
    std::vector<Entry> accepted;
    std::function<void()> onAccepted;
    bool setIsAccepted(int team,int squad,bool acceptedValue,OrderType order,SquadOrderType kind,bool local) override {
        check(!acceptedValue&&!local,"ability clears acceptance without local flag");
        accepted.push_back({team,squad,static_cast<int>(order),static_cast<int>(kind)});
        if(onAccepted)onAccepted();return false;
    }
};
struct AbilityClasses : ClassManager {
    IBase* createInstance(unsigned cid,unsigned iid,IBase* outer) override {
        if(cid==world::CID_BinTreeCuller)return world::BinTreeCuller::create(iid,outer);
        if(cid==world::CID_QuadTreeCuller)return world::QuadTreeCuller::create(iid,outer);
        actionUnsupported("ability fixture factory");
    }
};
struct AbilityObjects : world::ObjectManager {
    std::vector<world::IObject*> hits;
    std::vector<int> lods;
    std::vector<Vec3> origins;
    std::function<void()> onQuery;
    void setOverrideLineTestLod(int value) override {lods.push_back(value);}
    void getObjectsFromGrid(const Vec3& origin,const Vec3& direction,
                            std::vector<world::IObject*>& out,world::ObjectPredicator* predicate,
                            bool mesh,int lod,bool first) override {
        check(origin.y==2000&&direction.x==0&&direction.y==-4500&&direction.z==0,"vehicle validation downward ray");
        check(out.empty()&&predicate->getFlags()==0x100&&mesh&&lod==0&&!first,"vehicle query contract");
        origins.push_back(origin);out=hits;if(onQuery)onQuery();
    }
};
struct AbilityObject : world::Object {
    AbilityObject():Object(nullptr){}
    void setTemplateForTest(const world::IObjectTemplate* value){m_template=value;}
    void setParentForTest(world::IObject* value){m_parent=value;m_cachedRootParent=nullptr;m_flags&=~0x01000000u;}
};
struct AbilityTemplate : world::ObjectTemplate {
    std::string name="blocked";
    mutable unsigned names=0;
    std::function<void()> onName;
    void addRef() override {actionUnsupported("template addRef");}
    int getRef() override {actionUnsupported("template getRef");}
    int release() override {actionUnsupported("template release");}
    void* queryInterface(unsigned) const override {actionUnsupported("template query");}
    world::IObject* createObject() const override {actionUnsupported("template createObject");}
    const std::string& getName() const override {++names;if(onName)onName();return name;}
};

static void checkSquads(const AbilityActionSquads& squads,int kind,int firstTeam) {
    check(squads.accepted.size()==10,"ability visits exactly ten squads");
    bool ordered=true;
    for(unsigned i=0;i<squads.accepted.size();++i) {
        const auto& entry=squads.accepted[i];
        ordered=ordered&&entry.squad==static_cast<int>(i)&&entry.order==2&&entry.kind==kind&&entry.team==firstTeam+static_cast<int>(i);
    }
    check(ordered,"ability repeats team callback and visits squad0 through9");
}

int main() {
    setvbuf(stdout,nullptr,_IONBF,0);
    std::puts("Ability action fixture: native construction");
    AbilityActionEvents events;eventManager=&events;
    Players players;world::playerManager=&players;
    CommandPlayer player;players.local=&player;
    AbilityActionGame current;game=&current;
    AbilityActionLogic logic;gameLogic=&logic;
    AbilityActionSquads squads;squadManager=&squads;
    Sound sound;g_bfSound=&sound;
    ActionClock clock;g_system=&clock;
    void* storage=::operator new(sizeof(AbilityActionHud));
    auto* hud=new(storage)AbilityActionHud;hudManager=hud;
    HudCommander commander(nullptr);hud->m_field13c=&commander;
    hud->m_field0f8=nullptr;
    std::puts("Ability action fixture: wrapper and guards");

    hud->artillery();
    check(hud->launches==1&&sound.played==std::vector<std::string>{"mapAction"}&&hud->help.empty(),"failed artillery launch still plays sound");
    hud->launch=true;players.local=nullptr;hud->artillery();
    check(hud->help==std::vector<int>{-16}&&sound.played.size()==2&&squads.accepted.empty(),"successful artillery dismisses help even without player");
    players.local=&player;player.team=20;squads.onAccepted=[&]{++player.team;};hud->artillery();checkSquads(squads,0,20);
    squads.accepted.clear();squads.onAccepted=nullptr;player.team=2;

    unsigned queries=players.queries;
    check(!hud->supplies(false)&&!hud->uav(false)&&!hud->vehicleDrop(false)&&players.queries==queries,"null map guards precede player query");
    hud->commander=false;hud->mode=7;check(!hud->satellite(false)&&hud->mode==7,"hidden commander ignores satellite");
    hud->commander=true;players.local=nullptr;check(!hud->satellite(false)&&hud->mode==15,"satellite mode change precedes missing-player gate");
    players.local=&player;hud->setSatelliteEnabled(false);current.notice.clear();
    check(!hud->satellite(true)&&current.notice=="satellite_unavailable","satellite unavailable ignores silent");
    hud->setSatelliteEnabled(true);hud->setSatelliteAvailability(std::numeric_limits<float>::quiet_NaN());current.notice.clear();
    check(!hud->satellite(true)&&current.notice=="satellite_unavailable","unordered satellite availability is unavailable");
    hud->setSatelliteAvailability(1);current.notice.clear();check(!hud->satellite(true)&&current.notice.empty(),"silent satellite busy arm is quiet");
    check(!hud->satellite(false)&&current.notice=="satellite_in_use","missing map satellite is busy");

    // [TEMP] One byte supplies only a non-null address for methods whose Map
    // dependence is exclusively a pointer-presence gate. No Map field/vptr read.
    unsigned char mapPresenceToken;
    hud->m_field0f8=reinterpret_cast<HudMap*>(&mapPresenceToken);
    hud->setUAVEnabled(false);current.notice.clear();
    check(!hud->uav(true)&&current.notice=="uav_unavailable","UAV unavailable ignores silent");
    hud->setUAVEnabled(true);hud->setUAVAvailability(std::numeric_limits<float>::quiet_NaN());current.notice.clear();
    check(!hud->uav(true)&&current.notice=="uav_unavailable","unordered UAV is unavailable before map read");
    hud->setVehicleDropAvailability(0);queries=players.queries;check(!hud->vehicleDrop(false)&&players.queries==queries,"drop availability zero precedes player query");
    hud->setVehicleDropAvailability(std::numeric_limits<float>::quiet_NaN());check(!hud->vehicleDrop(false)&&players.queries==queries,"unordered drop availability is rejected");
    hud->setVehicleDropAvailability(1);commander.m_vehicleDropReloading=true;current.notice.clear();
    check(!hud->vehicleDrop(true)&&current.notice.empty(),"silent busy drop is quiet");
    check(!hud->vehicleDrop(false)&&current.notice=="vehicledrop_in_use","busy drop notice");
    commander.m_vehicleDropReloading=false;hud->positions={{-4000,-4000}};current.notice.clear();
    check(!hud->vehicleDrop(false)&&current.notice=="vehicledrop_in_use","sentinel drop position is busy");
    commander.m_supplyReloading=true;current.notice.clear();
    check(!hud->supplies(true)&&current.notice.empty(),"silent busy supplies is quiet");
    check(!hud->supplies(false)&&current.notice=="supplies_in_use","busy supplies notice");
    commander.m_supplyReloading=false;hud->positions={{-4000,-4000}};
    check(!hud->supplies(true),"supply sentinel fails before request");

    // Construct query objects before publishing ObjectManager so Object's real
    // constructor takes its own supported null-manager branch.
    std::puts("Ability action fixture: native object query");
    auto* root=new AbilityObject;auto* child=new AbilityObject;auto* objectTemplate=new AbilityTemplate;
    child->setParentForTest(root);root->setTemplateForTest(objectTemplate);
    auto* classes=new AbilityClasses;classManager=classes;
    auto* objects=new AbilityObjects;world::objectManager=objects;
    check(hud->isVehicleDropPositionValid(Vec3{3,99,4})&&objects->lods==std::vector<int>{1,-1},"empty query resets LOD and accepts");
    objects->hits={nullptr,child};logic.invalid={{"other",true},{"blocked",false}};objects->lods.clear();
    check(hud->isVehicleDropPositionValid(Vec3{3,99,4})&&objectTemplate->names==2&&logic.invalidReads==1,"query skips null object and compares each root template name");
    logic.invalid.push_back({"blocked",true});objects->lods.clear();
    check(!hud->isVehicleDropPositionValid(Vec3{3,99,4})&&objects->lods==std::vector<int>{1},"blocked drop leaves LOD override at1 as shipped");
    logic.invalid={{std::string("blocked\0suffix",14),true}};objects->lods.clear();
    check(hud->isVehicleDropPositionValid(Vec3{3,99,4}),"invalid template match uses complete native string length");
    logic.invalid={{"blocked",true}};*reinterpret_cast<unsigned char*>(&logic.invalid.front().second)=2;
    check(hud->isVehicleDropPositionValid(Vec3{3,99,4}),"invalid template flag must be byte exactly1");
    *reinterpret_cast<unsigned char*>(&logic.invalid.front().second)=1;
    logic.invalid={{"other",true},{"blocked",true}};
    objectTemplate->onName=[&]{root->setTemplateForTest(nullptr);};objects->lods.clear();
    check(hud->isVehicleDropPositionValid(Vec3{3,99,4}),"template pointer is reread after each name callback");
    objectTemplate->onName=nullptr;root->setTemplateForTest(objectTemplate);

    std::puts("Ability action fixture: drop and supply submission");
    logic.invalid={{"blocked",true}};hud->positions={{1,2},{3,4},{5,6}};
    const auto soundsBefore=sound.played.size();objects->lods.clear();
    check(!hud->vehicleDrop(false)&&objects->origins.back().x==5&&objects->origins.back().z==4,"drop validation combines two fresh position callbacks");
    check(sound.played.size()==soundsBefore+1&&!commander.m_vehicleDropReloading&&current.events.empty(),"invalid drop plays sound without starting cooldown or sending event");
    logic.invalid.clear();hud->positions={{std::numeric_limits<float>::quiet_NaN(),-4000},{3,4},{5,6},{7,8}};
    WorldPref::resetWorldTime(100);commander.m_vehicleDropDuration=20;
    squads.onAccepted=[&]{++player.team;};player.team=2;squads.accepted.clear();
    check(hud->vehicleDrop(false)&&commander.m_vehicleDropReloading&&commander.m_vehicleDropRemaining==20&&commander.m_vehicleDropDeadline==120,"unordered position passes sentinel and starts vehicle cooldown");
    check(current.events.size()==1&&current.events[0]->getType()==51,"drop hands off real vehicle event");checkSquads(squads,2,2);
    current.clearEvents();squads.accepted.clear();squads.onAccepted=nullptr;player.team=2;
    commander.m_vehicleDropReloading=false;logic.server=true;hud->positions={{1,2},{3,4},{5,6},{7,8}};
    check(hud->vehicleDrop(true)&&events.posted.size()==1&&events.posted[0]->getType()==51,"server drop posts next-update event");events.clearEvents();logic.server=false;

    world::supplyDropManager->reset();clock.exact=10;hud->positions={{1,2},{3,4}};hud->help.clear();squads.accepted.clear();
    check(!hud->supplies(false)&&!commander.m_supplyReloading&&current.events.empty()&&hud->help.empty(),"real supply throttle refusal leaves cooldown and help alone");
    clock.exact=100;hud->positions={{1,2},{3,4}};commander.m_supplyDuration=25;WorldPref::resetWorldTime(200);
    player.team=2;squads.onAccepted=[&]{++player.team;};
    check(hud->supplies(false)&&commander.m_supplyReloading&&commander.m_supplyRemaining==25&&commander.m_supplyDeadline==225,"accepted real supply request starts cooldown");
    check(current.events.size()==1&&current.events[0]->getType()==37&&hud->help==std::vector<int>{-17},"supply relays its real event and dismisses help17");checkSquads(squads,3,2);current.clearEvents();

    // The client x87 expression can round differently from an intermediate
    // double sum. PC64 is local to this check and is restored afterwards.
    unsigned savedControl=0,ignoredControl=0;
    _controlfp_s(&savedControl,0,0);_controlfp_s(&ignoredControl,_PC_64,_MCW_PC);
    squads.onAccepted=nullptr;player.team=2;
    const double midpoint=1.0+std::ldexp(1.0,-24);
    const float tiny=std::ldexp(1.0f,-60);
    WorldPref::resetWorldTime(midpoint);commander.m_vehicleDropReloading=false;
    commander.m_vehicleDropDuration=tiny;hud->positions={{1,2},{3,4},{5,6},{7,8}};
    check(hud->vehicleDrop(true)&&commander.m_vehicleDropDeadline==std::nextafter(1.0f,2.0f),"vehicle deadline rounds the unspilled x87 sum");current.clearEvents();
    world::supplyDropManager->reset();commander.m_supplyReloading=false;commander.m_supplyDuration=tiny;
    hud->positions={{1,2},{3,4}};
    check(hud->supplies(true)&&commander.m_supplyDeadline==std::nextafter(1.0f,2.0f),"supply deadline rounds the unspilled x87 sum");current.clearEvents();
    _controlfp_s(&ignoredControl,savedControl,_MCW_PC);

    alignas(UAVEvent) unsigned char uavStorage[sizeof(UAVEvent)];std::memset(uavStorage,0xa5,sizeof(uavStorage));
    auto* scan=new(uavStorage)UAVEvent(0x102,Vec2{11,12},true);
    float x,z;std::memcpy(&x,uavStorage+12,4);std::memcpy(&z,uavStorage+16,4);
    check(sizeof(UAVEvent)==24&&scan->getType()==45&&x==11&&z==12&&uavStorage[20]==2&&uavStorage[21]==1,"client UAV constructor payload and scan flag");scan->~UAVEvent();
    scan=new(uavStorage)UAVEvent(1,Vec2{21,22},false);
    check(uavStorage[21]==0,"UAV producer clears scan selector for drone request");scan->~UAVEvent();
    alignas(DropVehicleEvent) unsigned char dropStorage[sizeof(DropVehicleEvent)];std::memset(dropStorage,0xa5,sizeof(dropStorage));
    auto* drop=new(dropStorage)DropVehicleEvent(0x103,Vec2{13,14});std::memcpy(&x,dropStorage+12,4);std::memcpy(&z,dropStorage+16,4);
    check(sizeof(DropVehicleEvent)==24&&drop->getType()==51&&x==13&&z==14&&dropStorage[20]==3&&dropStorage[21]==0xa5,"client Drop constructor leaves serialized byte15 untouched");drop->~DropVehicleEvent();

    // HUD's wider destructor is still declaration-only. Clean the native members
    // constructed by this fixture; query objects deliberately live to process exit.
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();::operator delete(storage);hudManager=nullptr;
    std::printf("HUD ability actions: %u/%u checks passed\n",checks-failures,checks);return failures?1:0;
}
