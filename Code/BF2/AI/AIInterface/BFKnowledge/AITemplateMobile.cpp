// [INFERRED] Source placement.

#include "AITemplateMobile.h"

#include "AIObjectMobile.h"

#include "../../AIDLL/AI/AIUtils/checkPointer.h"

#include "../../AIDLL/AI/Main/BFPathfindingCourier.h"   // IAIPathfindingCourier
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

namespace dice {
namespace hfe {
namespace ai {

// Every store read; +0x10 is not among them (see the header).
AITemplateMobile::AITemplateMobile()
    : m_vehicleNumber(-1), m_maxSpeed(0.0f), m_turnRadius(0.0f), m_coverSearchRadius(0.0f),
      m_lodHeight(0.75f), m_soundSphereRadius2(30.0f), m_soundSphereRadius1(0.0f),
      m_hearingProbability2(0.1f), m_hearingProbability1(0.01f), m_avoidCollisionLookAhead(2.0f),
      m_isTurnable(false)
{
}

// - the vptr store and, for , the delete. Nothing else.
AITemplateMobile::~AITemplateMobile()
{
}

// createPlugIn - , : `new AIObjectMobile(object, this)` (0x28 B) through checkPointer.
IAIObjectPlugIn* AITemplateMobile::createPlugIn(IAIObject* object, AITemplate* /*aiTemplate*/)
{
    return checkPointer(new AIObjectMobile(object, this));
}

// `mov eax, 3`.
int AITemplateMobile::getId()
{
    return 3;
}

void AITemplateMobile::setCoverSearchRadius(float radius)
{
    m_coverSearchRadius = radius;
}

// first -> +0x20, second -> +0x1C.
void AITemplateMobile::setSoundSphereRadius(float first, float second)
{
    m_soundSphereRadius1 = first;
    m_soundSphereRadius2 = second;
}

// first -> +0x28, second -> +0x24.
void AITemplateMobile::setHearingProbability(float first, float second)
{
    m_hearingProbability1 = first;
    m_hearingProbability2 = second;
}

// Requires the pathfinding courier to exist before scripts run. Unknown map names resolve to -1.
void AITemplateMobile::setPathfindingMap(const std::string& name)
{
    m_vehicleNumber = static_cast<IAIPathfindingCourier*>(CommonSingletonServer::m_iaiPathfindingCourier)
                          ->getVehicleNumberFromName(name);
}

} // namespace ai
} // namespace hfe
} // namespace dice
