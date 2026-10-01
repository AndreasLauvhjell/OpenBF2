// [INFERRED] Source placement.

#include "WeaponTemplateManager.h"

#include "WeaponTemplate.h"

#include "../../../IO/Console/Console.h"     // io::mainConsole->getWorkingFile - create

namespace dice {
namespace hfe {

namespace ai {

WeaponTemplateManager* WeaponTemplateManager::m_instance = 0;

WeaponTemplateManager::WeaponTemplateManager(int kind)
{
    m_kind = kind;
    // last wins, and it is the only way anything reaches this object: the class registers
    // nothing with ClassManager.
    m_instance = this;
    m_active = 0;
}

// each: reset directly, the instance cleared unconditionally
// then the two maps' destructors.
WeaponTemplateManager::~WeaponTemplateManager()
{
    reset();
    m_instance = 0;
}

// Deletes templates and wrappers, erases map nodes, and clears working file names.
int WeaponTemplateManager::reset()
{
    m_active = 0;
    std::map<std::string, MgrTemplate*, NoCaseStringCompare>::iterator it = m_byName.begin();
    while (it != m_byName.end())
    {
        if (it->second)
        {
            delete it->second->m_template;
            delete it->second;
        }
        m_byName.erase(it++);
    }
    m_workingFiles.clear();
    return 1;
}

// Creates a template. A duplicate clears the active selection and returns an error
// without a fatal diagnostic; allocations do not use checkPointer.
std::string WeaponTemplateManager::create(std::string name)
{
    if (m_byName.find(name) != m_byName.end())
    {
        m_active = 0;
        return "Error: Template '" + name + "' already exists";
    }

    m_active = new MgrTemplate;
    m_active->m_name = name;
    m_active->m_template = new WeaponTemplate(m_kind);
    m_byName[name] = m_active;

    // getWorkingFile is asked once; the reference is tested and then copied.
    const std::string& workingFile = io::mainConsole->getWorkingFile();
    if (!workingFile.empty())
        m_workingFiles[m_active->m_template] = workingFile;

    return "WeaponTemplate '" + name + "' successfully created";
}

// the named template becomes active, or none - a miss clears it.
void WeaponTemplateManager::setActive(std::string name)
{
    std::map<std::string, MgrTemplate*, NoCaseStringCompare>::iterator it = m_byName.find(name);
    if (it == m_byName.end())
        m_active = 0;
    else
        m_active = it->second;
}

// the wrapper's name, or "" (constructed from a literal) with none active.
std::string WeaponTemplateManager::getActive() const
{
    if (m_active)
        return m_active->m_name;
    return "";
}

WeaponTemplate* WeaponTemplateManager::getActiveTemplate() const
{
    if (m_active)
        return m_active->m_template;
    return 0;
}

// Looks up a weapon template by case-insensitive name; returns null when absent.
WeaponTemplate* WeaponTemplateManager::getTemplate(const std::string& name)
{
    std::map<std::string, MgrTemplate*, NoCaseStringCompare>::iterator it = m_byName.find(name);
    if (it != m_byName.end())
        return it->second->m_template;

    return 0;
}
} // namespace ai
} // namespace hfe
} // namespace dice
