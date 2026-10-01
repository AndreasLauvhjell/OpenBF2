// [TEMP] [CHOSEN, not recovered] Actual HUD control bodies, real information
// storage, Data/Object lifetime, Ref2System/System construction and Pointer.
// Only the three declaration-only send slots are modeled as explicit external
// observers. No network/chat-send acceptance. /FORCE leaves uncalled UI frontiers.
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <list>
#include <vector>
#include <functional>
#include <cstdint>
#define private public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#undef private
#include "../BF2/Menu/Ref2System.h"
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/External/BFMeme/Data.h"
#include "../BF2/External/BFMeme/Pointer.h"

namespace dice { namespace hfe {
EventManager* eventManager = 0;
IBase* g_hudInformationLayer = 0;
hud::BF2HudManager* hudManager = 0;
} }
using namespace dice::hfe;
using namespace dice::hfe::hud;
using dice::meme::Data;
using dice::meme::System;
static int checks, failures;
static std::vector<std::string> order;
static void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", label); }
}
// [TEMP] Missing sends have known signatures. These observers deliberately model
// only which send is selected and the state it sees, returning false to prove the
// wrappers ignore it. They do not replace any written production HUD body.
class SendObserver : public BF2HudManager {
public:
    bool sendSayAll() override { order.push_back("all:" + m_field08c); return false; }
    bool sendSayTeam() override { order.push_back("team:" + m_field08c); return false; }
    bool sendSaySquad() override { order.push_back("squad:" + m_field08c); return false; }
};
class InputData : public Data {
public:
    int calls = 0;
    System* expectedSystem = 0;
    std::function<void()> callback;
    bool setString(System* system, const char* value) override {
        ++calls;
        check(system == expectedSystem, "text wrapper passes its current UI system");
        check(value == hudManager->m_field08c.c_str(), "text wrapper passes committed HUD buffer directly");
        order.push_back("data:" + std::string(value));
        if (callback) { auto action = callback; callback = {}; action(); }
        return false; // Meaningful failure: the corresponding send still occurs.
    }
};
class InformationObserver : public HudInformationLayer {
public:
    int disconnectCalls = 0, deathCalls = 0;
    std::vector<bool> flags;
    std::function<void()> callback;
    void setDisconnectMessage(std::wstring value) override {
        ++disconnectCalls;
        flags.push_back(hudManager->m_field0bd);
        HudInformationLayer::setDisconnectMessage(value);
        if (callback) { auto action = callback; callback = {}; action(); }
    }
    void setDeathMessage(std::wstring value) override {
        ++deathCalls;
        HudInformationLayer::setDeathMessage(value);
    }
};
static bool pointerFaults(BF2HudManager* hud) {
    __try { hud->disablePointer(); }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { return true; }
    return false;
}
static void destroyConstructorOnly(SendObserver* hud) {
    // The full HUD destructor is a frontier; no substitute body is provided.
    // SendObserver adds no data. Dispose only real constructor-owned members.
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
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    EventManager events;
    eventManager = &events;
    void* allocation = ::operator new(sizeof(SendObserver));
    std::memset(allocation, 0x5a, sizeof(SendObserver));
    SendObserver* hud = new(allocation) SendObserver;
    hudManager = hud;
    {
        InformationObserver info;
        g_hudInformationLayer = &info;
        check(!hud->m_field0bd && !hud->isMenuLoading(), "real HUD constructor starts disconnect/loading false");
        hud->clearDisconnectMessage();
        check(info.disconnectCalls == 0, "clear without prior show skips storage callback");
        info.callback = [&] { hud->showDisconnectMessage(L"reentrant ignored"); };
        hud->showDisconnectMessage(L"first");
        check(info.disconnectCalls == 1 && info.flags.back(), "show sets once flag before callback and gates reentrant show");
        check(std::wcscmp(*info.getWideStringByName("DisconnectMessage"), L"first") == 0, "actual information storage receives original text");
        g_hudInformationLayer = 0;
        hud->showDisconnectMessage(L"also ignored");
        check(info.disconnectCalls == 1, "once gate skips even an absent information global");
        g_hudInformationLayer = &info;
        info.callback = [&] { hud->clearDisconnectMessage(); };
        hud->clearDisconnectMessage();
        check(info.disconnectCalls == 2 && !info.flags.back() && !hud->m_field0bd, "clear drops flag before callback and gates recursive clear");
        check((*info.getWideStringByName("DisconnectMessage"))[0] == 0, "actual clear updates stored text to empty");
        hud->setMenuLoading(true);
        info.callback = [&] { hud->clearDisconnectMessage(); };
        hud->showDisconnectMessage(L"cleared inside callback");
        check(info.disconnectCalls == 4 && !hud->m_field0bd, "reentrant clear inside show remains effective after outer return");
        check(info.flags[2] && !info.flags[3] && hud->isMenuLoading(), "show/clear callback flags preserve independent loading state");
        hud->setMenuLoading(false);
        check(!hud->isMenuLoading(), "loading toggle does not depend on disconnect state");
        info.HudInformationLayer::setDeathMessage(L"death");
        hud->clearDeathMessage(); hud->clearDeathMessage();
        check(info.deathCalls == 2 && (*info.getWideStringByName("DeathMessage"))[0] == 0, "death clear always calls real storage, including already-empty state");

        menu::Ref2System root(640, 480);
        dice::meme::Pointer pointer;
        pointer.setEnabled(true);
        root.getPointerObject() = &pointer;
        hud->m_field15c = true;
        hud->m_field0f4 = 0;
        check(!pointerFaults(hud) && !hud->isPointerEnabled() && pointer.isEnabled(), "pointer disable clears HUD flag and guards absent root");
        hud->m_field0f4 = &root; hud->m_field15c = true;
        hud->disablePointer();
        check(!hud->isPointerEnabled() && !pointer.isEnabled(), "pointer disable reaches actual root and Pointer storage");
        root.getPointerObject() = 0; hud->m_field15c = true;
        check(pointerFaults(hud) && !hud->isPointerEnabled(), "retail missing pointer guard faults after HUD flag clear");
        root.getPointerObject() = &pointer;

        InputData input, unused;
        input.expectedSystem = &root;
        hud->m_field0a8[0] = &input; hud->m_field0a8[1] = &unused;
        order.clear();
        hud->sendSayAllText("all text");
        check(order == std::vector<std::string>{"data:all text", "all:all text"}, "all wrapper stores data before selected send despite false result");
        order.clear(); hud->sendSayTeamText("");
        check(order == std::vector<std::string>{"data:", "team:"}, "team wrapper forwards empty input and sends");
        order.clear(); hud->sendSaySquadText(std::string("x\0tail", 6));
        check(order.size() == 2 && order[0] == "data:x" && order[1] == std::string("squad:x\0tail", 12), "Data sees C string while stored/send buffer retains embedded null suffix");
        check(unused.calls == 0, "all three wrappers use primary input only");
        order.clear();
        input.callback = [&] { hud->m_field08c = "changed by data"; };
        hud->sendSayAllText("outer");
        check(order == std::vector<std::string>{"data:outer", "all:changed by data"}, "send observes data callback mutation without restoration");
        order.clear();
        input.callback = [&] { hud->sendSayTeamText("nested"); };
        hud->sendSaySquadText("outer");
        check(order == std::vector<std::string>{"data:outer", "data:nested", "team:nested", "squad:nested"}, "nested Data callback preserves corresponding inner and outer send order");
        hud->m_field0f4 = 0; input.expectedSystem = 0;
        order.clear(); hud->sendSayAllText("without root");
        check(order == std::vector<std::string>{"data:without root", "all:without root"}, "text wrapper forwards absent root without adding a guard");
        hud->m_field0a8[0] = hud->m_field0a8[1] = 0;
        root.getPointerObject() = 0;
        g_hudInformationLayer = 0;
    }
    destroyConstructorOnly(hud);
    hudManager = 0;
    std::printf("HUD controls: %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
