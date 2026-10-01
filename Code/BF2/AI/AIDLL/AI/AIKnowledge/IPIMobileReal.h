// Live mobile information. [INFERRED] Source placement beside IPIMobile.
#ifndef AI_AIDLL_AI_AIKNOWLEDGE_IPIMOBILEREAL_H
#define AI_AIDLL_AI_AIKNOWLEDGE_IPIMOBILEREAL_H

#include "IPIMobile.h"

namespace dice {
namespace hfe {
namespace ai {

class IPIMobileReal : public IPIMobile
{
public:
    explicit IPIMobileReal(Information* information);
    virtual ~IPIMobileReal();

    virtual void setTime();
    virtual void setTime(double time);
    virtual void setHasContact();
    virtual void setLostContact();
    virtual const Vec3& getSpeed();
    virtual const Vec3& getRotationalSpeed();
    virtual float getSpeedMagnitude();
    virtual float getSoundSphereRadius() const;
    virtual void getPositionEstimate(Vec3& position);
    virtual void getPositionEstimate(double time, Vec3& position);
    virtual unsigned int getSector() const;
    virtual int getLastPassedPortal() const;
    virtual void setLastValidInfantryPos(const ValidPos& position);
    virtual void setLastValidVehiclePos(const ValidPos& position);
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_IPIMOBILEREAL_H
