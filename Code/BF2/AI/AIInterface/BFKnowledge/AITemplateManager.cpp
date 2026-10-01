// [INFERRED] Source placement.

#include "AITemplateManager.h"

#include "AITemplate.h"
#include "AITemplatePlugInManager.h"

#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../../Core/Debug.h"
#include "../../../IO/Console/Console.h"     // io::mainConsole->getWorkingFile - create

namespace dice {
namespace hfe {

namespace ai {

AITemplateManager* AITemplateManager::m_instance = 0;

AITemplateManager::AITemplateManager()
{
    // last wins, and it is the only way anything reaches this object: the class registers
    // nothing with ClassManager.
    m_instance = this;
    m_active = 0;
}

// Resets owned objects and clears the singleton unconditionally before member destruction.
AITemplateManager::~AITemplateManager()
{
    reset();
    m_instance = 0;
}

// Deletes templates and wrappers and clears the active selection. Map nodes remain;
// lookups are invalid after reset until the manager is destroyed or rebuilt.
int AITemplateManager::reset()
{
    m_active = 0;
    for (std::map<std::string, MgrTemplate*, NoCaseStringCompare>::iterator it = m_byName.begin();
         it != m_byName.end(); ++it)
    {
        if (it->second)
        {
            delete it->second->m_template;
            delete it->second;
        }
    }
    return 1;
}

// Creates and selects a template with its defining console file. A case-insensitive
// duplicate emits a fatal diagnostic; if error diagnostics are disabled, it clears the
// active template and returns an error string.
std::string AITemplateManager::create(std::string name)
{
    if (m_byName.find(name) != m_byName.end())
    {
        Debug(DebugTypeError, "AI/AIInterface/BFKnowledge/AITemplateManager.cpp", 38, "")
            << "Template '" << name << "' already exists";
        m_active = 0;
        return "Error: Template '" + name + "' already exists";
    }

    m_active = checkPointer(new MgrTemplate);
    m_active->m_name = name;
    m_active->m_template = checkPointer(new AITemplate(name));
    m_byName[name] = m_active;

    // getWorkingFile is asked once; the reference is tested and then copied.
    const std::string& workingFile = io::mainConsole->getWorkingFile();
    if (!workingFile.empty())
        m_workingFiles[m_active->m_template] = workingFile;

    return "AITemplate '" + name + "' successfully created";
}

// Looks up the plug-in by exact case-sensitive name. Requires a manager; a missing
// plug-in returns an error string without logging.
std::string AITemplateManager::addPlugIn(std::string name)
{
    if (m_active)
    {
        AITemplatePlugIn* const plugIn = AITemplatePlugInManager::m_instance->getTemplatePlugIn(name);
        if (plugIn)
        {
            m_active->m_template->addPlugIn(plugIn);
            return "Successfully added plugin " + name;
        }
        return "AITemplatePlugIn '" + name + "' doesn't exist";
    }
    return "Error: No active template";
}

// the active template is the named one, or none - a miss clears it.
void AITemplateManager::setActive(std::string name)
{
    std::map<std::string, MgrTemplate*, NoCaseStringCompare>::iterator it = m_byName.find(name);
    if (it == m_byName.end())
        m_active = 0;
    else
        m_active = it->second;
}

// the wrapper's name, or "" (constructed from a literal) with none active.
std::string AITemplateManager::getActive() const
{
    if (m_active)
        return m_active->m_name;
    return "";
}

AITemplate* AITemplateManager::getActiveTemplate() const
{
    if (m_active)
        return m_active->m_template;
    return 0;
}

// Creates wreck templates for all registered AI templates.
void AITemplateManager::createWreckTemplates()
{
    for (std::map<std::string, MgrTemplate*, NoCaseStringCompare>::iterator it = m_byName.begin();
         it != m_byName.end(); ++it)
    {
        if (it->second)
            it->second->m_template->createWreckTemplate();
    }
}

// Returns the template from a case-insensitive name lookup, or null when absent.
// The string parameter is passed by value as required by the interface.
AITemplate* AITemplateManager::getTemplate(std::string name)
{
    std::map<std::string, MgrTemplate*, NoCaseStringCompare>::iterator it = m_byName.find(name);
    if (it != m_byName.end())
        return it->second->m_template;

    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
