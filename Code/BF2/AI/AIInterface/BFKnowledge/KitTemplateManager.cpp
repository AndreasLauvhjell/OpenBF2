// [INFERRED] Source placement.

#include "KitTemplateManager.h"

#include "KitTemplate.h"
#include <set>

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../../../IO/Console/Console.h"
#include "../SingletonServer.h"   // BFSingletonServer::updateStaticMethods

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

KitTemplateManager* KitTemplateManager::m_instance = 0;

// First wins in, exact match out.
IKitTemplateManager::IKitTemplateManager()
{
    if (!CommonSingletonServer::m_iKitTemplateManager)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->registerSimpleSingleton(std::string("IKitTemplateManager"), this);
        BFSingletonServer::updateStaticMethods();
    }
}

IKitTemplateManager::~IKitTemplateManager()
{
    if (CommonSingletonServer::m_iKitTemplateManager == this)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->unregisterSimpleSingleton(std::string("IKitTemplateManager"));
        BFSingletonServer::updateStaticMethods();
    }
}

KitTemplateManager::KitTemplateManager()
    : m_active(m_byName.end())
{
    // last wins - an unconditional store, unlike the interface's first-wins registration.
    m_instance = this;
    // thirteen. The kit-type space, seeded with nulls exactly as AIPlayerFactory seeds 256.
    m_byKitType.resize(13, 0);
}

KitTemplateManager::~KitTemplateManager()
{
    // unconditional singleton clear precedes deletions.
    m_instance = 0;
    m_active = m_byName.begin();
    while (m_active != m_byName.end())
    {
        delete m_active->second;
        ++m_active;
    }
}

// Strings are returned by value and name is passed by value.
std::string KitTemplateManager::create(std::string name, KitType type)
{
    if (type == static_cast<KitType>(13))
        return "Error: Template '" + name + "' is of type 'KTNoKit'";

    if (m_byName.find(name) != m_byName.end())
    {
        m_active = m_byName.end();
        return "Error: Template '" + name + "' already exists";
    }

    KitTemplate* kit = new KitTemplate(type);
    if (!kit)
    {
        m_active = m_byName.end();
        return "Error: Template '" + name + "' could not be created";
    }

    m_byName[name] = kit;
    m_byKitType[type] = kit;
    m_active = m_byName.find(name);
    const std::string& file = io::mainConsole->getWorkingFile();
    if (!file.empty())
        m_workingFiles[m_active->second] = file;
    return "KitTemplate '" + name + "' successfully created";
}

void KitTemplateManager::setActive(std::string name)
{
    m_active = m_byName.find(name);
}

// an owned string copy, including the empty result.
std::string KitTemplateManager::getActive()
{
    if (m_active == m_byName.end())
        return std::string();
    return m_active->first;
}

KitTemplate* KitTemplateManager::getActiveTemplate() const
{
    if (m_active == m_byName.end())
        return 0;
    return m_active->second;
}

// the name lookup is case-sensitive in both ports.
KitTemplate* KitTemplateManager::getTemplate(const std::string& name) const
{
    const auto found = m_byName.find(name);
    if (found == m_byName.end())
        return 0;
    return found->second;
}

// Only KTNoKit is guarded.
IKitTemplate* KitTemplateManager::getTemplate(KitType type) const
{
    if (type == static_cast<KitType>(13))
        return 0;
    return m_byKitType[type];
}

// return the existing vector at +0x20.
// The borrowed reference uses native STL storage; no copy, cache or filtering occurs.
const std::vector<IKitTemplate*>& KitTemplateManager::getAllTemplates() const
{
    return m_byKitType;
}

// append in pointer-map order, without clearing.
void KitTemplateManager::getAllUsedTemplates(std::list<KitTemplate*>& templates) const
{
    for (auto it = m_workingFiles.begin(); it != m_workingFiles.end(); ++it)
        templates.insert(templates.end(), it->first);
}

// The missing-entry dereference is shipped.
std::string KitTemplateManager::getWorkingFileAndPath(KitTemplate* kit) const
{
    return m_workingFiles.find(kit)->second;
}

// read in full; PE twin not located. Unique sorted file names are appended.
void KitTemplateManager::getAllUsedTemplateFiles(std::list<std::string>& files) const
{
    std::set<std::string> uniqueFiles;
    for (auto it = m_workingFiles.begin(); it != m_workingFiles.end(); ++it)
        uniqueFiles.insert(it->second);
    for (auto it = uniqueFiles.begin(); it != uniqueFiles.end(); ++it)
        files.insert(files.end(), *it);
}

// AL=1 establishes bool.
bool KitTemplateManager::reset()
{
    m_active = m_byName.begin();
    while (m_active != m_byName.end())
    {
        delete m_active->second;
        ++m_active;
    }
    m_byName.clear();
    m_workingFiles.clear();
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
