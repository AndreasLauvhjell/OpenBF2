// [TEMP] [RETIRE-WHEN: HUD-GAME-LOGIC-EVENTS-FIXTURE]
// Actual ECGameLogic router/handler, kit helpers and network lookup. Typed
// virtual service observers do not replace the recovered handler or helpers.
#define main unusedChatGameLogicRegression
#include "hud_chat_send_runtime.cpp"
#undef main
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Game/Main/BF2ClientSeams.h"
#include "../BF2/Game/GameEvents/KilledByEvent.h"
#include "../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../BF2/IO/Network/NetworkManager.h"
#define private public
#include "../BF2/Menu/HudInformationAdapter.h"
#undef private
#include "../BF2/Core/Debug.h"
namespace dice { namespace hfe {
IBase* g_hudInformationLayer=nullptr;
namespace ai {IBase* g_aiInstance=nullptr;}
namespace io {NetworkManager* networkManager=nullptr;}
namespace world {ObjectTemplateManager* objectTemplateManager=nullptr;}
} }
struct LogicPlayer : Player {
    int id=3; bool commander=false; std::string name="Alice";
    mutable int ids=0,teamReads=0,names=0; int voiceCalls=0;
    std::function<void()> onId;
    int getId() const override {++ids;trace.push_back("id"+std::to_string(id));if(onId)onId();return id;}
    int getTeam() const override {++teamReads;trace.push_back("team"+std::to_string(id));return Player::getTeam();}
    const std::string& getName() const override {++names;trace.push_back("name"+std::to_string(id));return name;}
    bool getIsCommander() const override {trace.push_back("commanderPlayer");return commander;}
    world::IObject* getVehicle() const override {trace.push_back("vehicle");return nullptr;}
    void enableVOHelp(bool value) override {check(!value,"round disables VOHelp, distinct from VoIP");++voiceCalls;trace.push_back("voiceHelp");}
};
struct LogicPlayers : Players {
    std::map<int,world::IPlayer*> byId;
    std::list<world::IPlayer*> list;
    std::function<void(int)> onFind;
    world::IPlayer* getPlayerFromId(int id) override {trace.push_back("find"+std::to_string(id));if(onFind)onFind(id);auto it=byId.find(id);return it==byId.end()?nullptr:it->second;}
    const std::list<world::IPlayer*>& getPlayers() override {trace.push_back("allPlayers");return list;}
};
struct LogicHud : BF2HudManager {
    struct Chat {std::wstring text;world::IPlayer* player;int team;};
    std::vector<Chat> server,teamChat;
    std::vector<std::string> keys;
    std::function<void(const std::string&)> onLocale;
    std::function<void()> onTeamChat,onServerChat,onMode;
    int mode=0,done=0,refresh=0,resultSounds=0;bool commander=false;
    std::wstring connection;
    std::wstring getLocalizedString(std::string key) override {keys.push_back(key);trace.push_back("locale:"+key);if(onLocale)onLocale(key);return std::wstring(key.begin(),key.end());}
    std::wstring replaceWideInputKeys(std::wstring value) override {trace.push_back("keys");return value+L" [keys]";}
    std::wstring replaceWideTokenFromString(std::wstring text,std::string token,std::string value) override {
        trace.push_back(token+":"+value);return text+L"|"+std::wstring(value.begin(),value.end());
    }
    std::wstring replaceWideToken(std::wstring text,std::string token,std::wstring value) override {
        trace.push_back(token);return text+L"|"+value;
    }
    void addServerChat(std::wstring text,int team) override {trace.push_back("serverChat");server.push_back({text,nullptr,team});if(onServerChat)onServerChat();}
    void addTeamChat(std::wstring text,world::IPlayer* player,int team) override {trace.push_back("teamChat");teamChat.push_back({text,player,team});if(onTeamChat)onTeamChat();}
    int getMode() override {trace.push_back("mode");return mode;}
    void setMode(int value,bool save) override {check(save,"game logic mode saves previous");trace.push_back("mode"+std::to_string(value));mode=value;if(onMode)onMode();}
    bool isCommanderShown() override {trace.push_back("commander");return commander;}
    void setDone(bool value) override {check(value,"team equality acknowledges done");++done;}
    void refreshChatCategory(int value) override {check(value==8,"team change refreshes category8");++refresh;}
    void showConnectionMessage(std::wstring value) override {trace.push_back("showConnection");connection=value;}
    void clearConnectionMessage() override {trace.push_back("clearConnection");connection.clear();}
    void clearDisconnectMessage() override {trace.push_back("clearDisconnect");}
    void clearDeathMessage() override {trace.push_back("clearDeath");}
    void setMenuLoading(bool value) override {check(!value,"real reset clears loading");trace.push_back("loading");}
    void playRoundResultSound() override {++resultSounds;trace.push_back("resultSound");}
};
struct LogicInfo : HudInformationLayer {
    std::wstring death;
    void setDeathMessage(std::wstring value) override {death=value;trace.push_back("death");}
};
struct LogicAdapter : HudInformationAdapter {
    bool getBool(std::string key) override {check(key=="SpawnShow","kit queries SpawnShow");return false;}
    bool setBool(std::string,bool value) override {check(!value,"reset flags false");return false;}
};
struct LogicGame : Game {
    unsigned radios=0,lastId=0;world::IPlayer* lastPlayer=nullptr;
    bool radioMessage(TargetChannel channel,const std::string& text,world::IPlayer* player,unsigned id,bool a,bool b) override {
        check(channel==14&&text=="AUTO_MOODGP_friendlyfire"&&a&&b,"teamdamage radio six arguments");++radios;lastId=id;lastPlayer=player;return false;
    }
};
struct LogicGameLogic : Logic {
    int status=0;std::function<void()> onStatus;
    int getGameStatus() override {if(onStatus)onStatus();return status;}
};
struct LogicTemplates : world::ObjectTemplateManager {
    mutable std::vector<unsigned> queried;
    world::IObjectTemplate* getTemplate(unsigned id) const override {queried.push_back(id);return nullptr;}
};
static bool hasTrace(const std::string& entry) {return std::find(trace.begin(),trace.end(),entry)!=trace.end();}
static LONG WINAPI reportLogicException(EXCEPTION_POINTERS* fault) {
    std::printf("Fixture exception %08lx at %p, last callbacks:",fault->ExceptionRecord->ExceptionCode,fault->ExceptionRecord->ExceptionAddress);
    for(const auto& entry:trace)std::printf(" %s",entry.c_str());
    std::puts("");return EXCEPTION_CONTINUE_SEARCH;
}
int main() {
    setvbuf(stdout,nullptr,_IONBF,0);
    AddVectoredExceptionHandler(1,reportLogicException);
    EventManager events;eventManager=&events;
    LogicPlayers players;world::playerManager=&players;
    auto* hud=new LogicHud;hudManager=hud;
    LogicPlayer local,victim;victim.id=7;victim.name="Victim";victim.team=1;
    players.local=&local;players.byId={{3,&local},{7,&victim}};
    LogicInfo info;g_hudInformationLayer=&info;
    Sound sound;g_bfSound=&sound;
    LogicGame current;game=&current;
    LogicGameLogic logic;gameLogic=&logic;
    LogicTemplates templates;world::objectTemplateManager=&templates;
    auto* network=new io::NetworkManager;io::networkManager=network;
    std::puts("Game logic fixture: guards and score messages");
    for(unsigned id: {0u,2u,5u,6u,7u,13u,15u,0xFFFFFFFFu}) {trace.clear();hud->handleEvent(ECGameLogic,id,nullptr,nullptr);check(trace.empty(),"unhandled event has no callbacks");}
    for(unsigned id:{3u,8u}) {trace.clear();hud->handleEvent(ECGameLogic,id,nullptr,nullptr);check(trace==std::vector<std::string>{"player"},"null guarded payload still queries local");}
    for(unsigned id:{9u,10u,11u,12u}) {players.local=nullptr;trace.clear();hud->handleEvent(ECGameLogic,id,nullptr,nullptr);check(trace==std::vector<std::string>{"player"},"score missing player returns before payload");}
    players.local=&local;local.commander=true;
    for(unsigned id:{9u,10u,12u}) {trace.clear();hud->handleEvent(ECGameLogic,id,nullptr,nullptr);check(trace==std::vector<std::string>{"player","commanderPlayer"},"commander skips score payload");}
    local.commander=false;
    const char* assist[]={nullptr,"HUD_HUD_ASSIST_KILL_TARGETING","HUD_HUD_ASSIST_KILL_KILL_DAMAGE","HUD_HUD_ASSIST_KILL_DRIVER_PASSENGER"};
    const char* replenish[]={"HUD_HUD_REPLENISH_GOT_HEALING_POINT","HUD_HUD_REPLENISH_GOT_REPAIR_POINT","HUD_HUD_REPLENISH_GOT_AMMO_POINT","HUD_HUD_REPLENISH_DRIVER","HUD_HUD_REPLENISH_REVIVE_SCORE","HUD_HUD_DESTROY_STRATEGIC"};
    const char* conquest[]={"HUD_HUD_CONQUEST_FULL_CAPTURE","HUD_HUD_CONQUEST_DEFENDED_FLAG","HUD_HUD_CONQUEST_CAPTURE_ASSIST","HUD_HUD_CONQUEST_NEUTRALIZE","HUD_HUD_CONQUEST_NEUTRALIZE_ASSIST"};
    for(unsigned event:{9u,10u,11u,12u}) {
        const int count=event==9?4:event==10?6:event==11?2:5;
        for(int kind=0;kind<count;++kind) {
            int payload[]={kind,99,88};trace.clear();const auto before=hud->server.size();
            hud->handleEvent(ECGameLogic,event,nullptr,payload);
            if(event==9&&kind==0) {check(hud->server.size()==before,"assist0 has no message");continue;}
            const std::string key=event==9?assist[kind]:event==10?replenish[kind]:event==11?(kind?"HUD_HUD_TEAM_DAMAGE_VEHICLE":"HUD_HUD_TEAM_DAMAGE_PLAYER"):conquest[kind];
            check(hud->server.size()==before+1&&hud->server.back().team==2&&hud->keys.back()==key,"score subtype exact key/team");
            check(trace[trace.size()-4]=="team3"&&trace[trace.size()-3]=="locale:"+key&&trace[trace.size()-2]=="keys"&&trace.back()=="serverChat","score team-before-localization callback order");
        }
    }
    turnOffAllDebug(DEBUGTYPE_6);
    for(unsigned event:{9u,10u,11u,12u}) {int payload[]={99,99,99};const auto before=hud->server.size();hud->handleEvent(ECGameLogic,event,nullptr,payload);check(hud->server.size()==before,"filtered real Debug invalid subtype has no fabricated chat");}
    int damage[]={123,3,0x12345678};local.onId=[&]{damage[2]=7;};hud->handleEvent(ECGameLogic,11,nullptr,damage);local.onId=nullptr;
    check(current.radios==1&&current.lastPlayer==&local&&current.lastId==0x12345678,"teamdamage captures third payload before id callback");
    int ticketTeam=2;hud->handleEvent(ECGameLogic,4,nullptr,&ticketTeam);ticketTeam=1;hud->handleEvent(ECGameLogic,4,nullptr,&ticketTeam);
    check(sound.played==std::vector<std::string>{"tickets_low"},"tickets sound only matching local team");
    players.local=nullptr;hud->handleEvent(ECGameLogic,4,nullptr,&ticketTeam);players.local=&local;
    std::puts("Game logic fixture: kill-feed");
    KilledByData killed={};killed.killerPlayerId=3;killed.victimPlayerId=7;killed.isKillerPlayerAlive=true;killed.vehicleTemplateId=42;killed.someObjTemplateId=43;
    local.name=std::string("Killer\0ignored",14);victim.name=std::string("Victim\0ignored",14);
    hud->m_field2ec=91;trace.clear();hud->handleEvent(ECGameLogic,3,nullptr,&killed);
    check(hud->server.back().text==L"HUD_CHAT_YOUGOTAKILLSCORE|Victim [keys]","kill-score uses C-string-truncated victim name");
    check(hud->teamChat.back().text==L"HUD_HUD_KILLEDBY_KILLED|Killer||Victim"&&hud->teamChat.back().player==&local&&hud->teamChat.back().team==2,"no template kill uses generic message and ordered replacements");
    check(templates.queried==std::vector<unsigned>{42}&&hud->m_field2ec==7,"numeric template lookup and selected victim");
    check(local.teamReads>=4&&hasTrace("vehicle"),"kill score and feed reread team and fallback vehicle");
    hud->onLocale=[&](const std::string& key){if(key.empty()){victim.name[0]='Y';local.name[0]='X';}};
    hud->handleEvent(ECGameLogic,3,nullptr,&killed);hud->onLocale=nullptr;
    check(hud->teamChat.back().text==L"HUD_HUD_KILLEDBY_KILLED|Xiller||Victim","victim copied before weapon localization, killer copied afterward");
    victim.name[0]='V';local.name[0]='K';
    const auto beforeMutatedKill=hud->server.size();
    local.onId=[&]{killed.killerPlayerId=99;};hud->handleEvent(ECGameLogic,3,nullptr,&killed);local.onId=nullptr;
    check(hud->server.size()==beforeMutatedKill,"local id callback precedes live killer-byte comparison");killed.killerPlayerId=3;
    hud->onTeamChat=[&]{victim.id=17;};hud->handleEvent(ECGameLogic,3,nullptr,&killed);check(hud->m_field2ec==17,"selected victim id reread after chat callback");hud->onTeamChat=nullptr;victim.id=7;
    victim.team=2;hud->handleEvent(ECGameLogic,3,nullptr,&killed);check(hud->keys.back()=="HUD_HUD_KILLEDBY_TEAMKILLS","same-team kill text");victim.team=1;
    players.local=&victim;hud->handleEvent(ECGameLogic,3,nullptr,&killed);
    check(info.death==L"HUD_TEXT_MENU_YOU_WERE_KILLED [keys]"&&hud->server.back().text==L"HUD_CHAT_YOUWEREKILLEDBY|Killer [keys]","local victim receives death and killer messages");
    players.local=&local;killed.victimPlayerId=3;hud->m_field2ec=88;hud->handleEvent(ECGameLogic,3,nullptr,&killed);
    check(hud->teamChat.back().player==nullptr&&hud->teamChat.back().text==L"HUD_HUD_KILLEDBY_MYSELF [keys]|Killer"&&hud->m_field2ec==88,"suicide repeats lookup, keys before token and preserves selected id");
    killed.victimPlayerId=7;killed.isKillerPlayerAlive=false;
    for(unsigned code:{0xFDu,0xFEu,0xFCu}) {killed.killerPlayerId=static_cast<unsigned char>(code);const auto before=hud->teamChat.size();hud->handleEvent(ECGameLogic,3,nullptr,&killed);check(hud->m_field2ec==7,"environment kill stores victim even without chat");if(code==0xFC)check(hud->teamChat.size()==before,"other absent-killer id emits no feed");else check(hud->teamChat.back().player==&victim&&hud->teamChat.back().team==(code==0xFD?2:1)&&hud->teamChat.back().text.find(L"| |HUD_TEXT_COMMANDER_ARTILLERY|Victim")!=std::wstring::npos,"artillery uses blank killer, victim pointer and opposing/same team");}
    killed.victimPlayerId=99;hud->m_field2ec=66;hud->handleEvent(ECGameLogic,3,nullptr,&killed);check(hud->m_field2ec==66,"missing victim leaves selected id");
    // A full backing buffer makes the retail event14 defect safe to observe here.
    killed={};killed.victimPlayerId=7;trace.clear();hud->handleEvent(ECGameLogic,14,nullptr,&killed);
    check(std::count(trace.begin(),trace.end(),"player")==2&&hasTrace("find7")&&hud->m_field2ec==7,"revival with absent network object falls through into kill feed");
    std::puts("Game logic fixture: team and round transitions");
    LogicAdapter adapter;hud->m_field104=&adapter;HudKitSelection kit;hud->m_field11c=&kit;
    int teamChange[]={3,1};kit.m_pendingTeam=-1;players.replies={&local,nullptr};hud->handleEvent(ECGameLogic,8,nullptr,teamChange);
    check(kit.m_selectedTeam==1&&hud->refresh==0,"pending-minus-one still updates selected team then returns");
    kit.m_pendingTeam=2;kit.m_teamChangePending=true;teamChange[1]=-1;players.replies={&local,nullptr};hud->handleEvent(ECGameLogic,8,nullptr,teamChange);
    check(kit.m_selectedTeam==2&&!kit.m_teamChangePending,"cancel selects live local team and clears pending flag");
    teamChange[1]=1;kit.m_pendingTeam=2;hud->mode=1;players.replies={&local,nullptr};hud->handleEvent(ECGameLogic,8,nullptr,teamChange);
    check(info.death==L"HUD_HUD_MENU_TEAMS_EQUALIZE [keys]"&&hud->done==1&&hud->refresh==1&&sound.played.back()=="USMC_TeamSelect","team equality refusal text sound done refresh");
    kit.m_pendingTeam=1;hud->mode=1;local.alive=false;players.replies={&local,nullptr,&local};hud->handleEvent(ECGameLogic,8,nullptr,teamChange);
    check(info.death==L"HUD_TEXT_MENU_CHANGED_TEAM [keys]"&&hud->refresh==2&&kit.m_pendingTeam==1,"accepted team runs real guarded suicide then refresh");
    hud->mode=0;players.replies={&local,nullptr,&local,nullptr};hud->handleEvent(ECGameLogic,8,nullptr,teamChange);
    check(hud->mode==1&&kit.m_selectedTeam==1&&hud->refresh==3,"accepted team opens mode then repeats real displayed-team helper");
    HudKitSelection replacementKit;kit.m_pendingTeam=2;hud->mode=1;local.alive=true;logic.status=1;
    logic.onStatus=[&]{hud->m_field11c=&replacementKit;teamChange[1]=2;};
    teamChange[0]=99;teamChange[1]=1;local.onId=[&]{teamChange[0]=3;};players.replies={&local,nullptr};
    hud->handleEvent(ECGameLogic,8,nullptr,teamChange);local.onId=nullptr;logic.onStatus=nullptr;logic.status=0;
    check(replacementKit.m_field005&&replacementKit.m_selectedTeam==1&&logic.teams.back()==2,"id precedes live payload read, status reloads owner, sound reloads payload team");
    check(hud->done==2&&hud->refresh==4,"team branch retains original pending and target across callbacks");
    hud->m_field11c=nullptr;hud->m_field104=nullptr;players.local=nullptr;
    int state=4;hud->commander=true;trace.clear();hud->handleEvent(ECGameLogic,1,nullptr,&state);
    check(hud->connection==L"GAME_MESSAGE_GAME_PAUSED"&&hud->mode==0&&hud->m_field324==4&&!hasTrace("keys")&&!hasTrace("player"),"pause shows raw localized text and stores status without player query");
    check(hasTrace("mode15"),"pause closes commander before mode zero");
    state=1;trace.clear();hud->handleEvent(ECGameLogic,1,nullptr,&state);check(trace==std::vector<std::string>{"clearConnection"}&&hud->m_field324==1,"resume from paused skips reset and player query");
    hud->m_field324=0;trace.clear();hud->handleEvent(ECGameLogic,1,nullptr,&state);check(hasTrace("loading")&&hasTrace("allPlayers")&&hasTrace("mode1")&&trace.back()=="clearConnection","fresh play uses actual reset then absent-player spawn mode");
    state=2;players.local=&local;trace.clear();hud->onMode=[&]{state=99;hud->m_field324=-9;};hud->handleEvent(ECGameLogic,1,nullptr,&state);hud->onMode=nullptr;
    check(local.voiceCalls==1&&logic.requests==1&&hud->resultSounds==1&&hud->m_field324==2&&trace.back()=="resultSound","end round selects group0 then disables VOHelp then sound");
    state=99;trace.clear();hud->handleEvent(ECGameLogic,1,nullptr,&state);check(trace.empty()&&hud->m_field324==99,"unknown status only stored");
    // Real constructor listeners are removed before scoped EventManager teardown.
    delete network;
    // Retain the HUD: the complete world/map shutdown remains outside this fixture.
    std::printf("HUD game-logic events: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
