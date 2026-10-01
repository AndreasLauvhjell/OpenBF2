// Rotating aerodynamic surface backed by PhysicsWing, also used for water rudders.
// The original source path is Game/Objects/Vehicle/Wing.cpp.

#ifndef GAME_OBJECTS_GAME_WING_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_WING_H

#include "RotationalBundle.h"

namespace dice {
namespace hfe {
namespace world {

class WingTemplate;

class Wing : public RotationalBundle
{
public:
    explicit Wing(const WingTemplate* objectTemplate);
    virtual ~Wing();

    virtual void init();
    virtual void handleUpdate(float, unsigned int);                          // written
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);

    float calculateNeutralLift(const Vec3& airflow, float pitchDegrees, float scale);

    friend class PhysicsWing;                                                // writes m_liftRegulator

protected:
    /* 0x1B0 */ const WingTemplate* m_template;
    /* 0x1B4 */ float m_liftRegulator; // Clamped [-1, 1] regulator output from PhysicsWing::updatePhysics. [CHOSEN name]
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_WING_H
