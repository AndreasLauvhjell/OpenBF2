// [TEMP] Only the two extracted adapter methods are claimed as tested production.
// Chosen native lifetimes construct/destruct real containers, omit registry/world work.
#include "selector_queries_runtime.h"
#include "selector_queries_collaborators.inl"
#include <windows.h>
#include <crtdbg.h>
static dice::hfe::world::IPlayer** currentPlayer;
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
AIPlayer::AIPlayer(world::IPlayer* player) : m_player(player), m_i8(0), m_viewDistance(0) { currentPlayer = &m_player; }
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
    ISpawnGroup* selected = nullptr;
    IBase* replacement = nullptr;
    std::vector<int> groups;
    ISpawnGroup* getSpawnGroup(int id) override
    {
        groups.push_back(id);
        if (replacement) spawnManager = replacement;
        return selected;
    }
};
int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4, "MSVC x86 fixture required");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    require(argc == 2, "mode");
    const std::string mode(argv[1]);
    if (mode == "player_values" || mode == "player_replacement")
    {
        PlayerObserver first, second;
        AIPlayer adapter(&first);
        IAIPlayer& query = adapter;
        if (mode == "player_values")
        {
            for (bool value : {false, true, true, false})
            {
                first.ai = value;
                require(query.getIsAIPlayer() == value, "exact bool forwarding");
            }
            require(first.calls == 4, "one player query each");
        }
        else
        {
            second.ai = true;
            require(!query.getIsAIPlayer(), "original player false");
            *currentPlayer = &second;
            require(query.getIsAIPlayer(), "fresh replacement player true");
            require(first.calls == 1 && second.calls == 1, "distinct query receivers");
        }
    }
    else
    {
        BFGame adapter;
        const IAIGame& query = adapter;
        ManagerObserver manager, replacement;
        GroupObserver first, second;
        manager.selected = &first; replacement.selected = &second;
        spawnManager = &manager;
        if (mode == "control_bits")
        {
            const unsigned int values[] = {0u, 1u, 255u, 0x7fffffffu, 0x80000000u, 0xffffffffu};
            const int ids[] = {0, -1, INT_MIN, INT_MAX, 191, 255};
            for (int i = 0; i < 6; ++i)
            {
                first.control = values[i];
                require(static_cast<unsigned int>(query.getControlPointId(ids[i])) == values[i], "all 32 result bits preserved");
                require(first.calls == i + 1 && manager.groups.back() == ids[i], "exact call counts and signed group argument");
            }
            require(manager.groups.size() == 6, "one lookup per call including zero ID");
        }
        else
        {
            require(mode == "manager_replacement", "known mode");
            first.control = 0xffffffffu; second.control = 72;
            manager.replacement = &replacement;
            require(query.getControlPointId(-9) == -1, "returned group retained after global replacement");
            require(first.calls == 1 && second.calls == 0 && replacement.groups.empty(), "no second manager lookup");
            require(query.getControlPointId(37) == 72, "fresh manager next invocation");
            require(manager.groups == std::vector<int>({-9}) && replacement.groups == std::vector<int>({37}), "lookup arguments and receiver sequence");
            require(first.calls == 1 && second.calls == 1, "one control query per selected group");
        }
        spawnManager = nullptr;
    }
    std::printf("PASS %s\n", mode.c_str());
    return 0;
}
