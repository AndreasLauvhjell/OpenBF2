
#include "MapProperties.h"

#include <cmath>

namespace dice {
namespace hfe {
namespace ai {

// Initializes map traversal properties and their default limits.
MapProperties::MapProperties()
    : m_maxSlopeCos(0.0f)
    , m_radius(0.0f)
    , m_headClearance(2.0f)
    , m_allowedHeightDiff(2.5f)
{
}

// The whole body is the set's own teardown, which a C++ destructor does on its own. Empty here.
MapProperties::~MapProperties()
{
}

// Tests only the Y component against the slope cosine. Requires a unit surface normal
// for slope semantics; the material argument is ignored.
bool MapProperties::isValidGroundMesh(const Vec3& normal, unsigned char /*material*/) const
{
    return m_maxSlopeCos <= normal.y;
}

// Stores maximum slope as a cosine. The getter clamps it to [-1, 1] before acosf
// and returns the resulting angle in radians.
void MapProperties::setMaxSlope(float radians)
{
    m_maxSlopeCos = cosf(radians);
}

float MapProperties::getMaxSlope() const
{
    float cosine = m_maxSlopeCos;
    if (cosine < -1.0f)
        cosine = -1.0f;
    else if (cosine > 1.0f)
        cosine = 1.0f;

    return acosf(cosine);
}

void MapProperties::setHeadClearance(float clearance)
{
    m_headClearance = clearance;
}

float MapProperties::getHeadClearance() const
{
    return m_headClearance;
}

void MapProperties::setRadius(float radius)
{
    m_radius = radius;
}

float MapProperties::getRadius() const
{
    return m_radius;
}

// no matching getter ships - see the header.
void MapProperties::setAllowedHeightDiff(float diff)
{
    m_allowedHeightDiff = diff;
}

} // namespace ai
} // namespace hfe
} // namespace dice
