// [TEMP] [CHOSEN, not recovered]
// Tests SwiffHost server-hosting operations with real MapList, ServerSettings, and settings storage.
// Observers supply map validation, bot settings, profile saving, and server-settings application.
// Client image only; unexercised force-linked dependencies remain unresolved. Empty config paths avoid file writes.
#include "../BF2/Core/Ids.h"
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/Menu/SwiffHost.h"
#include "../BF2/Game/IGame.h"
#include "../BF2/Game/MapList.h"
#include "../BF2/Game/IModInfo.h"
#include "../BF2/Game/GameServer/ServerSettings.h"
#include "GeneralSettingsStandIn.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace dice::hfe;

namespace dice { namespace hfe {
// [TEMP] The Core/Globals.cpp definitions this isolated executable does not link.
IGame* game = nullptr;
ClassManager* classManager = nullptr;
SettingsRepostitory* settings = nullptr;
} }

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* what)
{
    ++checks;
    if (!ok)
    {
        ++failures;
        std::printf("FAIL: %s\n", what);
    }
}

std::vector<std::string> g_calls;             // "LPM+0x40", "GSS+0x18", "GS+0xA8", in order
void record(const char* owner, int offset)
{
    char b[32];
    std::snprintf(b, sizeof b, "%s+0x%X", owner, offset);
    g_calls.push_back(b);
}

// A raw object whose every one of 128 slots records "<owner>+<offset>" and answers true - the
// thiscall shape of a no-argument bool slot (saveCurrentProfile, apply), called through the table.
template<int Owner> struct RecordingTable
{
    void** table;
    void* slots[128];
    RecordingTable();
    template<int I> static bool __fastcall stub(RecordingTable*, void*)
    {
        record(Owner == 0 ? "LPM" : "GSS", I * 4);
        return true;
    }
};
template<int Owner, int I> struct Fill
{
    static void run(void** s) { s[I] = reinterpret_cast<void*>(&RecordingTable<Owner>::template stub<I>); Fill<Owner, I + 1>::run(s); }
};
template<int Owner> struct Fill<Owner, 128> { static void run(void**) {} };
template<int Owner> RecordingTable<Owner>::RecordingTable() : table(slots) { Fill<Owner, 0>::run(slots); }

struct GeneralWitness : GeneralSettingsStandIn
{
    bool useBots = false;
    bool getUseBots() const override { record("GS", 0xA8); return useBots; }
};

// The current mod: dalian_plant (gpm_cq, gpm_coop, sp2) and wake_island_2007 (gpm_cq).
struct ModInfoWitness : IModInfo
{
    Map dalian, wake;
    ModInfoWitness()
    {
        dalian.path = "dalian_plant"; dalian.name = "Dalian Plant";
        const char* modes[] = { "gpm_cq", "gpm_coop", "sp2" };
        for (const char* m : modes) { GameMode g; g.name = m; dalian.gameModes.push_back(g); }
        wake.path = "wake_island_2007"; wake.name = "Wake Island 2007";
        GameMode cq; cq.name = "gpm_cq"; wake.gameModes.push_back(cq);
    }
    void addRef() override { std::abort(); }
    int getRef() override { std::abort(); }
    int release() override { std::abort(); }
    void* queryInterface(unsigned int) const override { std::abort(); }
    bool init() override { std::abort(); }
    std::vector<Mod>* getModList() override { std::abort(); }
    const Map* getMap(const std::string&) const override { std::abort(); }
    const Mod* getCurrentMod() const override { std::abort(); }
    std::string getCurrentModPath() override { return "mods/bf2"; }
    const Map* findMapFromCurrentMod(const std::string& map) override
    {
        if (map == dalian.path) return &dalian;
        if (map == wake.path) return &wake;
        return nullptr;
    }
    const Map* findMapFromMod(const Mod*, const std::string&) override { std::abort(); }
    const Mod* getModFromName(const std::string&) override { std::abort(); }
    const Mod* getModFromPath(const std::string&) override { std::abort(); }
    const Map* getMapFromGameSpyId(int) override { std::abort(); }
    std::string getMapNameFromMap(const std::string&) override { std::abort(); }
    std::string getMapPathFromMap(const std::string&) override { std::abort(); }
    std::string getMapNameFromMap(const Mod*, const std::string&) override { std::abort(); }
    std::string getMapPathFromMap(const Mod*, const std::string&) override { std::abort(); }
    void addNameForGameMode(const std::string&, const std::string&) override { std::abort(); }
    std::string getNameFromGameMode(const std::string&) override { std::abort(); }
};

RecordingTable<0> g_profiles;
RecordingTable<1> g_gameServerSettings;
GeneralWitness g_general;
ModInfoWitness g_modInfo;
bool g_generalPresent = true;

