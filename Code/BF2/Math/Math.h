// Math subsystem lifecycle declarations. [INFERRED] Source placement.

#ifndef MATH_MATH_H     // [INFERRED] guard style
#define MATH_MATH_H

namespace dice {
namespace hfe {

// Initialization returns success; shutdown has no result.
bool initMath();
void closeMath();

namespace world { class PlayerControlObject; class GroundContactSound; }

// ELF 83B7F10..83B808C / PE 68D2E0..68D3E6. Native carrier for a borrowed
// target float, current value and proportional rate. Field names are [INFERRED].
class PController
{
public:
    PController();
    PController(const float* target, float rate);
    void init(const float* target, float rate);
    void update(int steps);

private:
    friend class world::PlayerControlObject;
    friend class world::GroundContactSound;
    const float* m_target; // retail +0; borrowed, pointer-sized on native builds
    float m_value;        // retail +4
    float m_rate;         // retail +8
};

// Returns a critically damped step from current toward target and updates velocity in place.
// Uses smoothTime for response duration and dt for elapsed time.
float smooth_in_out(float current, float target, float* velocity, float smoothTime, float dt);

} // namespace hfe
} // namespace dice

#endif // MATH_MATH_H
