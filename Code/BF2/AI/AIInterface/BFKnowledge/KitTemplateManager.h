// Owns AI kit templates.
// Preserve virtual declaration order. [INFERRED] Return types require validation where implementations remain incomplete.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_KITTEMPLATEMANAGER_H
#define AI_AIINTERFACE_BFKNOWLEDGE_KITTEMPLATEMANAGER_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "AIAdapterTypes.h"

namespace dice {
namespace hfe {

class IBase;

namespace ai {

class KitTemplate;

class IKitTemplateManager
{
public:
    IKitTemplateManager();
    virtual IKitTemplate* getTemplate(KitType type) const = 0;
    virtual const std::vector<IKitTemplate*>& getAllTemplates() const = 0;
    virtual bool reset() = 0;

    // Keep the destructor after the three interface methods to preserve virtual slot order.
    virtual ~IKitTemplateManager();
};

class KitTemplateManager : public IKitTemplateManager
{
public:
    KitTemplateManager();
    virtual ~KitTemplateManager();

    virtual IKitTemplate* getTemplate(KitType type) const;
    virtual const std::vector<IKitTemplate*>& getAllTemplates() const;
    virtual bool reset();

    std::string create(std::string name, KitType type);
    void setActive(std::string name);
    std::string getActive();
    KitTemplate* getActiveTemplate() const;
    KitTemplate* getTemplate(const std::string& name) const;
    void getAllUsedTemplates(std::list<KitTemplate*>& templates) const;
    std::string getWorkingFileAndPath(KitTemplate* kit) const;
    void getAllUsedTemplateFiles(std::list<std::string>& files) const;

    static KitTemplateManager* m_instance;      // ds:

private:
    /* +0x00 */ // vptr
    // Offsets are the retail ELF's STL layout, not native compiler layout promises.
    // The name map owns its pointees. reset clears both maps but leaves the vector unchanged.
    /* +0x04 */ std::map<std::string, KitTemplate*> m_byName;
    /* +0x10 */ std::map<std::string, KitTemplate*>::iterator m_active;
    /* +0x14 */ std::map<KitTemplate*, std::string> m_workingFiles;
    /* +0x20 */ std::vector<IKitTemplate*> m_byKitType;   // ctor resizes to thirteen nulls
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_KITTEMPLATEMANAGER_H
