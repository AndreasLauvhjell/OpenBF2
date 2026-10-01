// [INFERRED] Source placement.

#include "AITemplateCover.h"

#include <list>

#include "AIObjectCover.h"
#include "BFGame.h"                                          // IAIGame::getIsPrecaching (+0x68)
#include "IAIObject.h"
#include "PlanePolygon.h"

#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

namespace dice {
namespace hfe {
namespace ai {

// : 1.0f at +8, zero everywhere else (+0x10 as a byte).
AITemplateCover::AITemplateCover()
    : m_polygon(0), m_coverValue(1.0f), m_height(0.0f), m_initialized(false), m_transparency(0.0f),
      m_portals(0), m_pathfindingPlaneCount(0), m_pathfindingPlaneCorners(0)
{
}

// Deletes the polygon virtually, then frees the portal arrays and corner storage in order.
AITemplateCover::~AITemplateCover()
{
    delete m_polygon;
    if (m_portals)
    {
        delete[] m_portals->m_xzExtentR;
        delete[] m_portals->m_xzExtentL;
        delete[] m_portals->m_centers;
        delete[] m_portals->m_normals;
        delete m_portals;
    }
    delete[] m_pathfindingPlaneCorners;
}

// createPlugIn - , : `new AIObjectCover(object, this)` (0xC B) through checkPointer.
IAIObjectPlugIn* AITemplateCover::createPlugIn(IAIObject* object, AITemplate* /*aiTemplate*/)
{
    return checkPointer(new AIObjectCover(object, this));
}

// Initializes once outside precaching with a one-point polygon at (0, 0).
// Queries the engine object without using it to measure a footprint.
void AITemplateCover::initFromObject(IAIObject* object)
{
    if (m_initialized ||
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getIsPrecaching())     // +0x68
        return;

    delete m_polygon;
    std::list<Vec2> corners;
    Vec2 origin;
    origin.x = 0.0f;
    origin.y = 0.0f;
    corners.push_back(origin);
    m_polygon = checkPointer(new PlanePolygon(corners));
    object->getObject();                                                            // result unused
    m_initialized = true;
}

// unchecked reads of the portal block and the corner array.
const Vec3& AITemplateCover::getPortalCenter(unsigned int portal) const
{
    return m_portals->m_centers[portal];
}

const Vec3& AITemplateCover::getPortalNormal(unsigned int portal) const
{
    return m_portals->m_normals[portal];
}

float AITemplateCover::getPortalXZExtentR(unsigned int portal) const
{
    return m_portals->m_xzExtentR[portal];
}

float AITemplateCover::getPortalXZExtentL(unsigned int portal) const
{
    return m_portals->m_xzExtentL[portal];
}

const Vec3& AITemplateCover::getPathfindingPlaneCorner(int plane, int corner) const
{
    return m_pathfindingPlaneCorners[plane * 3 + corner];
}

// `mov eax, 2`.
int AITemplateCover::getId()
{
    return 2;
}

} // namespace ai
} // namespace hfe
} // namespace dice
