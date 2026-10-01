// [TEMP] [CHOSEN, not recovered] actual-source AIPlayerFactory lookup fixture.
// Only the existing registry entry and the factory's private mapping are fixture state.
// The production constructor, destructor and getPlayerFromId body are linked unchanged.
// Uncalled factory methods retain their /FORCE link boundaries; this proves no bot spawn.
#include <climits>
#include <cstdio>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>
#define private public
#include "../BF2/AI/AIInterface/BFKnowledge/AIPlayerFactory.h"
#undef private
#include "../BF2/AI/AIDLL/AI/Main/CommonSingletonServer.h"

namespace dice { namespace hfe { namespace ai {
void* CommonSingletonServer::m_iaiPlayerFactory = 0;
} } }

int main()
{
    using namespace dice::hfe::ai;
    int registryEntry = 0;
    int playerA = 0;
    int playerB = 0;
    // A pre-existing entry exercises the constructor/destructor's recovered first-wins guard.
    CommonSingletonServer::m_iaiPlayerFactory = &registryEntry;
    AIPlayerFactory factory;
    IAIPlayerFactory* const lookup = &factory;
    int checks = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); return 1; } } while (false)
    CHECK(factory.m_players.size() == 256);
    CHECK(lookup->getPlayerFromId(-1) == 0);
    CHECK(lookup->getPlayerFromId(INT_MIN) == 0);
    CHECK(lookup->getPlayerFromId(0) == 0);
    CHECK(lookup->getPlayerFromId(255) == 0);
    // Opaque identity tokens: the lookup must return the pointer without dereferencing it.
    IAIPlayer* const first = reinterpret_cast<IAIPlayer*>(&playerA);
    IAIPlayer* const last = reinterpret_cast<IAIPlayer*>(&playerB);
    factory.m_players[0] = first;
    factory.m_players[255] = last;
    CHECK(lookup->getPlayerFromId(0) == first);
    CHECK(lookup->getPlayerFromId(255) == last);
    CHECK(lookup->getPlayerFromId(127) == 0);
    factory.m_players[0] = last;
    CHECK(lookup->getPlayerFromId(0) == last);
    CHECK(lookup->getPlayerFromId(-1) == 0);
    std::printf("PASS AIPlayerFactory lookup: %d checks\n", checks);
    return 0;
}
