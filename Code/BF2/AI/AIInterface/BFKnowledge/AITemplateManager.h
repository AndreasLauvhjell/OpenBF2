// Owns named AI templates and selects the active template for console commands.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEMANAGER_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEMANAGER_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "AIAdapterTypes.h"

#include "../../../IO/File/IFileSystem.h"   // hfe::NoCaseStringCompare - m_byName's comparator

namespace dice {
namespace hfe {

class IBase;

namespace ai {

class AITemplate;   // AITemplate.h - 0x24 bytes (`operator new(0x24)` in `create`, )

class AITemplateManager
{
public:
    // Stores a name and template pointer; the record is 8 bytes with the original ELF STL
    // and 32 bytes with the original PE STL. Member names are inferred. getActive copies it.
    struct MgrTemplate
    {
        /* +0 */ std::string m_name;          // [INFERRED name] create's name argument
        /* +4 */ AITemplate* m_template;
    };

    AITemplateManager();
    // Non-virtual destructor; reset is the only virtual slot.
    ~AITemplateManager();

    virtual int reset();                       // - `mov eax, 1` [INFERRED int]

    // The `aiTemplate` console object's four targets (ELF ConsoleClass93/94/102). Every name is
    // taken by value and every string is returned by value (`...createESs`, `...addPlugInESs`
    // `...setActiveESs`; hidden-pointer returns).
    std::string create(std::string name);
    std::string addPlugIn(std::string name);
    void setActive(std::string name);          // - [INFERRED void]: no caller reads eax
    std::string getActive() const;
    AITemplate* getActiveTemplate() const;     // - getActiveAITemplate's only callee

    // Called by `BFConsole::aiMainInit` on the level script's `ai.init` line, as its last
    // statement. : one walk of the name map.
    void createWreckTemplates();

    // Takes the name by value; this lookup is non-const in the original interface.
    AITemplate* getTemplate(std::string name);

    // not declared: getAllUsedTemplates , getWorkingFileAndPath and
    // getAllUsedTemplateFiles . None has a caller in either image.

    static AITemplateManager* m_instance;      // ds:

private:
    /* +0x00 */ // vptr
    // Template names use NoCaseStringCompare to match script lookup semantics.
    /* +0x04 */ std::map<std::string, MgrTemplate*, NoCaseStringCompare> m_byName;
    // The active template's wrapper: `create` sets it (or clears it on a duplicate), `setActive`
    // looks it up (null on a miss), `reset` clears it. Every `aiTemplate.<property>` line writes
    // through it. [INFERRED name]
    /* +0x10 */ MgrTemplate* m_active;
    // +0x14: 4 bytes that the constructor never stores and no method reads. [UNRESOLVED] Not
    // declared - no code in either image touches them.
    // The defining .con file of each template, keyed by the template (create stores
    // mainConsole->getWorkingFile when it is non-empty). `_Rb_tree<AITemplate*, pair<AITemplate*
    // const, std::string>, ..., std::less<AITemplate*>>` in the ELF. [INFERRED name]
    /* +0x18 */ std::map<AITemplate*, std::string> m_workingFiles;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEMANAGER_H
