// [TEMP] [CHOSEN, not recovered] HUD paint/query component fixture.
// Runs the actual HUD constructor and four production virtual bodies. Observers
// override only PlayerManager's local-player query and Ref2System's paint callback.
// /FORCE leaves uncalled HUD lifetime, UI descriptors and unrelated subsystems open.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <new>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#define private public
#define protected public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Menu/Ref2System.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#undef protected
#undef private

namespace dice { namespace hfe {
class IMenuLogic; class IGame; class IGameLogic;
IMenuLogic* menuLogic = nullptr;
IGame* game = nullptr;
IGameLogic* gameLogic = nullptr;
namespace world {
PlayerManager* playerManager = nullptr;
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager = nullptr;
}
} }

using namespace dice::hfe;
using hud::BF2HudManager;
static std::string trace;
static unsigned checks, failures;
static void check(bool value, const char* label)
{
    ++checks;
    if (!value) { ++failures; std::printf("FAIL: %s\n", label); std::fflush(stdout); }
}

struct RootObserver : menu::Ref2System
{
    explicit RootObserver(char id) : Ref2System(320.0f, 200.0f), marker(id) {}
    char marker;
    unsigned calls = 0;
    bool result = true;
    std::function<void()> callback;
    bool paint() override
    {
        ++calls;
        trace += marker;
        if (callback) callback();
        return result;
    }
};

struct PlayerObserver : world::PlayerManager
{
    unsigned calls = 0;
    world::IPlayer* result = nullptr;
    std::function<void()> callback;
    world::IPlayer* getLocalHumanPlayer() override
    {
        ++calls;
        trace += 'P';
        if (callback) callback();
        return result;
    }
};

