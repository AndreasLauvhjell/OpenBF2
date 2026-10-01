// [TEMP] [CHOSEN, not recovered] Bounded client HUD query fixture.
// Actual HUD, PlayerManager and OldConsole bodies. No HUD method is replaced.
// /FORCE only covers uncalled lifecycle/widget/gameplay dependencies.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#define private public
#define protected public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/IO/Console/Console.h"
#undef protected
#undef private

namespace dice { namespace hfe { namespace world {
PlayerManager* playerManager = nullptr;
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager = nullptr;
} } }

int main()
{
    using namespace dice::hfe;
    using namespace dice::hfe::hud;
    static_assert(sizeof(void*) == 4, "x86 only");
    unsigned checks = 0, failures = 0;
    auto check = [&](bool value, const char* label) {
        ++checks;
        if (!value) { ++failures; std::printf("FAIL: %s\n", label); }
        else std::printf("PASS: %s\n", label);
        std::fflush(stdout);
    };
    void* storage = ::operator new(sizeof(BF2HudManager));
    std::memset(storage, 0x5a, sizeof(BF2HudManager));
    BF2HudManager* hud = new (storage) BF2HudManager;
    float r, g, b, a;
    for (int first = 0; first != 2; ++first)
        for (int second = 0; second != 2; ++second)
        {
            hud->selectBoolColor(first != 0, second != 0, r, g, b, a, 1, 2, 3, 4, 5, 6, 7, 8);
            const int base = first == second ? 5 : 1;
            check(r == base && g == base + 1 && b == base + 2 && a == base + 3, "bool color equality and store order");
        }
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float values[][2] = {{1, 2}, {2, 1}, {1, 1}, {0, -0.0f}, {nan, 1}, {1, nan}, {nan, nan}, {inf, inf}, {-inf, inf}};
    const bool firstArm[] = {false, true, true, true, true, true, true, true, false};
    for (unsigned i = 0; i != sizeof(values) / sizeof(values[0]); ++i)
    {
        hud->selectFloatColor(values[i][0], values[i][1], r, g, b, a, 1, 2, 3, 4, 5, 6, 7, 8);
        const int base = firstArm[i] ? 1 : 5;
        check(r == base && g == base + 1 && b == base + 2 && a == base + 3, "float comparison including unordered and signed zero");
    }
    hud->selectBoolColor(true, true, r, r, r, r, 1, 2, 3, 4, 5, 6, 7, 8);
    check(r == 8, "aliased bool outputs preserve sequential stores");
    hud->selectFloatColor(nan, 1, r, r, r, r, 1, 2, 3, 4, 5, 6, 7, 8);
    check(r == 4, "aliased unordered outputs preserve sequential stores");
    for (unsigned mask = 0; mask != 8; ++mask)
    {
        hud->m_field0b0 = (mask & 1) != 0;
        hud->m_field0b1 = (mask & 2) != 0;
        hud->m_field0b2 = (mask & 4) != 0;
        check(hud->isChatActive() == (mask != 0), "three chat flags form a disjunction");
        if (mask)
            check(hud->hasActiveMenu(), "real active-menu path dispatches new chat virtual");
    }
    check(hud->getPlayerSoldier(nullptr) == nullptr, "null player has no soldier");
    check(hud->getPlayerKit(nullptr) == nullptr, "null kit query dispatches soldier query");
    check(!hud->isPlayerOnFoot(nullptr), "null player is not on foot");
    check(hud->getPlayerVehicleType(nullptr) == -1, "null vehicle-type query returns signed sentinel");
    check(hud->listIgnoredPlayers().empty(), "empty ignored list returns native string");
    check(!hud->isPlayerIgnored(-1), "empty ignored list misses signed id");
    hud->m_field0e8.assign({-1, 5, -1, 7});
    check(hud->isPlayerIgnored(-1) && hud->isPlayerIgnored(7) && !hud->isPlayerIgnored(4), "signed ignored-id membership");
    hud->unignorePlayer(-1);
    check(hud->m_field0e8 == std::list<int>({5, -1, 7}), "remove only first matching ignored id");
    hud->unignorePlayer(42);
    check(hud->m_field0e8.size() == 3, "missing ignored-id removal leaves list intact");
    hud->unignorePlayer(-1);
    check(!hud->isPlayerIgnored(-1), "second removal clears remaining duplicate");
    hud->m_field0e8.clear();
    world::PlayerManager players;
    world::playerManager = &players;
    check(hud->getLocalPlayerTeam() == 0, "real PlayerManager empty local-player query");
    io::mainConsole->m_lines.clear();
    hud->ignorePlayer(-17);
    check(hud->m_field0e8.empty() && io::mainConsole->m_lines.back() == "Error: Could not locate player: -17", "missing player uses real console-output path");
    hud->m_field0e8.push_back(-17);
    hud->ignorePlayer(-17);
    check(hud->m_field0e8.size() == 1 && io::mainConsole->m_lines.back() == "Error: Player is already ignored!", "duplicate player reports error before lookup");
    hud->m_field0e8.clear();
    std::string text = "A\xA7" "0B\xA7" "1C\xA7" "2D\xA7" "3E\xA7" "0F\xA7" "4G";
    const std::string original(text);
    hud->outputChatMessage(text);
    check(text == original && io::mainConsole->m_lines.back() == "ABCDEF\xA7" "4G", "all four byte markers stripped from a by-value copy");
    const auto before = io::mainConsole->m_lines.size();
    hud->outputChatMessage("");
    check(io::mainConsole->m_lines.size() == before, "empty text still reaches real console empty-line behavior");
    hud->outputChatMessage(std::string("a\0b", 3));
    check(io::mainConsole->m_lines.back() == std::string("a\0b", 3), "embedded null survives native stream and console output");
    const int unsupported[] = {-1, 2, 4, 6, 19, 27, 2147483647};
    for (int type : unsupported)
        check(hud->getVehicleTypeIcon(type) == 0, "unsupported vehicle icon returns zero without touching root");
    world::playerManager = nullptr;
    delete hud->m_field2dc;
    using W = std::wstring; using S = std::string; using L = std::list<int>;
    using SL = std::list<std::string>; using V = std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W(); hud->m_field038.~W(); hud->m_field060.~W();
    hud->m_field08c.~S(); hud->m_field0c0.~S(); hud->m_field304.~S();
    hud->m_field0e8.~L(); hud->m_field2e0.~SL(); hud->m_field33c.~SL(); hud->m_field32c.~V();
    ::operator delete(storage);
    std::printf("HUD queries: %u/%u checks passed; native x86 size %u\n", checks - failures, checks, unsigned(sizeof(BF2HudManager)));
    return failures ? 1 : 0;
}
