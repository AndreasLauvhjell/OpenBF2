// Path: [INFERRED] beside VehicleHudCompTemplate.h; the type name is ELF-attested.
#ifndef GAME_OBJECTS_VEHICLEHUDICONS_H
#define GAME_OBJECTS_VEHICLEHUDICONS_H

#include <string>
#include <vector>

namespace dice { namespace hfe { namespace world {

// ELF constructor 0x822CE50; BF2.exe constructor 0x65B020. The containing
// template initializes the three scalar handles after this constructor returns.
// Retail ELF size 0x48, VC7.1 size 0xA0. Native STL determines this build's size;
// never copy either retail layout over these containers.
struct VehicleHudIcons
{
    VehicleHudIcons();

    std::vector<std::string> vehicleIcons;    // ELF +00; PE +00
    std::vector<std::string> abilityIcons;    // ELF +0C; PE +10
    std::string miniMapIcon;                 // ELF +18; PE +20
    std::string typeIcon;                    // ELF +1C; PE +3C
    std::string spottedIcon;                 // ELF +20; PE +58
    // Element type is attested by vector<int> manglings in the ELF setters.
    // On the client these integer handles carry Ref2System texture addresses.
    std::vector<int> vehicleIconPtrs;         // ELF +24; PE +74
    std::vector<int> abilityIconPtrs;         // ELF +30; PE +84
    int miniMapIconPtr;                      // ELF +3C; PE +94
    int typeIconPtr;                         // ELF +40; PE +98
    int spottedIconPtr;                      // ELF +44; PE +9C
};

} } }

#endif
