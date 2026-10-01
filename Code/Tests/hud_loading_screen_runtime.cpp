// [TEMP] Real nonvirtual loading record and HudInformationLayer/EventManager.
// Callback observers delegate to actual storage; no production method is shadowed.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <functional>
#include <limits>
#include <new>
#include <string>
#include <vector>
#define private public
#include "../BF2/Menu/HudLoadingScreen.h"
#undef private
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/Debug.h"

namespace dice { namespace hfe {
EventManager* eventManager = nullptr;
class ServerSettings;
ServerSettings* serverSettings = nullptr;
IBase* g_hudInformationLayer = nullptr;
} }
using namespace dice::hfe;
using hud::HudLoadingScreen;
static unsigned checks, failures;
static void check(bool value, const char* label)
{
    ++checks;
    if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", label); }
}
static std::uint32_t bits(float value)
{
    std::uint32_t result; std::memcpy(&result, &value, sizeof(result)); return result;
}
static bool filled(const void* buffer, unsigned size, unsigned char value)
{
    const unsigned char* p = static_cast<const unsigned char*>(buffer);
    for (unsigned i = 0; i != size; ++i) if (p[i] != value) return false;
    return true;
}
struct Registration { std::string name; char type; const void* address; int id; };
struct ObservedLayer : HudInformationLayer
{
    std::vector<Registration> calls;
    std::function<void(const std::string&)> callback;
    bool overrideResult = false;
    int registerBool(const std::string& name, bool* value) override
    {
        int id = HudInformationLayer::registerBool(name, value);
        calls.push_back({name, 'b', value, id});
        if (callback) callback(name);
        return overrideResult ? -777 : id;
    }
    int registerFloat(const std::string& name, float* value) override
    {
        int id = HudInformationLayer::registerFloat(name, value);
        calls.push_back({name, 'f', value, id});
        if (callback) callback(name);
        return overrideResult ? -777 : id;
    }
    int registerString(const std::string& name, const char** value) override
    {
        int id = HudInformationLayer::registerString(name, value);
        calls.push_back({name, 's', value, id});
        if (callback) callback(name);
        return overrideResult ? -777 : id;
    }
};

