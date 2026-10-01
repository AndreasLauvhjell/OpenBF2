// [INFERRED] path. Client-only class read from BF2.exe: the dedicated ELF looks the singleton up
// by name but carries no such class.
#include "GameServerSettings.h"
#if !DEDICATED
#include "GameServer/ServerSettings.h"    // serverSettings - the dedicated record this class mirrors
#include "../Core/Ids.h"
#include "../Core/SettingsRepostitory.h"  // settings - GSModDirectory
#include "../IO/Console/Console.h"        // io::mainConsole - runs serversettings.con
#include "../IO/File/FileManager.h"       // io::fileManager - the load gate
#include <new>
#include <sstream>

namespace dice { namespace hfe {
namespace {
// ..4B4072: the four friendly-fire percentages are clamped into 0..100 before the store.
int clampPercent(int value)
{
    if (value >= 0) {
        if (value > 100) value = 100;
    } else {
        value = 0;
    }
    return value;
}
// : the shared float clamp, upper bound tested first, then the lower bound.
float clampFloat(float value, float low, float high)
{
    float result = value;
    if (value >= high) result = high;
    if (result < low) return low;
    return result;
}
}

// [CHOSEN, not recovered] Defaults autoRecordRounds to 10 and friendlyFireWithMines to false.
// The original leaves these values uninitialized on some reset paths.
GameServerSettings::GameServerSettings() : m_refCount(1), m_autoRecordRounds(10), m_friendlyFireWithMines(false)
{
    reset();
}

//: the thirteen strings in reverse order, which the compiler reproduces.
GameServerSettings::~GameServerSettings() = default;

//: the settings-factory shape - aggregation refused, 520 retail bytes, construction only
// on a non-null allocation, unconditional queryInterface (the shipped null defect is kept).
IBase* GameServerSettings::create(unsigned int iid, IBase* outer)
{
    if (outer) return nullptr;
    void* allocation = ::operator new(sizeof(GameServerSettings));
    GameServerSettings* owner = allocation ? new (allocation) GameServerSettings : nullptr;
    IBase* result = static_cast<IBase*>(owner->queryInterface(iid));
    if (!result) owner->release();
    return result;
}
void GameServerSettings::addRef() { ++m_refCount; }
int GameServerSettings::getRef() { return m_refCount; }
//: decrement, destroy through the (withheld) deleting destructor at zero.
int GameServerSettings::release()
{
    const int references = --m_refCount;
    if (!references) delete this;
    return references;
}
//: IBase, then IGameServerSettings (dword_892C98 = 121047).
void* GameServerSettings::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IGameServerSettings)
        return const_cast<GameServerSettings*>(this);
    return nullptr;
}

std::string GameServerSettings::getFileName() { return std::string("ServerSettings.con"); }

