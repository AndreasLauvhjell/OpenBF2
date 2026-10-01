// Exposes cover geometry for an AI object.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTCOVER_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTCOVER_H

#include "IAIObjectPlugIn.h"

namespace dice {
namespace hfe {
namespace ai {

class AITemplateCover;

class AIObjectCover : public IAIObjectCover
{
public:
    AIObjectCover(IAIObject* object, AITemplateCover* aiTemplate);
    virtual ~AIObjectCover();

    virtual int getId() const;                                  // +0x08
    virtual void createInformationPlugIn(unsigned int handle, bool real);   // +0x0C
    virtual void init();                                        // +0x10
    virtual void update();                                      // +0x14, - empty

    virtual IPlanePolygon* getObjectOutline(Mat4& transformation);          // +0x18
    virtual IPlanePolygon* getObjectOutline();                  // +0x1C
    virtual IPlanePolygon* getLocalObjectOutline();             // +0x20
    virtual float getCoverValue() const;                        // +0x24
    virtual float getHeight() const;                            // +0x28
    virtual float getTransparency() const;                      // +0x2C
    virtual unsigned int getPortalCount() const;                // +0x30
    virtual unsigned int getPortalVertexCount(unsigned int portal) const;   // +0x34
    virtual const Vec3& getPortalVertex(unsigned int portal, unsigned int vertex) const;   // +0x38
    virtual void getPortal(unsigned int portal, std::vector<Vec3>& out) const;   // +0x3C
    virtual const Vec3& getPortalCenter(unsigned int portal) const;         // +0x40
    virtual const Vec3& getPortalNormal(unsigned int portal) const;         // +0x44
    virtual float getPortalXZExtentR(unsigned int portal) const;            // +0x48
    virtual float getPortalXZExtentL(unsigned int portal) const;            // +0x4C
    virtual int getPathfindingPlaneCount() const;               // +0x50
    virtual const Vec3& getPathfindingPlaneCorner(int plane, int corner) const;   // +0x54

private:
    /* +0x08 */ AITemplateCover* m_template;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTCOVER_H
