// Script-defined movement and navigation properties for AI objects.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEMOBILE_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEMOBILE_H

#include <string>

#include "AITemplatePlugIn.h"

namespace dice {
namespace hfe {
namespace ai {

class AITemplateMobile : public AITemplatePlugIn
{
public:
    AITemplateMobile();
    virtual ~AITemplateMobile();

    virtual IAIObjectPlugIn* createPlugIn(IAIObject* object, AITemplate* aiTemplate);
    virtual int getId();                                // returns 3

    void setCoverSearchRadius(float radius);
    void setSoundSphereRadius(float first, float second);
    void setHearingProbability(float first, float second);
    void setPathfindingMap(const std::string& name);

    /* +0x04 */ int   m_vehicleNumber;                  // vehicleNumber / setPathfindingMap; ctor -1
    /* +0x08 */ float m_maxSpeed;                       // maxSpeed; ctor 0
    /* +0x0C */ float m_turnRadius;                     // turnRadius; ctor 0
    /* +0x10 */ float m_drag;                           // never initialised, and no console
                                                        // property writes it; AIObjectMobile::
                                                        // getDrag reads it (flds) - named from there
    /* +0x14 */ float m_coverSearchRadius;              // coverSearchRadius; ctor 0
    /* +0x18 */ float m_lodHeight;                      // lodHeight; ctor 0.75
    /* +0x1C */ float m_soundSphereRadius2;             // setSoundSphereRadius's second; ctor 30
    /* +0x20 */ float m_soundSphereRadius1;             // setSoundSphereRadius's first; ctor 0
    /* +0x24 */ float m_hearingProbability2;            // setHearingProbability's second; ctor 0.1
    /* +0x28 */ float m_hearingProbability1;            // setHearingProbability's first; ctor 0.01
    /* +0x2C */ float m_avoidCollisionLookAhead;        // avoidCollisionLookAhead; ctor 2
    /* +0x30 */ bool  m_isTurnable;                     // isTurnable; ctor false
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEMOBILE_H
