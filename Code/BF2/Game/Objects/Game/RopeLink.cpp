// Path:     [INFERRED] - see RopeLink.h.
// Contents: RopeLink's complete object surface, recovered from bf2.i64 and checked against
// bf2_w32ded.exe.

#include "RopeLink.h"

#include "GrapplingHookRope.h"
#include "../TemplateLeaves.h"
#include "../../../Core/Ids.h"

#include <cmath>

namespace dice {
namespace hfe {
namespace world {

// ELF C2 0x81A5010 / C1 0x81A5070, PE counterpart. The final +0x14C dword is not written by
// either constructor; initialize later stores the owning soldier's kit type on the server.
RopeLink::RopeLink(const RopeLinkTemplate* objectTemplate)
    : SimpleObject(objectTemplate)
    , m_refCount(1)
    , m_rope(0)
    , m_startParticle(0)
    , m_endParticle(0)
    , m_climbDirection(0)
{
}

// /0x81A5100/0x81A5130: no owned tail state.
RopeLink::~RopeLink()
{
}

// . Explicit components preserve the shipped subtraction, half-scale, then add.
Vec3 RopeLink::getMidpoint()
{
    Vec3 midpoint;
    midpoint.x = (m_endParticle->m_position.x - m_startParticle->m_position.x) * 0.5f
               + m_startParticle->m_position.x;
    midpoint.y = (m_endParticle->m_position.y - m_startParticle->m_position.y) * 0.5f
               + m_startParticle->m_position.y;
    midpoint.z = (m_endParticle->m_position.z - m_startParticle->m_position.z) * 0.5f
               + m_startParticle->m_position.z;
    return midpoint;
}

// . Hook links place their model 60 percent along the segment and retain its forward
// direction; ordinary links use the midpoint and reverse it. Both construct the same orthonormal
// frame, exchange its first/third rows with the shipped sign flip, and raise the model by 0.05.
void RopeLink::setTransformation()
{
    const Vec3 start = m_startParticle->m_position;
    const Vec3 end = m_endParticle->m_position;
    const Vec3 rawDirection = {
        end.x - start.x,
        end.y - start.y,
        end.z - start.z
    };
    Vec3 direction = rawDirection;
    const float lengthSquared = direction.x * direction.x +
                                direction.y * direction.y +
                                direction.z * direction.z;
    const float epsilon = 1.1920929e-7f;
    if (std::fabs(lengthSquared - 1.0f) >= epsilon)
    {
        if (std::fabs(lengthSquared) >= epsilon)
        {
            const float inverseLength = 1.0f / std::sqrt(lengthSquared);
            direction.x *= inverseLength;
            direction.y *= inverseLength;
            direction.z *= inverseLength;
        }
        else
        {
            direction.x = direction.y = direction.z = 0.0f;
        }
    }

    Mat4 transformation = Mat4::getIdentity();
    if (getTemplate()->getClassID() == CID_HookLinkTemplate)
    {
        static const float percent = 0.6f; // ds:0x8987DA4
        transformation.m[12] = start.x + rawDirection.x * percent;
        transformation.m[13] = start.y + rawDirection.y * percent;
        transformation.m[14] = start.z + rawDirection.z * percent;
    }
    else
    {
        transformation.m[12] = start.x + rawDirection.x * 0.5f;
        transformation.m[13] = start.y + rawDirection.y * 0.5f;
        transformation.m[14] = start.z + rawDirection.z * 0.5f;
        direction.x *= -1.0f;
        direction.y *= -1.0f;
        direction.z *= -1.0f;
    }

    transformation.m[13] += 0.05f;
    Vec3 up = { 0.0f, 0.0f, 1.0f };
    if (std::fabs(direction.x) < 0.1f && std::fabs(direction.y) < 0.1f)
    {
        up.y = 1.0f;
        up.z = 0.0f;
    }
    transformation.m[8] = direction.x;
    transformation.m[9] = direction.y;
    transformation.m[10] = direction.z;
    transformation.m[4] = up.x;
    transformation.m[5] = up.y;
    transformation.m[6] = up.z;
    reinterpret_cast<Vec3*>(&transformation.m[0])->makeOrthonormalBasis(
        *reinterpret_cast<Vec3*>(&transformation.m[4]),
        *reinterpret_cast<Vec3*>(&transformation.m[8]));

    const Vec3 firstRow = {
        transformation.m[0], transformation.m[1], transformation.m[2]
    };
    const Vec3 thirdRow = {
        transformation.m[8], transformation.m[9], transformation.m[10]
    };
    transformation.m[0] = -thirdRow.x;
    transformation.m[1] = -thirdRow.y;
    transformation.m[2] = -thirdRow.z;
    transformation.m[8] = firstRow.x;
    transformation.m[9] = firstRow.y;
    transformation.m[10] = firstRow.z;

    const float scale = getTemplate()->getClassID() == CID_HookLinkTemplate ? 2.5f : 1.0f;
    transformation.m[0] *= scale;
    transformation.m[1] *= scale;
    transformation.m[2] *= scale;
    transformation.m[4] *= scale;
    transformation.m[5] *= scale;
    transformation.m[6] *= scale;
    transformation.m[8] *= scale;
    transformation.m[9] *= scale;
    transformation.m[10] *= scale;

    setAbsoluteTransformation(transformation);
}

// deliberately no null guard.
void RopeLink::setAwake()
{
    m_rope->setAwake();
}

void RopeLink::addRef() { ++m_refCount; }
int RopeLink::getRef() { return m_refCount; }
int RopeLink::release()
{
    const int left = --m_refCount;
    if (left == 0)
        delete this;
    return left;
}

// exactly two answers; the ConsoleSaveable subobject is not exposed.
void* RopeLink::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObject)
        return const_cast<RopeLink*>(this);
    return 0;
}

// is a genuine five-byte empty override.
void RopeLink::initNetworkableComp()
{
}

} // namespace world
} // namespace hfe
} // namespace dice
