// [TEMP] [CHOSEN, not recovered]
// Checks the complete server-console registration order, access flags, and binding shapes.
// Null game/gameLogic exclude running-server refusals and time-limit round shifts.
#include "../BF2/Core/ConsoleObjectMacro.h"
#include "../BF2/Core/ConsoleObjects.h"
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/Core/SmartItf.h"
#include "../BF2/Game/GameServer/ServerInfo.h"
#include "../BF2/Game/GameServer/ServerSettings.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace dice::hfe;
namespace dice { namespace hfe { ConsoleObjectList& serverSettingsConsoleObjects(); } }
namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", label);
    std::fflush(stdout);
}
ConsoleObject* find(const char* name)
{
    for (ConsoleObject* object : serverSettingsConsoleObjects())
        if (std::strcmp(object->getFunctionName(), name) == 0) return object;
    return 0;
}
// A type-1 read/write property: argument and return type are the same spelling.
void property(const char* name, const char* type)
{
    ConsoleObject* object = find(name);
    check(object != 0, name);
    if (!object) return;
    check(object->getType() == 1 && object->getAccess() == 1 && object->getCategory() == 0,
          "read/write property, access 1, category 0");
    check(object->getMinNoArgs() == 0 && object->getMaxNoArgs() == 1 && object->isDynamic(),
          "zero/one argument dynamic binding");
    check(std::strcmp(object->getArgType(0), type) == 0
          && std::strcmp(object->getReturnType(), type) == 0, "retail argument/return type");
    check(object->isActive() && !object->hasRange(), "settings receiver active, no range");
}
// The status-string family: registered as a method (type 0) whose reply is always a std::string.
void statusProperty(const char* name, const char* argType)
{
    ConsoleObject* object = find(name);
    check(object != 0, name);
    if (!object) return;
    check(object->getType() == 0 && object->getAccess() == 1 && object->getCategory() == 0,
          "status-string binding registered as a method");
    check(object->getMinNoArgs() == 0 && object->getMaxNoArgs() == 1 && object->isDynamic(),
          "zero/one argument dynamic binding");
    check(std::strcmp(object->getArgType(0), argType) == 0
          && std::strcmp(object->getReturnType(), "std::string") == 0,
          "retail argument type with std::string reply");
}
std::string set(ConsoleObject* object, const char* text)
{
    std::string value(text);
    return object->execute(&value, 1);
}
std::string get(ConsoleObject* object) { return object->execute(0, 0); }
}
int main()
{
    // metadata, one per shape
    property("serverIP", "std::string");
    property("interfaceIP", "std::string");
    property("gameSpyPort", "int");
    property("allowFreeCam", "bool");
    property("maxConnectionType", "ConnectionType");   // spelling differs from the C++ storage
    property("noVehicles", "float");                   // declared float, means bool
    property("coopBotCount", "int");
    property("spawnTime", "float");
    statusProperty("password", "std::string");
    statusProperty("serverPort", "int");
    statusProperty("numReservedSlots", "int");
    statusProperty("maxPlayers", "int");
    statusProperty("internet", "bool");
    {
        ConsoleObject* load = find("load");
        check(load != 0 && load->getType() == 0 && load->getMinNoArgs() == 0
              && load->getMaxNoArgs() == 0 && std::strcmp(load->getReturnType(), "bool") == 0,
              "sv.load is an argc-0 method returning bool");
    }

    // the complete table, in the PE initializer table's order
    // 0x82AC74..0x82AD8C are access 1; 0x82AD90 opens access 2 for the last two. `save` (0x82AC78)
    // is the one deliberate omission - ServerSettings::save is not recovered.
    static const char* const ordered[] = {
        "load", "configFile", "serverName", "password", "internet", "maxConnectionType",
        "welcomeMessage", "serverIP", "serverPort", "allowFreeCam", "allowExternalViews",
        "allowNoseCam", "hitIndicator", "maxPlayers", "numReservedSlots",
        "numPlayersNeededToStart", "notEnoughPlayersRestartDelay", "startDelay", "endDelay",
        "spawnTime", "manDownTime", "endOfRoundDelay", "ticketRatio", "roundsPerMap", "timeLimit",
        "scoreLimit", "soldierFriendlyFire", "vehicleFriendlyFire", "soldierSplashFriendlyFire",
        "vehicleSplashFriendlyFire", "tkPunishEnabled", "tkNumPunishToKick", "tkPunishByDefault",
        "votingEnabled", "voteTime", "minPlayersForVoting", "teamVoteOnly", "voipEnabled",
        "voipServerRemote", "voipServerRemoteIP", "voipServerPort", "voipBFClientPort",
        "voipBFServerPort", "voipSharedPassword", "voipQuality", "gameSpyPort",
        "allowNATNegotiation", "interfaceIP", "timeBeforeRestartMap", "autoBalanceTeam",
        "teamRatioPercent", "coopBotRatio", "coopBotCount", "coopBotDifficulty", "noVehicles",
        "autoRecord", "demoIndexURL", "demoDownloadURL", "autoDemoHook", "demoQuality",
        "adminScript", "sponsorText", "sponsorLogoURL", "communityLogoURL", "radioSpamInterval",
        "radioMaxSpamFlagCount", "radioBlockedDurationTime", "useGlobalRank", "useGlobalUnlocks",
        "friendlyFireWithMines", "ranked", "punkBuster" };
    const unsigned expected = sizeof(ordered) / sizeof(ordered[0]);
    unsigned index = 0;
    bool orderOk = true, accessOk = true;
    for (ConsoleObject* object : serverSettingsConsoleObjects())
    {
        if (index >= expected || std::strcmp(object->getFunctionName(), ordered[index]) != 0)
            orderOk = false;
        else if (object->getAccess() != (index < expected - 2 ? 1u : 2u))
            accessOk = false;
        ++index;
    }
    check(orderOk, "relative PE registration order across the whole sv table");
    check(accessOk, "access 1 for every binding but the trailing ranked/punkBuster pair");
    check(index == expected, "complete sv table: 72 bindings, save deliberately absent");
    if (failures) return 1;

    ConsoleObjects registry;
    check(registry.registerConsoleObjects(serverSettingsConsoleObjects()), "register actual sv bindings");
    check(registry.getConsoleObject("sv", "radioSpamInterval") == find("radioSpamInterval")
          && registry.getConsoleObject("sv", "sponsorLogoURL") == find("sponsorLogoURL"),
          "new bindings resolve through the real console registry");

    // the pre-existing free-camera regression, unchanged
    ConsoleObject* freecam = find("allowFreeCam");
    check(serverSettings->m_allowFreeCam && !serverInfo->m_block.allowFreeCam,
          "retail constructor defaults differ before ServerInfo publication");
    check(get(freecam) == "1" && freecam->hasReturnValue(),
          "GET reads stored default rather than initially-false mirror");
    for (const char* text : {"0", "1", "0"})
    {
        const bool expectedFlag = text[0] == '1';
        serverInfo->setDirty(false);
        check(set(freecam, text).empty() && !freecam->hasReturnValue(), "freecam SET has no reply");
        check(serverSettings->m_allowFreeCam == expectedFlag &&
              (serverInfo->m_block.allowFreeCam != 0) == expectedFlag && serverInfo->isDirty(),
              "freecam SET updates stored value, camera mirror and replication dirty flag");
        serverInfo->setDirty(false);
        check(get(freecam) == text && freecam->hasReturnValue() && !serverInfo->isDirty(),
              "freecam GET returns numeric bool without dirtying replication");
    }

    ConsoleObject* port = find("gameSpyPort");
    check(get(port) == "29900" && port->hasReturnValue(), "constructor port read-back");
    for (const char* text : {"29984", "-1", "65536"})
    {
        check(set(port, text).empty() && !port->hasReturnValue(), "SET has no reply");
        check(get(port) == text, "signed unvalidated port retained without narrowing");
    }
    for (const char* name : {"serverIP", "interfaceIP"})
    {
        ConsoleObject* object = find(name);
        for (const char* text : {"127.0.0.1", "", "unresolved host with spaces"})
        {
            check(set(object, text).empty() && !object->hasReturnValue(), "string SET has no reply");
            check(get(object) == text, "string field preserves raw argument and empty value");
        }
    }

    // the rest of the raw-field bindings
    {
        ConsoleObject* object = find("maxConnectionType");
        check(get(object) == "5", "maxConnectionType reads the constructor default of 5");
        check(set(object, "2").empty() && get(object) == "2"
              && serverSettings->m_maxConnectionType == 2,
              "maxConnectionType round-trips through the field GameServer::init reads");
        set(object, "5");
    }
    for (const char* name : {"voipServerPort", "voipBFClientPort", "voipBFServerPort",
                             "voipQuality", "voteTime", "minPlayersForVoting", "roundsPerMap",
                             "scoreLimit", "startDelay", "tkNumPunishToKick"})
    {
        ConsoleObject* object = find(name);
        check(set(object, "7").empty() && get(object) == "7", "raw int field round-trips unclamped");
    }
    for (const char* name : {"voipSharedPassword", "demoIndexURL", "demoDownloadURL",
                             "autoDemoHook", "adminScript", "sponsorText", "sponsorLogoURL",
                             "communityLogoURL", "voipServerRemoteIP", "configFile"})
    {
        ConsoleObject* object = find(name);
        check(set(object, "value/with/slashes").empty() && get(object) == "value/with/slashes",
              "raw string field round-trips verbatim");
    }
    for (const char* name : {"tkPunishEnabled", "autoRecord", "autoBalanceTeam",
                             "allowNATNegotiation", "voipServerRemote", "voipEnabled"})
    {
        ConsoleObject* object = find(name);
        check(set(object, "1").empty() && get(object) == "1" && set(object, "0").empty()
              && get(object) == "0", "raw bool field round-trips both ways");
    }

    // in-binding clamps
    for (const char* name : {"soldierFriendlyFire", "vehicleFriendlyFire",
                             "soldierSplashFriendlyFire", "vehicleSplashFriendlyFire"})
    {
        ConsoleObject* object = find(name);
        check(set(object, "-5").empty() && get(object) == "0", "friendly fire floors at 0");
        check(set(object, "150").empty() && get(object) == "100", "friendly fire caps at 100");
        check(set(object, "37").empty() && get(object) == "37", "in-range friendly fire kept");
    }
    {
        ConsoleObject* object = find("demoQuality");
        check(set(object, "0").empty() && get(object) == "1", "demoQuality floors at 1");
        check(set(object, "99").empty() && get(object) == "10", "demoQuality caps at 10");
    }
    {
        ConsoleObject* object = find("coopBotRatio");
        check(set(object, "-20").empty() && get(object) == "0", "coopBotRatio floors at 0");
        check(set(object, "250").empty() && get(object) == "100", "coopBotRatio caps at 100");
        object = find("coopBotDifficulty");
        check(set(object, "0").empty() && get(object) == "10", "coopBotDifficulty floors at 10");
        check(set(object, "250").empty() && get(object) == "100", "coopBotDifficulty caps at 100");
    }

    // setters that clamp, mirror, or both
    {
        ConsoleObject* object = find("endDelay");
        check(set(object, "5").empty() && get(object) == "10", "setEndDelay floors at 10");
        check(set(object, "45").empty() && get(object) == "45", "setEndDelay has no ceiling");
        object = find("endOfRoundDelay");
        check(set(object, "5").empty() && get(object) == "15", "setEndOfRoundDelay floors at 15");
        check(set(object, "60").empty() && get(object) == "30", "setEndOfRoundDelay caps at 30");
        check(set(object, "20").empty() && get(object) == "20", "in-range end-of-round delay kept");
    }
    {
        // Every mirror-and-dirty setter, checked against the ServerInfo copy the browser publishes.
        struct { const char* name; unsigned char* mirror; } mirrored[] = {
            { "allowExternalViews", &serverInfo->m_block.allowExternalViews },
            { "allowNoseCam",       &serverInfo->m_block.allowNoseCam },
            { "hitIndicator",       &serverInfo->m_block.hitIndicator },
            { "tkPunishByDefault",  &serverInfo->m_block.tkPunishDefault },
            { "votingEnabled",      &serverInfo->m_block.votingEnabled },
            { "teamVoteOnly",       &serverInfo->m_block.teamVoteOnly } };
        for (auto& entry : mirrored)
        {
            ConsoleObject* object = find(entry.name);
            serverInfo->setDirty(false);
            check(set(object, "1").empty() && get(object) == "1" && *entry.mirror != 0
                  && serverInfo->isDirty(), "bool setter mirrors into ServerInfo and dirties it");
            serverInfo->setDirty(false);
            check(set(object, "0").empty() && get(object) == "0" && *entry.mirror == 0
                  && serverInfo->isDirty(), "clearing the flag mirrors and dirties as well");
        }
    }
    {
        struct { const char* name; int* mirror; } radio[] = {
            { "radioSpamInterval",        &serverInfo->m_block.radioSpamInterval },
            { "radioMaxSpamFlagCount",    &serverInfo->m_block.radioMaxSpamFlagCount },
            { "radioBlockedDurationTime", &serverInfo->m_block.radioBlockedDuration } };
        for (auto& entry : radio)
        {
            ConsoleObject* object = find(entry.name);
            serverInfo->setDirty(false);
            check(set(object, "42").empty() && get(object) == "42" && *entry.mirror == 42
                  && serverInfo->isDirty(), "radio limit mirrors into ServerInfo and dirties it");
        }
    }
    {
        // THE ONE mirror that does NOT dirty - a shipped asymmetry, asserted so a future
        // "cleanup" that adds the missing call fails here instead of silently changing behaviour.
        ConsoleObject* object = find("friendlyFireWithMines");
        serverInfo->setDirty(false);
        check(set(object, "1").empty() && get(object) == "1"
              && serverInfo->m_block.friendlyFireWithMines != 0 && !serverInfo->isDirty(),
              "friendlyFireWithMines mirrors WITHOUT marking ServerInfo dirty (as shipped)");
        set(object, "0");
    }
    {
        ConsoleObject* object = find("welcomeMessage");
        serverInfo->setDirty(false);
        check(set(object, "hello").empty() && get(object) == "hello"
              && serverInfo->getWelcomeMsg(0) == "hello" && serverInfo->isDirty(),
              "welcomeMessage publishes into the ServerInfo welcome list and dirties it");
    }
    {
        // timeLimit's round-shift arm needs a live GameLogic; with none, the setter stores and
        // returns, which is the arm a config file takes at load time.
        ConsoleObject* object = find("timeLimit");
        check(set(object, "-30").empty() && get(object) == "0", "setTimeLimit floors at 0");
        check(set(object, "1200").empty() && get(object) == "1200", "setTimeLimit stores the request");
    }

    // ranked-overriding getters
    {
        ConsoleObject* ticket = find("ticketRatio");
        set(ticket, "60");
        check(get(ticket) == "60", "unranked ticketRatio reports the stored value");
        const bool savedRanked = serverSettings->m_ranked;
        serverSettings->m_ranked = true;
        check(get(ticket) == "100", "ranked ticketRatio reports the forced 100");
        check(get(find("spawnTime")) == "15", "ranked spawnTime reports the forced 15");
        check(get(find("teamRatioPercent")) == "100", "ranked teamRatioPercent reports 100");
        check(get(find("coopBotCount")) == "16", "ranked coopBotCount reports 16");
        check(set(ticket, "20").empty() && get(ticket) == "100" && serverSettings->m_ticketRatio == 20,
              "ranked SET still writes the field while GET keeps reporting the override");
        serverSettings->m_ranked = savedRanked;
        check(get(ticket) == "20", "clearing ranked exposes the value written underneath it");
    }
    {
        // Compound getters: both are false on a LAN server whatever the config asked for.
        ConsoleObject* rank = find("useGlobalRank");
        ConsoleObject* unlocks = find("useGlobalUnlocks");
        const bool savedInternet = serverSettings->m_internet;
        serverSettings->m_internet = false;
        check(set(rank, "1").empty() && get(rank) == "0" && serverSettings->m_useGlobalRank,
              "useGlobalRank stores 1 but reports 0 on a LAN server");
        check(set(unlocks, "1").empty() && get(unlocks) == "0" && serverSettings->m_useGlobalUnlocks,
              "useGlobalUnlocks stores 1 but reports 0 on a LAN server");
        serverSettings->m_internet = true;
        check(get(unlocks) == "1", "an internet server reports its stored unlock setting");
        serverSettings->m_internet = savedInternet;
    }

    // the status-string family
    {
        ConsoleObject* object = find("serverPort");
        check(set(object, "70000") == "illegal port value" && object->hasReturnValue(),
              "out-of-range port is refused with the shipped lower-case message");
        check(set(object, "-1") == "illegal port value",
              "a negative port takes the same unsigned arm");
        check(set(object, "16700").empty() && serverSettings->m_serverPort == 16700,
              "an in-range port stores and replies empty");
        check(get(object) == "16700" && object->hasReturnValue(), "serverPort GET formats the field");
    }
    {
        ConsoleObject* object = find("password");
        check(set(object, "hunter2").empty() && get(object) == "hunter2",
              "password round-trips and the GET shows the configured value");
        const bool savedRanked = serverSettings->m_ranked;
        serverSettings->m_ranked = true;
        check(get(object) == "hunter2" && serverSettings->getPassword().empty(),
              "the console still shows a ranked server's password although getPassword blanks it");
        serverSettings->m_ranked = savedRanked;
    }
    {
        ConsoleObject* slots = find("numReservedSlots");
        check(set(find("maxPlayers"), "16").empty(), "maxPlayers set for the reserved-slot cap");
        check(set(slots, "-3").empty() && get(slots) == "0", "reserved slots floor at 0");
        check(set(slots, "10").empty() && get(slots) == "4",
              "reserved slots cap at a quarter of maxPlayers");
        check(set(slots, "2").empty() && get(slots) == "2", "an in-range reserved-slot count is kept");
    }

    // the declared-float, means-bool property
    {
        ConsoleObject* object = find("noVehicles");
        check(set(object, "1").empty() && !object->hasReturnValue() && serverSettings->m_noVehicles,
              "noVehicles SET stores the widened bool and discards its status string, as shipped");
        // getNoVehicles reads "GSGameMode" through `settings` with NO null guard, exactly as both
        // ports do, so the GET arm needs a repository. An empty one hands back the "gpm_cq"
        // default and the getter then clears the flag, because the mode is not coop.
        SettingsRepostitory* savedSettings = settings;
        SettingsRepostitory fixtureSettings;
        settings = &fixtureSettings;
        check(get(object) == "0" && object->hasReturnValue() && !serverSettings->m_noVehicles,
              "noVehicles GET reports 0 and the getter clears the flag outside gpm_coop");
        fixtureSettings.stringSet(std::string("GSGameMode"), std::string("gpm_coop"));
        set(object, "1");
        check(get(object) == "1", "in coop the same GET reports the stored flag as 1");
        settings = savedSettings;
    }

    // coopBotCount's clamp and its GeneralSettings republish
    {
        // The setter dereferences classManager unconditionally, exactly as both ports do, so the
        // fixture supplies a real but empty ClassManager: getSingleton("GeneralSettings") returns
        // null and the two [UNRESOLVED] slot calls are correctly skipped.
        ClassManager* savedManager = classManager;
        ClassManager fixtureManager;
        classManager = &fixtureManager;
        ConsoleObject* object = find("coopBotCount");
        check(set(object, "0").empty() && get(object) == "1", "coopBotCount floors at 1");
        check(set(object, "99").empty() && get(object) == "48", "coopBotCount caps at 48");
        check(set(object, "24").empty() && get(object) == "24", "an in-range bot count is kept");
        classManager = savedManager;
    }

    // absent receiver
    ServerSettings* saved = serverSettings;
    serverSettings = 0;
    check(!port->isActive() && !find("interfaceIP")->isActive() && !find("serverIP")->isActive(),
          "absent settings marks bindings inactive");
    check(!freecam->isActive() && registry.getConsoleObject("sv", "allowFreeCam") == 0,
          "absent receiver makes freecam unavailable through registry");
    check(!find("load")->isActive() && !find("coopBotCount")->isActive()
          && !find("radioSpamInterval")->isActive() && !find("password")->isActive(),
          "every new shape reports inactive without a settings receiver");
    serverSettings = saved;
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
