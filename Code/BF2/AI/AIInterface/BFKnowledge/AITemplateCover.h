// Script-defined cover properties used to create an AI cover plug-in.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATECOVER_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATECOVER_H

#include "AITemplatePlugIn.h"

namespace dice {
namespace hfe {

// Portal-mesh and plane-polygon interfaces are declared in PlanePolygon.h.
class IPlanePolygon;

namespace ai {

class AITemplateCover : public AITemplatePlugIn
{
public:
    // The portal block at +0x18 [INFERRED names]. Plain arrays; the Cover's destructor frees all
    // four with delete[] (each null-tested) and then the block itself.
    struct CoverPortals
    {
        /* +0x0 */ float* m_xzExtentR;
        /* +0x4 */ float* m_xzExtentL;
        /* +0x8 */ Vec3*  m_centers;
        /* +0xC */ Vec3*  m_normals;
    };

    AITemplateCover();
    virtual ~AITemplateCover();

    virtual IAIObjectPlugIn* createPlugIn(IAIObject* object, AITemplate* aiTemplate);
    virtual int getId();                                // returns 2

    void initFromObject(IAIObject* object);
    const Vec3& getPortalCenter(unsigned int portal) const;
    const Vec3& getPortalNormal(unsigned int portal) const;
    float getPortalXZExtentR(unsigned int portal) const;
    float getPortalXZExtentL(unsigned int portal) const;
    const Vec3& getPathfindingPlaneCorner(int plane, int corner) const;

    /* +0x04 */ IPlanePolygon* m_polygon;
    /* +0x08 */ float m_coverValue;
    /* +0x0C */ float m_height;
    /* +0x10 */ bool  m_initialized;
    /* +0x14 */ float m_transparency;
    /* +0x18 */ CoverPortals* m_portals;
    /* +0x1C */ int   m_pathfindingPlaneCount;
    /* +0x20 */ Vec3* m_pathfindingPlaneCorners;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATECOVER_H
