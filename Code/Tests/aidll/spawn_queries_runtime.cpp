// [TEMP] Four exact production adapters with real headers and typed observers.
// Native chosen lifetimes below omit registry, player/world and manager teardown.
// They do not validate those lifetimes, SpawnManager filtering or SpawnGroup updates.
#include "spawn_queries_runtime.h"
#include "queries_collaborators.inl"
#include <windows.h>
#include <crtdbg.h>

namespace dice { namespace hfe {
IBase* spawnManager = nullptr;
SpawnManager::SpawnManager() : m_refCount(1), m_unknown14(0), m_nextGroupId(1), m_unknown24(0) {}
SpawnManager::~SpawnManager() {}
namespace ai {
IAIGame::IAIGame() {}
IAIGame::~IAIGame() {}
BFGame::BFGame() : m_isPrecaching(true) {}
BFGame::~BFGame() {}
IAIPlayer::~IAIPlayer() {}
AIPlayer::AIPlayer(world::IPlayer* player) : m_player(player), m_i8(0), m_viewDistance(0) {}
AIPlayer::~AIPlayer() {}
}
namespace world {
IPlayer::IPlayer() : m_weakControl(nullptr) {}
IPlayer::~IPlayer() {}
}
} }

using namespace dice::hfe;
using namespace dice::hfe::ai;
using namespace dice::hfe::world;

class ManagerObserver : public SpawnManager
{
public:
    std::list<int> ids;
    ISpawnGroup* selected = nullptr;
    IBase* replacement = nullptr;
    std::vector<int> teams;
    std::vector<int> groups;
    const std::list<int>& getGroupsForTeam(int team) override
    {
        teams.push_back(team);
        if (replacement) spawnManager = replacement;
        return ids;
    }
    ISpawnGroup* getSpawnGroup(int id) override
    {
        groups.push_back(id);
        if (replacement) spawnManager = replacement;
        return selected;
    }
};

static ManagerObserver* originalManager;
static ManagerObserver* replacementManager;
static GroupObserver* replacementGroup;
static void changeAfterTeam(const GroupObserver& group)
{
    const_cast<GroupObserver&>(group).team = 2;
    originalManager->selected = replacementGroup;
    spawnManager = replacementManager;
}
static void changeAfterActive(const GroupObserver& group)
{
    const_cast<GroupObserver&>(group).team = 1;
    const_cast<GroupObserver&>(group).active = !group.active;
}
static void changeAfterHuman(const GroupObserver& group)
{
    const_cast<GroupObserver&>(group).active = !group.active;
}

int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4, "MSVC x86 fixture required");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    require(argc == 2, "mode");
    const std::string mode(argv[1]);
    BFGame adapter;
    const IAIGame& game = adapter;
    if (mode == "player")
    {
        PlayerObserver player;
        AIPlayer concrete(&player);
        const IAIPlayer& query = concrete;
        for (int value : {INT_MIN, -1, 0, 1, 255, INT_MAX})
        {
            player.group = value;
            require(query.getSpawnGroup() == value, "unchanged signed player group");
        }
        require(player.calls == 6, "one player virtual per query");
    }
    else if (mode == "zero")
    {
        spawnManager = nullptr;
        require(!game.isEligibleSpawnpoint(INT_MIN, 0), "zero group before manager read");
    }
    else
    {
        ManagerObserver manager, other;
        GroupObserver group, second;
        manager.selected = &group;
        other.selected = &second;
        spawnManager = &manager;
        if (mode == "list")
        {
            manager.ids = {-7, 0, 191, 255};
            other.ids = {81};
            manager.replacement = &other;
            const std::list<int>& first = game.getSpawnPointList(-3);
            require(&first == &manager.ids && first == std::list<int>({-7, 0, 191, 255}), "borrowed list identity/order");
            manager.ids.push_back(72);
            require(first.back() == 72, "borrowed list remains live");
            const std::list<int>& next = game.getSpawnPointList(2);
            require(&next == &other.ids && next.front() == 81, "fresh manager per list query");
            other.ids.clear();
            require(game.getSpawnPointList(0).empty(), "empty borrowed list");
            require(manager.teams == std::vector<int>({-3}) && other.teams == std::vector<int>({2, 0}), "list argument forwarding");
            require(manager.groups.empty() && other.groups.empty(), "no group lookup for list");
        }
        else if (mode == "position")
        {
            group.position.x = 1; group.position.y = -2; group.position.z = 3;
            second.position.x = -4; second.position.y = 5; second.position.z = -6;
            manager.replacement = &other;
            const Vec3& first = game.getSpawnPointPosition(-9);
            require(&first == &group.position && first.y == -2, "returned group and borrowed position");
            group.position.x = 19;
            require(first.x == 19, "borrowed position remains live");
            const Vec3& next = game.getSpawnPointPosition(17);
            require(&next == &second.position && next.z == -6, "fresh manager for next position");
            require(manager.groups == std::vector<int>({-9}) && other.groups == std::vector<int>({17}), "position group arguments");
            require(group.trace == "pos," && second.trace == "pos,", "one getPos per lookup");
        }
        else if (mode == "null")
        {
            manager.selected = nullptr;
            require(!game.isEligibleSpawnpoint(2, -13), "missing negative group refuses");
            require(manager.groups == std::vector<int>({-13}) && group.trace.empty(), "null lookup no group calls");
        }
        else if (mode == "mismatch")
        {
            group.team = 2;
            group.afterActive = changeAfterActive;
            require(!game.isEligibleSpawnpoint(1, 7), "team captured before active mutation");
            require(group.team == 1 && group.trace == "team,active,", "active called before mismatch decision");
        }
        else if (mode == "inactive")
        {
            group.active = false;
            group.afterHuman = changeAfterHuman;
            require(!game.isEligibleSpawnpoint(1, 7), "captured inactive despite human callback mutation");
            require(group.active && group.trace == "team,active,human,", "human query runs when inactive");
        }
        else if (mode == "human")
        {
            group.human = true;
            require(!game.isEligibleSpawnpoint(1, 7), "active human-only group refuses");
            require(group.trace == "team,active,human,", "human-only query order");
        }
        else if (mode == "active")
        {
            group.afterHuman = changeAfterHuman;
            require(game.isEligibleSpawnpoint(1, 7), "captured active despite human callback mutation");
            require(!group.active && group.trace == "team,active,human,", "active query order");
        }
        else if (mode == "captured")
        {
            originalManager = &manager;
            replacementManager = &other;
            replacementGroup = &second;
            group.afterTeam = changeAfterTeam;
            group.afterActive = changeAfterActive;
            require(game.isEligibleSpawnpoint(1, 7), "captured team and active on retained group");
            require(group.trace == "team,active,human," && second.trace.empty(), "same group through all callbacks");
            require(other.groups.empty() && spawnManager == &other, "no global reload within query");
        }
        else unexpected("unknown mode");
        if (mode != "list" && mode != "position" && mode != "null")
            require(manager.groups == std::vector<int>({7}), "one group lookup");
    }
    std::printf("PASS %s\n", mode.c_str());
    return 0;
}
