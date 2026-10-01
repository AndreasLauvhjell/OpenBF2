// Client kit and weapon records. [INFERRED] Record names and source placement.
// Documented offsets refer to retail VC7.1 records; native STL objects must not cross retail DLL boundaries.
#ifndef MENU_MENUTEAMMANAGERDATA_H
#define MENU_MENUTEAMMANAGERDATA_H

#include "../Core/BuildConfig.h"
#if !DEDICATED
#include <string>
#include <vector>

namespace dice { namespace hfe {

struct MenuEquipment
{
    // Retail constructor 0x4AC0D0 constructs only the four strings. Its scalar
    // members are deliberately uninitialized until their individual writers run.
    std::string tweakFile;       // +0x00: templateName + ".tweak"
    std::string templateName;    // +0x1C: ObjectTemplate.addTemplate argument
    std::string displayName;     // +0x38: Weapon.desc name/locid, else templateName
    std::string description;     // +0x54: Weapon.desc description/locid
    int stat70;                 // +0x70: [UNRESOLVED] semantic name; addWeapon arg 2
    int stat74;                 // +0x74: [UNRESOLVED] semantic name; addWeapon arg 3
    int stat78;                 // +0x78: [UNRESOLVED] semantic name; addWeapon arg 4
    int unlockLevel;            // +0x7C: ObjectTemplate.unlockLevel
    int itemIndex;              // +0x80: ObjectTemplate.itemIndex
    int equipmentType;          // +0x84: Weapon.desc root attribute "eq"
};                             // retail sizeof 136

struct MenuKit
{
    unsigned int index;         // +0x00: index in the owning team's kit vector
    std::string file;           // +0x04: addKit argument
    std::string templateName;   // +0x20: ObjectTemplate.create kit argument
    std::string hudName;        // +0x3C: ObjectTemplate.vehicleHud.hudName
    float sprintStaminaDissipationFactor; // +0x58: same-named ObjectTemplate property
    std::vector<MenuEquipment> equipment; // +0x5C
};                             // retail sizeof 108

struct MenuTeam
{
    int teamId;                 // +0x00: setTeamId argument
    unsigned int index;         // +0x04: index in the manager's team vector
    std::string name;           // +0x08: addTeam argument 1
    std::string path;           // +0x24: addTeam argument 2
    std::vector<MenuKit> kits;  // +0x40
};                             // retail sizeof 80

// [INFERRED] native name/inheritance spelling. The retail aggregate at 0x4AF270
// copies the entire equipment record followed by a vector of matching kit indices.
struct MenuEquipmentWithKits : MenuEquipment
{
    std::vector<unsigned int> kitIndices; // retail +0x88
};

} }
#endif // !dedicated
#endif // MENU_MENUTEAMMANAGERDATA_H
