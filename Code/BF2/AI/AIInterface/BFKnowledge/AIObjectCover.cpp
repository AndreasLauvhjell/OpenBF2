// [INFERRED] Source placement.

#include "AIObjectCover.h"

#include "AITemplateCover.h"
#include "PlanePolygon.h"

#include "../SingletonServer.h"                              // BFSingletonServer::m_iInformationFactory
#include "../../AIDLL/AI/AIKnowledge/InformationFactory.h"        // IInformationFactory

#include "../../../Game/Objects/IObject.h"
#include "../../../Scene/Object/ObjectUtil.h"                // world::getPortalMesh, geom::IPortalMesh

namespace dice {
namespace hfe {
namespace ai {

namespace {

geom::IPortalMesh* portalMesh(IAIObject* object)
{
    return static_cast<geom::IPortalMesh*>(world::getPortalMesh(object->getObject(), 0));
}

} // anonymous namespace

AIObjectCover::AIObjectCover(IAIObject* object, AITemplateCover* aiTemplate)
    : IAIObjectCover(object), m_template(aiTemplate)
{
}

AIObjectCover::~AIObjectCover()
{
}

int AIObjectCover::getId() const
{
    return AIOFCover;
}

void AIObjectCover::createInformationPlugIn(unsigned int handle, bool real)
{
    static_cast<IInformationFactory*>(BFSingletonServer::m_iInformationFactory)
        ->requestInformationPlugin(handle, getId(), real);                         // +0x20
}

// The template's first-instance init, then a portal-mesh lookup whose result is
// discarded. Reproduced.
void AIObjectCover::init()
{
    if (!m_template->m_initialized)
        m_template->initFromObject(m_object);
    world::getPortalMesh(m_object->getObject(), 0);
}

// empty.
void AIObjectCover::update()
{
}

// the template's footprint placed by the caller's transformation, or by the
// object's own (getTransformation, +0xDC). A new polygon the caller owns.
IPlanePolygon* AIObjectCover::getObjectOutline(Mat4& transformation)
{
    return m_template->m_polygon->fromTransform(transformation);                   // +0x08
}

IPlanePolygon* AIObjectCover::getObjectOutline()
{
    IPlanePolygon* const polygon = m_template->m_polygon;
    return polygon->fromTransform(m_object->getTransformation());
}

// template reads.
IPlanePolygon* AIObjectCover::getLocalObjectOutline()
{
    return m_template->m_polygon;
}

float AIObjectCover::getCoverValue() const
{
    return m_template->m_coverValue;
}

float AIObjectCover::getHeight() const
{
    return m_template->m_height;
}

float AIObjectCover::getTransparency() const
{
    return m_template->m_transparency;
}

int AIObjectCover::getPathfindingPlaneCount() const
{
    return m_template->m_pathfindingPlaneCount;
}

const Vec3& AIObjectCover::getPathfindingPlaneCorner(int plane, int corner) const
{
    return m_template->getPathfindingPlaneCorner(plane, corner);
}

// the only portal query that tests for a mesh.
unsigned int AIObjectCover::getPortalCount() const
{
    geom::IPortalMesh* const mesh = portalMesh(m_object);
    if (mesh)
        return mesh->getPortalCount();                                              // +0x10
    return 0;
}

// - no null test on the mesh (callers ask getPortalCount first).
unsigned int AIObjectCover::getPortalVertexCount(unsigned int portal) const
{
    return portalMesh(m_object)->getPortalVertexCount(portal);                      // +0x14
}

const Vec3& AIObjectCover::getPortalVertex(unsigned int portal, unsigned int vertex) const
{
    return portalMesh(m_object)->getPortalVertex(portal, vertex);                   // +0x1C
}

void AIObjectCover::getPortal(unsigned int portal, std::vector<Vec3>& out) const
{
    portalMesh(m_object)->getPortal(portal, out);                                   // +0x18
}

// the template's portal block.
const Vec3& AIObjectCover::getPortalNormal(unsigned int portal) const
{
    return m_template->getPortalNormal(portal);
}

const Vec3& AIObjectCover::getPortalCenter(unsigned int portal) const
{
    return m_template->getPortalCenter(portal);
}

float AIObjectCover::getPortalXZExtentR(unsigned int portal) const
{
    return m_template->getPortalXZExtentR(portal);
}

float AIObjectCover::getPortalXZExtentL(unsigned int portal) const
{
    return m_template->getPortalXZExtentL(portal);
}

} // namespace ai
} // namespace hfe
} // namespace dice
