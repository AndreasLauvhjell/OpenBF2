// Script-defined unit strength, equipment type and entry settings.
// Strength setters require valid indices; they do not check array bounds.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEUNIT_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEUNIT_H

#include <string>

#include "AITemplatePlugIn.h"

#include "../../../IO/Input/Input.h"            // io::PlayerInputMap

namespace dice {
namespace hfe {
namespace ai {

class AITemplateUnit : public AITemplatePlugIn
{
public:
    AITemplateUnit();
    virtual ~AITemplateUnit();

    virtual IAIObjectPlugIn* createPlugIn(IAIObject* object, AITemplate* aiTemplate);
    virtual int getId();                                // returns 5

    // Both non-const (`_ZN`); the getter's result is assigned
    // straight into a std::string by the console (ConsoleClass168's get arm) - a reference.
    const std::string& getEquipmentTypeName();
    void setEquipmentTypeName(const std::string& name);

    // / , - AIObjectUnit::init's first-instance pass.
    void initFromObject(world::IObject* object);
    void traverseObjects(world::IObject* object);

    /* +0x04 */ int  m_maxAmmo;
    /* +0x08 */ int* m_battleStrengths;
    /* +0x0C */ int* m_strategicStrengths;
    /* +0x10 */ int  m_equipmentType;
    /* +0x14 */ bool m_initialized;
    /* +0x15 */ bool m_useNoPathfindingToGetToObject;
    /* +0x16 */ bool m_hasExposedSoldier;
    /* +0x17 */ bool m_alwaysTargetSoldier;
    /* +0x18 */ float m_cullDistance;
    /* +0x1C */ float m_cullDistanceSquared;
    /* +0x20 */ io::PlayerInputMap m_selectKey;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEUNIT_H
