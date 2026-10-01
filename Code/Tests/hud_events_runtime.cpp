// [TEMP] [RETIRE-WHEN: HUD-EVENTS-FIXTURE] Actual secondary router, game
// handler and reset. External typed observers replace services, never HUD bodies.
// No incomplete map, vehicle widget or higher category handler is constructed.
#define main unusedChatSendEventsRegression
#include "hud_chat_send_runtime.cpp"
#undef main
#define private public
#include "../BF2/Menu/HudMessages.h"
#include "../BF2/Menu/HudQuestionManager.h"
#include "../BF2/Menu/HudInformationAdapter.h"
#undef private
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/AI/AIInterface/AIInstance.h"
// Access-only aliases to the real private methods; no replacement body.
#pragma comment(linker,"/alternatename:?resetHudState@BF2HudManager@hud@hfe@dice@@QAEXXZ=?resetHudState@BF2HudManager@hud@hfe@dice@@AAEXXZ")
#pragma comment(linker,"/alternatename:?configureMap@BF2HudManager@hud@hfe@dice@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z=?configureMap@BF2HudManager@hud@hfe@dice@@AAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z")
namespace dice { namespace hfe { IBase* g_hudInformationLayer=nullptr; namespace ai {IBase* g_aiInstance=nullptr;} } }
struct EventHud : BF2HudManager {
    bool commander=false; int mode=0, spawns=0, loading=0, done=0, chats=0, chatTeam=0;
    std::wstring chat;
    std::function<void()> onLocalization;
    bool isCommanderShown() override {trace.push_back("commander");return commander;}
    void setMode(int value,bool remember) override {check(remember,"event mode remembers");trace.push_back("mode"+std::to_string(value));mode=value;}
    int getMode() override {return mode;}
    bool closeChat() override {trace.push_back("closeChat");return false;}
    void clearDisconnectMessage() override {trace.push_back("disconnect");}
    void clearConnectionMessage() override {trace.push_back("connection");}
    void clearDeathMessage() override {trace.push_back("death");}
    void setMenuLoading(bool value) override {check(!value,"loading cleared");++loading;trace.push_back("loading");}
    void onSpawn() override {++spawns;trace.push_back("spawn");}
    void setDone(bool value) override {check(value,"spawn done true");++done;}
    std::wstring getLocalizedString(std::string key) override {
        trace.push_back(key);if(onLocalization)onLocalization();return key=="HUD_TEXT_MENU_YOU_WERE_KILLED"?L"killed":L"self #PLAYERNAME#";
    }
    std::wstring replaceWideInputKeys(std::wstring text) override {return text+L" keys";}
    void addTeamChat(std::wstring text,world::IPlayer* p,int team) override {check(!p,"suicide chat has null player");++chats;chat=text;chatTeam=team;}
};
struct EventPlayer : Player {
    int id=3,resets=0; std::string name="Alice";
    int getId() const override {trace.push_back("id"+std::to_string(id));return id;}
    const std::string& getName() const override {return name;}
    void resetSpamFilter() override {++resets;}
    world::IObject* getVehicle() const override {trace.push_back("vehicle");return nullptr;}
};
struct EventPlayers : Players {
    world::IPlayer* found=nullptr;int searched=0;std::list<world::IPlayer*> list;
    world::IPlayer* getPlayerFromId(int id) override {searched=id;return found;}
    const std::list<world::IPlayer*>& getPlayers() override {trace.push_back("list");return list;}
};
struct EventLogic : Logic {
    int status=0,queries=0;std::function<void()> onStatus;
    int getGameStatus() override {++queries;if(onStatus)onStatus();return status;}
};
struct EventBus : EventManager {
    struct Posted {EventCategory category;unsigned id;void* payload;float delay;};
    std::vector<Posted> posted;
    unsigned int postEvent(EventCategory c,unsigned id,void* p,float d) override {posted.push_back({c,id,p,d});return 0;}
};
struct EventSound : BfSoundNull {
    std::string stopped;
    void stopSound(const std::string& value) override {stopped=value;trace.push_back("stopSound");}
};
struct EventInformation : HudInformationLayer {
    std::wstring death;
    void setDeathMessage(std::wstring value) override {death=value;}
};
struct EventAdapter : HudInformationAdapter {
    std::function<void()> onSet;
    std::vector<std::string> keys;
    bool setBool(std::string key,bool value) override {check(!value,"reset hides flags");keys.push_back(key);if(onSet)onSet();return false;}
};
static EventHud* makeHud() {return new(::operator new(sizeof(EventHud))) EventHud;}
static void freeHud(EventHud* h) {
    delete h->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<HudPendingRequest>;
    using F=std::function<void()>;
    h->onLocalization.~F();h->chat.~W();
    h->m_field010.~W();h->m_field038.~W();h->m_field060.~W();h->m_field08c.~S();h->m_field0c0.~S();h->m_field304.~S();
    h->m_field0e8.~L();h->m_field2e0.~SL();h->m_field33c.~SL();h->m_field32c.~V();::operator delete(h);
}
int main() {
    EventBus events;eventManager=&events;
    EventPlayers players;world::playerManager=&players;
    auto* h=makeHud();auto* global=makeHud();hudManager=h;
    EventSound sound;g_bfSound=&sound;
    EventInformation info;g_hudInformationLayer=&info;
    EventPlayer player,other;other.id=7;players.local=&player;players.found=&player;
    HudKitSelection kit;HudQuestionManager questions;HudMessages messages;
    h->m_field11c=&kit;h->m_field100=&questions;h->m_field148=&messages;
    h->m_field248=true;
    for (int category : {-1,3,4,5,6,7,8,9,11,100}) h->handleEvent(static_cast<EventCategory>(category),1,nullptr,nullptr);
    check(players.queries==0,"unknown categories have no callbacks");
    h->handleEvent(static_cast<EventCategory>(10),99,nullptr,nullptr);
    check(players.queries==0,"unknown category10 event does nothing");
    h->handleEvent(ECGame,99,nullptr,nullptr);check(players.queries==1,"unknown game event still queries local");
    messages.m_visible=true;h->handleEvent(ECGame,11,nullptr,&other);check(messages.m_visible,"other player's item selection persists");
    trace.clear();h->handleEvent(ECGame,11,nullptr,&player);
    check(!messages.m_visible&&trace==std::vector<std::string>{"player","id3","id3"},"local item selection hides after ordered id reads");
    struct KillBytes {int id;unsigned char self;unsigned char padding[3];} killed={3,0,{255,254,253}};
    h->m_field2ec=91;h->handleEvent(ECGame,8,nullptr,&killed);
    check(info.death==L"killed keys"&&h->spawns==1&&h->m_field2ec==-1,"local death text and spawn callback");
    kit.m_field005=true;killed.self=1;player.name=std::string("Alice\0ignored",13);
    h->onLocalization=[&]{player.name="Bob";};
    h->handleEvent(ECGame,8,nullptr,&killed);
    check(!kit.m_field005&&h->chatTeam==1&&h->chat==L"self Alice keys","team-switch death flips team and captures truncated name before localization");
    h->onLocalization=nullptr;
    h->mode=11;const int beforeSpawn=h->spawns;h->handleEvent(ECGame,8,nullptr,&killed);
    check(h->spawns==beforeSpawn,"round result suppresses spawn callback");
    players.found=nullptr;h->m_field2ec=72;h->handleEvent(ECGame,8,nullptr,&killed);
    check(h->m_field2ec==72,"missing killed player preserves selected id");players.found=&player;
    players.local=nullptr;h->handleEvent(ECGame,8,nullptr,nullptr);players.local=&player;
    kit.m_pendingTeam=-1;kit.m_teamChangePending=true;events.posted.clear();
    h->handleEvent(ECGame,6,nullptr,&player);
    check(events.posted.size()==1&&events.posted[0].id==31&&events.posted[0].payload==h&&events.posted[0].delay==0,"spawn posts HUD31 with primary pointer");
    check(h->done==1&&!kit.m_teamChangePending&&kit.m_pendingTeam==2,"spawn done and pending team");
    kit.m_pendingTeam=1;h->handleEvent(ECGame,6,nullptr,&player);check(kit.m_pendingTeam==1,"existing pending team preserved");
    h->commander=true;h->m_field304="menu_loop";trace.clear();
    h->handleEvent(static_cast<EventCategory>(10),1,nullptr,nullptr);
    check(sound.stopped=="menu_loop"&&h->mode==0,"disconnect uses string sound overload and clears mode");
    check(trace.front()=="commander"&&trace[1]=="mode15"&&trace[trace.size()-2]=="disconnect"&&trace.back()=="stopSound","disconnect callback order");
    EventLogic first,second;hudManager=global;gameLogic=&first;
    first.status=0;second.status=4;first.onStatus=[&]{gameLogic=&second;ai::g_aiInstance=nullptr;};
    // AI pointer is tested only for presence; it is never dereferenced.
    ai::g_aiInstance=reinterpret_cast<IBase*>(1);events.posted.clear();
    h->handleEvent(static_cast<EventCategory>(10),2,nullptr,nullptr);
    check(global->loading==1&&h->loading==0&&first.queries==1&&second.queries==1,"load event uses global HUD and snapshots AI before callback");
    check(events.posted.size()==2&&events.posted[0].payload==h&&events.posted[1].id==67&&events.posted[1].delay==4,"load event posts immediate2 then delayed67");
    first.onStatus=nullptr;gameLogic=&first;first.status=4;events.posted.clear();
    h->handleEvent(static_cast<EventCategory>(10),2,nullptr,nullptr);check(events.posted.size()==1,"status4 without AI has no delayed event");
    first.status=1;events.posted.clear();h->handleEvent(static_cast<EventCategory>(10),2,nullptr,nullptr);
    check(events.posted.size()==2,"status1 has delayed event without AI");
    hudManager=h;gameLogic=nullptr;players.local=nullptr;
    EventAdapter adapter,secondAdapter;h->m_field104=&adapter;
    adapter.onSet=[&]{h->m_field104=&secondAdapter;};
    players.list={&player,nullptr,&other};messages.m_field020=2.5;messages.m_field028=0;
    h->resetHudState();
    check(adapter.keys==std::vector<std::string>{"IngameHelpShow"}&&secondAdapter.keys==std::vector<std::string>{"CommanderShow"},"reset reloads information adapter between callbacks");
    check(messages.m_field028==2.5&&player.resets==1&&other.resets==1,"reset copies message delay and resets every non-null player");
    h->configureMap("ignored");check(first.teams.empty(),"missing map avoids team-name queries");
    h->handleEvent(static_cast<EventCategory>(10),3,nullptr,nullptr);
    h->m_field100=nullptr;h->m_field11c=nullptr;h->m_field104=nullptr;h->m_field148=nullptr;
    freeHud(h);freeHud(global);hudManager=nullptr;
    std::printf("HUD event router/game/reset: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
