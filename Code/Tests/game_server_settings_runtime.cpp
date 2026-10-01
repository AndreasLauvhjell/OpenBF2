// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: GAME-SERVER-SETTINGS-FIXTURE]
// Actual GameServerSettings owner, lifetime, fallback defaults, script and accessors. The
// settings repository is the real class; the file manager, console and dedicated record
// stay null, so reset takes its hard-coded branch and apply its null branch. Nothing
// GameServerSettings owns is shadowed.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "../BF2/Game/GameServerSettings.h"
#include "../BF2/Game/GameServer/ServerSettings.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/IO/Console/Console.h"
#include "../BF2/IO/File/FileManager.h"
using namespace dice::hfe;
static unsigned checks, failures;
static void check(bool value,const char* label)
{ ++checks; if(!value){++failures;std::printf("FAIL %s\n",label);} }

namespace dice { namespace hfe {
// [TEMP] Globals this isolated executable does not link from their owning units.
SettingsRepostitory* settings=nullptr;
ServerSettings* serverSettings=nullptr;
ClassManager* classManager=nullptr;
EventManager* eventManager=nullptr;
namespace io { OldConsole* mainConsole=nullptr; FileManager* fileManager=nullptr; }
struct GameServerSettingsFixture {
    static GameServerSettings& owner(IBase* base){ return *static_cast<GameServerSettings*>(base); }
    static int refs(GameServerSettings& g){ return g.m_refCount; }
    static bool& votingEnabled(GameServerSettings& g){ return g.m_votingEnabled; }
};
} }

// The fifty-one accessor pairs, in table order.
#define GSS_PAIRS(X) \
    X(std::string,ServerName) X(std::string,Password) X(bool,Internet) X(int,BandwidthChoke) X(int,MaxPlayers) \
    X(std::string,GameMode) X(int,StartDelay) X(int,EndDelay) X(float,SpawnTime) X(float,ManDownTime) \
    X(int,TicketRatio) X(int,RoundsPerMap) X(int,TimeLimit) X(int,ScoreLimit) X(int,SoldierFF) X(int,VehicleFF) \
    X(int,SoldierSplashFF) X(int,VehicleSplashFF) X(bool,PunishTeamKills) X(int,VoteTime) X(int,MinPlayersForVoting) \
    X(bool,TeamVoteOnly) X(bool,VoipEnabled) X(int,VoipQuality) X(bool,VoipServerRemote) X(std::string,VoipServerRemoteIP) \
    X(int,VoipServerPort) X(int,VoipBFClientPort) X(int,VoipBFServerPort) X(std::string,VoipSharedPassword) X(int,GameSpyPort) \
    X(bool,AllowNATNegotiation) X(std::string,InterfaceIP) X(bool,AutoRecord) X(std::string,DemoIndexURL) X(std::string,DemoDownloadURL) \
    X(std::string,DemoHook) X(std::string,AdminScript) X(float,TimeBeforeRestartMap) X(std::string,SponsorText) X(std::string,SponsorLogoURL) \
    X(bool,AutoBalanceTeam) X(int,AutoRecordRounds) X(float,TeamRatio) X(bool,SvPunkBuster) X(std::string,Field1D8) \
    X(bool,FriendlyFireWithMines) X(float,CoopBotRatio) X(int,CoopBotCount) X(float,CoopBotDifficulty) X(bool,NoVehicles)

template<class T> struct Sample;
template<> struct Sample<int> { static int value(){ return 4321; } };
template<> struct Sample<float> { static float value(){ return 12.5f; } };
template<> struct Sample<std::string> { static std::string value(){ return std::string("sample value"); } };
template<class T> static void checkPair(IGameServerSettings* api,T (IGameServerSettings::*get)(),void (IGameServerSettings::*set)(T),const char* name)
{
    (api->*set)(Sample<T>::value());
    check((api->*get)()==Sample<T>::value(),name);
}
static void checkPair(IGameServerSettings* api,bool (IGameServerSettings::*get)(),void (IGameServerSettings::*set)(bool),const char* name)
{
    const bool before=(api->*get)();
    (api->*set)(!before);
    check((api->*get)()==!before,name);
    (api->*set)(before);
}
#define GSS_CHECK(T,N) checkPair(api,&IGameServerSettings::get##N,&IGameServerSettings::set##N,"accessor pair " #N);

