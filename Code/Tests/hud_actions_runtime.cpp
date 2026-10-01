// [TEMP] Seven real HUD actions with explicit typed external collaborators.
// No tested HUD action, RequestEvent body, or EventManager body is overridden.
// /FORCE resolves only uncalled wider HUD/chat/network methods. This is not HUD init.
#include <windows.h>
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
#include <vector>
#define private public
#define protected public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Game/GameEvents/RequestEvent.h"
#undef protected
#undef private
#include "../BF2/Menu/HudChat.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Game/MapInfo.h"
#include "../BF2/Game/IGame.h"
#include "../BF2/Game/IGameLogic.h"
#include "../BF2/Game/Objects/IPlayer.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/Game/GameLogic/SquadManager/ISquadManager.h"
[[noreturn]] static void actionUnsupported(const char* name) {
    std::fprintf(stderr,"[FRONTIER] hud actions fixture unexpected %s\n",name);
    std::fflush(stderr); std::abort();
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
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager=nullptr;
}
} }
using namespace dice::hfe;
using namespace dice::hfe::hud;
static unsigned checks, failures;
static std::string trace;
static bool trackNextRequest;
static void* requestAllocation;
static unsigned requestDeletes;
// [TEMP] Allocation observation only: all allocations retain ordinary heap ownership.
void* operator new(std::size_t bytes) {
    void* result=std::malloc(bytes?bytes:1);
    if(!result)throw std::bad_alloc();
    if(trackNextRequest && bytes==sizeof(RequestEvent)) {
        trackNextRequest=false; requestAllocation=result; trace+='A';
    }
    return result;
}
void operator delete(void* address) noexcept {
    if(address && address==requestAllocation) {
        requestAllocation=nullptr; ++requestDeletes; trace+='Z';
    }
    std::free(address);
}
void operator delete(void* address,std::size_t) noexcept { ::operator delete(address); }
static void check(bool condition,const char* label) {
    ++checks;
    if(!condition){++failures;std::fprintf(stderr,"FAIL %s (trace=%s)\n",label,trace.c_str());}
}
struct Player : world::ActionPlayerBoundary {
    int kit=4, team=2, squad=3, id=11;
    mutable unsigned squadRead=0;
    std::vector<int> squadScript;
    std::function<void()> onId,onSquad,onTeam;
    int getKit() const override { trace+='K';return kit; }
    int getId() const override {trace+='D';if(onId)onId();return id;}
    int getSquadId() const override {
        trace+='S'; if(onSquad)onSquad();
        return squadRead<squadScript.size()?squadScript[squadRead++]:squad;
    }
    int getTeam() const override {trace+='T';if(onTeam)onTeam();return team;}
};
struct Players : world::PlayerManager {
    world::IPlayer* local=nullptr;
    int emptyAt=10;
    std::list<world::IPlayer*> occupied,empty;
    std::vector<int> queried;
    std::function<void(int,int)> onLookup;
    world::IPlayer* getLocalHumanPlayer() override {trace+='P';return local;}
    const std::list<world::IPlayer*>& getPlayersInSquad(int team,int squad) override {
        trace+='F';queried.push_back(squad);if(onLookup)onLookup(team,squad);
        return squad==emptyAt?empty:occupied;
    }
};
struct Logic : ActionLogicBoundary {
    bool server=false;
    std::function<void()> callback;
    bool isServer() const override {trace+='L';bool answer=server;if(callback)callback();return answer;}
};
struct Game : ActionGameBoundary {
    char tag='R';
    TargetChannel channel=TCInvalid;
    std::string message;
    world::IPlayer* sender=nullptr;
    GameEvent* sent=nullptr;
    unsigned connection=0, radioCalls=0, eventCalls=0;
    std::function<void()> onRadio,onSend;
    bool radioMessage(TargetChannel c,const std::string& text,world::IPlayer* player,
                      unsigned value,bool first,bool second) override {
        trace+=tag;++radioCalls;channel=c;message=text;sender=player;
        check(value==0&&first&&!second,"radio arguments preserve zero/true/false");
        if(onRadio)onRadio();return false;
    }
    void sendGameEvent(unsigned target,GameEvent* event) override {
        trace+='G';++eventCalls;connection=target;sent=event;if(onSend)onSend();
    }
};
struct Squads : ActionSquadBoundary {
    char renameTag='N',removeTag='X',inviteTag='V';
    std::vector<int> renameArgs,removeArgs,inviteArgs,leaderArgs,commanderTeams;
    const std::string* renameAddress=nullptr;
    std::string renameValue;
    std::function<void()> onRename,onRemove,onInvite;
    int leader=255,commander=255;
    bool setSquadName(int team,int squad,int player,const std::string& name,bool flag) override {
        trace+=renameTag;renameArgs={team,squad,player};renameAddress=&name;renameValue=name;
        check(!flag,"rename flag false");if(onRename)onRename();return false;
    }
    void removeFromSquad(int team,int squad,int player,bool flag) override {
        trace+=removeTag;removeArgs={team,squad,player};check(!flag,"remove flag false");if(onRemove)onRemove();
    }
    void inviteToSquad(int team,int squad,int player,bool first,bool second) override {
        trace+=inviteTag;inviteArgs={team,squad,player};check(!first&&!second,"invite flags false");if(onInvite)onInvite();
    }
    int getSquadLeader(int team,int squad) override {trace+='Q';leaderArgs={team,squad};return leader;}
    int getCommander(int team) override {trace+='C';commanderTeams.push_back(team);return commander;}
};
struct Hud : BF2HudManager {
    std::wstring localized=L"kit KITNUMBER / KITNUMBER";
    std::function<void()> onLocalize;
    std::wstring getLocalizedString(std::string key) override {
        trace+='I';check(key=="HUD_HUD_CHAT_KIT_MESSAGE","kit localization key");
        if(onLocalize)onLocalize();return localized;
    }
};
struct Chat : HudChat {
    int capacity=1,count=4,first=12,drops=0;
    int* droppedCount=nullptr;int droppedRow=-1;
    char capacityTag='c',countTag='n',firstTag='f',dropTag='d';
    std::function<void()> onCount,onDrop;
    explicit Chat():HudChat(nullptr){}
    int getCapacity(int kind) override {trace+=capacityTag;check(kind==7,"chat category forwarded to capacity");return capacity;}
    int* getCount(int kind) override {trace+=countTag;check(kind==7,"chat category forwarded to count");if(onCount)onCount();return &count;}
    int getFirstRow(int kind) override {trace+=firstTag;check(kind==7,"chat category forwarded to first row");return first;}
    void dropRow(int row,int* value) override {trace+=dropTag;++drops;droppedRow=row;droppedCount=value;if(onDrop)onDrop();}
};
struct Sink : IEventListener {
    int payloadSquad=-1,payloadType=-1;
    unsigned calls=0;
    void handleEvent(EventCategory category,unsigned id,EventNode*,void* payload) override {
        trace+='E';++calls;check(category==ECHud&&id==25,"real request posts synchronous HUD25");
        std::memcpy(&payloadSquad,payload,4);
        std::memcpy(&payloadType,static_cast<char*>(payload)+8,4); // untouched middle word is not read
    }
};
static void releaseSent(Game& destination) {
    GameEvent* event=destination.sent;destination.sent=nullptr;
    check(event&&event->getRef()==1,"client event retains transferred initial reference");
    if(event)check(event->release()==0,"real event release deletes final reference");
}
int main() {
    static_assert(sizeof(void*)==4,"native x86 only");
    static_assert(sizeof(RequestEvent)==20,"native scalar RequestEvent layout");
    trace.reserve(4096);
    EventManager events;eventManager=&events;
    Players players,otherPlayers;world::playerManager=&players;
    Player player,otherPlayer;players.occupied.push_back(&player);otherPlayers.occupied.push_back(&otherPlayer);
    Logic logic;gameLogic=&logic;
    Game firstGame,secondGame;secondGame.tag='r';game=&firstGame;
    Squads squads,otherSquads;squadManager=&squads;otherSquads.inviteTag='v';
    void* storage=::operator new(sizeof(Hud));Hud* hud=new(storage)Hud;hudManager=hud;
    BF2HudManager* actual=hud;
    trace.clear();actual->sendKitMessage();actual->sendRequestForOrder();actual->submitSquadRename();actual->leaveSquad();
    check(trace=="PPPP","four null-player gates stop before collaborator access");
    players.local=&player;
    trace.clear();actual->submitSquadRename();check(trace=="P","empty rename gate precedes player identity callbacks");
    hud->onLocalize=[&]{game=&secondGame;players.local=&otherPlayer;player.kit=8;};
    player.kit=-3;trace.clear();actual->sendKitMessage();
    check(trace=="PKIr","kit read precedes localization and game singleton reload");
    check(secondGame.message=="kit -3 / KITNUMBER"&&secondGame.channel==TCTeam&&secondGame.sender==&player,
          "real substitutions replace first marker using cached kit and player");
    check(!firstGame.radioCalls,"localization callback replaces radio receiver");
    hud->onLocalize=nullptr;players.local=&player;game=&firstGame;
    firstGame.onRadio=[&]{player.squad=260;player.team=258;players.local=&otherPlayer;game=&secondGame;};
    trace.clear();trackNextRequest=true;unsigned deletedBefore=requestDeletes;
    actual->sendRequestForOrder();
    check(trace=="PRSTLAG","false radio result still requests with squad/team read afterwards");
    RequestEvent* request=static_cast<RequestEvent*>(secondGame.sent);
    check(request&&request==requestAllocation&&request->getType()==50&&request->m_teamId==2&&
          request->m_squadId==4&&request->m_requestType==1,"real RequestEvent has truncated callback-mutated team/squad and type1");
    check(secondGame.connection==~0u&&secondGame.eventCalls==1&&requestDeletes==deletedBefore,
          "client dispatch transfers event to reloaded game without caller release");
    releaseSent(secondGame);check(requestDeletes==deletedBefore+1,"transferred request destruction observed");
    firstGame.onRadio=nullptr;players.local=&player;game=&firstGame;
    // Server decision is captured before allocation, then queried again by executeServer.
    Sink sink;events.registerEventHandler(ECHud,&sink,0);
    logic.server=true;unsigned logicReads=0;
    logic.callback=[&]{if(++logicReads==1){check(!requestAllocation&&trackNextRequest,"server query precedes allocation");logic.server=false;}};
    trace.clear();trackNextRequest=true;deletedBefore=requestDeletes;
    actual->sendRequestEvent(258,259,static_cast<RequestEvent::RequestTypes>(1));
    check(trace=="LAQCCELZ","captured server branch executes real request, repeats commander query, then releases");
    check(squads.leaderArgs==std::vector<int>({2,3})&&squads.commanderTeams==std::vector<int>({2,2}),
          "server authority checks use byte fields and retain commander reads");
    check(sink.calls==1&&sink.payloadSquad==3&&sink.payloadType==1&&requestDeletes==deletedBefore+1,
          "server event posts payload before ownership release");
    check(!firstGame.eventCalls&&logicReads==2,"server branch remains chosen after callback changes isServer");
    logic.callback=nullptr;logic.server=true;squads.leader=7;squads.commander=9;
    trace.clear();trackNextRequest=true;deletedBefore=requestDeletes;
    actual->sendRequestEvent(-1,-2,static_cast<RequestEvent::RequestTypes>(4));
    check(trace=="LAQCLZ"&&sink.calls==1&&requestDeletes==deletedBefore+1,
          "unauthorized server request still releases; no local HUD notification");
    logic.server=false;events.unregisterEventHandler(ECHud,&sink);
    hud->m_field0c0="before";player.team=2;player.squad=3;player.id=11;
    player.onId=[&]{hud->m_field0c0.clear();squadManager=&otherSquads;};
    trace.clear();actual->submitSquadRename();
    check(trace=="PDSTN"&&otherSquads.renameArgs==std::vector<int>({2,3,11}),"rename identity callback order and manager reload");
    check(otherSquads.renameValue.empty()&&otherSquads.renameAddress==&hud->m_field0c0,
          "rename passes current string by reference without repeating nonempty gate");
    player.onId=nullptr;squadManager=&squads;
    player.squad=0;trace.clear();actual->leaveSquad();check(trace=="PS","zero squad gates removal and invitation");
    player.squadScript={-1,6};player.squadRead=0;
    squads.onRemove=[&]{player.team=8;player.id=91;player.squad=4;players.local=&otherPlayer;squadManager=&otherSquads;};
    trace.clear();actual->leaveSquad();
    check(trace=="PSSTDXDTv","negative initial squad enters; second squad retained through removal callback");
    check(squads.removeArgs==std::vector<int>({2,6,11})&&otherSquads.inviteArgs==std::vector<int>({8,6,91}),
          "invitation rereads cached player's id/team and singleton, retains prior squad");
    squads.onRemove=nullptr;player.squadScript.clear();players.local=&player;squadManager=&squads;
    for(int empty=1;empty<=10;++empty) {
        world::playerManager=&players;players.emptyAt=empty;players.queried.clear();trace.clear();
        int result=actual->findEmptySquad(-7);
        check(result==(empty==10?-1:empty),"first empty squad or minus-one sentinel");
        check(players.queried.size()==static_cast<unsigned>(empty==10?9:empty)&&players.queried.front()==1&&
              players.queried.back()==(empty==10?9:empty),"scan is bounded to squad ids1 through9");
    }
    players.emptyAt=10;otherPlayers.emptyAt=2;players.queried.clear();otherPlayers.queried.clear();
    players.onLookup=[&](int team,int squad){check(team==71&&squad==1,"first lookup uses caller team");world::playerManager=&otherPlayers;};
    otherPlayers.onLookup=[&](int team,int squad){check(team==71&&squad==2,"second lookup reloads manager and retains team");};
    trace.clear();check(actual->findEmptySquad(71)==2&&trace=="FF","empty-squad loop reloads global manager on each iteration");
    players.onLookup=nullptr;otherPlayers.onLookup=nullptr;world::playerManager=&players;
    hud->m_field144=nullptr;trace.clear();actual->refreshChatCategory(7);check(trace.empty(),"missing chat gate");
    Chat oldChat,newChat;newChat.capacityTag='C';newChat.countTag='N';newChat.firstTag='F';newChat.dropTag='D';
    hud->m_field144=&oldChat;oldChat.capacity=-1;trace.clear();actual->refreshChatCategory(7);
    check(trace=="c"&&!oldChat.drops,"negative chat capacity skips row removal");
    oldChat.capacity=3;newChat.capacity=1;oldChat.onCount=[&]{hud->m_field144=&newChat;};
    trace.clear();actual->refreshChatCategory(7);
    check(trace=="cnfDC"&&newChat.droppedCount==&oldChat.count&&newChat.droppedRow==oldChat.first,
          "chat count callback preserves old first-row receiver, reloads drop receiver and loop bound");
    check(!oldChat.drops&&newChat.drops==1,"updated capacity stops loop after first removal");
    oldChat.onCount=nullptr;hud->m_field144=&newChat;newChat.capacity=1;newChat.drops=0;
    newChat.onDrop=[&]{if(newChat.drops==1)newChat.capacity=3;};
    trace.clear();actual->refreshChatCategory(7);
    check(newChat.drops==3&&trace=="CNFDCNFDCNFDC","chat capacity growth is observed between iterations");
    hud->m_field144=nullptr;
    // Actual HUD construction, manual native-member teardown: full HUD lifecycle is still a frontier.
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();
    hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();
    ::operator delete(storage);hudManager=nullptr;
    std::printf("HUD actions: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
