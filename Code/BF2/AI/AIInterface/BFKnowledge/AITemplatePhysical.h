// Script-defined physical properties for AI objects.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEPHYSICAL_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEPHYSICAL_H

#include "AITemplatePlugIn.h"

namespace dice {
namespace hfe {
namespace ai {

class AITemplatePhysical : public AITemplatePlugIn
{
public:
    AITemplatePhysical();
    virtual ~AITemplatePhysical();

    virtual IAIObjectPlugIn* createPlugIn(IAIObject* object, AITemplate* aiTemplate);
    virtual int getId();                                // returns 4

    // Non-virtual; AIObjectPhysical::init calls it for the first instance.
    void initFromObject(IAIObject* object);

    // [INFERRED] Names follow the getters for health, mass, volume, armor, and pathfinding.
    // Initialization leaves the fields at +0x10 and +0x18 unchanged.
    /* +0x04 */ float m_maxHealth;                      // initFromObject: the armour's max HP; ctor 0
    /* +0x08 */ float m_mass;                           // initFromObject: the physics node's mass
    /* +0x0C */ float m_volume;                         // initFromObject: 1.0 (see there); ctor 0
    /* +0x10 */ float m_bestArmour;                     // ctor 0; nothing in this image writes it
    /* +0x14 */ float m_radius;                         // initFromObject: collision LOD-1 radius.
                                                        // a float (fstp dword) - was `int`
    /* +0x18 */ int   m_strType;                        // getStrType / setStrType; ctor 0
    /* +0x1C */ bool  m_initialized;                    // initFromObject's once-flag
    /* +0x1D */ bool  m_affectsPathfinding;             // DestroyableObjectTemplate::usedByAi
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEPHYSICAL_H
