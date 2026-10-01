// [TEMP] [CHOSEN, not recovered] Isolated native x86 radio-family validation.
// Real radio/HUD getter/localization/lexicon/adapter/storage/EventManager bodies.
// The console override records radio dispatch only, not console interpretation.
// /FORCE permits uncalled dependencies; it is not HUD initialization acceptance.
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <clocale>
#include <new>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <functional>
#include <type_traits>
#include <cstdint>
#define private public
#include "../BF2/Menu/HudRadio.h"
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/IO/Localization/ExcelLexicon.h"
#undef private
#include "../BF2/Menu/HudInformationAdapter.h"
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/IO/Console/Console.h"

// Observe native member allocation ownership over a closed radio lifetime.
static long allocationBalance;
void* operator new(std::size_t n) {
    void* p = std::malloc(n ? n : 1);
    if (!p) throw std::bad_alloc();
    ++allocationBalance;
    return p;
}
void operator delete(void* p) noexcept { if (p) { --allocationBalance; std::free(p); } }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }

namespace dice { namespace hfe {
EventManager* eventManager = 0;
IBase* g_hudInformationLayer = 0;
hud::BF2HudManager* hudManager = 0;
namespace io { IBase* lexicon = 0; }
} }
using namespace dice::hfe;
using namespace dice::hfe::hud;
static int checks, failures;
static std::vector<std::string> order;
static void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", label); }
}
class RecordingRadio : public HudRadio {
public:
    std::vector<std::string> keys;
    std::function<void(std::string&)> prefixCallback;
    std::function<void(const std::string&)> textCallback;
    void setPrefix(std::string value) override {
        if (prefixCallback) prefixCallback(value);
        HudRadio::setPrefix(value);
    }
    std::wstring getText(std::string key) override {
        keys.push_back(key);
        order.push_back("text:" + key);
        if (textCallback) textCallback(key);
        return HudRadio::getText(key);
    }
};
class RecordingAdapter : public HudInformationAdapter {
public:
    std::vector<std::pair<std::string, std::wstring>> writes;
    std::function<void()> afterWrite;
    bool setWideString(std::string key, std::wstring value) override {
        writes.emplace_back(key, value);
        order.push_back("write:" + key);
        const bool result = HudInformationAdapter::setWideString(key, value);
        if (afterWrite) afterWrite();
        return result;
    }
};
class RecordingLexicon : public io::ExcelLexicon {
public:
    mutable std::function<void()> callback;
    std::wstring getString(const std::string& key) const override {
        if (callback) { auto action = callback; callback = {}; action(); }
        return ExcelLexicon::getString(key);
    }
};
class RecordingConsole : public io::OldConsole {
public:
    int calls = 0;
    std::string line;
    std::string* output = 0;
    bool flags[5] = {};
    std::function<void()> callback;
    void executeLine(const std::string& value, std::string& out,
                     bool a, bool b, bool c, bool d, bool e) override {
        ++calls; line = value; output = &out;
        flags[0] = a; flags[1] = b; flags[2] = c; flags[3] = d; flags[4] = e;
        order.push_back("console:" + value);
        if (callback) callback();
    }
};
static bool updateFaults(HudRadio* radio, const char** destination, const char* source) {
    __try { radio->updateString(destination, source); }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { return true; }
    return false;
}
static void destroyConstructedHud(BF2HudManager* hud) {
    // HUD's full destructor remains incomplete; destroy only its real constructor
    // ownership as in the existing HUD prefix fixture, without replacing it.
    delete hud->m_field2dc;
    using S = std::string; using W = std::wstring;
    using L = std::list<int>; using SL = std::list<std::string>;
    using V = std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field33c.~SL(); hud->m_field32c.~V(); hud->m_field304.~S();
    hud->m_field2e0.~SL(); hud->m_field0e8.~L(); hud->m_field0c0.~S();
    hud->m_field08c.~S(); hud->m_field060.~W(); hud->m_field038.~W(); hud->m_field010.~W();
    ::operator delete(hud);
}
int main() {
    static_assert(sizeof(void*) == 4, "x86 only");
    static_assert(!std::has_virtual_destructor<HudRadio>::value, "retail has no virtual destructor");
    std::setlocale(LC_ALL, "C");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    {
        alignas(HudRadio) unsigned char memory[sizeof(HudRadio)];
        std::memset(memory, 0x5a, sizeof memory);
        HudRadio* radio = new (memory) HudRadio;
        check(!radio->isActive() && radio->isEnabled(), "constructor flags");
        check(memory[6] == 0x5a && memory[7] == 0x5a, "constructor leaves flag padding untouched");
        check(radio->m_path == "1" && radio->m_prefix.empty() && radio->m_count == 1, "constructor path prefix count");
        check(radio->m_text.empty() && radio->m_commands.empty(), "constructor maps empty");
        radio->setActive(true); radio->setEnabled(false);
        check(radio->isActive() && !radio->isEnabled(), "independent virtual flag setters");
        radio->~HudRadio();
    }
    EventManager events;
    eventManager = &events;
    void* hudStorage = ::operator new(sizeof(BF2HudManager));
    std::memset(hudStorage, 0x5a, sizeof(BF2HudManager));
    hudManager = new (hudStorage) BF2HudManager;
    RecordingLexicon lexicon;
    lexicon.m_strings.insert("ALPHA", L"First");
    lexicon.m_strings.insert("BETA", L"Second");
    io::lexicon = &lexicon;
    io::OldConsole* savedConsole = io::mainConsole;
    {
        RecordingConsole console;
        io::mainConsole = &console;
        HudInformationLayer information;
        g_hudInformationLayer = &information;
        RecordingAdapter adapter, otherAdapter;
        hudManager->m_field104 = &adapter;
        check(hudManager->getInformationAdapter() == &adapter, "real typed HUD information-adapter getter");
        {
            HudRadio owners[5];
            const char* prefixes[] = {"Radio", "RadioVehicle", "SquadLeader", "Squad", "Commander"};
            for (int i = 0; i < 5; ++i) {
                owners[i].configure(prefixes[i], 11);
                check(owners[i].m_prefix == prefixes[i] && owners[i].m_count == 11 &&
                      owners[i].m_path == "1" && !owners[i].isActive() && owners[i].isEnabled(),
                      "each actual HUD init configuration preserves constructor state");
            }
        }
        RecordingRadio radio;
        std::string prefix("Radio");
        radio.prefixCallback = [&](std::string& value) {
            check(radio.m_count == 1, "configure forwards before storing new count");
            value = "Radio";
            radio.m_count = 77;
            prefix = "caller changed";
        };
        radio.configure(prefix, 3);
        check(radio.m_prefix == "Radio" && radio.m_count == 3 && prefix == "caller changed", "configure by-value callback and final count overwrite");
        radio.prefixCallback = {};
        radio.addEntry("entry", "ALPHA", "command.one");
        check(radio.getText("entry") == L"First" && radio.getCommand("entry") == "command.one", "insert localizes text and stores third command argument");
        radio.addEntry("entry", "BETA", "command.two");
        check(radio.getText("entry") == L"Second" && radio.getCommand("entry") == "BETA", "existing command uses second argument retail quirk");
        radio.addEntry("entry", "", "nonempty");
        check(radio.getText("entry") == L"Second" && radio.getCommand("entry").empty(), "empty text skips text update but clears existing command");
        radio.addEntry("only-command", "", "kept");
        check(radio.getText("only-command").empty() && radio.getCommand("only-command") == "kept", "independent command insertion gate");
        radio.addEntry("only-text", "ALPHA", "");
        check(radio.getText("only-text") == L"First" && radio.getCommand("only-text").empty(), "independent text insertion gate");
        const auto texts = radio.m_text.size(), commands = radio.m_commands.size();
        radio.addEntry("", "ALPHA", "ignored");
        check(radio.m_text.size() == texts && radio.m_commands.size() == commands, "empty key rejects both maps");
        check(radio.getText("ENTRY").empty() && radio.getCommand("ENTRY").empty(), "radio maps use case-sensitive comparator");
        check(radio.getText("").empty() && radio.getCommand("missing").empty(), "empty and absent lookup return owned empty values");
        check(radio.m_text.size() == texts && radio.m_commands.size() == commands, "missing lookup does not insert");
        auto textCopy = radio.getText("entry"); auto commandCopy = radio.getCommand("only-command");
        textCopy[0] = L'X'; commandCopy[0] = 'X';
        check(radio.getText("entry") == L"Second" && radio.getCommand("only-command") == "kept", "lookup returns independent string copies");
        // Missing key is decided before localization. The callback inserts it;
        // the original outer insert must leave that callback's value intact.
        lexicon.callback = [&] { radio.m_text.insert(std::make_pair("racing", L"callback")); };
        radio.addEntry("racing", "ALPHA", "");
        check(radio.getText("racing") == L"callback", "lookup outcome precedes localization callback insertion");
        std::string callerKey("by-value"), callerId("ALPHA"), callerCommand("original-command");
        lexicon.callback = [&] { callerKey = "changed"; callerId = "BETA"; callerCommand = "changed"; };
        radio.addEntry(callerKey, callerId, callerCommand);
        check(radio.getText("by-value") == L"First" && radio.getCommand("by-value") == "original-command", "all three entry arguments survive caller mutation during localization");
        radio.addEntry("Interface_1:1", "ALPHA", "");
        radio.addEntry("Interface_1:3", "BETA", "");
        radio.setActive(true); radio.setEnabled(false);
        order.clear(); radio.keys.clear(); adapter.writes.clear();
        radio.reset();
        check(radio.m_path == "1" && !radio.isActive() && !radio.isEnabled(), "reset ignores enabled and clears active");
        check(adapter.writes.size() == 3, "reset writes all signed count entries including empty text");
        check(adapter.writes[0] == std::make_pair(std::string("Radio1String"), std::wstring(L"First")), "reset first formatted binding");
        check(adapter.writes[1].second.empty() && adapter.writes[2].second == L"Second", "reset empty and third binding values");
        check(std::wcscmp(*information.getWideStringByName("Radio1String"), L"First") == 0, "real adapter and information storage receive reset output");
        check(order[0] == "text:Interface_1:1" && order[1] == "write:Radio1String", "reset text then writer ordering");
        // Keep the old c_str allocation alive while the virtual text callback
        // swaps it away; this distinguishes cached prefix from a later reload.
        std::string heldPrefix;
        radio.setPrefix("LongRadioPrefixBeforeCallback");
        radio.textCallback = [&](const std::string&) {
            if (heldPrefix.empty()) { heldPrefix.swap(radio.m_prefix); radio.setPrefix("After"); radio.m_count = 2; }
        };
        adapter.writes.clear(); otherAdapter.writes.clear();
        adapter.afterWrite = [&] { hudManager->m_field104 = &otherAdapter; };
        radio.reset();
        check(adapter.writes.size() == 1 && otherAdapter.writes.size() == 1, "reset reloads HUD adapter and mutable count each iteration");
        check(adapter.writes[0].first == "LongRadioPrefixBeforeCallback1String" && otherAdapter.writes[0].first == "After2String", "reset caches prefix pointer before virtual text callback");
        radio.textCallback = {}; adapter.afterWrite = {}; hudManager->m_field104 = &adapter;
        radio.configure("Radio", 3);
        radio.addEntry("Interface_1:7:2", "BETA", "");
        adapter.writes.clear(); radio.keys.clear(); radio.setActive(true);
        const int consoleBefore = console.calls;
        check(radio.selectOption("7"), "select submenu always returns true");
        check(radio.m_path == "1:7" && radio.isActive(), "submenu appends path and preserves active");
        check(adapter.writes.size() == 1 && adapter.writes[0].first == "Radio2String" && adapter.writes[0].second == L"Second", "submenu only writes nonempty options");
        check(console.calls == consoleBefore && radio.keys.size() == 3, "submenu avoids console after scanning every entry");
        // The path is reread after getText, and the final concatenation also uses
        // the then-current path. A cached initial path would miss this child.
        radio.m_path = "1";
        radio.addEntry("Interface_2:9:2", "ALPHA", "");
        radio.keys.clear();
        radio.textCallback = [&](const std::string&) { radio.m_path = "2"; };
        check(radio.selectOption("9") && radio.m_path == "2:9", "selection rereads path after virtual callbacks and before append");
        check(radio.keys[0] == "Interface_1:9:1" && radio.keys[1] == "Interface_2:9:2", "later option keys use callback-mutated path");
        radio.textCallback = {};
        radio.m_path = "1:7";
        radio.addEntry("Interface_1:7", "ALPHA", "");
        radio.addEntry("Interface_1:7:4", "", "radio.command");
        radio.keys.clear(); console.callback = [&] { radio.setActive(true); };
        check(radio.selectOption("4"), "leaf selection returns true");
        check(console.calls == consoleBefore + 1 && console.line == "radio.command\n", "leaf executes command with appended newline");
        check(console.output == &io::Console::ignoredString_, "leaf passes real shared ignored output string");
        check(!console.flags[0] && !console.flags[1] && !console.flags[2] && !console.flags[3] && !console.flags[4], "leaf forwards five false console flags");
        check(!radio.isActive() && radio.m_path == "1:7", "leaf clears active after console callback without changing path");
        check(radio.keys.back() == "Interface_1:7", "leaf performs discarded context text lookup");
        radio.configure("Radio", -2); radio.keys.clear(); adapter.writes.clear();
        check(radio.selectOption("missing") && console.line == "\n", "negative count still executes empty missing command plus newline");
        check(radio.keys.size() == 1 && adapter.writes.empty(), "negative count skips option and adapter loop but reads context");
        radio.setActive(true); radio.reset();
        check(!radio.isActive() && radio.m_path == "1" && adapter.writes.empty(), "negative count reset state without callbacks");
        radio.configure("Radio", 0); radio.reset();
        check(adapter.writes.empty(), "zero count reset has no entries");
        char* initialString = static_cast<char*>(::operator new(1));
        *initialString = 0;
        const char* value = initialString;
        radio.updateString(&value, "");
        check(value == initialString, "equal empty string keeps identity");
        radio.updateString(&value, "changed");
        check(std::strcmp(value, "changed") == 0, "string helper clones replacement source");
        const char* old = value;
        radio.updateString(&value, "changed");
        check(value == old, "equal string preserves allocation");
        radio.updateString(&value, 0);
        check(value == old && !updateFaults(&radio, 0, 0), "null source skips every destination access");
        radio.updateString(&value, "replacement");
        check(std::strcmp(value, "replacement") == 0, "different string replaces owned allocation");
        ::operator delete(const_cast<char*>(value));
        const char* nullValue = 0;
        check(updateFaults(&radio, &nullValue, "x"), "retail null old string guard remains absent");
        check(updateFaults(&radio, 0, "x"), "retail null destination guard remains absent");
        // No callbacks/formatting/global warmup inside this ownership interval.
        const long before = allocationBalance;
        {
            HudRadio owned;
            owned.setPrefix(std::string(200, 'p'));
            for (int i = 0; i < 200; ++i) {
                const std::string key = std::to_string(i) + std::string(40, 'k');
                owned.m_text.insert(std::make_pair(key, std::wstring(70, L'w')));
                owned.m_commands.insert(std::make_pair(key, std::string(90, 'c')));
            }
            check(owned.m_text.size() == 200 && owned.m_commands.size() == 200, "native map growth before destruction");
        }
        check(allocationBalance == before, "concrete destructor releases all native map/string allocations");
        hudManager->m_field104 = 0;
        g_hudInformationLayer = 0;
    }
    io::mainConsole = savedConsole;
    io::lexicon = 0;
    destroyConstructedHud(hudManager);
    hudManager = 0;
    std::printf("HUD radio: %d/%d checks passed; native x86 size %u\n", checks - failures, checks, unsigned(sizeof(HudRadio)));
    return failures ? 1 : 0;
}