static bool fallbackDefaults(IGameServerSettings* api,GameServerSettings& g)
{
    return api->getServerName()=="Battlefield-2 Server" && api->getPassword().empty() && !api->getInternet()
        && api->getBandwidthChoke()==0 && api->getMaxPlayers()==16 && api->getGameMode()=="gpm_cq"
        && api->getStartDelay()==2 && api->getMinPlayersForVoting()==2 && api->getEndDelay()==10
        && api->getSpawnTime()==5.0f && api->getManDownTime()==10.0f && api->getTicketRatio()==100
        && api->getRoundsPerMap()==3 && api->getTimeLimit()==0 && api->getScoreLimit()==0
        && api->getSoldierFF()==100 && api->getVehicleFF()==100 && api->getSoldierSplashFF()==100 && api->getVehicleSplashFF()==100
        && api->getPunishTeamKills() && GameServerSettingsFixture::votingEnabled(g) && api->getVoteTime()==60 && api->getTeamVoteOnly()
        && api->getVoipEnabled() && api->getVoipQuality()==5 && !api->getVoipServerRemote() && api->getVoipServerRemoteIP()=="127.0.0.1"
        && api->getVoipServerPort()==55125 && api->getVoipBFClientPort()==55123 && api->getVoipBFServerPort()==55124
        && api->getVoipSharedPassword().empty() && api->getGameSpyPort()==29900 && !api->getAllowNATNegotiation()
        && !api->getAutoRecord() && api->getTimeBeforeRestartMap()==30.0f && !api->getAutoBalanceTeam()
        && api->getTeamRatio()==100.0f && api->getAutoRecordRounds()==10
        && api->getDemoIndexURL()=="http://server.com/demos/index.html" && api->getDemoDownloadURL()=="http://server.com/demos/dl/{file}"
        && api->getDemoHook()=="rotate_demo.py" && api->getAdminScript()=="default" && api->getInterfaceIP().empty()
        && api->getSponsorText().empty() && api->getSponsorLogoURL().empty() && !api->getSvPunkBuster()
        && api->getCoopBotRatio()==50.0f && api->getCoopBotCount()==16 && api->getCoopBotDifficulty()==50.0f && !api->getNoVehicles();
}
static const char* const defaultScript=
    "GameServerSettings.setServerName \"Battlefield-2 Server\"\nGameServerSettings.setPassword \"\"\nGameServerSettings.setInternet 0\n"
    "GameServerSettings.setMaxPlayers 16\nGameServerSettings.setSpawnTime 5\nGameServerSettings.setManDownTime 10\n"
    "GameServerSettings.setTicketRatio 100\nGameServerSettings.setRoundsPerMap 3\nGameServerSettings.setTimeLimit 0\n"
    "GameServerSettings.setScoreLimit 0\nGameServerSettings.setSoldierFF 100\nGameServerSettings.setVehicleFF 100\n"
    "GameServerSettings.setSoldierSplashFF 100\nGameServerSettings.setVehicleSplashFF 100\nGameServerSettings.setPunishTeamKills 1\n"
    "GameServerSettings.setVotingEnabled 1\nGameServerSettings.setVoteTime 60\nGameServerSettings.setMinPlayersForVoting 2\n"
    "GameServerSettings.setTeamVoteOnly 1\nGameServerSettings.setVoipEnabled 1\nGameServerSettings.setVoipQuality 5\n"
    "GameServerSettings.setVoipServerRemote 0\nGameServerSettings.setVoipServerRemoteIP 127.0.0.1\nGameServerSettings.setVoipServerPort 55125\n"
    "GameServerSettings.setVoipBFClientPort 55123\nGameServerSettings.setVoipBFServerPort 55124\nGameServerSettings.setVoipSharedPassword \n"
    "GameServerSettings.setAutoRecord 0\nGameServerSettings.setSvPunkBuster 0\nGameServerSettings.setTeamRatio 100\n"
    "GameServerSettings.setAutoBalanceTeam 0\nGameServerSettings.setFriendlyFireWithMines 0\nGameServerSettings.setCoopBotRatio 50\n"
    "GameServerSettings.setCoopBotCount 16\nGameServerSettings.setCoopBotDifficulty 50\nGameServerSettings.setNoVehicles 0\n";

