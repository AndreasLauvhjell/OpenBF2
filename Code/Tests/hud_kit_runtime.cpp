// [TEMP] Real HudKitSelection bodies with explicit player/game/HUD collaborators.
// No production method is shadowed. Uncalled map/question bodies remain /FORCE
// frontiers; the original full HUD destructor is not used for fixture teardown.
#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <new>
#include <string>
#include <type_traits>
#include <vector>
#define private public
#include "../BF2/Menu/HudKitSelection.h"
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#undef private
#include "../BF2/Menu/HudInformationAdapter.h"
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Game/IGameLogic.h"
#include "../BF2/Game/Objects/IPlayer.h"
#include "../BF2/Scene/Object/PlayerManager.h"
[[noreturn]] static void fixtureUnsupported(const char* name) {
    std::fprintf(stderr, "[FRONTIER] hud kit fixture unexpected %s\n", name);
    std::fflush(stderr); std::abort();
}
#include "hud_kit_observers.h"
namespace dice { namespace hfe {
EventManager* eventManager = nullptr;
IGameLogic* gameLogic = nullptr;
IBase* g_hudInformationLayer = nullptr;
hud::BF2HudManager* hudManager = nullptr;
namespace world {
PlayerManager* playerManager = nullptr;
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager = nullptr;
}
} }
using namespace dice::hfe;
using namespace dice::hfe::hud;
static unsigned checks, failures;
static std::string trace;
static void check(bool value, const char* message) {
    ++checks;
    if (!value) { ++failures; std::fprintf(stderr,"FAIL %s (trace=%s)\n",message,trace.c_str()); }
}
struct PlayerObserver : world::PlayerBoundary {
    bool alive=false, leader=false, unlocked=true;
    int kit=2, chosen=0, squad=0, team=1;
    mutable std::vector<int> teamReads;
    mutable unsigned teamIndex=0;
    mutable std::vector<int> chosenArgs;
    std::vector<int> unlockedArgs, storeArgs;
    bool getIsAlive() const override { trace+='A'; return alive; }
    bool getIsSquadLeader() const override { trace+='L'; return leader; }
    int getKit() const override { trace+='K'; return kit; }
    int getTeam() const override {
        trace+='T'; return teamIndex<teamReads.size()?teamReads[teamIndex++]:team;
    }
    int getSquadId() const override { trace+='S'; return squad; }
    int getChosenUnlockLevel(int k,int t) const override { trace+='C'; chosenArgs={k,t}; return chosen; }
    bool isUnlocked(int k,int t,int n) override { trace+='U'; unlockedArgs={k,t,n}; return unlocked; }
    void setChosenUnlockLevel(int k,int t,int n) override { trace+='W'; storeArgs={k,t,n}; }
};
struct Players : world::PlayerManager {
    world::IPlayer* result=nullptr;
    std::vector<world::IPlayer*> script;
    unsigned calls=0;
    std::function<void()> callback;
    world::IPlayer* getLocalHumanPlayer() override {
        trace+='P'; ++calls; if(callback)callback();
        return calls<=script.size()?script[calls-1]:result;
    }
};
struct Logic : LogicBoundary {
    std::vector<int> selection;
    world::IPlayer* selectedPlayer=nullptr;
    std::function<void()> callback;
    void suicide() override { trace+='X'; if(callback)callback(); }
    bool selectKitAndUnlockLevel(world::IPlayer* p,int kit,int unlock) override {
        trace+='G'; selectedPlayer=p; selection={kit,unlock}; if(callback)callback(); return false;
    }
    bool selectTeam(world::IPlayer* p,int team) override {
        trace+='R'; selectedPlayer=p; selection={team}; if(callback)callback(); return false;
    }
};
struct Adapter : HudInformationAdapter {
    unsigned writes=0;
    std::function<void()> callback;
    bool getBool(std::string key) override {
        trace+='b'; return HudInformationAdapter::getBool(key);
    }
    bool setBool(std::string key,bool value) override {
        trace+='B'; ++writes; if(callback)callback();
        return HudInformationAdapter::setBool(key,value);
    }
};
struct Information : HudInformationLayer {
    std::function<void()> callback;
    int updatedTeam=-1;
    void updateKits(int team) override { trace+='I'; updatedTeam=team; if(callback)callback(); }
    void setDeathMessage(std::wstring text) override {
        trace+='D'; HudInformationLayer::setDeathMessage(text);
    }
};
struct Hud : BF2HudManager {
    int modeArgument=-1; bool save=false;
    std::function<void()> callback;
    void setMode(int mode,bool remember) override {
        trace+='M'; modeArgument=mode; save=remember; if(callback)callback();
    }
    void leaveSquad() override { trace+='Q'; if(callback)callback(); }
    HudQuestionManager* getQuestionManager() override { trace+='?'; return nullptr; }
    void disableVOHelpMessage(int id) override { trace+='V'; check(id==61,"unlock disables help id61"); }
    std::wstring getLocalizedString(std::string key) override {
        trace+='N'; check(key=="HUD_TEXT_MENU_COMMITTED_SUICIDE","suicide localization id"); return L"suicide";
    }
};
struct EventSink : IEventListener {
    void handleEvent(EventCategory category,unsigned id,EventNode*,void* data) override {
        trace+='E'; check(category==ECHud&&id==2&&!data,"suicide synchronous HUD event payload");
    }
};
static unsigned long nullPlayersFault(HudKitSelection* selection) {
    __try { selection->selectKit(1); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
    return 0;
}
static unsigned long nullMapSpawnFault(HudKitSelection* selection,int group) {
    __try { selection->setSpawnGroup(group); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
    return 0;
}
int main() {
    static_assert(sizeof(void*)==4,"native x86 only");
    static_assert(!std::is_polymorphic<HudKitSelection>::value,"nonvirtual kit family");
    static_assert(sizeof(HudKitSelection)==36,"retail scalar layout");
    alignas(HudKitSelection) unsigned char bytes[sizeof(HudKitSelection)];
    std::memset(bytes,0x5A,sizeof(bytes));
    HudKitSelection* selection=new(bytes) HudKitSelection;
    check(bytes[6]==0x5A&&bytes[7]==0x5A,"constructor preserves alignment padding");
    check(selection->m_pendingTeam==-1&&!selection->m_teamChangePending&&!selection->m_field005,
          "constructor initializes pending state");
    check(selection->m_selectedTeam==2,"constructor selects team2");
    for(int i=0;i!=3;++i) check(selection->m_selectedKit[i]==2&&selection->m_selectedSpawnGroup[i]==0,"constructor arrays");
    selection->m_pendingTeam=51; selection->m_field005=true; selection->m_selectedTeam=1;
    selection->m_selectedKit[1]=6; selection->m_teamChangePending=true;
    selection->reset();
    check(selection->m_pendingTeam==51&&selection->m_field005&&selection->m_selectedTeam==1&&
          selection->m_selectedKit[1]==6&&!selection->m_teamChangePending,"reset only clears pending flag and spawn groups");
    for(int i=0;i!=3;++i)check(selection->m_selectedSpawnGroup[i]==-1,"reset spawn sentinel");
    EventManager events; eventManager=&events;
    {
        Information information; g_hudInformationLayer=&information;
        Adapter adapter;
        events.unregisterEventHandler(ECHud,&information);
        events.unregisterEventHandler(ECHud,&adapter);
        EventSink eventSink; events.registerEventHandler(ECHud,&eventSink,0);
        Players players; world::playerManager=&players;
        PlayerObserver player;
        Logic logic; gameLogic=&logic;
        void* hudStorage=::operator new(sizeof(Hud));
        Hud* hud=new(hudStorage)Hud; hudManager=hud; hud->m_field104=&adapter;
        trace.clear(); selection->selectKit(4); selection->suicide(); selection->showSpawnMenu(true); selection->cycleUnlock(4);
        check(trace=="PPPPP","null local player cuts off operations; cycle still queries twice");
        players.result=&player; players.calls=0;
        hud->m_field250=0; trace.clear(); selection->selectKit(2);
        check(trace=="PK"&&selection->m_selectedKit[0]==2&&selection->m_selectedKit[2]==2,"equal kit avoids server request but updates all team slots");
        trace.clear(); selection->selectKit(5);
        check(trace=="PKCG"&&logic.selection==std::vector<int>({5,0})&&player.chosenArgs==std::vector<int>({5,1}),"kit request uses displayed team unlock level");
        check(selection->m_selectedKit[0]==5&&selection->m_selectedKit[1]==5&&selection->m_selectedKit[2]==5,"false game request does not suppress local selection");
        hud->m_field250=1; adapter.writes=0;
        adapter.callback=[&]{check(selection->m_selectedKit[0]==5,"adapter callbacks precede selection stores");};
        trace.clear(); selection->selectKit(2); adapter.callback=nullptr;
        check(adapter.writes==8,"real selected-kit helper includes failed index7 update");
        check(selection->m_selectedKit[0]==2&&selection->m_selectedKit[2]==2,"selection stores follow adapter loop");
        hud->m_field250=0;
        for(int show=0;show!=2;++show)for(int leader=0;leader!=2;++leader) {
            player.leader=leader!=0; trace.clear(); selection->showSpawnMenu(show!=0);
            check(hud->modeArgument==(show?(leader?17:18):1)&&hud->save,"spawn menu role-dependent mode");
            check(trace==(show?"PLM":"PM"),"false show bypasses squad leader query");
        }
        player.alive=false; trace.clear(); selection->suicide(); check(trace=="PA","dead player does not suicide");
        player.alive=true; trace.clear(); selection->suicide();
        check(trace=="PANDXE","suicide message then request then synchronous event");
        check(std::wstring(*information.getWideStringByName("DeathMessage"))==L"suicide","real death message storage");
        players.script={&player,nullptr}; players.calls=0;
        player.teamReads={1,2,3}; player.teamIndex=0; player.chosen=1; player.unlocked=true;
        trace.clear(); selection->cycleUnlock(4);
        check(trace=="PPTCTUTWIGV","unlock retains first player across second null local query");
        check(player.chosenArgs==std::vector<int>({4,1})&&player.unlockedArgs==std::vector<int>({4,2,2})&&
              player.storeArgs==std::vector<int>({4,3,2}),"unlock rereads team separately for all three player calls");
        check(logic.selectedPlayer==&player&&logic.selection==std::vector<int>({4,2})&&information.updatedTeam==1,"unlock update arguments and displayed-team refresh");
        players.calls=0; player.teamIndex=0; player.chosen=0; player.unlocked=false;
        trace.clear(); selection->cycleUnlock(4);
        check(trace=="PPTCTUTWI"&&player.storeArgs[2]==0,"unavailable next unlock wraps to zero and unchanged level skips request/help");
        players.script.clear(); players.calls=0; player.teamReads.clear(); player.squad=-1;
        selection->m_teamChangePending=false;
        hud->callback=[&]{check(selection->m_teamChangePending&&selection->m_pendingTeam==2,"team state set before callbacks");selection->selectTeam(7);};
        trace.clear(); selection->selectTeam(2); hud->callback=nullptr;
        check(trace=="PTSQM?R"&&logic.selection==std::vector<int>({2}),"negative nonzero squad leaves before mode/question/team request, reentry gated");
        players.calls=0; trace.clear(); selection->selectTeam(0);
        check(trace.empty()&&players.calls==0,"pending team change gate precedes player access");
        selection->reset(); players.result=nullptr; trace.clear(); selection->selectTeam(2);
        check(trace=="P"&&!selection->m_teamChangePending,"null local player leaves pending flag clear");
        selection->m_selectedTeam=1;selection->m_selectedSpawnGroup[1]=73;
        trace.clear();selection->setSpawnGroup(-5);
        check(trace=="P"&&selection->m_selectedSpawnGroup[1]==73,"absent player leaves group and map untouched");
        players.callback=[&]{check(selection->m_selectedTeam==2,"displayed team stored before player callback");};
        trace.clear();selection->setDisplayedTeam(2);players.callback=nullptr;
        check(trace=="Pb"&&selection->m_selectedTeam==2,"absent player still checks SpawnShow after displayed team store");
        check(selection->m_selectedSpawnGroup[1]==73&&selection->m_selectedSpawnGroup[2]==-1,"displayed-team no-player path preserves group slots");
        players.result=&player;players.script={&player,nullptr};players.calls=0;player.team=1;
        trace.clear();selection->selectTeam(1);
        check(trace=="PTPb"&&selection->m_selectedTeam==1,"existing team reaches displayed-team fallback with repeated player query");
        players.calls=0;player.teamReads={2,-1};player.teamIndex=0;
        trace.clear();selection->selectTeam(-1);
        check(trace=="PTTPb"&&selection->m_selectedTeam==2,"unset-team fallback chooses team2 after both shipped team reads");
        players.calls=0;player.teamReads={2,1,0};player.teamIndex=0;
        trace.clear();selection->selectTeam(-1);
        check(trace=="PTTTPb"&&selection->m_selectedTeam==0,"automatic team fallback retains third team query");
        players.script.clear();players.calls=0;player.teamReads.clear();
        selection->m_selectedTeam=1;trace.clear();
        check(nullMapSpawnFault(selection,-8)==EXCEPTION_ACCESS_VIOLATION,"present player retains missing map null guard");
        check(trace=="P"&&selection->m_selectedSpawnGroup[1]==-8,"group store precedes missing map fault and retains signed value");
        world::playerManager=nullptr;
        check(nullPlayersFault(selection)==EXCEPTION_ACCESS_VIOLATION,"retail missing manager guard retained");
        events.unregisterEventHandler(ECHud,&eventSink);
        // Manual teardown of actual constructor-owned members avoids the unrelated
        // incomplete HUD destructor and avoids deleting the borrowed adapter above.
        delete hud->m_field2dc;
        using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
        hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();
        hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
        hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();
        ::operator delete(hudStorage);hudManager=nullptr;gameLogic=nullptr;
    }
    unsigned char before[sizeof(bytes)]; std::memcpy(before,bytes,sizeof(bytes));
    selection->~HudKitSelection();
    check(std::memcmp(before,bytes,sizeof(bytes))==0,"nonvirtual destructor touches no storage");
    std::printf("HUD kit selection: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
