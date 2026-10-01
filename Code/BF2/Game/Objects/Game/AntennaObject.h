// Motion-driven antenna with two damped bend axes and a speed-dependent wobble.
// Its visual update replaces the base implementation. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_ANTENNAOBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_ANTENNAOBJECT_H

#include "../Base/SimpleObject.h"

namespace dice {
namespace anim { class Skeleton; }
namespace hfe {
namespace world {

class AntennaObjectTemplate;

class AntennaObject : public SimpleObject
{
public:
    explicit AntennaObject(const AntennaObjectTemplate* objectTemplate);
    virtual ~AntennaObject();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual void  handleVisualUpdate(float, float);

protected:
    /* 0x138 */ int   m_refCount;
    /* 0x13C */ const AntennaObjectTemplate* m_template;
    /* 0x140 */ anim::Skeleton* m_skeleton;

    // The sway state. The two angle/velocity pairs are interleaved as the shipped layout has them
    // (both angles, then both velocities); the template's maxSwayAngle[0] clamps the X angle and
    // maxSwayAngle[1] the Z angle.
    /* 0x144 */ float m_swayAngleX;          // degrees, bend about the local X axis
    /* 0x148 */ float m_swayAngleZ;          // degrees, bend about the local Z axis
    /* 0x14C */ float m_swayVelocityX;       // degrees/second
    /* 0x150 */ float m_swayVelocityZ;
    /* 0x154 */ Vec3  m_previousPosition;    // absolute; all-zero means "not seeded yet"
    /* 0x160 */ Vec3  m_previousVelocity;    // world space, last frame's
    /* 0x16C */ float m_heading;             // degrees in [0, 360), the smoothed lean direction
    /* 0x170 */ float m_wobblePhase;         // degrees, accumulated from speed; drives a sine wobble
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_ANTENNAOBJECT_H
