// [TEMP] Friend access preserves MSVC's private member mangling in the fixture.
#include "../BF2/Menu/HudCommander.h"
namespace dice { namespace hfe { namespace hud {
struct HudCommanderFixture {
    static void artillery(HudCommander& owner) { owner.refreshArtillery(); }
    static void supplies(HudCommander& owner) { owner.refreshSupplies(); }
    static void vehicleDrop(HudCommander& owner) { owner.refreshVehicleDrop(); }
    static void satellite(HudCommander& owner) { owner.refreshSatellite(); }
    static void uav(HudCommander& owner) { owner.refreshUAV(); }
};
} } }
void refreshTestArtillery(dice::hfe::hud::HudCommander& owner) { dice::hfe::hud::HudCommanderFixture::artillery(owner); }
void refreshTestSupplies(dice::hfe::hud::HudCommander& owner) { dice::hfe::hud::HudCommanderFixture::supplies(owner); }
void refreshTestVehicleDrop(dice::hfe::hud::HudCommander& owner) { dice::hfe::hud::HudCommanderFixture::vehicleDrop(owner); }
void refreshTestSatellite(dice::hfe::hud::HudCommander& owner) { dice::hfe::hud::HudCommanderFixture::satellite(owner); }
void refreshTestUAV(dice::hfe::hud::HudCommander& owner) { dice::hfe::hud::HudCommanderFixture::uav(owner); }
