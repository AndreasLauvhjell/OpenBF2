// HUD blink controllers for threshold and float outputs. [INFERRED] Names and source placement.
#include "HudBlinker.h"
#if !DEDICATED

namespace dice { namespace hfe { namespace hud {

HudBlinker::HudBlinker(float* source, bool* output, float maximum, float minimum,
                       float period, float otherPeriod, bool adaptive)
    : m_source(source), m_output(output), m_maximum(maximum), m_minimum(minimum),
      m_period(period), m_otherPeriod(otherPeriod), m_adaptive(adaptive), m_elapsed(0.0f)
{
}

// Preserve strict >, reset-to-zero (no remainder), and the
// source-less oscillator. The x87 keeps the elapsed sum and period in registers
// across the float store; double temporaries retain that excess precision here.
void HudBlinker::update(float dt)
{
    if (!m_output)
        return;

    if (m_source)
    {
        double period = m_period;
        if (m_adaptive)
            period = (double(m_period) - m_otherPeriod) / m_maximum * *m_source + m_otherPeriod;

        const double elapsed = double(dt) + m_elapsed;
        m_elapsed = static_cast<float>(elapsed);
        // The PE's parity branch also takes the invalid arm for unordered values.
        // The adaptive division above happens before the zero-maximum guard.
        if (!(*m_source < m_maximum) || m_maximum == 0.0f || !(*m_source >= 0.0f))
        {
            if (*m_output && elapsed > period)
            {
                m_elapsed = 0.0f;
                *m_output = false;
            }
        }
        else if (elapsed > period)
        {
            *m_output = *m_source > m_minimum && !*m_output;
            m_elapsed = 0.0f;
        }
    }
    else
    {
        const double elapsed = double(dt) + m_elapsed;
        m_elapsed = static_cast<float>(elapsed);
        if (elapsed > m_period)
        {
            *m_output = !*m_output;
            m_elapsed = 0.0f;
        }
    }
}

// 0x771B50 (64 bytes), table 0x92FDD4. The inherited maximum/minimum words serve
// as lower/upper bounds in this variant; keep the shipped adaptive denominator.
HudHighBlinker::HudHighBlinker(float* source, bool* output, float lower, float upper,
                               float period, float otherPeriod, bool adaptive)
    : HudBlinker(source, output, lower, upper, period, otherPeriod, adaptive)
{
}

// 0x771B90 (236 bytes). Unlike HudBlinker, the open interval is (lower, upper).
void HudHighBlinker::update(float dt)
{
    if (!m_output)
        return;

    if (m_source)
    {
        double period = m_period;
        if (m_adaptive)
            period = (double(m_period) - m_otherPeriod) / m_maximum * *m_source + m_otherPeriod;

        const double elapsed = double(dt) + m_elapsed;
        m_elapsed = static_cast<float>(elapsed);
        if (!(*m_source > m_maximum) || m_maximum == 0.0f || !(*m_source >= 0.0f))
        {
            if (*m_output && elapsed > period)
            {
                m_elapsed = 0.0f;
                *m_output = false;
            }
        }
        else if (elapsed > period)
        {
            *m_output = *m_source < m_minimum && !*m_output;
            m_elapsed = 0.0f;
        }
    }
    else
    {
        const double elapsed = double(dt) + m_elapsed;
        m_elapsed = static_cast<float>(elapsed);
        if (elapsed > m_period)
        {
            *m_output = !*m_output;
            m_elapsed = 0.0f;
        }
    }
}

// Initializes the float-output blinker with adaptive mode enabled. [UNRESOLVED] Original enabling expression.
HudFloatBlinker::HudFloatBlinker(bool* gate, float* output, bool gateValue,
                                 float period, bool smooth)
    : HudBlinker(nullptr, nullptr, 0.0f, 0.0f, period, 0.0f, true),
      m_floatOutput(output), m_smooth(smooth), m_direction(-1.0f),
      m_gate(gate), m_gateValue(gateValue)
{
}

// Updates the float blink output. Smooth cycles reverse at >= and discard excess time; discrete cycles turn at >.
// Elapsed time is rounded to float before comparison.
void HudFloatBlinker::update(float dt)
{
    if (!m_floatOutput)
        return;

    const float elapsed = static_cast<float>(double(dt) + m_elapsed);
    const float period = m_period;
    m_elapsed = elapsed;
    if (m_gate && *m_gate != m_gateValue)
    {
        // An inactive gate finishes the fade even in discrete mode, and skips
        // the positive-period guard below. Unordered output also returns here.
        if (!(*m_floatOutput < 1.0f))
            return;
    }
    else
    {
        if (elapsed > period && !m_smooth)
        {
            // The gated discrete mode dereferences m_source although this constructor sets it null.
            // Observed callers use smooth mode; this branch retains the existing defect.
            if ((m_gate && !(*m_source > m_minimum)) || *m_floatOutput > 0.0f)
                *m_floatOutput = 0.0f;
            else
                *m_floatOutput = 1.0f;
            m_elapsed = 0.0f;
            return;
        }
        if (!m_smooth || !(period > 0.0f))
            return;
    }

    if (elapsed >= period)
    {
        m_elapsed = 0.0f;
        m_direction = -m_direction;
    }
    double value = m_elapsed;
    if (!(m_direction > 0.0f))
        value = value * m_direction + period;
    *m_floatOutput = static_cast<float>(value / period);
}

// only constructs the vector. No vptr and no preallocated controllers.
HudBlinkManager::HudBlinkManager() : m_blinkers() {}

// nonvirtual deletion of each controller, then vector destruction.
// In particular, do not turn this into a virtual destructor dispatch.
HudBlinkManager::~HudBlinkManager()
{
    for (auto it = m_blinkers.begin(); it != m_blinkers.end(); ++it)
        delete *it;
}

// ordinary push_back; null is not filtered out.
void HudBlinkManager::add(HudBlinker* blinker)
{
    m_blinkers.push_back(blinker);
}

// tests the current end on every iteration, with no null-element guard.
void HudBlinkManager::update(float dt)
{
    for (auto it = m_blinkers.begin(); it != m_blinkers.end(); ++it)
        (*it)->update(dt);
}

} } }
#endif
