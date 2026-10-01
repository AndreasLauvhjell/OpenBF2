// Script-defined armament settings used to create AI weapon access plug-ins.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEARMAMENT_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEARMAMENT_H

#include "AITemplatePlugIn.h"

namespace dice {
namespace hfe {
namespace world { class IObject; }
namespace ai {

class AITemplateArmament : public AITemplatePlugIn
{
public:
    AITemplateArmament();
    virtual ~AITemplateArmament();

    virtual IAIObjectPlugIn* createPlugIn(IAIObject* object, AITemplate* aiTemplate);
    virtual int getId();                                // returns 0

    // Non-virtual. [INFERRED void] both - no caller reads a result.
    void initFromObject(world::IObject* object);
    void traverseObjects(world::IObject* object);

    /* +0x04 */ bool m_initialized;                     // ctor false
    /* +0x08 */ int  m_isAntiAircraft;                  // setIsAntiAircraft; ctor 0
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEARMAMENT_H