int main()
{
    static_assert(sizeof(void*) == 4, "native x86 fixture only");
    static_assert(std::is_same<decltype(&BF2HudManager::paint),
                  void (BF2HudManager::*)()>::value, "paint has no result contract");
    static_assert(std::is_same<decltype(&BF2HudManager::isSayAllActive),
                  bool (BF2HudManager::*)()>::value, "chat byte boolean contract");
    void* storage = ::operator new(sizeof(BF2HudManager));
    std::memset(storage, 0x5a, sizeof(BF2HudManager));
    BF2HudManager* hud = new (storage) BF2HudManager;
    {
        PlayerObserver players;
        RootObserver first('A'), second('B');
        // These are presence tokens, never dereferenced or called. The original
        // body only compares the three globals and discards the player result.
        unsigned presence[4] = {};
        IMenuLogic* presentMenu = reinterpret_cast<IMenuLogic*>(&presence[0]);
        IGame* presentGame = reinterpret_cast<IGame*>(&presence[1]);
        IGameLogic* presentLogic = reinterpret_cast<IGameLogic*>(&presence[2]);
        world::IPlayer* presentPlayer = reinterpret_cast<world::IPlayer*>(&presence[3]);
        auto reset = [&]() {
            trace.clear(); players.calls = first.calls = second.calls = 0;
            players.callback = nullptr; first.callback = second.callback = nullptr;
            players.result = nullptr; first.result = second.result = true;
            menuLogic = nullptr; game = presentGame; gameLogic = presentLogic;
            world::playerManager = &players;
            hud->m_field248 = true; hud->m_field0f4 = &first;
        };

        // Exhaust all five independent guards. The query is still mandatory when
        // either of the two post-query HUD fields disables painting.
        for (unsigned mask = 0; mask != 32; ++mask) {
            reset();
            menuLogic = mask & 1 ? presentMenu : nullptr;
            game = mask & 2 ? presentGame : nullptr;
            gameLogic = mask & 4 ? presentLogic : nullptr;
            hud->m_field248 = (mask & 8) != 0;
            hud->m_field0f4 = mask & 16 ? &first : nullptr;
            const bool query = !(mask & 1) && (mask & 2) && (mask & 4);
            const bool paint = query && (mask & 8) && (mask & 16);
            hud->paint();
            check(players.calls == unsigned(query), "all guard combinations: player query count");
            check(first.calls == unsigned(paint) && second.calls == 0,
                  "all guard combinations: root paint count");
            check(trace == (paint ? "PA" : query ? "P" : ""),
                  "all guard combinations: query before root paint");
        }
        // A null playerManager is safe only behind an earlier failing guard.
        for (unsigned mask = 0; mask != 8; ++mask) {
            if (mask == 6) continue;
            reset(); world::playerManager = nullptr;
            menuLogic = mask & 1 ? presentMenu : nullptr;
            game = mask & 2 ? presentGame : nullptr;
            gameLogic = mask & 4 ? presentLogic : nullptr;
            hud->paint();
            check(trace.empty(), "failed pre-query guard never touches null playerManager");
        }

        reset();
        hud->m_field248 = false;
        players.callback = [&]() { hud->m_field248 = true; };
        hud->paint();
        check(trace == "PA", "query callback enables initially disabled paint");
        reset();
        players.callback = [&]() { hud->m_field248 = false; };
        hud->paint();
        check(trace == "P", "query callback disables paint before flag read");
        reset();
        players.callback = [&]() { hud->m_field0f4 = &second; };
        hud->paint();
        check(trace == "PB" && first.calls == 0, "query callback replaces root before root read");
        reset();
        hud->m_field0f4 = nullptr;
        players.callback = [&]() { hud->m_field0f4 = &second; };
        hud->paint();
        check(trace == "PB", "query callback installs initially null root");
        reset();
        players.callback = [&]() { hud->m_field0f4 = nullptr; };
        hud->paint();
        check(trace == "P", "query callback removes root before root read");
        reset();
        players.callback = [&]() {
            menuLogic = presentMenu; game = nullptr; gameLogic = nullptr;
            world::playerManager = nullptr;
        };
        hud->paint();
        check(trace == "PA", "pre-query global guards are not repeated after callback");
        reset();
        first.result = false;
        hud->paint();
        check(trace == "PA" && first.calls == 1, "false root result is discarded with no retry");
        reset();
        players.result = presentPlayer;
        hud->paint();
        check(trace == "PA", "nonnull player result is as irrelevant as null result");
        reset();
        first.callback = [&]() { hud->m_field0f4 = &second; hud->m_field248 = false; };
        hud->paint();
        check(trace == "PA" && second.calls == 0, "paint callback does not cause a second root dispatch");

        for (unsigned mask = 0; mask != 8; ++mask) {
            hud->m_field0b0 = (mask & 1) != 0;
            hud->m_field0b1 = (mask & 2) != 0;
            hud->m_field0b2 = (mask & 4) != 0;
            check(hud->isSayAllActive() == bool(mask & 1), "all chat combinations: all getter");
            check(hud->isSayTeamActive() == bool(mask & 2), "all chat combinations: team getter");
            check(hud->isSaySquadActive() == bool(mask & 4), "all chat combinations: squad getter");
            check(hud->isChatActive() == bool(mask), "individual chat getters agree with aggregate");
        }
        world::playerManager = nullptr;
        hud->m_field0f4 = nullptr;
        menuLogic = nullptr; game = nullptr; gameLogic = nullptr;
    } // Actual Ref2System/System/OStream and PlayerManager destruction.
    check(world::explosionEffectManager == nullptr, "real PlayerManager destroys its owned singleton");

    // Constructor-only HUD teardown: never define or call its missing destructor.
    delete hud->m_field2dc;
    using W = std::wstring; using S = std::string; using L = std::list<int>;
    using SL = std::list<std::string>; using V = std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W(); hud->m_field038.~W(); hud->m_field060.~W();
    hud->m_field08c.~S(); hud->m_field0c0.~S(); hud->m_field304.~S();
    hud->m_field0e8.~L(); hud->m_field2e0.~SL(); hud->m_field33c.~SL(); hud->m_field32c.~V();
    ::operator delete(storage);
    std::printf("hud_paint_runtime: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
