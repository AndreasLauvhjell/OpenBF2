// [INFERRED] Source placement.

#include "PlanePolygon.h"

#include "AIFrontier.h"

namespace dice {
namespace hfe {

// an empty list, or a copy of the caller's.
PlanePolygon::PlanePolygon()
{
}

PlanePolygon::PlanePolygon(const std::list<Vec2>& vertices)
    : m_vertices(vertices)
{
}

// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-07-knowledge]
// Paired ABI evidence fixes this signature (, ELF imported symbol), but the 2,037-byte
// PE body and BoundingBox surface are not recovered. IPICover's physical fallback now stops at the
// exact callee instead of fabricating an outline.
PlanePolygon::PlanePolygon(BoundingBox* /*box*/, const Mat4& /*transformation*/)
{
    BF2_AI_FRONTIER("dice::hfe::PlanePolygon::PlanePolygon(BoundingBox*, const Mat4&)");
}

// the list's own destructor.
PlanePolygon::~PlanePolygon()
{
}

// Each corner goes XZ -> Vec3 (y = 0), through the transformation
// as a point (row vector: x' = m0*x + m4*y + m8*z + m12, and so on), and back to XZ, appended to a
// temporary list; a new empty PlanePolygon then takes that list through its own setVertices slot.
// Each of the three components is computed from the original x/y/z - the image holds y' and z' in
// registers while it writes x' - so the in-place order does not matter. The sums run in x87 and
// are rounded once at the store; written in double (the dedicated PE's 53-bit precision).
IPlanePolygon* PlanePolygon::fromTransform(const Mat4& transformation) const
{
    std::list<Vec2> transformed;
    Vec2 flat;
    flat.x = 0.0f;
    flat.y = 0.0f;
    Vec3 point;
    point.x = 0.0f;
    point.y = 0.0f;
    point.z = 0.0f;

    PlanePolygon* const polygon = new PlanePolygon();
    const float* const m = transformation.m;
    for (std::list<Vec2>::const_iterator it = m_vertices.begin(); it != m_vertices.end(); ++it)
    {
        vec2ToVec3XZ(*it, point);
        const double y = static_cast<double>(m[9]) * point.z + static_cast<double>(m[5]) * point.y +
                         static_cast<double>(m[1]) * point.x + m[13];
        const double z = static_cast<double>(point.z) * m[10] + static_cast<double>(point.y) * m[6] +
                         static_cast<double>(point.x) * m[2] + m[14];
        point.x = static_cast<float>(static_cast<double>(m[4]) * point.y +
                                     static_cast<double>(m[0]) * point.x +
                                     static_cast<double>(m[8]) * point.z + m[12]);
        point.y = static_cast<float>(y);
        point.z = static_cast<float>(z);
        vec3ToVec2XZ(point, flat);
        transformed.push_back(flat);
    }
    polygon->setVertices(transformed);                                              // +0x0C
    return polygon;
}

// cleared, then refilled from the caller's list.
void PlanePolygon::setVertices(std::list<Vec2>& vertices)
{
    m_vertices.clear();
    m_vertices.assign(vertices.begin(), vertices.end());
}

const std::list<Vec2>& PlanePolygon::getVertices() const
{
    return m_vertices;
}

// `out` cleared, then each corner scaled per axis and appended.
void PlanePolygon::getScaledVertices(std::list<Vec2>& out, const Vec2& scale) const
{
    Vec2 scaled;
    scaled.x = 0.0f;
    scaled.y = 0.0f;
    out.clear();
    for (std::list<Vec2>::const_iterator it = m_vertices.begin(); it != m_vertices.end(); ++it)
    {
        scaled.x = scale.x * it->x;
        scaled.y = it->y * scale.y;
        out.push_back(scaled);
    }
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-07-knowledge]
bool PlanePolygon::intersects(const Vec2& /*a*/, const Vec2& /*b*/) const
{
    BF2_AI_FRONTIER("dice::hfe::PlanePolygon::intersects");
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-07-knowledge]
bool PlanePolygon::isInside(const Vec2& /*a*/, const Vec2& /*b*/) const
{
    BF2_AI_FRONTIER("dice::hfe::PlanePolygon::isInside");
}

} // namespace hfe
} // namespace dice
