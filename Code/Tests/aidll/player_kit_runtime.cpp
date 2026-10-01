// [TEMP] Only the extracted production getter is tested. Chosen native lifetimes
// omit AI/world lifecycle work; every unused collaborator virtual aborts.
#include "player_kit_runtime.h"
#include <windows.h>
#include <crtdbg.h>
static int checks;
static void require(bool result, const char* message)
{
    ++checks;
    if (!result) { std::fprintf(stderr, "FAIL %s\n", message); std::exit(1); }
}
[[noreturn]] void unexpected(const char* method)
{
    std::fprintf(stderr, "Unexpected collaborator %s\n", method); std::abort();
}
static dice::hfe::world::IPlayer** currentPlayer;
static void replacePlayer(dice::hfe::world::IPlayer* replacement) { *currentPlayer = replacement; }
#include "player_kit_collaborators.inl"
namespace dice { namespace hfe {
namespace ai {
IAIPlayer::~IAIPlayer() {}
AIPlayer::AIPlayer(world::IPlayer* player) : m_player(player), m_i8(0), m_viewDistance(0)
{
    currentPlayer = &m_player;
}
AIPlayer::~AIPlayer() {}
}
namespace world {
IPlayer::IPlayer() : m_weakControl(nullptr) {}
IPlayer::~IPlayer() {}
}
} }
using namespace dice::hfe::ai;
using namespace dice::hfe::world;
int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4, "x86 only");
    static_assert(std::is_same<decltype(&AIPlayer::getSpawnKit), int (AIPlayer::*)() const>::value, "adapter signature");
    static_assert(std::is_same<decltype(&IPlayer::getKit), int (IPlayer::*)() const>::value, "underlying signature");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    require(argc == 2, "mode");
    const std::string mode(argv[1]);
    PlayerObserver first, second;
    AIPlayer adapter(&first);
    const IAIPlayer& dispatch = adapter;
    if (mode == "values")
    {
        const int values[] = {INT_MIN, -65537, -1, 0, 1, 5, 13, 255, 65536, INT_MAX};
        int expectedCalls = 0;
        for (int value : values)
        {
            first.value = value;
            require(dispatch.getSpawnKit() == value, "unchanged full signed return");
            require(first.calls == ++expectedCalls && second.calls == 0, "exactly one underlying virtual call");
        }
    }
    else
    {
        require(mode == "replacement", "known mode");
        first.value = -17; second.value = INT_MAX; first.replacement = &second;
        require(dispatch.getSpawnKit() == -17, "current receiver result retained during callback mutation");
        require(first.calls == 1 && second.calls == 0 && *currentPlayer == &second, "callback replacement without a second query");
        require(dispatch.getSpawnKit() == INT_MAX, "next call reads replacement player");
        require(first.calls == 1 && second.calls == 1, "one call on each typed receiver");
    }
    std::printf("PASS %s %d checks\n", mode.c_str(), checks);
}