// [TEMP] The real ClassManager with only its by-name lookup observed.
struct ClassManagerObserver : ClassManager
{
    IBase* getSingleton(const std::string& name) override
    {
        if (name == "LocalProfileManager") return reinterpret_cast<IBase*>(&g_profiles);
        if (name == "GameServerSettings") return reinterpret_cast<IBase*>(&g_gameServerSettings);
        if (name == "GeneralSettings") return g_generalPresent ? static_cast<IBase*>(&g_general) : nullptr;
        if (name == "ModInfo") return reinterpret_cast<IBase*>(static_cast<IModInfo*>(&g_modInfo));
        std::printf("FAIL: unexpected singleton %s\n", name.c_str());
        ++failures;
        return nullptr;
    }
};

bool pollEvent(SwiffHost* host, MenuEvent& out) { out = MenuEvent(); return host->pollEvent(out); }
bool calls(std::initializer_list<const char*> expected)
{
    std::vector<std::string> want;
    for (const char* e : expected) want.push_back(e);
    return g_calls == want;
}
void resetMapList()
{
    mapList->clear(true);
    mapList->setCurrentMap(-1, true);
}
} // namespace

int main()
{
    ClassManagerObserver manager;
    classManager = &manager;
    settings = new SettingsRepostitory();
    SwiffHost* host = static_cast<SwiffHost*>(SwiffHost::create(IID_ISwiffHost, nullptr));
    check(host != nullptr, "the SwiffHost factory constructs the host");
    if (!host)
        return 1;
    host->setMessagesEnabled(true);

    // 1. getServerSettingsInfo: the local server's interface IP and its two ports.
    serverSettings->m_interfaceIP = "10.0.0.7";
    serverSettings->m_gameSpyPort = 29911;
    serverSettings->m_serverPort = 16711;
    {
        std::string ip("previous");
        int queryPort = -1, port = -1;
        check(host->getServerSettingsInfo(ip, &queryPort, &port), "getServerSettingsInfo answers true");
        check(ip == "10.0.0.7" && queryPort == 29911 && port == 16711,
              "getServerSettingsInfo: m_interfaceIP, m_gameSpyPort (the query port) and m_serverPort");
    }

    // 2. requestHostServer: event 7 carrying the level row's map and mode.
    {
        SwiffSinglePlayerLevel level;
        level.index = 5; level.name = "MEC vs US - The Dam Level 1"; level.map = "the_dam"; level.mode = "sp1";
        check(host->requestHostServer(&level), "requestHostServer answers true");
        MenuEvent e;
        check(pollEvent(host, e) && e.type == 7 && e.args.size() == 2 && e.args[0] == "the_dam" && e.args[1] == "sp1",
              "requestHostServer pushes event 7 (map, mode)");
        check(!pollEvent(host, e), "and nothing else");
    }

    // 3. A join under way (+0xEC4): false before anything else is touched.
    {
        g_calls.clear();
        host->setFlagEC4(true);
        host->addServerEntry("dalian_plant", "gpm_cq", 64);
        check(!host->hostServer(), "hostServer refuses while +0xEC4 is set");
        MenuEvent e;
        check(g_calls.empty() && !pollEvent(host, e) && mapList->getMapCount() == 0, "and touches nothing");
        host->setFlagEC4(false);
        host->clearServerEntries();
    }

    // 4. No entries: the list cleared, GSLoadLevel / GSGameMode erased, UseAI erased (no bots, not
    //    coop), then the {3, "nomapsselected", 3} message and false.
    {
        g_calls.clear();
        settings->stringSet("GSLoadLevel", "old_level");
        settings->stringSet("GSGameMode", "old_mode");
        const bool yes = true;
        settings->boolSet("UseAI", yes);
        mapList->appendEntry("wake_island_2007", "gpm_cq", 32, true);
        host->setPage(3);
        const int before = host->getMessageCount();
        check(!host->hostServer(), "hostServer with no entries answers false");
        check(calls({"LPM+0x40", "GSS+0x18", "GS+0xA8"}), "saveCurrentProfile (+0x40), apply (+0x18), then getUseBots (+0xA8)");
        check(host->getPage() == 0, "the page is reset to 0");
        check(mapList->getMapCount() == 0, "the map list is cleared");
        check(!settings->stringExists("GSLoadLevel") && !settings->stringExists("GSGameMode"), "GSLoadLevel and GSGameMode are erased");
        check(!settings->boolExists("UseAI"), "UseAI is erased without bots or co-op");
        check(host->getMessageCount() == before + 1, "one message is posted");
        const SwiffHostMessage* m = host->getMessage(host->getMessageCount() - 1);
        check(m && m->type == 3 && m->code == 3 && m->text == "nomapsselected", "the message is {3, \"nomapsselected\", 3}");
        MenuEvent e;
        check(!pollEvent(host, e), "no event");
    }

    // 5. Two entries (and one the MapList refuses): the list rebuilt in order, current map 0, UseAI
    //    SET by the bots flag, event 4 (the first entry's name and game mode).
    {
        g_calls.clear();
        g_general.useBots = true;
        host->addServerEntry("dalian_plant", "gpm_cq", 64);
        host->addServerEntry("no_such_map", "gpm_cq", 16);
        host->addServerEntry("wake_island_2007", "gpm_cq", 32);
        check(host->hostServer(), "hostServer with entries answers true");
        check(mapList->getMapCount() == 2 && mapList->getCurrentMap() == 0, "the valid entries are appended and map 0 is current");
        const MapList::Entry* first = mapList->getEntry(0);
        const MapList::Entry* second = mapList->getEntry(1);
        check(first && first->name == "dalian_plant" && first->gameMode == "gpm_cq" && first->mapSize == 64, "entry 0: dalian_plant gpm_cq 64");
        check(second && second->name == "wake_island_2007" && second->mapSize == 32, "entry 1: wake_island_2007 32");
        const bool no = false;
        check(settings->boolExists("UseAI") && settings->boolGet("UseAI", no), "UseAI is set when GeneralSettings uses bots");
        MenuEvent e;
        check(pollEvent(host, e) && e.type == 4 && e.args.size() == 2 && e.args[0] == "dalian_plant" && e.args[1] == "gpm_cq",
              "event 4 carries the current entry's name and game mode");
        g_general.useBots = false;
        host->clearServerEntries();
    }

    // 6. A co-op game mode also sets UseAI; without GeneralSettings UseAI is left alone.
    {
        host->setGameMode("gpm_coop");
        host->addServerEntry("dalian_plant", "gpm_coop", 64);
        check(host->hostServer(), "a co-op list hosts");
        const bool no = false;
        check(settings->boolGet("UseAI", no), "UseAI is set for gpm_coop");
        settings->boolErase("UseAI");
        g_generalPresent = false;
        check(host->hostServer(), "hosting without GeneralSettings");
        check(!settings->boolExists("UseAI"), "UseAI is neither set nor erased without GeneralSettings");
        g_generalPresent = true;
        host->setGameMode("gpm_cq");
        host->clearServerEntries();
        MenuEvent e;
        while (pollEvent(host, e)) {}
    }

    // 7. The command-line path (MNCommandLine): one entry from GSLoadLevel with the server's max
    //    players, "gpm_cq" without AI; MNCommandLine erased; straight to event 4.
    {
        resetMapList();
        g_calls.clear();
        const bool yes = true, no = false;
        settings->boolSet("MNCommandLine", yes);
        settings->stringSet("GSLoadLevel", "wake_island_2007");
        settings->boolSet("UseAI", no);
        serverSettings->setMaxPlayers(32);
        check(host->hostServer(), "the command-line path hosts");
        check(calls({"LPM+0x40", "GSS+0x18"}), "the command-line path never asks GeneralSettings");
        const MapList::Entry* e0 = mapList->getEntry(0);
        check(mapList->getMapCount() == 1 && e0 && e0->name == "wake_island_2007" && e0->gameMode == "gpm_cq" && e0->mapSize == 32,
              "one entry: GSLoadLevel, gpm_cq, getMaxPlayers");
        check(!settings->boolExists("MNCommandLine"), "MNCommandLine is erased");
        check(settings->stringExists("GSLoadLevel"), "GSLoadLevel is kept on this path");
        MenuEvent e;
        check(pollEvent(host, e) && e.type == 4 && e.args[0] == "wake_island_2007" && e.args[1] == "gpm_cq", "event 4 for the command-line level");
    }

    // 8. The command-line path with UseAI: the size picks the layer (24 players -> "sp2").
    {
        resetMapList();
        const bool yes = true;
        settings->boolSet("MNCommandLine", yes);
        settings->stringSet("GSLoadLevel", "dalian_plant");
        settings->boolSet("UseAI", yes);
        serverSettings->setMaxPlayers(24);
        check(host->hostServer(), "the command-line AI path hosts");
        const MapList::Entry* e0 = mapList->getEntry(0);
        check(e0 && e0->gameMode == "sp2" && e0->mapSize == 24, "24 players with AI is the sp2 layer");
        MenuEvent e;
        check(pollEvent(host, e) && e.args[1] == "sp2", "event 4 carries sp2");
        // gpm_coop wins over the layer.
        resetMapList();
        settings->boolSet("MNCommandLine", yes);
        host->setGameMode("gpm_coop");
        check(host->hostServer(), "the command-line co-op path hosts");
        e0 = mapList->getEntry(0);
        check(e0 && e0->gameMode == "gpm_coop", "a gpm_coop host mode replaces the layer");
        host->setGameMode("gpm_cq");
        while (pollEvent(host, e)) {}
    }

    std::printf("swiff_host_server_runtime: %d/%d checks passed; %d failures\n", checks - failures, checks, failures);
    return failures ? 1 : 0;
}
