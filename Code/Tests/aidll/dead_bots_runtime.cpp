// [TEMP] Exact getter and setNewBotKit definitions are extracted into a separate TU.
// Chosen native SAI lifetimes expose typed map pointers; no raw storage or header changes.
#include "dead_bots_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <type_traits>
#include <windows.h>
#include <crtdbg.h>

using namespace dice::hfe;
using namespace dice::hfe::ai;
using DeadBots = std::map<int, std::pair<int, int> >;
static_assert(sizeof(void*) == 4, "MSVC x86 only");
static_assert(std::is_same<decltype(&ISAI::getDeadBots), const DeadBots&(ISAI::*)() const>::value,
              "shared borrowed const map contract");
static_assert(std::is_same<decltype(&SAI::getDeadBots), const DeadBots&(SAI::*)() const>::value,
              "concrete borrowed const map contract");

static unsigned checks;
static DeadBots* constructingMap;
[[noreturn]] static void unexpected(const char* message)
{
    std::fprintf(stderr, "FAIL %s\n", message); std::fflush(stderr); std::abort();
}
static void require(bool condition, const char* message)
{
    ++checks;
    if (!condition) unexpected(message);
}
#include "dead_bots_mocks.inl"

namespace dice { namespace hfe { namespace ai {
// [CHOSEN, not recovered] Real native containers construct/destruct normally.
// Unrelated SAI allocation, singleton publication and timer work are not exercised.
ISAI::~ISAI() {}
SAI::SAI(int side, int, int, Vec2, Vec2) : m_side(side) { constructingMap = &m_map5C; }
SAI::~SAI() {}
} } }

__declspec(noinline) static const DeadBots& throughInterface(const ISAI& sai)
{
    return sai.getDeadBots();
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    if (argc != 2) unexpected("mode required");
    const std::string mode(argv[1]);
    SAI first(1, 0, 0, Vec2(), Vec2());
    DeadBots* firstMap = constructingMap;
    SAI second(2, 0, 0, Vec2(), Vec2());
    DeadBots* secondMap = constructingMap;
    const SAI& constantFirst = first;

    if (mode == "identity")
    {
        require(&constantFirst.getDeadBots() == firstMap, "direct getter borrows exact member");
        require(&throughInterface(first) == firstMap, "shared virtual dispatch borrows exact member");
        require(&throughInterface(second) == secondMap, "second instance returns its own member");
        require(firstMap != secondMap, "native maps have distinct identities");
        require(throughInterface(first).empty(), "empty map stays empty");
        require(&throughInterface(first) == &throughInterface(first), "repeated call retains map identity");
    }
    else if (mode == "contents")
    {
        const int low = (std::numeric_limits<int>::min)();
        const int high = (std::numeric_limits<int>::max)();
        firstMap->emplace(low, std::make_pair(high, -65537));
        firstMap->emplace(-1, std::make_pair(0, 13));
        firstMap->emplace(high, std::make_pair(low, 65536));
        const DeadBots& result = throughInterface(first);
        require(&result == firstMap, "populated map still borrowed");
        require(result.size() == 3, "no entry filtering");
        require(result.begin()->first == low, "signed key ordering retained");
        require(result.at(low) == std::make_pair(high, -65537), "full signed first payload retained");
        require(result.at(-1) == std::make_pair(0, 13), "sentinel kit retained");
        require(result.at(high) == std::make_pair(low, 65536), "full signed final payload retained");
        require(secondMap->empty(), "other instance untouched");
    }
    else if (mode == "borrow_and_copy")
    {
        firstMap->emplace(17, std::make_pair(900, 5));
        const DeadBots& borrowed = throughInterface(first);
        DeadBots snapshot = borrowed;
        first.setNewBotKit(17, -65537);
        require(borrowed.at(17) == std::make_pair(900, -65537), "borrow sees actual setter update");
        require(snapshot.at(17) == std::make_pair(900, 5), "caller copy remains independent");
        first.setNewBotKit(-1, 9);
        require(borrowed.size() == 1, "actual setter does not insert missing id");
        snapshot[17].first = -99;
        require(borrowed.at(17).first == 900, "copy mutation does not alter member");
        firstMap->emplace(-7, std::make_pair(-8, -9));
        require(borrowed.size() == 2 && borrowed.count(-7) == 1, "borrow observes later insertion");
        require(snapshot.size() == 1, "snapshot excludes later insertion");
        firstMap->clear();
        require(borrowed.empty(), "borrow observes native clear");
        require(&throughInterface(first) == &borrowed, "map identity survives clear");
        require(snapshot.size() == 1, "copy survives source clear");
    }
    else unexpected("unknown mode");
    std::printf("PASS %s %u checks\n", mode.c_str(), checks);
    return 0;
}