int main()
{
    static_assert(sizeof(void*) == 4, "native x86 only");
    static_assert(sizeof(HudLoadingScreen) == 28, "nonvirtual retail record size");
    static_assert(offsetof(HudLoadingScreen, m_system) == 0, "first word is System pointer");
    static_assert(offsetof(HudLoadingScreen, m_show) == 4, "bool offset");
    static_assert(offsetof(HudLoadingScreen, m_progress) == 8, "float offset");
    static_assert(offsetof(HudLoadingScreen, m_picture) == 12, "untouched picture offset");
    static_assert(offsetof(HudLoadingScreen, m_buildNumber) == 24, "last pointer offset");
    EventManager events;
    eventManager = &events;
    {
        ObservedLayer layer, laterLayer;
        g_hudInformationLayer = &layer;
        void* memory = ::operator new(sizeof(HudLoadingScreen));
        std::memset(memory, 0xa5, sizeof(HudLoadingScreen));
        HudLoadingScreen* loading = new(memory) HudLoadingScreen(nullptr);
        check(loading->m_system == nullptr, "constructor retains borrowed null System");
        check(!loading->m_show && bits(loading->m_progress) == 0, "constructor bool and positive-zero defaults");
        check(filled(static_cast<unsigned char*>(memory) + 5, 3, 0xa5), "constructor leaves alignment padding untouched");
        check(filled(&loading->m_picture, 4, 0xa5), "constructor leaves entire picture pointer untouched");
        check(loading->m_mapName == nullptr && loading->m_debug == nullptr, "constructor clears map and debug pointers");
        check(std::strcmp(loading->m_buildNumber, "BF2 Build #3153") == 0, "fixed retail build string");
        char* sharedBuild = const_cast<char*>(loading->m_buildNumber);
        sharedBuild[0] = '!';
        HudLoadingScreen other(nullptr);
        check(other.m_buildNumber == sharedBuild && loading->m_buildNumber == sharedBuild,
              "instances share one build buffer");
        check(std::strcmp(loading->m_buildNumber, "BF2 Build #3153") == 0,
              "later construction reformats the existing shared build buffer");

        const char* before = "fixture picture before registration";
        loading->m_picture = before;
        unsigned observedBefore = 0;
        layer.callback = [&](const std::string&) { if (loading->m_picture == before) ++observedBefore; };
        loading->registerData();
        const char* names[] = {"ShowLoadingScreen", "LoadingBarValue", "LoadingPicturePathString",
                              "LoadingScreenMapName", "LoadingScreenDebug", "LoadingScreenBuildNr"};
        const char kinds[] = {'b','f','s','s','s','s'};
        const void* addresses[] = {&loading->m_show, &loading->m_progress, &loading->m_picture,
                                    &loading->m_mapName, &loading->m_debug, &loading->m_buildNumber};
        check(layer.calls.size() == 6 && observedBefore == 6, "all six callbacks precede default-picture store");
        for (unsigned i = 0; i != 6; ++i) {
            check(layer.calls[i].name == names[i] && layer.calls[i].type == kinds[i], "exact registration name/type/order");
            check(layer.calls[i].address == addresses[i] && layer.calls[i].id == 425 + int(i),
                  "real shared-ID storage points into loading record");
        }
        check(std::strcmp(loading->m_picture, "Load/default_loadscreen.tga") == 0, "default picture assigned after registration");
        check(layer.getBoolByName(names[0]) == &loading->m_show, "bool lookup aliases record");
        check(layer.getFloatByName(names[1]) == &loading->m_progress, "float lookup aliases record");
        for (unsigned i = 2; i != 6; ++i)
            check(layer.getStringByName(names[i]) == addresses[i], "string lookup aliases pointer field");
        *layer.getBoolByName(names[0]) = true;
        check(loading->m_show, "registered bool mutation reaches record");
        *layer.getStringByName(names[3]) = "dalian_plant";
        check(std::strcmp(loading->m_mapName, "dalian_plant") == 0, "registered map pointer mutation reaches record");

        const float percentages[] = {0.0f, 1.0f, 25.0f, 50.0f, 100.0f, -50.0f, 150.0f};
        const std::uint32_t expected[] = {0x3f800000, 0x3f7d70a4, 0x3f400000, 0x3f000000,
                                           0x32c00000, 0x3fc00000, 0xbeffffff};
        for (unsigned i = 0; i != 7; ++i) {
            loading->setProgress(percentages[i]);
            check(bits(*layer.getFloatByName(names[1])) == expected[i], "progress x87 single-store golden bits");
        }
        loading->setProgress(std::numeric_limits<float>::infinity());
        check(std::isinf(loading->m_progress) && loading->m_progress < 0, "positive infinite percentage becomes negative infinity");
        loading->setProgress(-std::numeric_limits<float>::infinity());
        check(std::isinf(loading->m_progress) && loading->m_progress > 0, "negative infinite percentage becomes positive infinity");
        loading->setProgress(std::numeric_limits<float>::quiet_NaN());
        check(std::isnan(loading->m_progress), "unordered progress remains NaN");

        loading->setDebug("abc");
        const char* debug = loading->m_debug;
        check(std::strcmp(debug, "abc") == 0 && filled(debug + 3, 509, 0), "short debug copy pads entire512-byte buffer");
        const std::string longText(600, 'X');
        other.setDebug(longText.c_str());
        check(other.m_debug == debug && filled(debug, 512, 'X'), "debug buffer shared with no truncation terminator");
        const std::string edgeText(511, 'Y');
        loading->setDebug(edgeText.c_str());
        check(filled(debug, 511, 'Y') && debug[511] == 0, "511-byte debug string retains final terminator");
        other.setDebug("");
        check(filled(debug, 512, 0) && *layer.getStringByName(names[4]) == debug, "empty debug pads buffer seen by existing registration");

        // Returns are ignored and the global is reloaded between each callback.
        turnOffAllDebug(6); // Real duplicate-name continuation behavior.
        layer.calls.clear(); layer.overrideResult = true; laterLayer.overrideResult = true;
        layer.callback = [&](const std::string&) { g_hudInformationLayer = &laterLayer; };
        other.m_picture = before;
        unsigned laterBefore = 0;
        laterLayer.callback = [&](const std::string&) { if (other.m_picture == before) ++laterBefore; };
        other.registerData();
        check(layer.calls.size() == 1 && laterLayer.calls.size() == 5 && laterBefore == 5,
              "registration reloads changed singleton and ignores negative return IDs");
        check(layer.getBoolByName(names[0]) == &loading->m_show,
              "duplicate registration preserves first name mapping in real storage");
        check(laterLayer.getFloatByName(names[1]) == &other.m_progress,
              "remaining callbacks register into replacement singleton");
        check(std::strcmp(other.m_picture, "Load/default_loadscreen.tga") == 0,
              "negative registration IDs do not suppress final picture assignment");

        unsigned char saved[sizeof(HudLoadingScreen)];
        std::memcpy(saved, loading, sizeof(saved));
        const void* oldBool = &loading->m_show;
        loading->~HudLoadingScreen();
        check(std::memcmp(saved, memory, sizeof(saved)) == 0, "empty destructor does not clear fields");
        check(layer.getBoolByName(names[0]) == oldBool, "destructor does not unregister borrowed storage");
        ::operator delete(memory);
        g_hudInformationLayer = nullptr;
    } // Actual HudInformationLayer destructors unregister from the actual manager.
    eventManager = nullptr;
    std::printf("hud_loading_screen_runtime: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
