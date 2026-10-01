// [TEMP] [CHOSEN, not recovered] Mode transitions using actual HUD, adapter,
// information storage and EventManager. Observers record callback ordering and
// mutation. /FORCE covers uncalled widgets; this does not publish the HUD.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
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
#include "../BF2/Menu/HudInformationAdapter.h"
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#undef protected
#undef private
#include "../BF2/Core/EventManager.h"

namespace dice { namespace hfe {
EventManager* eventManager = nullptr;
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
static std::vector<std::string> trace;
static std::function<void()> onModeQuery, onFlag;
static void check(bool value, const char* label) {
    ++checks;
    if (!value) { ++failures; std::printf("FAIL: %s\n", label); }
}
struct QueryHud : BF2HudManager {
    int getMode() override {
        const int result = BF2HudManager::getMode();
        if (onModeQuery) { auto action = onModeQuery; onModeQuery = {}; action(); }
        return result;
    }
};
struct Players : world::PlayerManager {
    unsigned queries = 0;
    int emptySquad = 1;
    std::vector<std::pair<int,int>> squadQueries;
    std::list<world::IPlayer*> empty, occupied {nullptr}; // [TEMP] occupancy only
    std::function<void()> onSquad;
    world::IPlayer* getLocalHumanPlayer() override { ++queries; return nullptr; }
    const std::list<world::IPlayer*>& getPlayersInSquad(int team, int squad) override {
        squadQueries.emplace_back(team, squad);
        if (onSquad) { auto action = onSquad; onSquad = {}; action(); }
        return squad == emptySquad ? empty : occupied;
    }
};
struct Adapter : HudInformationAdapter {
    std::string failAt;
    bool setBool(std::string name, bool value) override {
        trace.push_back(name + (value ? ":1" : ":0"));
        if (onFlag) { auto action = onFlag; onFlag = {}; action(); }
        if (name == failAt) return false;
        return HudInformationAdapter::setBool(name, value);
    }
};
struct Events : IEventListener {
    unsigned count = 0;
    void* payload = nullptr;
    void handleEvent(EventCategory cat, unsigned int id, EventNode*, void* data) override {
        if (cat == ECHud && id == 2) { ++count; payload = data; trace.push_back("event"); }
    }
};
static void destroyConstructedHud(QueryHud* hud) {
    delete hud->m_field2dc;
    using S=std::string; using W=std::wstring; using L=std::list<int>;
    using SL=std::list<std::string>; using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field33c.~SL(); hud->m_field32c.~V(); hud->m_field304.~S();
    hud->m_field2e0.~SL(); hud->m_field0e8.~L(); hud->m_field0c0.~S();
    hud->m_field08c.~S(); hud->m_field060.~W(); hud->m_field038.~W(); hud->m_field010.~W();
    ::operator delete(hud);
}
int main() {
    static_assert(sizeof(void*) == 4, "x86 only");
    EventManager events; eventManager = &events;
    Players players; world::playerManager = &players;
    void* storage = ::operator new(sizeof(QueryHud));
    std::memset(storage, 0x5a, sizeof(QueryHud));
    QueryHud* hud = new(storage) QueryHud; hudManager = hud;
    {
        HudInformationLayer info; g_hudInformationLayer = &info;
        // Test-only listener isolation: the layer's game-event handler is still
        // a frontier. The actual adapter and recording listener remain subscribed.
        events.unregisterEventHandler(ECHud, &info);
        Adapter adapter; Events observer;
        events.registerEventHandler(ECHud, &observer, 0);
        hud->m_field104 = &adapter;
        auto reset = [&](int current, int previous) {
            hudManager = hud; hud->m_field250 = current; hud->m_field254 = previous;
            hud->m_field0b0 = hud->m_field0b1 = hud->m_field0b2 = false;
            trace.clear(); observer.count = 0; observer.payload = nullptr;
            players.queries = 0; adapter.failAt.clear(); onModeQuery = {}; onFlag = {};
        };
        reset(6, 20);
        hud->BF2HudManager::setMode(6, true);
        check(trace.empty() && !players.queries && hud->m_field254 == 20,
              "same mode only queries current mode, preserving previous state");
        reset(6, 20);
        onFlag = [&] { check(hud->m_field250 == 4 && hud->m_field254 == 6,
                             "mode and saved previous value commit before adapter callback"); };
        hud->BF2HudManager::setMode(4, true);
        check(trace == std::vector<std::string>{"SetupShow:0","DemoRecInterfaceShow:0",
              "DemoCameraInterfaceShow:0","SpottedInterfaceShow:0","RadioInterfaceShow:0",
              "RadioVehicleInterfaceShow:0","RadioInterfaceShow:1","event"},
              "spotted exit clears both radios before enabling new radio and posting event");
        check(players.queries == 1 && observer.count == 1 && observer.payload == &adapter,
              "actual event delivery carries adapter and follows one local-player query");
        check(adapter.getBool("RadioInterfaceShow") && !adapter.getBool("SpottedInterfaceShow"),
              "actual information storage reflects mode transition");
        reset(20, 3); hud->BF2HudManager::setMode(26, true);
        check(trace[3] == "SquadLeaderMenuShow:0" && trace[4] == "InviteListShow:0" &&
              trace[5] == "InviteListShow:1", "leader menu exit falls through invite-list clearing");
        reset(17, 0); hud->BF2HudManager::setMode(9, true);
        check(std::find(trace.begin(),trace.end(),"SpawnShow:0") == trace.end(),
              "scoreboard transition preserves previous spawn display");
        reset(9, 0); hud->BF2HudManager::setMode(12, true);
        check(!adapter.getBool("ScoreboardShow") && !adapter.getBool("LevelsListShow") &&
              !adapter.getBool("ServerInfoSelected"), "mode12 clears old scoreboard without mode9's enable arm");
        reset(15, 0); hud->BF2HudManager::setMode(27, true);
        check(std::find(trace.begin(),trace.end(),"CommanderShow:0") == trace.end(),
              "choice mode retains commander display through previous-mode query");
        reset(8, 6); hud->BF2HudManager::setMode(4, false);
        check(hud->m_field254 == 6 && trace[3] == "SpottedInterfaceShow:0",
              "savePrevious false retains old previous value used by adapter");
        reset(6, 0); onModeQuery = [&] { hud->m_field250 = 20; };
        hud->BF2HudManager::setMode(4, true);
        check(hud->m_field254 == 20 && trace[3] == "SquadLeaderMenuShow:0",
              "saved previous value rereads storage after virtual mode-query callback");
        reset(6, 0); onFlag = [&] { hud->m_field254 = 4; };
        hud->BF2HudManager::setMode(4, true);
        check(!observer.count && trace.back() == "RadioInterfaceShow:1",
              "late previous-mode reread suppresses event after callback mutation");
        reset(2, 2); hud->m_field104 = nullptr; hud->BF2HudManager::setMode(-1, true);
        check(hud->m_field250 == -1 && hud->m_field254 == 2 && trace.empty(),
              "absent adapter does not prevent signed mode state update");
        hud->m_field104 = &adapter;
        reset(-1, 0); hud->BF2HudManager::setMode(32, true);
        check(trace.size() == 48 && trace[3] == "MapShow:1" && trace[25] == "MapShow:1" &&
              trace.back() == "event", "out-of-range previous and new modes both execute full reset arms");
        trace.clear();
        check(!adapter.setKitIconCount(7) && trace.size() == 1,
              "retail visibility name is absent from initial information registry");
        // [TEMP] Supply only the missing named bindings needed to exercise the
        // recovered loop beyond its first failed registration lookup.
        bool iconVisibility[7] = {};
        for (int i=0;i<7;++i)
            info.registerBool("PlayerKitIcon"+std::to_string(i)+"Show", &iconVisibility[i]);
        for (int count : {-1,0,1,6,7,8}) {
            trace.clear(); check(adapter.setKitIconCount(count), "all registered icon flags accept update");
            check(trace.size() == 7, "icon visibility writes exactly seven slots");
            for (int i=0;i<7;++i)
                check(i < static_cast<int>(trace.size()) &&
                      trace[i] == "PlayerKitIcon"+std::to_string(i)+"Show:"+(i<count?"1":"0"),
                      "icon visibility boundary preserves signed count");
        }
        trace.clear(); adapter.failAt = "PlayerKitIcon2Show";
        check(!adapter.setKitIconCount(7) && trace.size() == 3, "visibility loop stops on first false callback");
        adapter.failAt.clear(); trace.clear();
        adapter.setSelectedTeam(-1);
        check(trace == std::vector<std::string>{"Team1Selected:0","Team2Selected:0"},
              "invalid selected team clears both team flags in order");
        trace.clear(); adapter.failAt = "Team1Selected"; adapter.setSelectedTeam(2);
        check(trace.size() == 2 && adapter.getBool("Team2Selected"), "team selection ignores first failure");
        adapter.failAt.clear(); trace.clear();
        // The retail loop includes icon7 even though the production registry
        // only guarantees 0..6; it exits on a missing binding at that last slot.
        adapter.setSelectedKit(2);
        check(trace.size() == 8 && trace[2] == "PlayerKitIcon2SelectShow:1" &&
              trace.back() == "PlayerKitIcon7SelectShow:0", "kit selection includes eighth index");
        trace.clear(); adapter.failAt = "PlayerKitIcon1SelectShow";
        check(!adapter.setSelectedKit(7) && trace.size() == 2, "kit selection stops at first failed binding");
        check(hud->getPlayerKitIcon(nullptr, true, true) == 0 && !hud->isPlayerAlive(nullptr),
              "kit/alive null player guards require no collaborators");
        const unsigned oldQueries = players.queries;
        check(!hud->isWithinAimThreshold(nullptr, -1.0f) && players.queries == oldQueries + 1,
              "aim query retrieves local player before rejecting null object");
        hud->sendKitMessage(); hud->sendRequestForOrder(); hud->submitSquadRename(); hud->leaveSquad();
        check(players.queries == oldQueries + 5,
              "chat/request/squad null player arms stop before game and squad collaborators");
        hud->m_field144 = nullptr; hud->refreshChatCategory(-1);
        check(hud->findEmptySquad(-8) == 1 && players.squadQueries.front() == std::make_pair(-8,1),
              "squad scan starts at one and preserves signed team");
        players.squadQueries.clear(); players.emptySquad = 9;
        check(hud->findEmptySquad(2) == 9 && players.squadQueries.size() == 9,
              "squad scan includes ninth slot");
        players.squadQueries.clear(); players.emptySquad = 10;
        check(hud->findEmptySquad(2) == -1 && players.squadQueries.back().second == 9,
              "full squad list returns minus one without querying tenth slot");
        Players replacement; replacement.emptySquad = 2;
        players.squadQueries.clear();
        players.onSquad = [&] { world::playerManager = &replacement; };
        check(hud->findEmptySquad(3) == 2 && players.squadQueries.size() == 1 &&
              replacement.squadQueries == std::vector<std::pair<int,int>>{{3,2}},
              "squad scan reloads singleton after first query callback");
        world::playerManager = &players;
        events.unregisterEventHandler(ECHud, &observer);
        hud->m_field104 = nullptr;
    }
    g_hudInformationLayer = nullptr; hudManager = nullptr;
    destroyConstructedHud(hud);
    std::printf("HUD mode: %u/%u checks passed\n", checks-failures, checks);
    return failures ? 1 : 0;
}
