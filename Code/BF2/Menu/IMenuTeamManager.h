// Client menu-team manager interface. [INFERRED] Names and header placement.
// Use native record layouts and preserve virtual declaration order.
#ifndef MENU_IMENUTEAMMANAGER_H
#define MENU_IMENUTEAMMANAGER_H
#include "../Core/IBase.h"
#include "MenuTeamManagerData.h"
#if !DEDICATED
namespace dice { namespace hfe {

class IMenuTeamManager : public IBase
{
public:
    virtual bool init() = 0;                                               // +0x10
    virtual bool addTeam(const std::string& name, const std::string& path) = 0; // +0x14
    virtual bool setTeamId(int id) = 0;                                     // +0x18
    virtual bool addKit(const std::string& file) = 0;                        // +0x1C
    virtual void addWeapon(const std::string& name, int, int, int) = 0;      // +0x20
    virtual void addEquipment(const MenuEquipment& equipment) = 0;          // +0x24
    virtual MenuEquipment* findEquipment(const std::string& name) = 0;       // +0x28
    virtual int getTeamCount() = 0;                                        // +0x2C
    virtual MenuTeam* getTeam(unsigned int index) = 0;                      // +0x30
    virtual MenuTeam* findTeamById(int id) = 0;                             // +0x34
    virtual MenuTeam* findTeamByName(const std::string& name) = 0;           // +0x38
    virtual void visitEquipment() = 0;                                     // +0x3C
    virtual bool getWeaponKitMap(std::vector<MenuEquipmentWithKits>& out) = 0; // +0x40
};

} }
#endif
#endif