// (1,641 B): thirty-six rows. The server name and password insert c_str() in quotes; the
// VoIP remote IP and shared password insert the std::string unquoted; bools print as 0/1.
std::string GameServerSettings::getSettingsScript()
{
    std::stringstream stream;
    stream << "GameServerSettings.setServerName \"" << m_serverName.c_str() << "\"" << std::endl;
    stream << "GameServerSettings.setPassword \"" << m_password.c_str() << "\"" << std::endl;
    stream << "GameServerSettings.setInternet " << m_internet << std::endl;
    stream << "GameServerSettings.setMaxPlayers " << m_maxPlayers << std::endl;
    stream << "GameServerSettings.setSpawnTime " << m_spawnTime << std::endl;
    stream << "GameServerSettings.setManDownTime " << m_manDownTime << std::endl;
    stream << "GameServerSettings.setTicketRatio " << m_ticketRatio << std::endl;
    stream << "GameServerSettings.setRoundsPerMap " << m_roundsPerMap << std::endl;
    stream << "GameServerSettings.setTimeLimit " << m_timeLimit << std::endl;
    stream << "GameServerSettings.setScoreLimit " << m_scoreLimit << std::endl;
    stream << "GameServerSettings.setSoldierFF " << m_soldierFF << std::endl;
    stream << "GameServerSettings.setVehicleFF " << m_vehicleFF << std::endl;
    stream << "GameServerSettings.setSoldierSplashFF " << m_soldierSplashFF << std::endl;
    stream << "GameServerSettings.setVehicleSplashFF " << m_vehicleSplashFF << std::endl;
    stream << "GameServerSettings.setPunishTeamKills " << m_punishTeamKills << std::endl;
    stream << "GameServerSettings.setVotingEnabled " << m_votingEnabled << std::endl;
    stream << "GameServerSettings.setVoteTime " << m_voteTime << std::endl;
    stream << "GameServerSettings.setMinPlayersForVoting " << m_minPlayersForVoting << std::endl;
    stream << "GameServerSettings.setTeamVoteOnly " << m_teamVoteOnly << std::endl;
    stream << "GameServerSettings.setVoipEnabled " << m_voipEnabled << std::endl;
    stream << "GameServerSettings.setVoipQuality " << m_voipQuality << std::endl;
    stream << "GameServerSettings.setVoipServerRemote " << m_voipServerRemote << std::endl;
    stream << "GameServerSettings.setVoipServerRemoteIP " << m_voipServerRemoteIP << std::endl;
    stream << "GameServerSettings.setVoipServerPort " << m_voipServerPort << std::endl;
    stream << "GameServerSettings.setVoipBFClientPort " << m_voipBFClientPort << std::endl;
    stream << "GameServerSettings.setVoipBFServerPort " << m_voipBFServerPort << std::endl;
    stream << "GameServerSettings.setVoipSharedPassword " << m_voipSharedPassword << std::endl;
    stream << "GameServerSettings.setAutoRecord " << m_autoRecord << std::endl;
    stream << "GameServerSettings.setSvPunkBuster " << m_svPunkBuster << std::endl;
    stream << "GameServerSettings.setTeamRatio " << m_teamRatio << std::endl;
    stream << "GameServerSettings.setAutoBalanceTeam " << m_autoBalanceTeam << std::endl;
    stream << "GameServerSettings.setFriendlyFireWithMines " << m_friendlyFireWithMines << std::endl;
    stream << "GameServerSettings.setCoopBotRatio " << m_coopBotRatio << std::endl;
    stream << "GameServerSettings.setCoopBotCount " << m_coopBotCount << std::endl;
    stream << "GameServerSettings.setCoopBotDifficulty " << m_coopBotDifficulty << std::endl;
    stream << "GameServerSettings.setNoVehicles " << m_noVehicles << std::endl;
    return stream.str();
}

// Copies settings to the dedicated record, clamps percentages, and saves it; requires that record.
// Setter error messages are discarded.
bool GameServerSettings::apply()
{
    if (!serverSettings) return false;
    serverSettings->m_serverName = m_serverName;
    serverSettings->setPassword(m_password);
    serverSettings->setInternet(m_internet);
    serverSettings->setMaxPlayers(m_maxPlayers);
    serverSettings->m_spawnTime = m_spawnTime;
    serverSettings->m_manDownTime = m_manDownTime;
    serverSettings->m_ticketRatio = m_ticketRatio;
    serverSettings->m_roundsPerMap = m_roundsPerMap;
    serverSettings->m_scoreLimit = m_scoreLimit;
    serverSettings->setTimeLimit(m_timeLimit);
    serverSettings->m_soldierFriendlyFire = clampPercent(m_soldierFF);
    serverSettings->m_vehicleFriendlyFire = clampPercent(m_vehicleFF);
    serverSettings->m_soldierSplashFriendlyFire = clampPercent(m_soldierSplashFF);
    serverSettings->m_vehicleSplashFriendlyFire = clampPercent(m_vehicleSplashFF);
    serverSettings->m_tkPunishEnabled = m_punishTeamKills;
    serverSettings->setVotingEnabled(m_votingEnabled);
    serverSettings->m_voteTime = m_voteTime;
    serverSettings->m_minPlayersForVoting = m_minPlayersForVoting;
    serverSettings->setTeamVoteOnly(m_teamVoteOnly);
    serverSettings->m_voipEnabled = m_voipEnabled;
    serverSettings->m_voipQuality = m_voipQuality;
    serverSettings->m_voipServerRemote = m_voipServerRemote;
    serverSettings->m_voipServerRemoteIP = m_voipServerRemoteIP;
    serverSettings->m_voipServerPort = m_voipServerPort;
    serverSettings->m_voipBFClientPort = m_voipBFClientPort;
    serverSettings->m_voipBFServerPort = m_voipBFServerPort;
    serverSettings->m_voipSharedPassword = std::string(m_voipSharedPassword); //  copies a temporary
    serverSettings->m_autoRecord = m_autoRecord;
    serverSettings->m_punkBuster = m_svPunkBuster;
    serverSettings->m_autoBalanceTeam = m_autoBalanceTeam;
    serverSettings->m_teamRatioPercent = m_teamRatio;
    serverSettings->setFriendlyFireWithMines(m_friendlyFireWithMines);
    serverSettings->m_coopBotRatio = clampFloat(m_coopBotRatio, 0.0f, 100.0f);
    serverSettings->setCoopBotCount(m_coopBotCount);
    serverSettings->m_coopBotDifficulty = clampFloat(m_coopBotDifficulty, 10.0f, 100.0f);
    serverSettings->setNoVehicles(m_noVehicles);
    serverSettings->save();
    return true;
}

