// Generic engine for aircraft, boats and tracked vehicles, with physics and contact feedback.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_ENGINE_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_ENGINE_H

#include "EngineBase.h"

namespace dice {
namespace hfe {
namespace world {

class Engine : public EngineBase
{
public:
    explicit Engine(const EngineTemplate* objectTemplate);
    virtual ~Engine();

    virtual void  handleUpdate(float, unsigned int);
    virtual void  reset();
    virtual float getCurrentTorque() const;
    virtual void  updatePhysics(float dt);                                  // (+0x1B8)
    virtual float getCurrentRatio() const;                                  // (+0x1BC)

    void  feedbackLoop(Vec3 force, const Vec3& direction);
    float getAirDensity(float height) const;
    float forceIntoEngine(float input);
    void  debugDraw();
    void  resetTrackFriction();
    void  setTrackHasContact(float side);
    float getSlope();

    friend class ResponsePhysics;               // addFriction's tracked arm: the +0x210 / +0x214 friction mean

protected:
    /* 0x210 */ float    m_dword210;             // a float: the tracks' mean-friction sum (ResponsePhysics::addFriction) [UNRESOLVED name]
    /* 0x214 */ int      m_dword214;             // ... and how many wheels summed into it
    /* 0x218 */ unsigned m_trackContact;
    /* 0x21C */ float    m_slope;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_ENGINE_H
