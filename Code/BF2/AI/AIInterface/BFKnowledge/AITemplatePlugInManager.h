// Owns and selects AI plug-in templates. reset is its only virtual method.
// The constructor replaces its own singleton pointer; it is not in the shared singleton roster.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEPLUGINMANAGER_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEPLUGINMANAGER_H

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

class AITemplatePlugIn;

class AITemplatePlugInManager
{
public:
    // Named plug-in record owned by the manager. reset deletes the plug-in virtually,
    // then the wrapper. [INFERRED] Field names.
    struct MgrTemplatePlugIn
    {
        /* +0 */ std::string m_name;
        /* +4 */ AITemplatePlugIn* m_plugIn;
    };

    AITemplatePlugInManager();
    // Non-virtual destructor; BFConsole owns this manager.
    ~AITemplatePlugInManager();

    virtual int reset();                              // - `mov eax, 1`

    // `aiTemplatePlugIn.create <type> <name>` (ELF ConsoleClass139): ,
    // type first, then the name - both by value (`...createESsSs`).
    std::string create(std::string type, std::string name);
    void setActive(std::string name);                 // - [INFERRED void]
    std::string getActive() const;
    AITemplatePlugIn* getActiveTemplatePlugIn() const;
    // Name by value, not const - as getTemplate.
    AITemplatePlugIn* getTemplatePlugIn(std::string name);

    static AITemplatePlugInManager* m_instance;      // ds:

private:
    /* +0x00 */ // vptr
    // case-sensitive (std::less), unlike the AI template map beside it.
    /* +0x04 */ std::map<std::string, MgrTemplatePlugIn*> m_byName;
    /* +0x10 */ MgrTemplatePlugIn* m_active;           // [INFERRED name]
    // Each plug-in's defining .con file (`_Rb_tree<AITemplatePlugIn*, pair<AITemplatePlugIn* const
    // std::string>, ..., std::less<AITemplatePlugIn*>>`). [INFERRED name]
    /* +0x14 */ std::map<AITemplatePlugIn*, std::string> m_workingFiles;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEPLUGINMANAGER_H
