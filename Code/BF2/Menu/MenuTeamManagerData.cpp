// Client kit and weapon-description parsers. [INFERRED] Source split from MenuTeamManager.
#include "MenuTeamManager.h"

#if !DEDICATED
#include "../Game/Persistence/SimpleTokenizer.h"
#include "../Game/Utility/XMLDocument.h"
#include "../IO/File/FileManager.h"
#include <cstdlib>
#include <cstring>

namespace dice { namespace hfe {
namespace {

// [INFERRED] helper name. The shipped reader sizes its allocation and terminator
// from FileInfo, ignores bytesRead, and leaves both outputs untouched on failure.
bool readMenuFile(const std::string& file, char*& output, unsigned int& size)
{
    io::FileInfo info;
    if (!io::fileManager->getFileInfo(file, info))
        return false;
    char* buffer = new char[info.size + 1u];
    unsigned int bytesRead;
    if (!io::fileManager->readFile(file, buffer, info.size, bytesRead))
    {
        delete[] buffer;
        return false;
    }
    size = info.size;
    buffer[info.size] = 0;
    output = buffer;
    return true;
}

// Commands are case-sensitive; quotes are token delimiters, not an
// escaping/quoting grammar. Every matching itemIndex assignment is applied.
bool parseMenuWeapon(const std::string& file, MenuEquipment& equipment)
{
    char* buffer = 0;
    unsigned int size;
    if (!readMenuFile(file, buffer, size))
        return false;
    std::string text(buffer);
    delete[] buffer;
    SimpleTokenizer lines(text, "\n\r");
    std::string line;
    while (lines.next(line))
    {
        SimpleTokenizer tokens(line, " \t\"");
        std::string command;
        if (tokens.next(command) && command == "ObjectTemplate.itemIndex")
        {
            std::string value;
            if (tokens.next(value))
                equipment.itemIndex = std::atoi(value.c_str());
        }
    }
    return true;
}

// This deliberately interprets only the five observed commands. It does
// not run the object console, process includes, or parse comments specially.
bool parseMenuKit(const std::string& file, MenuKit& kit)
{
    char* buffer = 0;
    unsigned int size;
    if (!readMenuFile(file, buffer, size))
        return false;
    std::string text(buffer);
    delete[] buffer;
    SimpleTokenizer lines(text, "\n\r");
    std::string line;
    while (lines.next(line))
    {
        SimpleTokenizer tokens(line, " \t\"");
        std::string command;
        if (!tokens.next(command))
            continue;

        if (command == "ObjectTemplate.addTemplate")
        {
            std::string name;
            if (tokens.next(name) &&
                ::_stricmp(name.c_str(), "gasmask") != 0 &&
                ::_stricmp(name.c_str(), "nightvisiongoggles") != 0 &&
                name.find("_mask", 0, 5) == std::string::npos &&
                name.find("_nvg", 0, 4) == std::string::npos)
            {
                MenuEquipment equipment;
                equipment.itemIndex = 0;
                equipment.unlockLevel = 0;
                equipment.tweakFile = name + ".tweak";
                equipment.templateName = name;
                std::string descriptionFile("objects/weapons/handheld/");
                descriptionFile += equipment.templateName;
                descriptionFile += "/";
                descriptionFile += "Weapon.desc";
                equipment.displayName = equipment.templateName;
                XMLDocument document;
                if (document.parse(descriptionFile))
                {
                    equipment.equipmentType = std::atoi(document.getAttribute("eq").c_str());
                    for (unsigned int i = 0; i < document.nodes.size(); ++i)
                    {
                        XMLNode* node = document.nodes[i];
                        if (::_stricmp(node->name.c_str(), "name") == 0)
                        {
                            std::string value = node->getAttribute("locid");
                            if (value == "")
                                value = node->value;
                            equipment.displayName = value;
                        }
                        else if (::_stricmp(node->name.c_str(), "description") == 0)
                        {
                            std::string value = node->getAttribute("locid");
                            if (value == "")
                                value = node->value;
                            equipment.description = value;
                        }
                    }
                }
                kit.equipment.push_back(equipment);
            }
        }
        else if (command == "ObjectTemplate.create")
        {
            std::string type;
            if (tokens.next(type))
            {
                if (::_stricmp(type.c_str(), "kit") == 0)
                {
                    std::string name;
                    tokens.next(name); // Missing names still assign the empty token.
                    kit.templateName = name;
                }
                else if (::_stricmp(type.c_str(), "ItemContainer") == 0)
                {
                    std::string name;
                    tokens.next(name);
                    for (unsigned int i = 0; i < kit.equipment.size(); ++i)
                    {
                        if (::_stricmp(name.c_str(), kit.equipment[i].templateName.c_str()) == 0)
                        {
                            kit.equipment.erase(kit.equipment.begin() + i);
                            break; // Only the first matching equipment entry is removed.
                        }
                    }
                }
            }
        }
        else if (command == "ObjectTemplate.unlockLevel")
        {
            std::string value;
            if (tokens.next(value) && !kit.equipment.empty())
                kit.equipment.back().unlockLevel = std::atoi(value.c_str());
        }
        else if (command == "ObjectTemplate.sprintStaminaDissipationFactor")
        {
            std::string value;
            if (tokens.next(value))
                kit.sprintStaminaDissipationFactor = static_cast<float>(std::atof(value.c_str()));
        }
        else if (command == "ObjectTemplate.vehicleHud.hudName")
        {
            std::string value;
            if (tokens.next(value))
                kit.hudName = value;
            else
                kit.hudName = kit.templateName;
        }
    }
    return true;
}

} // namespace

// A readable kit is retained even when individual weapon scripts fail
// to load; only successful weapon reads publish an entry in the equipment catalogue.
bool MenuTeamManager::addKit(const std::string& file)
{
    if (m_teams.empty())
        return false;
    MenuKit kit;
    std::string kitFile(m_teams.back().path);
    kitFile += "/";
    kitFile += file;
    kit.file = file;
    kit.sprintStaminaDissipationFactor = 0.0f;
    if (!parseMenuKit(kitFile, kit))
        return false;
    for (unsigned int i = 0; i < kit.equipment.size(); ++i)
    {
        MenuEquipment& equipment = kit.equipment[i];
        std::string weaponFile("objects/weapons/handheld/");
        weaponFile += equipment.templateName;
        weaponFile += "/";
        weaponFile += equipment.tweakFile;
        if (parseMenuWeapon(weaponFile, equipment))
            addEquipment(kit.equipment[i]); // Virtual, as at PE 0x4AFAF6.
    }
    kit.index = static_cast<unsigned int>(m_teams.back().kits.size());
    m_teams.back().kits.push_back(kit);
    return true;
}

} }
#endif // !dedicated
