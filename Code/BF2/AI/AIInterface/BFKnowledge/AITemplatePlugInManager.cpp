// [INFERRED] Source placement.

#include "AITemplatePlugInManager.h"

#include "AITemplateArmament.h"
#include "AITemplateControlInfo.h"
#include "AITemplateCover.h"
#include "AITemplateMobile.h"
#include "AITemplatePhysical.h"
#include "AITemplatePlugIn.h"
#include "AITemplateUnit.h"

#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../../Core/Debug.h"
#include "../../../IO/Console/Console.h"     // io::mainConsole->getWorkingFile - create

namespace dice {
namespace hfe {

namespace ai {

AITemplatePlugInManager* AITemplatePlugInManager::m_instance = 0;

AITemplatePlugInManager::AITemplatePlugInManager()
{
    // last wins, and it is the only way anything reaches this object: the class registers
    // nothing with ClassManager.
    m_instance = this;
    m_active = 0;
}

// reset directly, the instance cleared unconditionally, then the
// two maps' own destructors - AITemplateManager's destructor, member for member.
AITemplatePlugInManager::~AITemplatePlugInManager()
{
    reset();
    m_instance = 0;
}

// Deletes owned plug-ins virtually, then their wrappers. Map nodes remain, so lookups
// are invalid after reset until the manager is destroyed or rebuilt.
int AITemplatePlugInManager::reset()
{
    m_active = 0;
    for (std::map<std::string, MgrTemplatePlugIn*>::iterator it = m_byName.begin();
         it != m_byName.end(); ++it)
    {
        if (it->second)
        {
            delete it->second->m_plugIn;
            delete it->second;
        }
    }
    return 1;
}

// Creates and selects a named plug-in template. Duplicate names emit a fatal error diagnostic.
std::string AITemplatePlugInManager::create(std::string type, std::string name)
{
    if (m_byName.find(name) != m_byName.end())
    {
        Debug(DebugTypeError, "AI/AIInterface/BFKnowledge/AITemplatePlugInManager.cpp", 37, "")
            << "TemplatePlugIn '" << name << "' already exists";
        m_active = 0;
        return "Error: TemplatePlugIn '" + name + "' already exists";
    }

    m_active = checkPointer(new MgrTemplatePlugIn);
    m_active->m_name = name;

    if (type == "Cover")
        m_active->m_plugIn = checkPointer(new AITemplateCover);
    else if (type == "Mobile")
        m_active->m_plugIn = checkPointer(new AITemplateMobile);
    else if (type == "Physical")
        m_active->m_plugIn = checkPointer(new AITemplatePhysical);
    else if (type == "Armament")
        m_active->m_plugIn = checkPointer(new AITemplateArmament);
    else if (type == "Unit")
        m_active->m_plugIn = checkPointer(new AITemplateUnit);
    else if (type == "ControlInfo")
        m_active->m_plugIn = checkPointer(new AITemplateControlInfo);
    else
    {
        delete m_active;
        m_active = 0;
        return "TemplatePlugIn type '" + type + "' doesn't exist";
    }

    m_byName.insert(std::make_pair(name, m_active));

    // getWorkingFile is asked once; the reference is tested and then copied.
    const std::string& workingFile = io::mainConsole->getWorkingFile();
    if (!workingFile.empty())
        m_workingFiles[m_active->m_plugIn] = workingFile;

    return "AITemplatePlugIn '" + name + "' successfully created";
}

// the named plug-in becomes active, or none - a miss clears it.
void AITemplatePlugInManager::setActive(std::string name)
{
    std::map<std::string, MgrTemplatePlugIn*>::iterator it = m_byName.find(name);
    if (it == m_byName.end())
        m_active = 0;
    else
        m_active = it->second;
}

// the wrapper's name, or "" (constructed from a literal) with none active.
std::string AITemplatePlugInManager::getActive() const
{
    if (m_active)
        return m_active->m_name;
    return "";
}

// The one callee of the six getActiveTemplatePlugInIfMatch<> gates.
AITemplatePlugIn* AITemplatePlugInManager::getActiveTemplatePlugIn() const
{
    if (m_active)
        return m_active->m_plugIn;
    return 0;
}

// `return found ? node->second->m_plugIn : 0` - an exact-case lookup, and no null test on the
// found wrapper (create never stores a null one). AITemplateManager::addPlugIn's only callee.
AITemplatePlugIn* AITemplatePlugInManager::getTemplatePlugIn(std::string name)
{
    std::map<std::string, MgrTemplatePlugIn*>::iterator it = m_byName.find(name);
    if (it != m_byName.end())
        return it->second->m_plugIn;

    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
