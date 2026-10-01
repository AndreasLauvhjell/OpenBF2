// Script-defined AI properties and plug-in templates for one kind of engine object.
// AITemplateManager owns these templates; runtime AI objects borrow them.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATE_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATE_H

#include <string>

#include "AIAdapterTypes.h"

namespace dice {
namespace hfe {
namespace ai {

class AITemplatePlugIn;

class AITemplate
{
public:
    // Every scalar zero, the name copied, no wreck.
    // (The ELF also has AITemplate(int, float, float, float, bool, const std::string&),
    // which leaves m_wreckTemplate UNinitialised; nothing in this tree calls it, so it is not
    // declared.)
    explicit AITemplate(const std::string& name);
    ~AITemplate();

    void initFromObject(world::IObject* object);
    // / - builds the runtime AI object (one of the four
    // AIObjectPrimary/Secondary x Real/Shared classes) and its plug-ins. [INFERRED ret]
    IAIObject* createObject(int side, world::IObject* object);
    void addPlugIn(AITemplatePlugIn* plugIn);
    bool isArmament() const;                              // m_plugIns[0] != 0
    bool isCover() const;                                 // m_plugIns[2] != 0
    bool isPhysical() const;                              // m_plugIns[4] != 0
    bool isMobile() const;                                // m_plugIns[3] != 0
    bool isUnit() const;                                  // m_plugIns[5] != 0
    bool createWreckTemplate();

    /* +0x00 */ AITemplatePlugIn** m_plugIns;
    /* +0x04 */ int   m_plugInMask;
    /* +0x08 */ int   m_types;
    /* +0x0C */ float m_degeneration;
    /* +0x10 */ float m_allowedTimeDiff;
    /* +0x14 */ float m_basicTemp;
    /* +0x18 */ bool  m_commonKnowledge;
    /* +0x19 */ bool  m_secondary;
    /* +0x1A */ bool  m_initialized;
    /* +0x1C */ std::string m_name;
    /* +0x20 */ AITemplate* m_wreckTemplate;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATE_H
