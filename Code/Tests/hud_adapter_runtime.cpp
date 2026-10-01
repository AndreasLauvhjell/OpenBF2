// [TEMP] Native x86 Data and HUD adapter fixture. Real production bodies are
// linked; no declaration-only production callback or virtual is overridden.
#include "../BF2/Menu/HudInformationAdapter.h"
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Game/Main/BF2ClientSeams.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/Debug.h"
#include "../BF2/External/BFMeme/Data.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include <cstdint>
#include <list>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <limits>
#define private public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#undef private
namespace dice { namespace hfe {
EventManager* eventManager = 0;
class ServerSettings;
ServerSettings* serverSettings = 0;
class IGameLogic; IGameLogic* gameLogic = 0;
class IGame; IGame* game = 0;
IBase* g_hudInformationLayer = 0;
hud::BF2HudManager* hudManager = 0;
namespace world { class PlayerManager; PlayerManager* playerManager = 0; }
namespace io { IBase* lexicon = 0; IBase* controlMapManager = 0; }
IBase* g_controlSettings = 0;
IBase* g_localProfileManager = 0;
} }
using namespace dice::hfe;
using namespace dice::meme;
static int checks, failures;
static void check(bool condition, const char* text) {
    ++checks;
    if (!condition) { ++failures; std::fprintf(stderr, "FAIL %s\n", text); }
}
// [TEMP] Concrete Data consumer records conversion and scalar dispatch. It only
// replaces Data's recovered default setter for this deliberately narrow case.
class RecordingData : public Data {
public:
    std::string text;
    bool setString(System*, const char* value) override { text = value; return false; }
};
// [TEMP] A long character source exercises HUD's real fixed extraction bound.
class LongTextData : public Data {
public:
    int getCharacter(System*, int index) override { return index < 300 ? 'x' : 0; }
};
// [TEMP] Full-register values distinguish the Data virtual ABI from the
// narrow and wide destination formats of its extraction helpers.
class IntegerCharacterData : public Data {
public:
    int value = -128;
    int getCharacter(System*, int index) override { return index ? 0 : value; }
};
// [TEMP] Install the actual name-tree root without invoking System::init's
// unresolved Pointer/Style closure. No System virtual is overridden.
class RegistrySystem : public System {
public:
    RegistrySystem() : System(800.0f, 600.0f) { m_registry = &root; }
    NameTreeRoot root;
};
class RecordingEvents : public EventManager {
public:
    int registered = 0, removed = 0;
    EventCategory registration[4], removal[4];
    bool registerEventHandler(EventCategory category, IEventListener* listener, int priority) override {
        registration[registered++] = category;
        return EventManager::registerEventHandler(category, listener, priority);
    }
    bool unregisterEventHandler(EventCategory category, IEventListener* listener) override {
        removal[removed++] = category;
        return EventManager::unregisterEventHandler(category, listener);
    }
};
class HudEventCapture : public IEventListener {
public:
    int count = 0;
    unsigned int id = 0;
    void handleEvent(EventCategory category, unsigned int eventId, EventNode*, void*) override {
        if (category == ECHud) { ++count; id = eventId; }
    }
};
int main() {
    static_assert(sizeof(Data) == 8, "retail Data layout");
    static_assert(sizeof(BoolRefData) == 16, "retail bound bool layout");
    static_assert(sizeof(hud::HudInformationAdapter) == 28, "retail adapter layout");
    Data unnamed;
    check(unnamed.getName() == 0, "null name retained");
    char sourceName[] = "owned";
    Data named(sourceName); sourceName[0] = 'x';
    check(std::strcmp(named.getName(), "owned") == 0, "owned name clone");
    check(named.setName("") && named.getName() && !named.getName()[0], "empty name distinct from null");
    check(named.setName(0) && named.getName() == 0, "clear owned name");
    check(!named.slot_0x20() && !named.slot_0x24() && named.onCreate(0), "retail base flags");
    check(named.streamProperties(0, 0) && named.visit0x2c(0, 0) && named.visit0x3c(0, 0), "base property dispatch");
    check(named.setFloat(0, 42) && named.setString(0, "ignored") && named.getFloat(0) == 0, "retail scalar defaults");
    check(named.getElementCount(0) == 1 && named.getElement(0) == &named && !named.getElement(-1) && !named.getElement(1), "single default element");
    check(named.getFloatElement(0, 18) == 0 && named.getCharacterElement(0, 25, 4) == 0, "default indexed dispatch");
    IntegerCharacterData integerCharacter;
    Data* characterApi = &integerCharacter;
    check(characterApi->getCharacter(0, 0) == -128 && characterApi->getCharacterElement(0, 4, 0) == -128,
          "scalar and indexed character virtuals preserve signed full-register return");
    integerCharacter.value = 0x10041;
    char truncatedCharacter[2]; wchar_t truncatedWideCharacter[2];
    check(characterApi->getCharacterElement(0, 9, 0) == 0x10041 &&
          characterApi->getString(0, truncatedCharacter, 2) == 1 && truncatedCharacter[0] == 'A' &&
          characterApi->getWideString(0, truncatedWideCharacter, 2) == 1 && truncatedWideCharacter[0] == L'A',
          "only extraction destinations narrow integer character values");
    RecordingData conversion;
    check(conversion.setWideString(0, L"A\x142") && conversion.text == "AB", "wide conversion uses low byte and ignores scalar failure");
    check(conversion.setStringElement(0, 123, "value") && conversion.text == "value", "indexed wrapper ignores index and scalar failure");
    BoolRefData fallback(0, "flag");
    check(fallback.getFloat(0) == 0 && fallback.getCharacter(0, 0) == L'f', "null binding owns false fallback");
    fallback.setFloat(0, 1.0f);
    check(fallback.getFloat(0) == 1, "exact-one float becomes true");
    fallback.setFloat(0, 2.0f); check(fallback.getFloat(0) == 0, "other nonzero float becomes false");
    fallback.setFloat(0, std::numeric_limits<float>::quiet_NaN());
    check(fallback.getFloat(0) == 0, "unordered float becomes false");
    fallback.setString(0, "TrUe"); check(fallback.getFloat(0) == 1, "case-insensitive true text");
    fallback.setString(0, "1"); check(fallback.getFloat(0) == 1, "exact one text");
    fallback.setString(0, "01"); check(fallback.getFloat(0) == 0, "other numeric text becomes false");
    bool external = false;
    fallback.setString(0, "true"); fallback.bind(0, &external);
    check(fallback.getFloat(0) == 0, "external binding replaces fallback");
    fallback.setString(0, "true"); check(external, "setter writes external storage");
    fallback.bind(0, 0); check(fallback.getFloat(0) == 1, "rebinding null preserves prior fallback");
    char narrow[8]; wchar_t wide[8];
    check(fallback.getString(0, narrow, 8) == 4 && std::strcmp(narrow, "true") == 0, "narrow extraction count");
    check(fallback.getWideString(0, wide, 8) == 4 && std::wcscmp(wide, L"true") == 0, "wide extraction count");
    fallback.setFloat(0, 0);
    check(fallback.getString(0, narrow, 5) == 5 && std::strcmp(narrow, "fals") == 0, "full buffer truncates final copied character");
    char guard[] = {'x', 'y', 'z'};
    check(fallback.getString(0, guard + 1, 0) == 0 && guard[0] == 0 && guard[1] == 'y', "zero capacity retains preceding-byte write");
    check(fallback.getCharacter(0, -1) == 0 && fallback.getCharacter(0, 5) == 0, "character bounds");
    fallback.bind(0, &external); *reinterpret_cast<unsigned char*>(&external) = 2;
    check(fallback.getFloat(0) == 0 && fallback.getCharacter(0, 0) == L't', "noncanonical byte differs between float and text");
    external = false;
    check(fallback.getType() == Data::getClassType() && fallback.getObjectSize() == 8, "bound bool inherits Data descriptor and reported size");
    TypeDescriptor* descriptor = Data::getClassType();
    check(descriptor->getBaseType() == Object::getClassType() && descriptor->getInstanceSize() == 8 && !descriptor->getDescription(), "Data descriptor identity and base");
    RuntimeObject* created = descriptor->create();
    check(created && created->getType() == descriptor && !static_cast<Data*>(created)->getName(), "real Data descriptor factory");
    created->destroy();
    descriptor->addInstance(); check(descriptor->getInstanceCount() == 1, "descriptor instance counter");
    descriptor->removeInstance(); check(descriptor->getInstanceCount() == 0, "descriptor decrement");

    RecordingEvents events; eventManager = &events;
    {
        HudInformationLayer layer; g_hudInformationLayer = &layer;
        {
            hud::HudInformationAdapter adapter;
            check(events.registered == 4 && events.registration[2] == ECGame && events.registration[3] == ECHud, "adapter subscription order with real EventManager");
            int initial[5]; std::memcpy(initial, reinterpret_cast<char*>(&adapter) + 8, sizeof(initial));
            check(initial[0] == 0 && initial[1] == 1 && initial[2] == 0 && initial[3] == 0 && initial[4] == 0, "adapter complete constructor state");
            hud::IHudInformationAdapter* api = &adapter;
            check(api->setBool("SpawnShow", true) && api->getBool("SpawnShow"), "typed bool storage dispatch");
            check(!api->setBool("absent", true) && !api->getBool("absent"), "missing bool path");
            check(api->setInt(std::string("MapZoom\0suffix", 14), 7) && api->getInt("MapZoom") == 7, "by-value name truncates at embedded NUL");
            check(!api->setInt("absent", 7) && api->getInt("absent") == 0, "missing integer path");
            check(api->setFloat("MenuBackgroundAlpha", 0.25f) && api->getFloat("MenuBackgroundAlpha") == 0.25f, "float storage dispatch");
            check(!api->setFloat("absent", 1) && api->getFloat("absent") == 0, "missing float path");
            check(api->setString("FriendlyCPString", "adapter") && std::strcmp(*layer.getStringByName("FriendlyCPString"), "adapter") == 0, "owned narrow replacement");
            check(api->setWideString("DeathMessage", L"wide adapter") && std::wcscmp(*layer.getWideStringByName("DeathMessage"), L"wide adapter") == 0, "owned wide replacement");
            check(!api->setString("absent", "x") && !api->setWideString("absent", L"x"), "missing string paths");
            {
                RegistrySystem registrySystem;
                api->setSystem(&registrySystem);
                check(api->registerData(), "real adapter attempts both System registrations");
                Data* left = static_cast<Data*>(registrySystem.root.findObject(&registrySystem, "MiddleLeft/LeftShow"));
                Data* top = static_cast<Data*>(registrySystem.root.findObject(&registrySystem, "TopLeft/TopShow"));
                check(left && top && left != top && !std::strcmp(left->getName(), "LeftShow") &&
                      !std::strcmp(top->getName(), "TopShow"), "real name tree owns two distinct named Boolean bindings");
                check(descriptor->getInstanceCount() == 2, "real System registration increments inherited Data descriptor twice");
                if (left && top) {
                    left->setString(&registrySystem, "false");
                    check(!api->getBool("SpawnShow") && top->getFloat(&registrySystem) == 0,
                          "LeftShow writes shared SpawnShow storage seen by TopShow");
                    top->setFloat(&registrySystem, 1.0f);
                    check(api->getBool("SpawnShow") && left->getFloat(&registrySystem) == 1,
                          "TopShow writes shared SpawnShow storage seen by LeftShow");
                    char registeredPath[256];
                    check(registrySystem.root.objectPath(left, registeredPath, 256) &&
                          !std::strcmp(registeredPath, "MiddleLeft/LeftShow"), "real registry reconstructs registered path");
                    NameTreeNode* leftEntry = registrySystem.root.findEntry(&registrySystem, left);
                    NameTreeNode* topEntry = registrySystem.root.findEntry(&registrySystem, top);
                    // Only the recovered entry-close bodies run. Root::close
                    // would invoke the declaration-only preClose visitor first.
                    check(leftEntry->close(&registrySystem) && topEntry->close(&registrySystem) &&
                          descriptor->getInstanceCount() == 0, "real entry close destroys bindings and balances descriptor count");
                }
                api->setSystem(0);
            }
            turnOffAllDebug(6);
            check(api->registerFunctions() && layer.getStringFunction("executeConsoleCommand"), "real callback registration");
            check(api->registerFunctions(), "adapter ignores duplicate callback rejection");
            // Real HUD construction and methods; teardown is constructor-only
            // because the production widget-owning destructor is still unwritten.
            void* hudStorage = ::operator new(sizeof(hud::BF2HudManager));
            std::memset(hudStorage, 0x5a, sizeof(hud::BF2HudManager));
            hud::BF2HudManager* hudObject = new(hudStorage) hud::BF2HudManager;
            hudObject->m_field104 = &adapter;
            hudManager = hudObject;
            hudObject->showMedalIcon("medal.tga", 4.0f);
            check(api->getBool("ShowMedalIcon") && !api->getBool("ShowRankUpIcon") &&
                  std::strcmp(*layer.getStringByName("MedalIconPathString"), "medal.tga") == 0 && hudObject->m_field238 == 4.0f,
                  "real HUD medal path enables icon and stores duration");
            hudObject->showRankUpIcon("rank.tga", 3.0f);
            check(api->getBool("ShowRankUpIcon") && !api->getBool("ShowMedalIcon") &&
                  std::strcmp(*layer.getStringByName("RankUpIconPathString"), "rank.tga") == 0 && hudObject->m_field234 == 3.0f,
                  "real HUD rank path hides medal and stores duration");
            hudObject->showMedalIcon("ignored.tga", 0.0f);
            check(api->getBool("ShowRankUpIcon") && !api->getBool("ShowMedalIcon") && hudObject->m_field238 == 4.0f &&
                  std::strcmp(*layer.getStringByName("MedalIconPathString"), "medal.tga") == 0,
                  "zero medal duration hides only medal without overwriting timer or path");
            hudObject->showRankUpIcon("ignored.tga", std::numeric_limits<float>::quiet_NaN());
            check(!api->getBool("ShowRankUpIcon") && hudObject->m_field234 == 3.0f,
                  "unordered rank duration takes retail hide arm");
            hudObject->showRankUpIcon("rank2.tga", 5.0f);
            hudObject->showMedalIcon("medal2.tga", 6.0f);
            check(!api->getBool("ShowRankUpIcon") && api->getBool("ShowMedalIcon"), "medal activation hides active rank");
            hudObject->showMedalIcon("ignored.tga", -1.0f);
            check(!api->getBool("ShowMedalIcon") && hudObject->m_field238 == 6.0f, "negative duration leaves prior medal timer");
            std::string hudText("previous");
            fallback.setString(0, "true");
            hudObject->getString(&fallback, hudText);
            check(hudText == "true", "real HUD getString reads bound Data through 256-byte buffer");
            hudObject->getString(&named, hudText);
            check(hudText.empty(), "real HUD getString copies default empty Data");
            LongTextData longText;
            hudObject->getString(&longText, hudText);
            check(hudText == std::string(255, 'x'), "real HUD extraction truncates to 255 characters");
            api->setBool("IngameHelpShow", true);
            hudObject->showWideHelp(L"help", 5.0f); // Real null-game branch.
            check(!api->getBool("IngameHelpShow") && hudObject->m_field22c == 0,
                  "real wide-help null-game path hides existing help");
            api->setBool("IngameHelpShow", true);
            hudObject->showHelp("", 3.0f);
            check(!api->getBool("IngameHelpShow"), "real narrow-help conversion and wide virtual dispatch");
            check(hudObject->getPlayerVehicleIcon(0, false) == 0, "real vehicle icon null-player path");
            delete hudObject->m_field2dc;
            using WideString = std::wstring;
            using String = std::string;
            using WordList = std::list<int>;
            using StringList = std::list<std::string>;
            // Derive the member type so fixture teardown follows the actual container declaration.
            using WordVector = decltype(hudObject->m_field32c);
            hudObject->m_field33c.~StringList();
            hudObject->m_field32c.~WordVector();
            hudObject->m_field304.~String();
            hudObject->m_field2e0.~StringList();
            hudObject->m_field0e8.~WordList();
            hudObject->m_field0c0.~String();
            hudObject->m_field08c.~String();
            hudObject->m_field060.~WideString();
            hudObject->m_field038.~WideString();
            hudObject->m_field010.~WideString();
            ::operator delete(hudStorage);
            hudManager = 0;
            adapter.handleEvent(ECHud, 123, 0, 0); // Observed no-op category; ECGame needs actual PlayerManager.
            // Isolate the real event posting from the layer's declaration-only
            // game-update handler. Production does not perform this fixture setup.
            events.EventManager::unregisterEventHandler(ECHud, &layer);
            HudEventCapture capture;
            events.EventManager::registerEventHandler(ECHud, &capture, 0);
            adapter.setMapStatic(true);
            check(api->getBool("MapStatic") && capture.count == 1 && capture.id == 10,
                  "map-static setter posts synchronous HUD event 10");
            events.EventManager::unregisterEventHandler(ECHud, &capture);
            events.EventManager::registerEventHandler(ECHud, &layer, 0);
        }
        check(events.removed == 2 && events.removal[0] == ECGame && events.removal[1] == ECHud, "adapter unsubscribes in retail order");
    }
    check(events.removed == 4, "real layer and adapter lifetimes complete");
    std::printf("HUD adapter/Data: %d checks, %d failures; native sizes %u/%u/%u\n", checks, failures,
        unsigned(sizeof(hud::HudInformationAdapter)), unsigned(sizeof(Data)), unsigned(sizeof(BoolRefData)));
    return failures ? 1 : 0;
}
