// [TEMP] Bounded native HUD fixture; no production virtual frontier is overridden.
// The real EventManager owns subscriptions. Only unknown HUD event IDs are dispatched.
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/Debug.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Menu/HudColors.h"
#include "../BF2/Game/GameServer/ServerSettings.h"
#include <limits>
#include <cstdio>
#include <cstring>
#include <new>
#include <cstddef>
namespace dice { namespace hfe {
EventManager* eventManager = 0;
ServerSettings* serverSettings = 0;
class IGameLogic;
IGameLogic* gameLogic = 0;
struct HudInformationLayerFixture {
    static int count(HudInformationLayer& layer) { return layer.m_nextId; }
    static void blink(HudInformationLayer& layer, float dt) { layer.m_blinkers->update(dt); }
    static void enableBleed(HudInformationLayer& layer) {
        layer.m_state.field88 = true; layer.m_state.field90 = true;
    }
    static bool blinkCompletesAllRows(HudInformationLayer& layer) {
        layer.m_state.field2b0 = false;
        layer.m_state.field2b4 = 7;
        for (int i = 0; i < 7; ++i) {
            layer.m_state.kitUnlockArrowShow[i] = true;
            layer.m_state.kitUnlockAlpha[i] = 0;
        }
        layer.updateKitUnlockBlink(0.25f);
        if (!layer.m_state.field2b0 || layer.m_state.field2b4 != 8) return false;
        for (int i = 0; i < 7; ++i)
            if (layer.m_state.kitUnlockAlpha[i] != 0) return false;
        layer.m_state.kitUnlockAlpha[6] = 0.5f;
        layer.updateKitUnlockBlink(0.25f);
        return layer.m_state.kitUnlockAlpha[6] == 0.5f;
    }
    static bool hitTimerFalseAndNan(HudInformationLayer& layer) {
        layer.m_state.field19c = -1;
        layer.updateHitDirection(false, 0.25f);
        if (layer.m_state.HitDirectionShow || layer.m_state.field19c != 0.1f) return false;
        layer.updateHitDirection(false, 0.25f);
        if (layer.m_state.field19c != 0.1f) return false;
        layer.m_state.field19c = std::numeric_limits<float>::quiet_NaN();
        layer.updateHitDirection(false, 0.25f);
        return layer.m_state.field19c != layer.m_state.field19c;
    }
    static void checkLayout() {
        static_assert(offsetof(HudInformationLayer::State, optionIcons) == 524 - 136, "option suffix offset");
        static_assert(offsetof(HudInformationLayer::State, EnemyTicketsShow) == 564 - 136, "overrun destination");
        static_assert(offsetof(HudInformationLayer::State, weaponFlags) == 696 - 136, "overlapping flags");
        static_assert(offsetof(HudInformationLayer::State, weaponIcons) == 1356 - 136, "final array");
    }
};
} }
using namespace dice::hfe;
static int checks, failures;
static void check(bool condition, const char* text) {
    ++checks;
    if (!condition) { ++failures; std::fprintf(stderr, "FAIL %s\n", text); }
}
static bool callback() { return true; }
static bool callbackFalse() { return false; }
static bool __fastcall stringCallback(const std::string& value) { return value == "abc"; }
class RecordingEvents : public EventManager {
public:
    int registered = 0, removed = 0;
    EventCategory registration[2], removal[2];
    bool registerEventHandler(EventCategory category, IEventListener* listener, int priority) override {
        registration[registered++] = category;
        return EventManager::registerEventHandler(category, listener, priority);
    }
    bool unregisterEventHandler(EventCategory category, IEventListener* listener) override {
        removal[removed++] = category;
        return EventManager::unregisterEventHandler(category, listener);
    }
};
int main() {
    RecordingEvents events;
    eventManager = &events;
    HudInformationLayerFixture::checkLayout();
    void* memory = ::operator new(sizeof(HudInformationLayer));
    std::memset(memory, 0xA5, sizeof(HudInformationLayer));
    HudInformationLayer* layer = new(memory) HudInformationLayer;
    check(events.registered == 2 && events.registration[0] == ECHud && events.registration[1] == ECGame, "real registration order");
    check(layer->getRef() == 1, "initial reference count");
    layer->addRef(); check(layer->getRef() == 2, "addRef");
    check(layer->release() == 1, "release retains live reference");
    check(layer->queryInterface(1) == layer && layer->queryInterface(120008) == layer &&
          layer->queryInterface(121008) == 0, "IBase and interface id, not CID");
    check(HudInformationLayerFixture::count(*layer) == 425, "425 constructor registrations");
    check(layer->getWideStringById(0) == layer->getWideStringByName("KitName0String"), "first shared id");
    check(layer->getStringById(1) == layer->getStringByName("KitIcon0Path"), "second shared id");
    check(layer->getBoolById(0) == 0 && layer->getFloatById(0) == 0, "wrong type has no pointer");
    check(layer->getIntByName("absent") == 0 && layer->getStringById(-1) == 0, "missing name/id");
    check(*layer->getIntByName("OrderMenuPosX") == 50 && *layer->getIntByName("OrderMenuPosY") == 50, "order menu coordinates");
    check(*layer->getIntByName("MapZoom") == 1, "zoom");
    check(*layer->getBoolByName("ShowIngameHud"), "ingame HUD default");
    check(*layer->getBoolByName("ChatListShow"), "chat default");
    check(*layer->getFloatByName("MenuBackgroundAlpha") == 0.7f, "background alpha");
    check(*layer->getFloatByName("MenuMapAlpha") == 0.8f && *layer->getFloatByName("MenuMapIconAlpha") == 0.9f, "map alpha");
    check(*layer->getFloatByName("CrossHairColorRed") == 1 && *layer->getFloatByName("CrossHairColorBlue") == 0, "crosshair color");
    unsigned char untouched[4];
    std::memcpy(untouched, layer->getFloatByName("FriendlyTextRed"), 4);
    check(untouched[0] == 0xA5 && untouched[3] == 0xA5, "uninitialized color stays untouched");
    check(layer->getBoolByName("PlayerWeaponIcon11Show") == layer->getBoolByName("PlayerWeaponIcon1ActiveShow"), "first overlapping flag group");
    check(layer->getBoolByName("PlayerWeaponIcon11ActiveShow") == layer->getBoolByName("PlayerWeaponIcon1SelectShow"), "second overlapping flag group");
    check(layer->getBoolByName("PlayerWeaponIcon11SelectShow") == layer->getBoolByName("ServerInfoSelected"), "server selection alias");
    check(layer->getBoolByName("PlayerWeaponIcon12SelectShow") == layer->getBoolByName("SinglePlayerActive"), "single player alias");
    const char* empty = *layer->getStringByName("CreateSquadActiveIconPath");
    const char* overwritten = 0;
    std::memcpy(&overwritten, layer->getBoolByName("EnemyTicketsShow"), sizeof(overwritten));
    check(overwritten == empty, "option-loop pointer overwrites ticket flags");
    bool custom = true; int integer = 42; float number = 2.5f; const char* text = ""; const wchar_t* wide = L"";
    check(layer->registerBool("customBool", &custom) == 425 && layer->getBoolById(425) == &custom, "register bool");
    check(layer->registerInt("customInt", &integer) == 426 && layer->getIntByName("customInt") == &integer, "register int");
    check(layer->registerFloat("customFloat", &number) == 427 && layer->getFloatByName("customFloat") == &number, "register float");
    check(layer->registerString("customString", &text) == 428 && layer->getStringByName("customString") == &text, "register narrow");
    check(layer->registerWideString("customWide", &wide) == 429 && layer->getWideStringByName("customWide") == &wide, "register wide");
    layer->registerInt("nullInt", 0); check(layer->getIntByName("nullInt") == 0, "null storage retained");
    turnOffAllDebug(6); // Real Debug filter permits the observed continuation branch.
    check(layer->registerFunction("callback", callback) && layer->getFunction("callback")(), "no-argument callback storage/invocation");
    check(!layer->registerFunction("callback", callbackFalse) && layer->getFunction("callback") == callback, "duplicate function leaves first callback");
    check(layer->getFunction("missing") == 0, "missing no-argument callback");
    check(layer->registerStringFunction("callback", stringCallback) && layer->getStringFunction("callback")("abc"), "fastcall string callback and separate namespaces");
    check(!layer->registerStringFunction("callback", 0) && layer->getStringFunction("callback") == stringCallback, "duplicate string callback leaves first");
    check(layer->getStringFunction("missing") == 0, "missing string callback");
    check(layer->registerFunction("null", 0) && layer->getFunction("null") == 0, "null function retained");
    check(!layer->unknown58() && !layer->unknown5C(), "two retail false virtual bodies");
    check(layer->isWithinRadius({0,0,0}, {3,0,4}, 5), "inclusive radius boundary");
    check(!layer->isWithinRadius({0,0,0}, {3,0,4}, 4.9f), "outside radius");
    check(!layer->isWithinRadius({0,0,0}, {0,0,0}, -1), "negative radius");
    check(!layer->isWithinRadius({0,0,0}, {std::numeric_limits<float>::quiet_NaN(),0,0}, 5), "unordered radius comparison");
    bool replacement = false;
    check(layer->registerBool("customBool", &replacement) == 431 &&
          layer->getBoolByName("customBool") == &custom &&
          layer->getBoolById(431) == &replacement, "duplicate keeps first name and allocates new id");
    const char** stringSlot = layer->getStringByName("FriendlyCPString");
    const char* original = *stringSlot;
    layer->updateString(stringSlot, 0); check(*stringSlot == original, "null string input ignored");
    layer->updateString(stringSlot, "alpha"); const char* allocated = *stringSlot;
    check(std::strcmp(*stringSlot, "alpha") == 0 && allocated != original, "string allocation");
    layer->updateString(stringSlot, "alpha"); check(*stringSlot == allocated, "equal string retains allocation");
    layer->updateString(stringSlot, "beta"); check(std::strcmp(*stringSlot, "beta") == 0, "replace string");
    const wchar_t** wideSlot = layer->getWideStringByName("DeathMessage");
    layer->updateWideString(wideSlot, L"wide"); check(std::wcscmp(*wideSlot, L"wide") == 0, "wide allocation");
    const wchar_t* same = *wideSlot; layer->updateWideString(wideSlot, L"wide");
    check(*wideSlot == same, "equal wide retains allocation");
    check(*layer->getFloatByName("FriendlyTicketBleed") == 0, "float blinker initial output");
    HudInformationLayerFixture::enableBleed(*layer);
    HudInformationLayerFixture::blink(*layer, 0.25f);
    check(*layer->getFloatByName("FriendlyTicketBleed") == 0.75f &&
          *layer->getFloatByName("EnemyTicketBleed") == 0.75f, "two smooth live field blinkers");
    layer->handleEvent(ECGame, 1, 0, 0); // Retail ECGame arm is empty.
    check(layer->release() == 0, "virtual lifetime release");
    check(events.removed == 2 && events.removal[0] == ECHud && events.removal[1] == ECGame, "real unregistration order and lifetime");
    events.registered = events.removed = 0;
    check(HudInformationLayer::create(1, reinterpret_cast<IBase*>(1)) == nullptr &&
          events.registered == 0, "factory rejects aggregation before construction");
    IBase* created = HudInformationLayer::create(120008, nullptr);
    check(created && created->getRef() == 1 && events.registered == 2,
          "factory returns supported interface with owned initial reference");
    auto* published = static_cast<HudInformationLayer*>(created);
    const auto originalTop = hud::teamTopColors[2];
    const auto originalEnemyText = hud::textColors[1];
    hud::teamTopColors[2] = {0.11f, 0.22f, 0.33f, 0.44f};
    hud::textColors[1] = {0.55f, 0.66f, 0.77f, 0.88f};
    published->refreshColors();
    check(*published->getFloatByName("FriendlyTeamTopRed") == 0.11f &&
          *published->getFloatByName("FriendlyTeamTopGreen") == 0.22f &&
          *published->getFloatByName("FriendlyTeamTopBlue") == 0.33f &&
          *published->getFloatByName("FriendlyTeamTopAlpha") == 0.44f,
          "refresh reads current friendly top palette, including alpha");
    check(*published->getFloatByName("EnemyTextRed") == 0.55f &&
          *published->getFloatByName("EnemyTextGreen") == 0.66f &&
          *published->getFloatByName("EnemyTextBlue") == 0.77f &&
          *published->getFloatByName("EnemyTextAlpha") == 0.88f,
          "enemy text uses its independent mutable palette");
    check(*published->getFloatByName("FriendlyTicketRed") == hud::ticketColors[2].r &&
          *published->getFloatByName("EnemyTicketBlue") == hud::ticketColors[1].b &&
          *published->getFloatByName("FriendlyTextGreen") == hud::textColors[2].g &&
          *published->getFloatByName("EnemyTeamTopAlpha") == hud::teamTopColors[1].a &&
          *published->getFloatByName("EnemyBrightRed") == hud::brightColors[1].r &&
          *published->getFloatByName("FriendlyBrightBlue") == hud::brightColors[2].b,
          "remaining palette families and team selection");
    check(*published->getFloatByName("CrossHairColorRed") == 1 &&
          *published->getFloatByName("MenuBackgroundAlpha") == 0.7f,
          "refresh leaves unrelated HUD colors intact");
    hud::teamTopColors[2] = originalTop;
    hud::textColors[1] = originalEnemyText;
    published->refreshColors();
    check(*published->getFloatByName("FriendlyTeamTopRed") == originalTop.r,
          "second refresh re-reads palette storage");
    published->handleEvent(ECHud, 0, nullptr, nullptr);
    published->handleEvent(ECHud, 999, nullptr, nullptr);
    *published->getBoolByName("MapFullSizeAndSpawnShow") = true;
    published->update(0.25f); // No GameLogic: must return before any other owner access.
    check(*published->getBoolByName("MapFullSizeAndSpawnShow"),
          "no GameLogic update returns before mutating HUD state");
    check(HudInformationLayerFixture::blinkCompletesAllRows(*published),
          "shared blink completion processes later rows, then gates the next call");
    check(HudInformationLayerFixture::hitTimerFalseAndNan(*published),
          "hidden hit indicator preserves timer and unordered behavior");
    check(created->release() == 0 && events.removed == 2,
          "ignored HUD event IDs preserve the factory object's lifetime");
    events.registered = events.removed = 0;
    check(HudInformationLayer::create(0xFFFFFFFFu, nullptr) == nullptr &&
          events.registered == 2 && events.removed == 2,
          "unsupported factory interface releases the constructed event owner");
    std::printf("HudInformationLayer: %d checks, %d failures; native size %u\n", checks, failures, static_cast<unsigned>(sizeof(HudInformationLayer)));
    return failures ? 1 : 0;
}
