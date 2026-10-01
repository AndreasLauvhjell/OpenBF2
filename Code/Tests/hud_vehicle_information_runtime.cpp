// [TEMP] [CHOSEN, not recovered] Bounded native vehicle-owner regression.
// Constructor substitute retired: actual78C560 registers the three listeners.
// The fixture does not dispatch into the still-unrecovered owner handler.
// The bounded fixture covers null player/kit paths, not live ability playback.
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <new>
#include <set>
#include <string>
#include <vector>
#include "../BF2/Menu/HudVehicleInformation.h"
#define private public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#undef private
#include "../BF2/Menu/HudInformationAdapter.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Game/IGameLogic.h"
#include "../BF2/Game/Objects/IPlayer.h"
#include "../BF2/Scene/Object/PlayerManager.h"
[[noreturn]] static void fixtureUnsupported(const char* name) {
    std::fprintf(stderr,"[FRONTIER] vehicle fixture unexpected %s\n",name);
    std::fflush(stderr);std::abort();
}
#include "hud_kit_observers.h"
namespace dice { namespace hfe {
EventManager* eventManager=nullptr;
IGameLogic* gameLogic=nullptr;
hud::BF2HudManager* hudManager=nullptr;
namespace world {
PlayerManager* playerManager=nullptr;
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager=nullptr;
}
} }
#include "hud_vehicle_storage_fixture.h"
using namespace dice::hfe;
using namespace dice::hfe::hud;
static unsigned checks, failures;
static std::string trace;
static void check(bool condition,const char* label) {
    ++checks;if(!condition){++failures;std::fprintf(stderr,"FAIL %s (trace %s)\n",label,trace.c_str());}
}
namespace dice { namespace hfe { namespace hud {
struct HudVehicleInformationFixture {
struct Players : world::PlayerManager {
    world::IPlayer* local=nullptr;
    world::IPlayer* getLocalHumanPlayer() override {trace+='P';return local;}
};
struct Logic : LogicBoundary {
    int status=4;
    int getGameStatus() override {trace+='G';return status;}
    void suicide() override {fixtureUnsupported("suicide");}
    bool selectTeam(world::IPlayer*,int) override {fixtureUnsupported("selectTeam");}
    bool selectKitAndUnlockLevel(world::IPlayer*,int,int) override {fixtureUnsupported("selectKit");}
};
struct Player : world::PlayerBoundary {
    bool getIsAlive() const override {fixtureUnsupported("getIsAlive");}
    int getTeam() const override {fixtureUnsupported("getTeam");}
    int getSquadId() const override {fixtureUnsupported("getSquadId");}
    bool getIsSquadLeader() const override {fixtureUnsupported("getIsSquadLeader");}
    int getKit() const override {fixtureUnsupported("getKit");}
    int getChosenUnlockLevel(int,int) const override {fixtureUnsupported("getChosenUnlockLevel");}
    void setChosenUnlockLevel(int,int,int) override {fixtureUnsupported("setChosenUnlockLevel");}
    bool isUnlocked(int,int,int) override {fixtureUnsupported("isUnlocked");}
};
struct Adapter : HudInformationAdapter {
    bool setBool(std::string name,bool value) override {
        trace+='B';check(name=="CloseToMine"&&!value,"reset clears exact adapter key");return false;
    }
};
struct Hud : BF2HudManager {
    Adapter* adapter=nullptr;
    world::Kit* getPlayerKit(world::IPlayer* player) override {
        trace+='K';check(player!=nullptr,"nonnull local player passed through");return nullptr;
    }
    HudInformationAdapter* getInformationAdapter() override {trace+='A';return adapter;}
};
struct Events : EventManager {
    HudVehicleInformation* vehicle=nullptr;
    std::vector<EventCategory> removed;
    bool unregisterEventHandler(EventCategory category,IEventListener* listener) override {
        if (!vehicle) return EventManager::unregisterEventHandler(category,listener);
        removed.push_back(category);
        check(listener==static_cast<IEventListener*>(vehicle),"destructor unregisters adjusted listener");
        check(vehicle->m_occupantX[0]==0&&vehicle->m_occupantY[9]==0,"occupant positions clear before unregister");
        check(vehicle->m_occupantNames[0]==nullptr,"owned occupant string freed and cleared before unregister");
        check(vehicle->m_connectionMessage=="still alive"&&vehicle->m_warningSources.size()==2,"native containers survive until after callbacks");
        return EventManager::unregisterEventHandler(category,listener);
    }
};
static int run() {
    std::fprintf(stderr,"[TEMP] HudVehicleInformation native constructor; no owner event dispatch\n");
    static_assert(sizeof(void*)==4,"x86 required");
    Events events;eventManager=&events;
    Players players;world::playerManager=&players;
    Logic logic;gameLogic=&logic;
    Adapter adapter;
    Hud* hud=new Hud;hud->adapter=&adapter;hudManager=hud;
    HudVehicleInformation* v=new HudVehicleInformation;events.vehicle=v;
    check(reinterpret_cast<char*>(static_cast<IEventListener*>(v))-reinterpret_cast<char*>(v)==4,"listener remains secondary +4");
    check(reinterpret_cast<char*>(&v->m_gunnerAngle)-reinterpret_cast<char*>(&v->m_playerHealth)==0x38,"intermediate 1DC float preserves gunner position");
    check(reinterpret_cast<char*>(&v->m_warningSources)-reinterpret_cast<char*>(&v->m_playerHealth)==0xB0,"flag region preserves warning-set placement relative to health");
    check(reinterpret_cast<char*>(&v->m_field2ac)-reinterpret_cast<char*>(&v->m_referenceCrossShow)==0x48,"post-set scalar suffix span");
    HudVehicleInformationPrimary* primary=v;
    auto* system=reinterpret_cast<dice::meme::System*>(0x1234);
    primary->setSystem(system);check(v->m_system==system,"primary setter stores borrowed system");
    check(primary->refresh(),"retail refresh returns true");
    primary->unknown0C();primary->unknown10();primary->unknown14();primary->unknown18();
    v->m_guiIndex=-17;check(primary->getGuiIndex()==-17,"primary index returns signed field");
    v->m_field170=2;v->m_field294=true;v->m_killedState=true;
    v->m_playerHealth=8;v->m_vehicleArmor=9;v->m_field0a4=3;
    v->m_vehiclePassengersShow=v->m_turretIconShow=v->m_usingParachute=true;
    v->m_closeToMedic=v->m_closeToRepair=v->m_closeToAmmo=true;
    trace.clear();v->reset(true);
    check(trace=="PPPAB","keep-state reset repeats three local queries before adapter");
    check(v->m_killedState&&v->m_playerHealth==8&&v->m_vehicleArmor==9,"keep-state reset preserves health and killed fields");
    check(v->m_field170==0&&!v->m_field294&&v->m_guiIndex==0&&v->m_field0a4==0,"reset scalar writes");
    check(!v->m_vehiclePassengersShow&&!v->m_turretIconShow&&!v->m_usingParachute,"reset visible flags");
    check(v->m_closeToMedic&&v->m_closeToRepair&&v->m_closeToAmmo,"absent player preserves cached proximity flags");
    trace.clear();v->reset(false);
    check(trace=="GPPPAB","state query precedes independent ability lookups");
    check(!v->m_killedState&&v->m_playerHealth==0&&v->m_vehicleArmor==0,"status four clears killed/health/armor");
    logic.status=3;trace.clear();v->reset(false);check(v->m_killedState,"other status sets killed state");
    Player player;players.local=&player;
    trace.clear();v->reset(true);check(trace=="PKPKPKAB","reset repeats full player and kit query when kits absent");
    using Setter=void(HudVehicleInformation::*)(bool);
    const Setter setters[]={&HudVehicleInformation::setCloseToMedic,&HudVehicleInformation::setCloseToRepair,&HudVehicleInformation::setCloseToAmmo};
    for(auto setter:setters) {
        players.local=nullptr;trace.clear();(v->*setter)(false);check(trace=="P","proximity setter stops on absent player");
        players.local=&player;trace.clear();(v->*setter)(false);check(trace=="PK","proximity setter stops on absent kit");
    }
    check(v->m_closeToMedic&&v->m_closeToRepair&&v->m_closeToAmmo,"absent kit preserves flags for all setters");
    v->m_warningSources={3,-4,8};v->m_field020=v->m_usingParachute=true;v->m_field018=7;v->m_field01c=8;
    v->m_field021=true;v->m_field024=9;v->clear();
    check(v->m_warningSources.empty()&&!v->m_field020&&!v->m_usingParachute&&v->m_field018==0&&v->m_field01c==0,"clear empties native tree and timed state");
    check(v->m_field021&&v->m_field024==9&&v->m_closeToMedic,"clear preserves unrelated flags");
    v->m_connectionMessage="still alive";v->m_warningSources={1,2};
    for(int i=0;i!=10;++i){v->m_occupantX[i]=i+1;v->m_occupantY[i]=i+2;}
    v->m_occupantNames[0]=static_cast<char*>(::operator new(5));
    v->m_localKitName=new char[2];v->m_vehicleName=new char[2];v->m_vehicleIconPath=new char[2];
    v->m_vehicleArmorString=new char[2];v->m_playerHealthString=new char[2];
    v->m_speedString=new char[2];v->m_altitudeString=new char[2];v->m_torqueString=new char[2];
    v->m_warningIconPath=new char[2];v->m_squadInfoText=new wchar_t[2];v->m_squadInfoIconPath=new char[2];
    delete v;
    check(events.removed==std::vector<EventCategory>({ECGame,ECGameLogic,ECHud}),"destructor unregister category order");
    events.vehicle=nullptr;
    // Wider HUD destructor remains outside this bounded fixture; destroy its native members only.
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();
    hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();::operator delete(hud);
    std::printf("HudVehicleInformation: %u/%u checks passed; native x86 size %u\n",checks-failures,checks,unsigned(sizeof(HudVehicleInformation)));
    std::fflush(stdout);
    return failures?1:0;
}
};
} } }
int main() {return HudVehicleInformationFixture::run();}
