// [TEMP] Native x86 validation of real late HUD state virtuals. No production
// method override, fake widget, raw vtable call or HUD destructor replacement.
// Opaque widget addresses are identity tokens only and are never dereferenced.
// /FORCE permits uncalled HUD lifecycle/game/render dependencies.
#include <cstdio>
#include <cstring>
#include <string>
#include <list>
#include <vector>
#include <cstdint>
#include <new>
#define private public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#undef private
using namespace dice::hfe::hud;
static int checks, failures;
static void check(bool value, const char* label) {
    ++checks;
    if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", label); }
}
static std::uint32_t bits(float value) { std::uint32_t result; std::memcpy(&result, &value, 4); return result; }
static float fromBits(std::uint32_t value) { float result; std::memcpy(&result, &value, 4); return result; }
// Keep a genuine virtual dispatch boundary in the optimized component fixture.
__declspec(noinline) static std::string readText(BF2HudManager* hud) { return hud->getSquadText(); }
__declspec(noinline) static unsigned enabledMask(BF2HudManager* hud) {
    return (hud->getSatelliteEnabled() ? 1u : 0u) |
           (hud->getUAVEnabled() ? 2u : 0u) |
           (hud->getArtilleryEnabled() ? 4u : 0u);
}
using FloatSetter = void (BF2HudManager::*)(float);
using FloatGetter = float (BF2HudManager::*)();
struct Availability { FloatSetter set; FloatGetter get; };
__declspec(noinline) static void writeAvailability(BF2HudManager* hud, FloatSetter set, float value) { (hud->*set)(value); }
__declspec(noinline) static float readAvailability(BF2HudManager* hud, FloatGetter get) { return (hud->*get)(); }
static void destroyConstructorOnly(BF2HudManager* hud) {
    // Full HUD destruction is not implemented. Dispose only real constructor
    // ownership, following the existing prefix fixture; no replacement body.
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
    void* allocation = ::operator new(sizeof(BF2HudManager));
    std::memset(allocation, 0x5a, sizeof(BF2HudManager));
    BF2HudManager* hud = new(allocation) BF2HudManager;
    check(!hud->getAmmo() && !hud->getVehicleInformation() && !hud->getMap() &&
          !hud->getSquad() && !hud->getCommander() && !hud->getMessages(), "constructor's null widget ownership is returned unchanged");
    alignas(void*) unsigned char identity[7][sizeof(void*)] = {};
    hud->m_field12c = reinterpret_cast<HudAmmo*>(identity[0]);
    hud->m_field130 = reinterpret_cast<HudVehicleInformation*>(identity[1]);
    hud->m_field0f8 = reinterpret_cast<HudMap*>(identity[2]);
    hud->m_field138 = reinterpret_cast<HudSquad*>(identity[3]);
    hud->m_field13c = reinterpret_cast<HudCommander*>(identity[4]);
    hud->m_field148 = reinterpret_cast<HudMessages*>(identity[5]);
    check(hud->getAmmo() == reinterpret_cast<HudAmmo*>(identity[0]), "ammo getter returns borrowed owner");
    check(hud->getVehicleInformation() == reinterpret_cast<HudVehicleInformation*>(identity[1]), "vehicle getter returns distinct owner");
    check(hud->getMap() == reinterpret_cast<HudMap*>(identity[2]), "map getter uses F8 owner");
    check(hud->getSquad() == reinterpret_cast<HudSquad*>(identity[3]), "squad getter uses138 owner");
    check(hud->getCommander() == reinterpret_cast<HudCommander*>(identity[4]), "commander getter uses13C owner");
    check(hud->getMessages() == reinterpret_cast<HudMessages*>(identity[5]), "messages getter uses148 owner");
    hud->m_field0f8 = reinterpret_cast<HudMap*>(identity[6]);
    check(hud->getMap() == reinterpret_cast<HudMap*>(identity[6]), "getter reloads externally replaced owner");
    hud->m_field0f8 = 0;
    check(hud->getMap() == 0, "cleared owner returns null without substitute");

    hud->setSquadCreateActive(true); hud->setSquadRenameActive(true);
    check(hud->isSquadCreateActive() && hud->isSquadRenameActive(), "squad create and rename may both be active");
    hud->setFlagE6(true); hud->setFlagE7(false);
    check(hud->m_field0e6 && !hud->m_field0e7 && hud->isSquadCreateActive() && hud->isSquadRenameActive(), "adjacent flag stores preserve both active modes");
    hud->setSquadCreateActive(false); hud->setFlagE7(true);
    check(!hud->isSquadCreateActive() && hud->isSquadRenameActive() && hud->m_field0e6 && hud->m_field0e7, "clear create leaves rename and other flags intact");
    hud->setSquadRenameActive(false); hud->setFlagE6(false);
    check(!hud->isSquadRenameActive() && !hud->m_field0e6 && hud->m_field0e7, "rename and E6 stores do not overlap E7");
    hud->m_field229 = true; hud->m_field22a = false;
    check(hud->getFlag229() && !hud->getFlag22A(), "independent observed229 and22A flags");
    hud->m_field229 = false; hud->m_field22a = true;
    check(!hud->getFlag229() && hud->getFlag22A(), "flag getters reread reversed state");

    const Availability properties[] = {
        {&BF2HudManager::setSatelliteAvailability, &BF2HudManager::getSatelliteAvailability},
        {&BF2HudManager::setArtilleryAvailability, &BF2HudManager::getArtilleryAvailability},
        {&BF2HudManager::setUAVAvailability, &BF2HudManager::getUAVAvailability},
        {&BF2HudManager::setVehicleDropAvailability, &BF2HudManager::getVehicleDropAvailability}
    };
    for (int i = 0; i < 4; ++i) writeAvailability(hud, properties[i].set, float(-i - 1));
    for (unsigned mask = 0; mask < 8; ++mask) {
        hud->setSatelliteEnabled((mask & 1u) != 0);
        hud->setUAVEnabled((mask & 2u) != 0);
        hud->setArtilleryEnabled((mask & 4u) != 0);
        check(enabledMask(hud) == mask, "all enable combinations preserve UAV/artillery byte identities");
        for (int i = 0; i < 4; ++i)
            check(readAvailability(hud, properties[i].get) == float(-i - 1), "enable mutation leaves availability values intact");
    }
    const std::uint32_t values[] = {0u, 0x80000000u, 0x3f800000u, 0xbf800000u,
        0x40200000u, 0x7f800000u, 0xff800000u, 0x7fc12345u, 1u};
    for (int target = 0; target < 4; ++target) {
        for (std::uint32_t value : values) {
            for (int i = 0; i < 4; ++i) writeAvailability(hud, properties[i].set, float(-i - 1));
            writeAvailability(hud, properties[target].set, fromBits(value));
            check(enabledMask(hud) == 7u, "availability writes do not alter enabled flags");
            for (int i = 0; i < 4; ++i) {
                const std::uint32_t expected = i == target ? value : bits(float(-i - 1));
                check(bits(readAvailability(hud, properties[i].get)) == expected,
                      "float virtual dispatch preserves IEEE value and neighboring fields without clamping");
            }
        }
    }

    check(readText(hud).empty(), "initial squad text is an owned empty copy");
    const std::string expected = std::string(160, 'x') + std::string("\0tail", 5);
    hud->m_field0c0 = expected;
    std::string first = readText(hud);
    check(first == expected && first.size() == 165, "squad getter copies long string including embedded null");
    first[0] = 'Y';
    check(hud->m_field0c0 == expected, "mutating returned copy cannot edit HUD storage");
    std::string survivor = readText(hud);
    hud->m_field0c0.assign(320, 'z');
    check(survivor == expected, "returned text survives original allocation replacement");
    hud->m_field0c0.clear(); hud->m_field0c0.shrink_to_fit();
    check(survivor == expected && readText(hud).empty(), "copy survives original clear and release");
    hud->m_field12c = 0; hud->m_field130 = 0; hud->m_field138 = 0;
    hud->m_field13c = 0; hud->m_field148 = 0;
    destroyConstructorOnly(hud);
    check(survivor == expected, "by-value result remains valid after HUD constructor ownership is destroyed");
    std::printf("HUD state: %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
