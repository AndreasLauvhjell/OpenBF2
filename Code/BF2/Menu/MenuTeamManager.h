// [INFERRED] path and non-console names. Client PE constructor 0x4B0310,
// factory 0x41053B, primary vtable 0x8A33B0.
#ifndef MENU_MENUTEAMMANAGER_H
#define MENU_MENUTEAMMANAGER_H
#include "IMenuTeamManager.h"
#if !DEDICATED
namespace dice { namespace hfe {

class MenuTeamManager : public IMenuTeamManager
{
public:
    MenuTeamManager();
    static IBase* create(unsigned int iid, IBase* outer);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int iid) const override;
    bool init() override;
    bool addTeam(const std::string& name, const std::string& path) override;
    bool setTeamId(int id) override;
    bool addKit(const std::string& file) override;
    void addWeapon(const std::string& name, int stat70, int stat74, int stat78) override;
    void addEquipment(const MenuEquipment& equipment) override;
    MenuEquipment* findEquipment(const std::string& name) override;
    int getTeamCount() override;
    MenuTeam* getTeam(unsigned int index) override;
    MenuTeam* findTeamById(int id) override;
    MenuTeam* findTeamByName(const std::string& name) override;
    void visitEquipment() override;
    bool getWeaponKitMap(std::vector<MenuEquipmentWithKits>& out) override;
    virtual ~MenuTeamManager(); // PE +0x44, deleting destructor 0x4B08A0

private:
    int m_refCount;
    bool m_initialized;
    std::vector<MenuTeam> m_teams;
    std::vector<MenuEquipment> m_equipment;
};

} }
#endif
#endif
