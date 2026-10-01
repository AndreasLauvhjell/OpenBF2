// Client team and kit-menu management. [INFERRED] Source placement.
#include "MenuTeamManager.h"
#if !DEDICATED
#include "../Core/Ids.h"
#include "../IO/Console/Console.h"
#include <cstring>
#include <map>
#include <new>

namespace dice { namespace hfe {
namespace {
void runMenuTeamScript(const char* file)
{
    io::mainConsole->run(std::string(file), std::string(""), std::string(""), std::string(""),
        std::string(""), std::string(""), std::string(""), std::string(""), std::string(""),
        io::Console::ignoredString_);
}
}

// Native STL owns its own layout; retail sizeof is 44.
MenuTeamManager::MenuTeamManager() : m_refCount(1), m_initialized(false) {}
MenuTeamManager::~MenuTeamManager() {}

// Retail rejects aggregation and queries even a null allocation result.
IBase* MenuTeamManager::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    void* allocation = ::operator new(sizeof(MenuTeamManager));
    MenuTeamManager* owner = allocation ? new (allocation) MenuTeamManager : 0;
    IBase* result = static_cast<IBase*>(owner->queryInterface(iid));
    if (!result)
        owner->release();
    return result;
}

void MenuTeamManager::addRef() { ++m_refCount; }
int MenuTeamManager::getRef() { return m_refCount; }
int MenuTeamManager::release()
{
    const int count = --m_refCount;
    if (count == 0)
        delete this;
    return count;
}
void* MenuTeamManager::queryInterface(unsigned int iid) const
{
    if (iid == 1 || iid == IID_IMenuTeamManager)
        return const_cast<MenuTeamManager*>(this);
    return 0;
}

bool MenuTeamManager::init()
{
    if (m_initialized)
        return true;
    std::vector<MenuTeam>().swap(m_teams);
    runMenuTeamScript("menu/external/flashmenu/menuTeams.con");
    runMenuTeamScript("menu/external/flashmenu/weaponStats.con");
    runMenuTeamScript("menu/external/flashmenu/gameModes.con");
    m_initialized = true;
    return true;
}

bool MenuTeamManager::addTeam(const std::string& name, const std::string& path)
{
    MenuTeam team;
    team.name = name;
    team.path = path;
    team.teamId = static_cast<int>(m_teams.size());
    team.index = static_cast<unsigned int>(m_teams.size());
    m_teams.push_back(team);
    return true;
}

bool MenuTeamManager::setTeamId(int id)
{
    if (id > 10)
        id = 9;
    m_teams.back().teamId = id;
    return true;
}

void MenuTeamManager::addWeapon(const std::string& name, int stat70, int stat74, int stat78)
{
    MenuEquipment* equipment = findEquipment(name);
    if (equipment)
    {
        equipment->stat70 = stat70;
        equipment->stat74 = stat74;
        equipment->stat78 = stat78;
    }
}

void MenuTeamManager::addEquipment(const MenuEquipment& equipment)
{
    for (std::size_t i = 0; i < m_equipment.size(); ++i)
        if (::_stricmp(m_equipment[i].tweakFile.c_str(), equipment.tweakFile.c_str()) == 0)
            return;
    m_equipment.push_back(equipment);
}

MenuEquipment* MenuTeamManager::findEquipment(const std::string& name)
{
    for (std::size_t i = 0; i < m_equipment.size(); ++i)
        if (::_stricmp(m_equipment[i].templateName.c_str(), name.c_str()) == 0)
            return &m_equipment[i];
    return 0;
}
int MenuTeamManager::getTeamCount() { return static_cast<int>(m_teams.size()); }
MenuTeam* MenuTeamManager::getTeam(unsigned int index) { return &m_teams[index]; }
MenuTeam* MenuTeamManager::findTeamById(int id)
{
    for (std::size_t i = 0; i < m_teams.size(); ++i)
        if (m_teams[i].teamId == id)
            return &m_teams[i];
    return 0;
}
MenuTeam* MenuTeamManager::findTeamByName(const std::string& name)
{
    for (std::size_t i = 0; i < m_teams.size(); ++i)
        if (::_stricmp(m_teams[i].name.c_str(), name.c_str()) == 0)
            return &m_teams[i];
    return 0;
}

void MenuTeamManager::visitEquipment() // 0x4ACA20; name [INFERRED]
{
    // The shipped nested traversal has no innermost side effect.
    for (std::size_t team = 0; team < m_teams.size(); ++team)
        for (std::size_t kit = 0; kit < m_teams[team].kits.size(); ++kit)
            for (std::size_t item = 0; item < m_teams[team].kits[kit].equipment.size(); ++item)
            {}
}

bool MenuTeamManager::getWeaponKitMap(std::vector<MenuEquipmentWithKits>& out)
{
    std::map<std::string, std::vector<unsigned int> > indices;
    // Retail assumes the first three teams exist, excludes later teams, and
    // preserves repeated kit indices for equipment shared across those teams.
    for (unsigned int team = 0; team < 3; ++team)
    {
        MenuTeam* owner = getTeam(team);
        for (unsigned int kit = 0; kit < owner->kits.size(); ++kit)
            for (unsigned int item = 0; item < owner->kits[kit].equipment.size(); ++item)
                indices[owner->kits[kit].equipment[item].templateName].push_back(kit);
    }
    for (auto it = indices.begin(); it != indices.end(); ++it)
    {
        const std::pair<std::string, std::vector<unsigned int> > entry(*it);
        MenuEquipment* equipment = findEquipment(entry.first);
        if (equipment)
        {
            MenuEquipmentWithKits record;
            static_cast<MenuEquipment&>(record) = *equipment;
            record.kitIndices = entry.second;
            out.push_back(record);
        }
    }
    return true;
}

} }
#endif