// Loads the mod's settings/serversettings.con and mirrors the dedicated record.
// Without a file manager, installs the built-in defaults.
void GameServerSettings::reset()
{
    std::string path(settings->stringGet(std::string("GSModDirectory"), std::string("mods/bf2")));
    path += "/settings/serversettings.con";
    if (io::fileManager) {
        std::string out;
        io::mainConsole->run(path, "", "", "", "", "", "", "", "", out);
        m_serverName = serverSettings->m_serverName;
        m_password = serverSettings->getPassword();
        m_internet = serverSettings->m_internet;
        m_bandwidthChoke = serverSettings->m_bandwidthChoke;
        m_maxPlayers = serverSettings->getMaxPlayers();
        m_gameMode = "";
        m_startDelay = serverSettings->m_startDelay;
        m_endDelay = serverSettings->m_endDelay;
        m_spawnTime = serverSettings->getSpawnTime();
        m_manDownTime = serverSettings->getManDownTime();
        m_ticketRatio = serverSettings->getTicketRatio();
        m_roundsPerMap = serverSettings->m_roundsPerMap;
        m_timeLimit = serverSettings->m_timeLimit;
        m_scoreLimit = serverSettings->m_scoreLimit;
        m_soldierFF = serverSettings->m_soldierFriendlyFire;
        m_vehicleFF = serverSettings->m_vehicleFriendlyFire;
        m_soldierSplashFF = serverSettings->m_soldierSplashFriendlyFire;
        m_vehicleSplashFF = serverSettings->m_vehicleSplashFriendlyFire;
        m_punishTeamKills = serverSettings->m_tkPunishEnabled;
        m_votingEnabled = serverSettings->m_votingEnabled;
        m_voteTime = serverSettings->m_voteTime;
        m_minPlayersForVoting = serverSettings->m_minPlayersForVoting;
        m_teamVoteOnly = serverSettings->m_teamVoteOnly;
        m_voipEnabled = serverSettings->m_voipEnabled;
        m_voipQuality = serverSettings->m_voipQuality;
        m_voipServerRemote = serverSettings->m_voipServerRemote;
        m_voipServerRemoteIP = serverSettings->m_voipServerRemoteIP;
        m_voipBFServerPort = serverSettings->m_voipBFServerPort;
        m_voipBFClientPort = serverSettings->m_voipBFClientPort;
        m_voipServerPort = serverSettings->m_voipServerPort;
        m_voipSharedPassword = std::string(serverSettings->m_voipSharedPassword); //  copies a temporary
        m_gameSpyPort = serverSettings->m_gameSpyPort;
        m_allowNATNegotiation = serverSettings->m_allowNATNegotiation;
        m_autoRecord = serverSettings->m_autoRecord;
        m_timeBeforeRestartMap = serverSettings->m_timeBeforeRestartMap;
        m_autoBalanceTeam = serverSettings->m_autoBalanceTeam;
        m_teamRatio = serverSettings->getTeamRatioPercent();
        m_demoIndexURL = serverSettings->m_demoIndexURL;
        m_demoDownloadURL = serverSettings->m_demoDownloadURL;
        m_demoHook = serverSettings->m_autoDemoHook;
        m_adminScript = serverSettings->m_adminScript;
        m_interfaceIP = serverSettings->m_interfaceIP;
        m_sponsorText = serverSettings->m_sponsorText;
        m_sponsorLogoURL = serverSettings->m_sponsorLogoURL;
        m_svPunkBuster = serverSettings->getSvPunkBuster();
        m_coopBotRatio = serverSettings->getCoopBotRatio();
        m_coopBotCount = serverSettings->getCoopBotCount();
        m_coopBotDifficulty = serverSettings->getCoopBotDifficulty();
        m_noVehicles = serverSettings->getNoVehicles();
    } else {
        m_serverName = "Battlefield-2 Server";
        m_password = "";
        m_internet = false;
        m_bandwidthChoke = 0;
        m_maxPlayers = 16;
        m_gameMode = "gpm_cq";
        m_startDelay = 2;
        m_minPlayersForVoting = 2;
        m_endDelay = 10;
        m_spawnTime = 5.0f;
        m_manDownTime = 10.0f;
        m_ticketRatio = 100;
        m_roundsPerMap = 3;
        m_timeLimit = 0;
        m_scoreLimit = 0;
        m_soldierFF = 100;
        m_vehicleFF = 100;
        m_soldierSplashFF = 100;
        m_vehicleSplashFF = 100;
        m_punishTeamKills = true;
        m_votingEnabled = true;
        m_voteTime = 60;
        m_teamVoteOnly = true;
        m_voipEnabled = true;
        m_voipQuality = 5;
        m_voipServerRemote = false;
        m_voipServerRemoteIP = "127.0.0.1";
        m_voipServerPort = 55125;
        m_voipBFClientPort = 55123;
        m_voipBFServerPort = 55124;
        m_voipSharedPassword = "";
        m_gameSpyPort = 29900;
        m_allowNATNegotiation = false;
        m_autoRecord = false;
        m_timeBeforeRestartMap = 30.0f;
        m_autoBalanceTeam = false;
        m_teamRatio = 100.0f;
        m_autoRecordRounds = 10;
        m_demoIndexURL = "http://server.com/demos/index.html";
        m_demoDownloadURL = "http://server.com/demos/dl/{file}";
        m_demoHook = "rotate_demo.py";
        m_adminScript = "default";
        m_interfaceIP = "";
        m_sponsorText = "";
        m_sponsorLogoURL = "";
        m_svPunkBuster = false;
        // ..4B49BC: the VoIP block is stored a second time.
        m_voipEnabled = true;
        m_voipServerRemoteIP = "127.0.0.1";
        m_voipServerPort = 55125;
        m_voipBFClientPort = 55123;
        m_voipBFServerPort = 55124;
        m_voipSharedPassword = "";
        m_coopBotRatio = 50.0f;
        m_coopBotCount = 16;
        m_coopBotDifficulty = 50.0f;
        m_noVehicles = false;
    }
}

