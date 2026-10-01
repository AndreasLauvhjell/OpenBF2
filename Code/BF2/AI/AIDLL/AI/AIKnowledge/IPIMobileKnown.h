// Cached mobile information. [INFERRED] Source placement beside IPIMobile.
#ifndef AI_AIDLL_AI_AIKNOWLEDGE_IPIMOBILEKNOWN_H
#define AI_AIDLL_AI_AIKNOWLEDGE_IPIMOBILEKNOWN_H

#include "IPIMobile.h"

namespace dice {
namespace hfe {
namespace ai {

class IPIMobileKnown : public IPIMobile
{
public:
    explicit IPIMobileKnown(Information* information);
    virtual ~IPIMobileKnown();

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

private:
    void stealLastValid();

    /* +0x48 */ Vec3 m_speed;
    /* +0x54 */ Vec3 m_rotationalSpeed;
    // Written to zero by the constructor but not read by any audited body.
    /* +0x60 */ Vec3 m_unresolvedVector;
    // Explicit because the shipped ELF's 4-byte double alignment would not insert it for us.
    // The constructor deliberately leaves it untouched.
    /* +0x6C */ unsigned char m_unwritten6C[4];
    /* +0x70 */ double m_time;
    // Both are deliberately left uninitialized by the constructor.
    /* +0x78 */ unsigned int m_sector;
    /* +0x7C */ int m_lastPassedPortal;
    /* +0x80 */ bool m_hasContact;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_IPIMOBILEKNOWN_H
