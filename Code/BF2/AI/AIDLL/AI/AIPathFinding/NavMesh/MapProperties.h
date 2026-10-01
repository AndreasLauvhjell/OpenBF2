// Map properties used by navigation-mesh loading and queries.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIPATHFINDING_NAVMESH_MAPPROPERTIES_H
#define AI_AIDLL_AI_AIPATHFINDING_NAVMESH_MAPPROPERTIES_H

#include <set>

#include "../../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class MapProperties
{
public:
    MapProperties();
    ~MapProperties();

    // Tests slope using normal.y and ignores the material argument.
    // Callers must supply a unit normal.
    bool isValidGroundMesh(const Vec3&, unsigned char) const;

    void setMaxSlope(float);            // - , stores the cosine
    float getMaxSlope() const;          // - , returns the angle
    void setHeadClearance(float);
    float getHeadClearance() const;
    void setRadius(float);
    float getRadius() const;
    void setAllowedHeightDiff(float);

    // MapContainer reads allowed height difference directly; this class provides only a setter.

    // Offsets describe the original 28-byte ELF object; the native set layout may differ.

    // [INFERRED] Material set; implemented behavior only constructs and destroys it.
    /*  +0 */ std::set<unsigned char> m_materials;
    /* +12 */ float m_maxSlopeCos;          // = 0.0f -> cos 0 == "any upward normal passes"
    /* +16 */ float m_radius;               // = 0.0f
    /* +20 */ float m_headClearance;        // = 2.0f
    /* +24 */ float m_allowedHeightDiff;    // = 2.5f
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPATHFINDING_NAVMESH_MAPPROPERTIES_H
