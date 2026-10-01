// Math initialization and proportional control.


#include "Math.h"

#include <cmath>

namespace dice {
namespace hfe {


// Initialization succeeds without allocating resources.

bool initMath()
{
    return true;
}


// No resources require shutdown.

void closeMath()
{
}

PController::PController() { init(0, 0.0f); }
PController::PController(const float* target, float rate) { init(target, rate); }

// ELF 83B7F90 / PE 68D2E0. The constructors inline or forward this same initialization.
void PController::init(const float* target, float rate)
{
    m_target = target;
    m_rate = rate;
    m_value = target ? *target : 0.0f;
}

// Does nothing without a target. Thresholds are strict; the negative clamp selects error for NaN.
// Negative steps and rates are accepted.
void PController::update(int steps)
{
    if (!m_target)
        return;
    const float error = *m_target - m_value;
    if (std::fabs(error) < 0.0000099999997f)
        return;

    float change;
    if (error > 0.0f)
    {
#if defined(_WIN32)
        change = static_cast<float>(static_cast<double>(steps) * m_rate * error);
#else
        change = static_cast<float>(steps) * error * m_rate;
#endif
        if (change > error) change = error;
    }
    else if (error < 0.0f)
    {
#if defined(_WIN32)
        change = static_cast<float>(static_cast<double>(steps) * m_rate * error);
#else
        change = static_cast<float>(steps) * error * m_rate;
#endif
        if (!(error < change)) change = error;
    }
    else
        return;

    m_value += change;
    if (std::fabs(m_value) < 0.000099999997f)
        m_value = 0.0f;
}

// see the header.
float smooth_in_out(float current, float target, float* velocity, float smoothTime, float dt)
{
    const float omega = 2.0f / smoothTime;
    const float x = omega * dt;
    const float exp = 1.0f / (1.0f + x + 0.47999999f * x * x + 0.235f * x * x * x);
    float change = current - target;
    if (std::fabs(change) < 0.000099999997f)
        change = 0.0f;
    const float temp = (*velocity + omega * change) * dt;
    *velocity = (*velocity - omega * temp) * exp;
    if (std::fabs(*velocity) < 0.000099999997f)
        *velocity = 0.0f;
    return target + (change + temp) * exp;
}

} // namespace hfe
} // namespace dice
