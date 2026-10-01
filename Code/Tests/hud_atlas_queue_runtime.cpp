// [TEMP] [CHOSEN, not recovered] Actual HUD atlas registration and deferred
// console queue. The console observer records callbacks, not command execution.
// /FORCE leaves only uncalled HUD/game/UI methods unresolved; no HUD init claim.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <list>
#include <string>
#include <type_traits>
#include <vector>
#define private public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#undef private
#include "../BF2/IO/Console/Console.h"

using namespace dice::hfe;
using hud::BF2HudManager;
static unsigned checks, failures;
static void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::printf("FAIL: %s\n", label); }
}
class ConsoleObserver : public io::OldConsole {
public:
    std::vector<std::string> calls;
    std::function<void()> callback;
    void executeLine(const std::string& text, std::string& result,
                     bool a, bool b, bool c, bool d, bool e) override {
        calls.push_back(text);
        check(&result == &io::Console::ignoredString_, "actual shared console result argument");
        check(!a && !b && !c && !d && !e, "all five retail flags are false");
        if (callback) callback();
    }
};
int main() {
    static_assert(sizeof(void*) == 4, "x86 only");
    static_assert(std::is_same<decltype(&BF2HudManager::getAtlasList),
        const std::list<std::string>& (BF2HudManager::*)()>::value, "borrowed atlas ABI");
    void* storage = ::operator new(sizeof(BF2HudManager));
    std::memset(storage, 0x5a, sizeof(BF2HudManager));
    BF2HudManager* owner = new(storage) BF2HudManager;
    const auto& borrowed = owner->getAtlasList();
    check(&borrowed == &owner->m_field33c && borrowed.empty(), "getter returns actual native member");
    std::string source = "menu.tai";
    owner->addTextureAtlas(source);
    source = "changed";
    check(borrowed.front() == "menu.tai", "atlas insertion owns its input copy");
    owner->addTextureAtlas("second.tai");
    owner->addTextureAtlas("menu.tai");
    owner->addTextureAtlas("MENU.TAI");
    owner->addTextureAtlas("");
    owner->addTextureAtlas("");
    check(std::vector<std::string>(borrowed.begin(), borrowed.end()) ==
          std::vector<std::string>({"menu.tai", "second.tai", "MENU.TAI", ""}),
          "ordered case-sensitive dedup preserves empty names");
    owner->addTextureAtlas(std::string("a\0b", 3));
    owner->addTextureAtlas(std::string("a\0c", 3));
    check(borrowed.size() == 6 && borrowed.back().size() == 3,
          "atlas equality compares complete strings, including embedded nulls");

    io::OldConsole* saved = io::mainConsole;
    io::mainConsole = nullptr;
    owner->executeQueuedConsoleCommands();
    check(owner->m_field2e0.empty(), "empty queue does not touch null console");
    {
        ConsoleObserver first, second;
        source = "first";
        owner->queueConsoleCommand(source);
        source = "modified";
        owner->queueConsoleCommand("second");
        owner->queueConsoleCommand("second");
        check(owner->m_field2e0.front() == "first" && owner->m_field2e0.size() == 3,
              "queue owns copies and retains duplicates");
        first.callback = [&]() {
            check(owner->m_field2e0.size() == 3, "queue remains intact during first dispatch");
            owner->queueConsoleCommand("appended");
            io::mainConsole = &second;
        };
        io::mainConsole = &first;
        owner->executeQueuedConsoleCommands();
        check(first.calls == std::vector<std::string>({"first"}), "first callback invoked once");
        check(second.calls == std::vector<std::string>({"second", "second", "appended"}),
              "live iteration visits append and reloads console after callback");
        check(owner->m_field2e0.empty(), "queue clears after the live walk");
        owner->executeQueuedConsoleCommands();
        check(second.calls.size() == 3, "drained queue does not dispatch again");
        owner->queueConsoleCommand("");
        owner->executeQueuedConsoleCommands();
        check(second.calls.back().empty(), "empty command is dispatched, not dropped");
    }
    io::mainConsole = saved;
    // Only constructed members: full HUD destruction is an explicit frontier.
    delete owner->m_field2dc;
    using W = std::wstring; using S = std::string; using L = std::list<int>;
    using SL = std::list<std::string>; using V = std::vector<dice::hfe::hud::HudPendingRequest>;
    owner->m_field010.~W(); owner->m_field038.~W(); owner->m_field060.~W();
    owner->m_field08c.~S(); owner->m_field0c0.~S(); owner->m_field304.~S();
    owner->m_field0e8.~L(); owner->m_field2e0.~SL(); owner->m_field33c.~SL(); owner->m_field32c.~V();
    ::operator delete(storage);
    std::printf("HUD atlas/queue: %u/%u checks passed; x86 HUD %u bytes\n", checks-failures, checks,
                unsigned(sizeof(BF2HudManager)));
    return failures ? 1 : 0;
}
