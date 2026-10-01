// [TEMP] Actual HUD chat sends/onSpawn, with typed external callback observers.
// Missing wider HUD methods remain uncalled /FORCE dependencies, not production stubs.
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <limits>
#include <new>
#include <string>
#include <vector>
#define private public
#define protected public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudKitSelection.h"
#include "../BF2/Menu/HudBlinker.h"
#undef protected
#undef private
#include "../BF2/External/BFMeme/Data.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/System.h"
#include "../BF2/Game/GameServer/ServerInfo.h"
#include "../BF2/Game/MapInfo.h"
#include "../BF2/Game/IGameLogic.h"
#include "../BF2/Game/Objects/IPlayer.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/Game/GameLogic/SquadManager/ISquadManager.h"
#include "../BF2/Game/GameLogic/ScoreSystem/ScoreManager.h"
#include "../BF2/Sound/BfSound.h"
#include "../BF2/Core/StringUtil.h"
[[noreturn]] static void actionUnsupported(const char* name) {
    std::fprintf(stderr,"[FRONTIER] chat-send fixture unexpected %s\n",name);
    std::fflush(stderr); std::abort();
}
#include "hud_action_observers.h"
namespace dice { namespace hfe {
EventManager* eventManager = nullptr;
IGameLogic* gameLogic = nullptr;
IGame* game = nullptr;
IBase* squadManager = nullptr;
hud::BF2HudManager* hudManager = nullptr;
IBfSound* g_bfSound = nullptr;
namespace world {
PlayerManager* playerManager = nullptr;
IBase* scoreManager = nullptr;
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager = nullptr;
}
} }
using namespace dice::hfe;
using namespace dice::hfe::hud;
static unsigned checks, failures;
static std::vector<std::string> trace;
static void check(bool condition,const char* label) {
    ++checks; if (!condition) { ++failures; std::printf("FAIL %s\n",label); }
}
struct Player : world::ActionPlayerBoundary {
    bool alive=true, spam=false;
    float spamTime=0, spawnTime=0;
    int team=2;
    unsigned spawnTick=0, spamCalls=0;
    std::function<void()> onSpam,onTimer,onTeam;
    int getId() const override { return 3; }
    int getKit() const override { return 2; }
    int getSquadId() const override { return 0; }
    int getTeam() const override { if(onTeam)onTeam();return team; }
    bool getIsAlive() const override { trace.push_back("alive");return alive; }
    bool getIsThisASpam(bool count) override {
        trace.push_back("spam"); ++spamCalls; check(count,"spam query counts this message");
        if(onSpam)onSpam(); return spam;
    }
    float getRadioSpamFilterFlagTimer() override { if(onTimer)onTimer();return spamTime; }
    unsigned getSpawnAtTick() const override {trace.push_back("spawnTick");return spawnTick;}
    float getTimeToSpawn() const override {trace.push_back("spawnTime");return spawnTime;}
};
struct Players : world::PlayerManager {
    world::IPlayer* local=nullptr;
    unsigned queries=0;
    std::deque<world::IPlayer*> replies;
    std::function<void()> onQuery;
    world::IPlayer* getLocalHumanPlayer() override {
        ++queries;trace.push_back("player");
        world::IPlayer* result=local;
        if(!replies.empty()){result=replies.front();replies.pop_front();}
        if(onQuery)onQuery();return result;
    }
};
struct Game : ActionGameBoundary {
    unsigned sends=0, notices=0, flags=0, channels=0, types=0;
    TargetChannel channel=TCInvalid;
    std::string text;
    std::function<void()> onFlags,onChannel,onType,onSend,onNotice;
    bool radioMessage(TargetChannel,const std::string&,world::IPlayer*,unsigned,bool,bool) override {actionUnsupported("radio");}
    void sendGameEvent(unsigned,GameEvent*) override {actionUnsupported("event send");}
    void setCurrentFlags(int value) override {++flags;check(value==0,"send clears current flags");if(onFlags)onFlags();}
    void setCurrentChannel(TargetChannel value) override {++channels;channel=value;if(onChannel)onChannel();}
    void setCurrentType(ChannelMessageType value) override {++types;check(value==2,"send selects type2");if(onType)onType();}
    bool say(const std::string& value) override {++sends;text=value;if(onSend)onSend();return false;}
    bool localGameInfoMessage(TargetChannel value,const std::string& message) override {
        ++notices;check(value==14&&message=="spam","spam notification channel/text");
        if(onNotice)onNotice();return false;
    }
};
struct Logic : ActionLogicBoundary {
    unsigned requests=0;
    world::IPlayer* player=nullptr;
    std::string teamName="USMC";
    std::vector<int> teams;
    const std::string& getTeamName(int team) override {teams.push_back(team);return teamName;}
    bool isServer() const override { actionUnsupported("isServer"); }
    bool selectSpawnGroup(world::IPlayer* value,int group) override {
        ++requests;player=value;check(group==0,"onSpawn selects group zero");return false;
    }
};
struct Scores : world::IScoreManager {
    int winner=0;
    unsigned queries=0;
    world::TeamScore first,second;
    std::vector<unsigned> teams;
    void addRef() override {actionUnsupported("score addRef");}
    int getRef() override {actionUnsupported("score getRef");}
    int release() override {actionUnsupported("score release");}
    void* queryInterface(unsigned) const override {actionUnsupported("score queryInterface");}
    world::TeamScore* getTeamScore(unsigned team) override {teams.push_back(team);return team==1?&first:&second;}
    int getWinner() override {++queries;return winner;}
    void setWinner(unsigned char) override {actionUnsupported("setWinner");}
    void forceSetWinner(unsigned char) override {actionUnsupported("forceSetWinner");}
    void setVictoryType(unsigned char) override {actionUnsupported("setVictoryType");}
    int getVictoryType() override {actionUnsupported("getVictoryType");}
    void setHealScoreLimit(float) override {actionUnsupported("setHealScoreLimit");}
    float getHealScoreLimit() override {actionUnsupported("getHealScoreLimit");}
    void setRepairScoreLimit(float) override {actionUnsupported("setRepairScoreLimit");}
    float getRepairScoreLimit() override {actionUnsupported("getRepairScoreLimit");}
    void setAmmoScoreLimit(float) override {actionUnsupported("setAmmoScoreLimit");}
    float getAmmoScoreLimit() override {actionUnsupported("getAmmoScoreLimit");}
    void setTeamDamageLimit(float) override {actionUnsupported("setTeamDamageLimit");}
    float getTeamDamageLimit() override {actionUnsupported("getTeamDamageLimit");}
    void setTeamVehicleDamageLimit(float) override {actionUnsupported("setTeamVehicleDamageLimit");}
    float getTeamVehicleDamageLimit() override {actionUnsupported("getTeamVehicleDamageLimit");}
    void reset() override {actionUnsupported("score reset");}
};
// [TEMP] Observe only the actual round-result sound callback; no sound engine is run.
struct Sound : BfSoundNull {
    std::vector<std::string> played;
    void playSound(const std::string& value) override {played.push_back(value);}
};
struct Hud : BF2HudManager {
    bool commander=false;
    int mode=0;
    unsigned echoes=0, spamNotices=0;
    std::deque<int> modes;
    std::vector<std::string> localized;
    std::wstring echoed, spamText;
    std::wstring connectionText;
    std::wstring replaceWideInputKeys(std::wstring value) override {
        trace.push_back("inputKeys");return value+L" [KEY]";
    }
    void showConnectionMessage(std::wstring value) override {connectionText=value;}
    world::IPlayer* echoPlayer=nullptr;
    int echoTeam=-1, spamTeam=-1;
    std::function<void()> onMode;
    bool isCommanderShown() override {return commander;}
    int getMode() override {
        int result=mode;if(!modes.empty()){result=modes.front();modes.pop_front();}return result;
    }
    void setMode(int value,bool save) override {check(save,"onSpawn saves previous mode");mode=value;if(onMode)onMode();}
    std::wstring getLocalizedString(std::string key) override {
        localized.push_back(key);
        if(key.empty())return L"";
        if(key=="HUD_CHAT_DEADPREFIX")return L"DEAD ";
        if(key=="HUD_HUD_MENU_CHAT_SPAM_MESSAGE")return L"wait #SEC# seconds";
        if(key=="HUD_HUD_DRAW_STRING")return L"Draw";
        if(key=="HUD_HUD_WIN_STRING")return L"#TEAMNAME# wins";
        if(key=="HUD_TEAMNAME_usmc")return std::wstring(L"Marines\0discarded",17);
        actionUnsupported("localization key");
    }
    void addGlobalChat(std::wstring text,world::IPlayer* player,int team) override {
        ++echoes;echoed=text;echoPlayer=player;echoTeam=team;
    }
    void addServerChat(std::wstring text,int team) override {++spamNotices;spamText=text;spamTeam=team;}
};
int main() {
    static_assert(sizeof(void*)==4,"x86 only");
    static_assert(sizeof(BF2HudManager)==824,"native HUD layout remains unchanged");
    EventManager events;eventManager=&events;
    Players players;world::playerManager=&players;
    Player first,second,third;
    Game g1,g2,g3,g4;game=&g1;
    void* storage=::operator new(sizeof(Hud));
    std::memset(storage,0x5a,sizeof(Hud));
    Hud* hud=new(storage)Hud;hudManager=hud;
    dice::meme::StringData regular(""),commander("command");
    hud->ref2System=nullptr;hud->m_field0a8[0]=&regular;hud->m_field0a8[1]=&commander;
    auto reset=[&] {
        trace.clear();hud->localized.clear();hud->echoes=hud->spamNotices=0;
        hud->commander=false;hud->onMode={};hud->mode=0;hud->modes.clear();
        players.queries=0;players.replies.clear();players.onQuery={};players.local=&first;
        first.alive=second.alive=true;first.spam=second.spam=false;
        first.onSpam={};first.onTimer={};first.onTeam={};first.team=2;
        g1.onFlags={};g1.onChannel={};g1.onType={};g1.onSend={};g1.onNotice={};
        g2.onChannel={};g3.onType={};game=&g1;gameLogic=nullptr;
        regular.setString(nullptr,"hello");
    };
    reset();regular.setString(nullptr,"");players.local=nullptr;
    check(hud->sendSayAll()&&players.queries==1&&!g1.sends&&hud->localized==std::vector<std::string>{""},
          "empty all-chat still localizes empty key and selects data without second player query");
    reset();players.local=nullptr;
    check(hud->sendSayAll()&&players.queries==2&&!g1.sends,"nonempty input with no player stops after second query");
    reset();first.alive=false;third.team=7;players.replies={&first,&second,&third};
    check(hud->sendSayAll()&&g1.text=="DEAD hello"&&g1.channel==TCGlobal,
          "all-chat localizes dead prefix before actual string conversion/send");
    check(players.queries==3&&hud->echoPlayer==&third&&hud->echoTeam==7&&hud->echoes==1,
          "all-chat echo rereads local player after false say result");
    reset();first.alive=false;second.team=5;players.replies={&first,&second};
    g1.onSend=[&]{players.local=&third;};
    check(hud->sendSayTeam()&&g1.text=="HUD_TEXT_CHAT_TEAMHUD_CHAT_DEADPREFIXhello"&&g1.channel==TCTeam,
          "team-chat keeps literal prefix keys, including dead key");
    check(players.queries==2&&hud->echoPlayer==&second&&hud->echoTeam==5&&hud->localized.empty(),
          "team echo uses captured spam-check player with no post-send lookup or localization");
    reset();hud->commander=true;
    check(hud->sendSayTeam()&&g1.text=="HUD_TEXT_CHAT_COMMANDERcommand"&&g1.channel==TCTeam,
          "commander team-chat reads alternate data and still selects team channel");
    reset();g1.onFlags=[&]{game=&g2;};g2.onChannel=[&]{game=&g3;};g3.onType=[&]{game=&g4;};
    hud->sendSayAll();
    check(g2.channels==1&&g3.types==1&&g4.sends==1&&g4.text=="hello",
          "each game configuration/send reloads receiver after previous callback");
    reset();first.spam=true;first.spamTime=100.25f;serverInfo->m_block.radioBlockedDuration=30;
    WorldPref::resetWorldTime(104.75);
    first.onTimer=[&]{serverInfo->m_block.radioBlockedDuration=900;};
    const unsigned sendsBefore=g1.sends;
    hud->sendSayAll();
    check(hud->spamText==L"wait 26 seconds"&&hud->spamNotices==1&&g1.sends==sendsBefore,
          "spam captures duration before timer callback and truncates using real world clock");
    check(players.queries==2&&hud->spamTeam==2,"spam retains player and queries team after localization");
    reset();first.spam=true;first.spamTime=1;serverInfo->m_block.radioBlockedDuration=-4;
    WorldPref::resetWorldTime(0.75);first.onSpam=[&]{game=nullptr;};
    hud->sendSayTeam();
    check(hud->spamText==L"wait -2 seconds"&&hud->spamNotices==1,
          "spam without game retains negative countdown and HUD notification");
    reset();first.spam=true;first.spamTime=std::numeric_limits<float>::infinity();
    unsigned short oldControl,customControl,afterControl;
    __asm fnstcw oldControl
    customControl=static_cast<unsigned short>((oldControl&~0xC00u)|0x800u);
    __asm fldcw customControl
    hud->sendSayAll();
    __asm fnstcw afterControl
    __asm fldcw oldControl
    check(afterControl==customControl&&hud->spamText==L"wait -2147483648 seconds",
          "x87 indefinite overflow result and caller rounding control are preserved");
    reset();HudKitSelection kit;hud->m_field11c=&kit;hud->m_field2ac=7.5f;
    hud->mode=9;hud->m_field24a=false;hud->onSpawn();
    check(hud->m_field24c&&hud->m_field228&&hud->m_field2a8==7.5f&&!hud->m_field24a&&hud->mode==9,
          "spawn commits flags/previous value, preserves scoreboard and done byte");
    reset();hud->m_field11c=&kit;first.spawnTick=1;first.spawnTime=2.0f;hud->onSpawn();
    check(!hud->m_field24a&&trace.back()=="spawnTime","spawn threshold excludes exactly two seconds");
    reset();first.spawnTime=2.01f;hud->onSpawn();check(hud->m_field24a,"spawn threshold accepts greater time");
    reset();first.spawnTick=0;hud->onSpawn();
    check(hud->m_field24a&&trace.back()=="spawnTick","zero spawn tick bypasses time query");
    reset();players.local=nullptr;hud->onSpawn();check(!hud->m_field24a,"spawn no-player arm clears done");
    reset();Logic logic,replacement;gameLogic=&logic;hud->modes={0,1};
    players.onQuery=[&]{gameLogic=&replacement;};hud->onSpawn();
    check(replacement.requests==1&&!logic.requests&&replacement.player==&first,
          "spawn selection reloads game logic after player lookup");
    hud->m_field11c=nullptr;
    reset();Scores scores;world::scoreManager=&scores;Sound sound;g_bfSound=&sound;
    gameLogic=&logic;players.local=nullptr;hud->playRoundResultSound();
    check(!scores.queries&&sound.played.empty(),"round sound has the retail local-player guard");
    players.local=&first;scores.winner=0;unsigned teamCalls=0;
    first.onTeam=[&]{++teamCalls;};hud->playRoundResultSound();
    check(teamCalls==1&&sound.played.back()=="usmc_loose"&&hud->m_field304==sound.played.back(),
          "draw plays lowercased loose sound, queries team once, and stores it first");
    scores.winner=0x102;first.team=2;teamCalls=0;hud->playRoundResultSound();
    check(teamCalls==2&&sound.played.back()=="usmc_win","sound consumes winner low byte and repeats player team query");
    scores.winner=1;hud->playRoundResultSound();
    check(sound.played.back()=="usmc_loose","loser preserves shipped loose spelling");
    hud->m_field320=true;unsigned oldQueries=players.queries;hud->showRoundResult();
    check(players.queries==oldQueries,"round text suppression precedes local-player query");
    hud->m_field320=false;players.local=nullptr;hud->showRoundResult();
    check(hud->connectionText.empty(),"round text requires a local player");
    players.local=&first;scores.winner=0x100;hud->showRoundResult();
    check(hud->connectionText==L"Draw","round draw uses winner low byte and localized draw text");
    scores.winner=1;scores.first.m_tickets=123;scores.second.m_tickets=45;
    hud->localized.clear();scores.teams.clear();hud->showRoundResult();
    check(hud->connectionText==L"Marines wins [KEY] (123 - 45)",
          "win message replaces input keys and team token, truncates localized team at embedded null, formats tickets");
    check(scores.teams==std::vector<unsigned>{1,2}&&hud->localized==std::vector<std::string>{"HUD_TEAMNAME_usmc","HUD_HUD_WIN_STRING"},
          "round result preserves score and localization query order");
    scores.winner=2;scores.teams.clear();hud->showRoundResult();
    check(scores.teams==std::vector<unsigned>{2,1}&&hud->connectionText==L"Marines wins [KEY] (45 - 123)",
          "team two displays winner tickets before other team");
    check(formatString(L"%s:%d",L"wide",-7)==L"wide:-7","wide formatter uses retail wide percent-s semantics");
    check(formatString(L"%s",std::wstring(5000,L'x').c_str())==
          L"WARNING: The original message was too long for formatString(). \n",
          "wide formatter replaces overlong output with complete warning");
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();
    hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();
    ::operator delete(storage);hudManager=nullptr;
    std::printf("HUD chat send/spawn/round result: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
