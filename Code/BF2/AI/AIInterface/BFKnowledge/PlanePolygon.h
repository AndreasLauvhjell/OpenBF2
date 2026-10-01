// Planar polygon geometry and transforms.
// [FRONTIER] Intersection and containment operations remain incomplete.
// [INFERRED] Source placement.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-07-knowledge] Complete and validate the remaining PlanePolygon operations.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_PLANEPOLYGON_H
#define AI_AIINTERFACE_BFKNOWLEDGE_PLANEPOLYGON_H

#include <list>

#include "../../../Math/Mat4.h"
#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {

class BoundingBox;

class IPlanePolygon
{
public:
    // - comdat copies after the TU's static initialiser, i.e. inline.
    virtual ~IPlanePolygon() {}                                                 // +0x00/+0x04

    // A new polygon (the caller owns it): every corner lifted to XZ, transformed, projected back.
    virtual IPlanePolygon* fromTransform(const Mat4& transformation) const = 0; // +0x08
    virtual void setVertices(std::list<Vec2>& vertices) = 0;                    // +0x0C
    virtual const std::list<Vec2>& getVertices() const = 0;                     // +0x10
    virtual void getScaledVertices(std::list<Vec2>& out, const Vec2& scale) const = 0;   // +0x14
    virtual bool intersects(const Vec2& a, const Vec2& b) const = 0;            // +0x18
    virtual bool isInside(const Vec2& a, const Vec2& b) const = 0;              // +0x1C
};

class PlanePolygon : public IPlanePolygon
{
public:
    PlanePolygon();
    explicit PlanePolygon(const std::list<Vec2>& vertices);
    PlanePolygon(BoundingBox* box, const Mat4& transformation); // [FRONTIER]
    virtual ~PlanePolygon();

    virtual IPlanePolygon* fromTransform(const Mat4& transformation) const;
    virtual void setVertices(std::list<Vec2>& vertices);
    virtual const std::list<Vec2>& getVertices() const;
    virtual void getScaledVertices(std::list<Vec2>& out, const Vec2& scale) const;
    virtual bool intersects(const Vec2& a, const Vec2& b) const;   // [FRONTIER]
    virtual bool isInside(const Vec2& a, const Vec2& b) const;     // [FRONTIER]

private:
    /* +0x04 */ std::list<Vec2> m_vertices;
};

} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_PLANEPOLYGON_H
