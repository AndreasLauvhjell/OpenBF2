// [TEMP] Focused x86 client fixture for the recovered HudComp console invokers.
// It supplies only the active-component lookup seam and does not model an object template manager.
#include "../BF2/Core/ConsoleObject.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Game/Objects/AbilityHudCompTemplate.h"
#include "../BF2/Game/Objects/HelpHudCompTemplate.h"
#include "../BF2/Game/Objects/HudComp/ModuleHudComp.h"
#include "../BF2/Game/Objects/PostProcessCompTemplate.h"
#include "../BF2/Game/Objects/VehicleHudCompTemplate.h"
#include "../BF2/Game/Objects/WarningHudCompTemplate.h"
#include "../BF2/Game/Objects/WeaponHudCompTemplate.h"

#include <cstdio>
#include <cstring>
#include <string>

// [TEMP]
// Tested transitions take no sound branch; the sound facade stays null.
namespace dice { namespace hfe { class IBfSound; IBfSound* g_bfSound = 0; } }
namespace dice { namespace hfe {
namespace hud { class BF2HudManager; }
hud::BF2HudManager* hudManager = 0;

namespace world {
extern const unsigned int IID_IObjectTemplateComponent = 50379;
extern const unsigned int IID_IObjectComponent = 50043;

namespace {
IObjectTemplateComponent* g_activeComponent = 0;
unsigned int g_activeInterface = 0;
}

IObjectTemplateComponent* getActiveTemplateComponent(unsigned int iid)
{
    return iid == g_activeInterface ? g_activeComponent : 0;
}

IObject* getRootParent(const IObject* object)
{
    return const_cast<IObject*>(object);
}
} // namespace world
} } // namespace dice::hfe

int main()
{
    using namespace dice::hfe;
    using namespace dice::hfe::world;

    PostProcessCompTemplate postProcess;
    VehicleHudCompTemplate vehicle;
    WeaponHudCompTemplate weapon;
    AbilityHudCompTemplate ability;
    WarningHudCompTemplate warning;
    HelpHudCompTemplate help;

    int checks = 0;
    int failures = 0;
#define CHECK(condition) do { ++checks; if (!(condition)) { ++failures; std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); } } while (0)

    std::list<ConsoleObject*>& bindings = hudCompConsoleObjects();
    CHECK(bindings.size() == 98);

    std::size_t index = 0;
    for (std::list<ConsoleObject*>::iterator it = bindings.begin(); it != bindings.end() && index < 92;
         ++it, ++index)
    {
        ConsoleObject* binding = *it;
        const char* name = binding->getFunctionName();
        if (std::strncmp(name, "postProcess.", 12) == 0)
        {
            g_activeInterface = IID_IPostProcessCompTemplate;
            g_activeComponent = static_cast<IObjectTemplateComponent*>(&postProcess);
        }
        else if (std::strncmp(name, "vehicleHud.", 11) == 0)
        {
            g_activeInterface = IID_IVehicleHudCompTemplate;
            g_activeComponent = static_cast<IObjectTemplateComponent*>(&vehicle);
        }
        else if (std::strncmp(name, "weaponHud.", 10) == 0)
        {
            g_activeInterface = IID_IWeaponHudCompTemplate;
            g_activeComponent = static_cast<IObjectTemplateComponent*>(&weapon);
        }
        else if (std::strncmp(name, "abilityHud.", 11) == 0)
        {
            g_activeInterface = IID_IAbilityHudCompTemplate;
            g_activeComponent = static_cast<IObjectTemplateComponent*>(&ability);
        }
        else if (std::strncmp(name, "warningHud.", 11) == 0)
        {
            g_activeInterface = IID_IWarningHudCompTemplate;
            g_activeComponent = static_cast<IObjectTemplateComponent*>(&warning);
        }
        else
        {
            CHECK(std::strncmp(name, "helpHud.", 8) == 0);
            g_activeInterface = IID_IHelpHudCompTemplate;
            g_activeComponent = static_cast<IObjectTemplateComponent*>(&help);
        }

        CHECK(binding->isActive());
        CHECK(binding->getDeltaAsString() == "No Delta");

        std::string argument;
        if (std::strcmp(name, "vehicleHud.addVehicleIcon") == 0)
            argument = "vehicle_added";
        else if (std::strcmp(name, "vehicleHud.addAbilityIcon") == 0)
            argument = "ability_added";
        else if (std::strcmp(name, "vehicleHud.addShowOnCamMode") == 0)
            argument = "8";
        else if (std::strcmp(name, "weaponHud.addShowOnCamMode") == 0)
            argument = "9";
        else if (std::strcmp(binding->getArgType(0), "std::string") == 0)
            argument = "hud_binding_value";
        else if (std::strcmp(binding->getArgType(0), "float") == 0)
            argument = "2.5";
        else if (std::strcmp(binding->getArgType(0), "bool") == 0)
            argument = "1";
        else if (std::strcmp(binding->getArgType(0), "Vec2") == 0)
            argument = "2.5/3.5";
        else
            argument = "7";

        const std::string setResult = binding->execute(&argument, 1);
        if (binding->getType() == 1)
        {
            CHECK(setResult.empty());
            CHECK(!binding->hasReturnValue());
            const std::string getResult = binding->execute(0, 0);
            CHECK(binding->hasReturnValue());
            CHECK(!getResult.empty());
        }
        else if (std::strstr(name, "getShowOnCamMode") != 0)
        {
            CHECK(binding->hasReturnValue());
            CHECK(setResult == "1");
        }
        else
        {
            CHECK(setResult.empty());
            CHECK(!binding->hasReturnValue());
        }
    }

    CHECK(index == 92);
    CHECK(vehicle.getHudName() == "hud_binding_value");
    CHECK(vehicle.getHudSid() == "hud_binding_value");
    CHECK(*static_cast<std::string*>(vehicle.getVehicleIcon(0)) == "hud_binding_value");
    CHECK(*static_cast<std::string*>(vehicle.getAbilityIcon(0)) == "hud_binding_value");
    CHECK(vehicle.getVehicleIconNumber() == 2);
    CHECK(*static_cast<std::string*>(vehicle.getVehicleIcon(1)) == "vehicle_added");
    CHECK(vehicle.getAbilityIconNumber() == 2);
    CHECK(*static_cast<std::string*>(vehicle.getAbilityIcon(1)) == "ability_added");
    CHECK(vehicle.getEnablePostProcessingOnGuiIndex() == 1); // bool console row -> int owner
    CHECK(vehicle.getShowOnCamMode(7));
    CHECK(vehicle.getShowOnCamMode(8));
    CHECK(weapon.getShowOnCamMode(7));
    CHECK(weapon.getShowOnCamMode(9));

    g_activeComponent = 0;
    g_activeInterface = 0;
    std::list<ConsoleObject*>::iterator first = bindings.begin();
    CHECK(!(*first)->isActive());

    std::printf("hud_bindings_runtime: %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