int main()
{
    check(sizeof(void*)==4,"x86 fixture");
    SettingsRepostitory repository; settings=&repository;   // GSModDirectory absent: the "mods/bf2" default path is built
    IBase* object=GameServerSettings::create(IID_IGameServerSettings,nullptr);
    check(object!=nullptr,"factory yields the owner for its interface id");
    IGameServerSettings* api=static_cast<IGameServerSettings*>(object);
    GameServerSettings& owner=GameServerSettingsFixture::owner(object);
    check(GameServerSettingsFixture::refs(owner)==1 && api->getRef()==1,"refcount starts at one and queryInterface does not retain");
    check(api->queryInterface(IID_IBase)==object && api->queryInterface(IID_IGameServerSettings)==object && !api->queryInterface(0xffffffff),
        "queryInterface accepts IBase and IGameServerSettings only");
    check(!GameServerSettings::create(IID_IGameServerSettings,reinterpret_cast<IBase*>(1)),"aggregation is refused before allocation");
    check(!GameServerSettings::create(0xffffffff,nullptr),"unsupported factory id releases the fresh owner");
    check(api->getFileName()=="ServerSettings.con","actual filename virtual");
    check(fallbackDefaults(api,owner) && api->getField1D8().empty(),"construction runs reset: without a file manager every field carries the hard-coded default");
    check(api->getSettingsScript()==defaultScript,"exact fallback script: thirty-six rows, quoted name/password, unquoted VoIP strings");
    // FriendlyFireWithMines is assigned by neither reset branch; the constructor's chosen false
    // is what the fallback script row above shows.
    GSS_PAIRS(GSS_CHECK)
    check(api->getSettingsScript()!=defaultScript,"changed accessors reach the script");
    api->setVoipQuality(5);
    check(api->getVoipQuality()==5 && api->getSettingsScript().find("GameServerSettings.setVoipQuality 5\n")!=std::string::npos,
        "the default profile's VoIP quality write is visible in the script");
    api->reset();
    check(fallbackDefaults(api,owner) && api->getSettingsScript()==defaultScript,"reset restores the fallback record after every accessor changed");
    check(api->getField1D8()=="sample value","reset leaves the unresolved +1D8 string alone, as neither retail branch touches it");
    api->setServerName(std::string("a\0b",3));
    check(api->getSettingsScript().find("GameServerSettings.setServerName \"a\"\n")!=std::string::npos,"quoted rows insert c_str and truncate at an embedded NUL");
    api->setVoipServerRemoteIP(std::string("1\0" "2",3));
    check(api->getSettingsScript().find(std::string("GameServerSettings.setVoipServerRemoteIP 1\0" "2\n",44))!=std::string::npos,"unquoted VoIP rows insert the whole std::string");
    const std::string beforeApply=api->getSettingsScript();
    check(!api->apply() && api->getSettingsScript()==beforeApply,"apply without the dedicated record returns false and changes nothing");
    api->addRef();
    check(api->release()==1,"non-final release keeps the owner");
    check(api->release()==0,"final release reports zero");
    std::printf("GameServerSettings: %u/%u passed; native owner=%zu\n",checks-failures,checks,sizeof(GameServerSettings));
    return failures?1:0;
}
