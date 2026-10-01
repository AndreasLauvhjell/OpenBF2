// [TEMP] Actual HUD camera bodies, Object/OverheadCameraTemplate lifetimes, and
// CommanderCamEvent construction/execution. Typed observers replace collaborators.
// /FORCE covers only uncalled wider methods; this is not client initialization.
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <new>
#include <string>
#include <vector>
#include "../BF2/Core/Ids.h"
#define private public
#define protected public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Game/GameEvents/CommanderCamEvent.h"
#undef protected
#undef private
#include "../BF2/Game/MapInfo.h"
#include "../BF2/Game/IGame.h"
#include "../BF2/Game/IGameLogic.h"
#include "../BF2/Game/Objects/IPlayer.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/Game/Objects/Object.h"
#include "../BF2/Game/Objects/OverheadCameraTemplate.h"
#include "../BF2/Game/GameLogic/SquadManager/ISquadManager.h"
[[noreturn]] static void actionUnsupported(const char* name) {
    std::fprintf(stderr,"[FRONTIER] hud camera fixture unexpected %s\n",name);
    std::fflush(stderr);std::abort();
}
#include "hud_action_observers.h"
namespace dice { namespace hfe {
EventManager* eventManager=nullptr;
IGameLogic* gameLogic=nullptr;
IGame* game=nullptr;
IBase* squadManager=nullptr;
hud::BF2HudManager* hudManager=nullptr;
namespace ai { IBase* g_aiInstance=nullptr; }
namespace world {
PlayerManager* playerManager=nullptr;
ObjectManager* objectManager=nullptr;
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager=nullptr;
}
} }
using namespace dice::hfe;
using namespace dice::hfe::hud;
static unsigned checks,failures,eventDeletes;
static std::string trace;
static bool trackEvent;
static void* eventAllocation;
static std::function<void()> onAllocation;
void* operator new(std::size_t bytes) {
    void* result=std::malloc(bytes?bytes:1);
    if(!result)throw std::bad_alloc();
    if(trackEvent&&bytes==sizeof(CommanderCamEvent)) {
        trackEvent=false;eventAllocation=result;trace+='A';
        if(onAllocation)onAllocation();
    }
    return result;
}
void operator delete(void* address) noexcept {
    if(address&&address==eventAllocation) {
        eventAllocation=nullptr;++eventDeletes;trace+='Z';
    }
    std::free(address);
}
void operator delete(void* address,std::size_t) noexcept {::operator delete(address);}
static void check(bool value,const char* label) {
    ++checks;
    if(!value){++failures;std::fprintf(stderr,"FAIL %s (trace=%s)\n",label,trace.c_str());}
}
struct Player : world::ActionPlayerBoundary {
    bool enabled=false;
    int id=17,team=2;
    std::function<void()> onId;
    int getKit() const override {actionUnsupported("getKit");}
    int getSquadId() const override {actionUnsupported("getSquadId");}
    int getId() const override {trace+='D';if(onId)onId();return id;}
    int getTeam() const override {trace+='T';return team;}
    bool getOverheadCameraEnabled() const override {trace+='E';return enabled;}
};
struct Players : world::PlayerManager {
    world::IPlayer* local=nullptr;
    world::IPlayer* getLocalHumanPlayer() override {trace+='P';return local;}
};
struct Logic : ActionLogicBoundary {
    bool server=false;
    std::function<void()> onServer;
    bool isServer() const override {trace+='L';if(onServer)onServer();return server;}
};
struct Game : ActionGameBoundary {
    GameEvent* sent=nullptr;
    unsigned target=0;
    void* queryInterface(unsigned iid) const override {
        trace+='Q';check(iid==IID_IGameServer,"server event queries IGameServer");return nullptr;
    }
    bool radioMessage(TargetChannel,const std::string&,world::IPlayer*,unsigned,bool,bool) override {
        actionUnsupported("radioMessage");
    }
    void sendGameEvent(unsigned id,GameEvent* event) override {trace+='G';target=id;sent=event;}
};
struct Squads : ActionSquadBoundary {
    world::IObject* camera=nullptr;
    int queriedTeam=0;
    void* getCommanderCam(int team) override {trace+='C';queriedTeam=team;return camera;}
    bool setSquadName(int,int,int,const std::string&,bool) override {actionUnsupported("setSquadName");}
    void removeFromSquad(int,int,int,bool) override {actionUnsupported("removeFromSquad");}
    void inviteToSquad(int,int,int,bool,bool) override {actionUnsupported("inviteToSquad");}
    int getSquadLeader(int,int) override {actionUnsupported("getSquadLeader");}
    int getCommander(int) override {actionUnsupported("getCommander");}
};
struct CameraObject : world::Object {
    CameraObject():Object(nullptr){}
    ~CameraObject(){m_template=nullptr;m_isObjectDestroyed=true;}
    void use(const world::IObjectTemplate* value){m_template=value;}
};
struct CameraTemplate : world::OverheadCameraTemplate {
    unsigned cid=world::CID_OverheadCameraTemplate;
    std::function<void()> onClass;
    void height(float value){m_height=value;}
    unsigned getClassID() const override {trace+='I';if(onClass)onClass();return cid;}
};
int main() {
    trace.reserve(512);
    Players players;Player player;Logic logic;Game observedGame;Squads squads;
    world::playerManager=&players;gameLogic=&logic;game=&observedGame;squadManager=&squads;
    void* storage=::operator new(sizeof(BF2HudManager));
    auto* hud=new(storage) BF2HudManager;
    hudManager=hud;
    CameraObject camera;CameraTemplate first,second;
    hud->hudMap=nullptr;
    trace.clear();const Vec2 position=hud->getCommanderPosition(false);
    check(position.x==0&&position.y==0&&trace.empty(),"absolute commander position with no map is zero without callbacks");
    trace.clear();check(!hud->zoomCommander()&&trace.empty(),"zoom with no map returns false without player lookup");
    hud->m_field2bc=81.0f;
    trace.clear();check(hud->getCommanderCameraHeight()==81.0f,"absent player preserves cache");
    check(trace=="P","absent player stops before player/global callbacks");
    players.local=&player;
    trace.clear();check(hud->getCommanderCameraHeight()==81.0f,"disabled camera preserves cache");
    check(trace=="PE","disabled camera stops before squad manager");
    player.enabled=true;
    trace.clear();check(hud->getCommanderCameraHeight()==81.0f,"missing commander camera preserves cache");
    check(trace=="PETC"&&squads.queriedTeam==2,"camera lookup reads and forwards player team");
    squads.camera=&camera;camera.use(&first);first.cid=123;
    trace.clear();check(hud->getCommanderCameraHeight()==81.0f,"wrong template class preserves cache");
    check(trace=="PETCI","template identity queried once");
    first.cid=world::CID_OverheadCameraTemplate;
    trace.clear();check(hud->getCommanderCameraHeight()==160.0f,"native template default height used");
    check(trace=="PETCI","valid height callback order");
    first.height(-3.5f);
    trace.clear();check(hud->getCommanderCameraHeight()==-3.5f,"negative height is not clamped");
    second.height(233.0f);first.onClass=[&]{camera.use(&second);};
    trace.clear();check(hud->getCommanderCameraHeight()==233.0f,"template is reread after class callback");
    check(trace=="PETCI","second template is not class-checked again");
    camera.use(&first);first.onClass=[&]{camera.use(nullptr);};
    trace.clear();check(hud->getCommanderCameraHeight()==233.0f,"null second template retains previous height");
    check(trace=="PETCI","null second template reached after class callback");
    first.onClass=nullptr;camera.use(nullptr);
    players.local=nullptr;gameLogic=nullptr;
    trace.clear();check(!hud->setCommanderCameraPosition(Vec2{5,7}),"absent local player returns false");
    check(trace=="P","absent local player does not read gameLogic");
    players.local=&player;gameLogic=&logic;hud->m_field2bc=10;
    logic.onServer=[&]{hud->m_field2bc=20;};
    onAllocation=[&]{hud->m_field2bc=30;};
    player.onId=[&]{hud->m_field2bc=40;};
    trackEvent=true;trace.clear();
    check(hud->setCommanderCameraPosition(Vec2{-2.5f,9999}),"client camera request returns true");
    check(trace=="PLADG","client allocates event before player ID and dispatch");
    auto* event=static_cast<CommanderCamEvent*>(observedGame.sent);
    check(event&&event==eventAllocation,"client dispatch owns actual allocated event");
    check(observedGame.target==17,"client dispatch uses local player ID");
    check(event->m_position.x==-2.5f&&event->m_position.y==30&&event->m_position.z==9999,
          "coordinates preserve input and read height after allocation before ID callback");
    check(event->getType()==36&&event->getRef()==1&&event->m_sendSequence==-1,
          "actual event constructor preserves type and base defaults");
    check(event->release()==0&&eventDeletes==1,"event releases through actual virtual lifetime");
    observedGame.sent=nullptr;onAllocation=nullptr;logic.onServer=nullptr;player.onId=nullptr;
    logic.server=true;trace.clear();trackEvent=true;
    check(hud->setCommanderCameraPosition(Vec2{1,2}),"server caller ignores real executeServer false result");
    check(trace=="PLDQ","server executes real event after player ID without send");
    check(trackEvent&&eventDeletes==1&&!eventAllocation,"server event uses stack lifetime");
    trackEvent=false;
    Vec3 source{4,5,6};CommanderCamEvent copied(source);source.x=90;
    check(copied.m_position.x==4&&copied.m_position.y==5&&copied.m_position.z==6,
          "value constructor copies all three coordinates");
    // [TEMP] The actual HUD constructor runs; wider destructor remains incomplete.
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();
    hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();
    ::operator delete(storage);hudManager=nullptr;
    std::printf("HUD camera: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