// The fifty-one accessor pairs, table +20..+1B4. Strings return a copy and take their value by
// value, as the sret/retn 1Ch bodies do; every other pair is a plain load/store.
std::string GameServerSettings::getServerName() { return m_serverName; }
void GameServerSettings::setServerName(std::string value) { m_serverName = value; }
std::string GameServerSettings::getPassword() { return m_password; }
void GameServerSettings::setPassword(std::string value) { m_password = value; }
bool GameServerSettings::getInternet() { return m_internet; }
void GameServerSettings::setInternet(bool value) { m_internet = value; }
int GameServerSettings::getBandwidthChoke() { return m_bandwidthChoke; }
void GameServerSettings::setBandwidthChoke(int value) { m_bandwidthChoke = value; }
int GameServerSettings::getMaxPlayers() { return m_maxPlayers; }
void GameServerSettings::setMaxPlayers(int value) { m_maxPlayers = value; }
std::string GameServerSettings::getGameMode() { return m_gameMode; }
void GameServerSettings::setGameMode(std::string value) { m_gameMode = value; }
int GameServerSettings::getStartDelay() { return m_startDelay; }
void GameServerSettings::setStartDelay(int value) { m_startDelay = value; }
int GameServerSettings::getEndDelay() { return m_endDelay; }
void GameServerSettings::setEndDelay(int value) { m_endDelay = value; }
float GameServerSettings::getSpawnTime() { return m_spawnTime; }
void GameServerSettings::setSpawnTime(float value) { m_spawnTime = value; }
float GameServerSettings::getManDownTime() { return m_manDownTime; }
void GameServerSettings::setManDownTime(float value) { m_manDownTime = value; }
int GameServerSettings::getTicketRatio() { return m_ticketRatio; }
void GameServerSettings::setTicketRatio(int value) { m_ticketRatio = value; }
int GameServerSettings::getRoundsPerMap() { return m_roundsPerMap; }
void GameServerSettings::setRoundsPerMap(int value) { m_roundsPerMap = value; }
int GameServerSettings::getTimeLimit() { return m_timeLimit; }
void GameServerSettings::setTimeLimit(int value) { m_timeLimit = value; }
int GameServerSettings::getScoreLimit() { return m_scoreLimit; }
void GameServerSettings::setScoreLimit(int value) { m_scoreLimit = value; }
int GameServerSettings::getSoldierFF() { return m_soldierFF; }
void GameServerSettings::setSoldierFF(int value) { m_soldierFF = value; }
int GameServerSettings::getVehicleFF() { return m_vehicleFF; }
void GameServerSettings::setVehicleFF(int value) { m_vehicleFF = value; }
int GameServerSettings::getSoldierSplashFF() { return m_soldierSplashFF; }
void GameServerSettings::setSoldierSplashFF(int value) { m_soldierSplashFF = value; }
int GameServerSettings::getVehicleSplashFF() { return m_vehicleSplashFF; }
void GameServerSettings::setVehicleSplashFF(int value) { m_vehicleSplashFF = value; }
bool GameServerSettings::getPunishTeamKills() { return m_punishTeamKills; }
void GameServerSettings::setPunishTeamKills(bool value) { m_punishTeamKills = value; }
int GameServerSettings::getVoteTime() { return m_voteTime; }
void GameServerSettings::setVoteTime(int value) { m_voteTime = value; }
int GameServerSettings::getMinPlayersForVoting() { return m_minPlayersForVoting; }
void GameServerSettings::setMinPlayersForVoting(int value) { m_minPlayersForVoting = value; }
bool GameServerSettings::getTeamVoteOnly() { return m_teamVoteOnly; }
void GameServerSettings::setTeamVoteOnly(bool value) { m_teamVoteOnly = value; }
bool GameServerSettings::getVoipEnabled() { return m_voipEnabled; }
void GameServerSettings::setVoipEnabled(bool value) { m_voipEnabled = value; }
int GameServerSettings::getVoipQuality() { return m_voipQuality; }
void GameServerSettings::setVoipQuality(int value) { m_voipQuality = value; }
bool GameServerSettings::getVoipServerRemote() { return m_voipServerRemote; }
void GameServerSettings::setVoipServerRemote(bool value) { m_voipServerRemote = value; }
std::string GameServerSettings::getVoipServerRemoteIP() { return m_voipServerRemoteIP; }
void GameServerSettings::setVoipServerRemoteIP(std::string value) { m_voipServerRemoteIP = value; }
int GameServerSettings::getVoipServerPort() { return m_voipServerPort; }
void GameServerSettings::setVoipServerPort(int value) { m_voipServerPort = value; }
int GameServerSettings::getVoipBFClientPort() { return m_voipBFClientPort; }
void GameServerSettings::setVoipBFClientPort(int value) { m_voipBFClientPort = value; }
int GameServerSettings::getVoipBFServerPort() { return m_voipBFServerPort; }
void GameServerSettings::setVoipBFServerPort(int value) { m_voipBFServerPort = value; }
std::string GameServerSettings::getVoipSharedPassword() { return m_voipSharedPassword; }
void GameServerSettings::setVoipSharedPassword(std::string value) { m_voipSharedPassword = value; }
int GameServerSettings::getGameSpyPort() { return m_gameSpyPort; }
void GameServerSettings::setGameSpyPort(int value) { m_gameSpyPort = value; }
bool GameServerSettings::getAllowNATNegotiation() { return m_allowNATNegotiation; }
void GameServerSettings::setAllowNATNegotiation(bool value) { m_allowNATNegotiation = value; }
std::string GameServerSettings::getInterfaceIP() { return m_interfaceIP; }
void GameServerSettings::setInterfaceIP(std::string value) { m_interfaceIP = value; }
bool GameServerSettings::getAutoRecord() { return m_autoRecord; }
void GameServerSettings::setAutoRecord(bool value) { m_autoRecord = value; }
std::string GameServerSettings::getDemoIndexURL() { return m_demoIndexURL; }
void GameServerSettings::setDemoIndexURL(std::string value) { m_demoIndexURL = value; }
std::string GameServerSettings::getDemoDownloadURL() { return m_demoDownloadURL; }
void GameServerSettings::setDemoDownloadURL(std::string value) { m_demoDownloadURL = value; }
std::string GameServerSettings::getDemoHook() { return m_demoHook; }
void GameServerSettings::setDemoHook(std::string value) { m_demoHook = value; }
std::string GameServerSettings::getAdminScript() { return m_adminScript; }
void GameServerSettings::setAdminScript(std::string value) { m_adminScript = value; }
float GameServerSettings::getTimeBeforeRestartMap() { return m_timeBeforeRestartMap; }
void GameServerSettings::setTimeBeforeRestartMap(float value) { m_timeBeforeRestartMap = value; }
std::string GameServerSettings::getSponsorText() { return m_sponsorText; }
void GameServerSettings::setSponsorText(std::string value) { m_sponsorText = value; }
std::string GameServerSettings::getSponsorLogoURL() { return m_sponsorLogoURL; }
void GameServerSettings::setSponsorLogoURL(std::string value) { m_sponsorLogoURL = value; }
bool GameServerSettings::getAutoBalanceTeam() { return m_autoBalanceTeam; }
void GameServerSettings::setAutoBalanceTeam(bool value) { m_autoBalanceTeam = value; }
int GameServerSettings::getAutoRecordRounds() { return m_autoRecordRounds; }
void GameServerSettings::setAutoRecordRounds(int value) { m_autoRecordRounds = value; }
float GameServerSettings::getTeamRatio() { return m_teamRatio; }
void GameServerSettings::setTeamRatio(float value) { m_teamRatio = value; }
bool GameServerSettings::getSvPunkBuster() { return m_svPunkBuster; }
void GameServerSettings::setSvPunkBuster(bool value) { m_svPunkBuster = value; }
std::string GameServerSettings::getField1D8() { return m_field1D8; }
void GameServerSettings::setField1D8(std::string value) { m_field1D8 = value; }
bool GameServerSettings::getFriendlyFireWithMines() { return m_friendlyFireWithMines; }
void GameServerSettings::setFriendlyFireWithMines(bool value) { m_friendlyFireWithMines = value; }
float GameServerSettings::getCoopBotRatio() { return m_coopBotRatio; }
void GameServerSettings::setCoopBotRatio(float value) { m_coopBotRatio = value; }
int GameServerSettings::getCoopBotCount() { return m_coopBotCount; }
void GameServerSettings::setCoopBotCount(int value) { m_coopBotCount = value; }
float GameServerSettings::getCoopBotDifficulty() { return m_coopBotDifficulty; }
void GameServerSettings::setCoopBotDifficulty(float value) { m_coopBotDifficulty = value; }
bool GameServerSettings::getNoVehicles() { return m_noVehicles; }
void GameServerSettings::setNoVehicles(bool value) { m_noVehicles = value; }

} }
#endif
