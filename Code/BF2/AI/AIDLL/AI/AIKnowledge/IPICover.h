// Cover information with owned polygon data. [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-07-knowledge] The non-cover constructor path must keep stopping
// until PlanePolygon(BoundingBox*, const Mat4&) is implemented.
#ifndef AI_AIDLL_AI_AIKNOWLEDGE_IPICOVER_H
#define AI_AIDLL_AI_AIKNOWLEDGE_IPICOVER_H

#include <vector>

#include "../../../../Math/Vec3.h"
#include "InformationPlugIn.h"

namespace dice {
namespace hfe {

class IPlanePolygon;

namespace ai {

class IPICover : public InformationPlugIn
{
public:
    explicit IPICover(Information* information);                        // ELF
    virtual ~IPICover();                                                 // ELF

    // IPICover adds no virtual query slots. Its table ends after these inherited overrides.
    virtual int getType();                                               // +2: 1
    virtual void setTime();                                              // +3: current game time
    virtual void setTime(double time);                                   // +4: time ignored

    IPlanePolygon* getLocalOutline();
    IPlanePolygon* getOutline();
    float getVolume();
    float getCoverValue();
    float getHeight();
    unsigned int getPortalCount() const;
    unsigned int getPortalVertexCount(unsigned int portal) const;
    const Vec3& getPortalVertex(unsigned int portal, unsigned int vertex) const;
    void getPortal(unsigned int portal, std::vector<Vec3>& out) const;
    const Vec3& getPortalCenter(unsigned int portal) const;
    Vec3 getPortalExtentDirection(unsigned int portal) const;
    const Vec3& getPortalNormal(unsigned int portal) const;
    float getPortalXZExtentR(unsigned int portal) const;
    float getPortalXZExtentL(unsigned int portal) const;
    float getTransparency();
    int getPathfindingPlaneCount() const;
    const Vec3& getPathfindingPlaneCorner(int plane, int corner) const;

private:
    /* +0x08 */ IPlanePolygon* m_outline;        // owned; [INFERRED] name
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_IPICOVER_H
