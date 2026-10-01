// Path:     [INFERRED] - beside its header. Contents: recovered from bf2.i64.
// The 96 named accessors are wrappers that virtual-dispatch the six generic cores - that is the
// ELF's own structure (`call [vptr+0x198..0x1AC]` in every wrapper), reproduced here as plain
// virtual calls. The (property, affector) constants each wrapper passes were spot-checked at both
// ends of the table (Alpha/Speed = 0,0; StartAge/Altitude = 4,3; SoundIntensity/SpeedInDof = 5,5).
// The BF2_EP_WRAPPERS macro is OUR compression of 16 identical-shaped one-liners per property -
// internals-free under the fidelity rule; the dispatch and constants are the recovered facts.

#include "EffectProperties.h"
#include "../../../IO/Stream/IStream.h"
#include <sstream>

namespace dice {
namespace hfe {
namespace world {

// / 0x898853C - the attested tables, in attested order.
const char* const EffectPropertyNames[6] =
    { "AlphaIntensity", "EmitScaleFactor", "EmitSpeedScale", "AgeScaleFactor", "StartAge",
      "SoundIntensity" };
const char* const EffectAffectorNames[6] =
    { "Speed", "RotationalSpeed", "Acceleration", "Altitude", "Engine", "SpeedInDof" };

// ctor - 0x81FD920, 235 B, raw-disasm verified: zero everything, then seed the six ranges -
// all (1,1) except StartAge (0,0). (The ELF also re-zeroes the property array in a second pass
// while filling used[]; one pass is the same observable.)
EffectProperties::EffectProperties()
{
    for (int p = 0; p < 6; ++p)
    {
        m_range[p].x = 1.0f;
        m_range[p].y = 1.0f;
        for (int a = 0; a < 6; ++a)
        {
            m_property[p][a].x = 0.0f;
            m_property[p][a].y = 0.0f;
        }
        m_used[p] = 0;
    }
    m_range[EP_StartAge].x = 0.0f;
    m_range[EP_StartAge].y = 0.0f;
}

EffectProperties::~EffectProperties()
{
}

// Full instructions read in both ports. Used flags and base ranges do not participate:
// any nonzero (including unordered/NaN) affector endpoint enables the physics-driven path.
bool EffectProperties::effectPropertiesUsed() const
{
    for (unsigned int property = 0; property < 6; ++property)
        for (unsigned int affector = 0; affector < 6; ++affector)
            if (m_property[property][affector].x != 0.0f ||
                m_property[property][affector].y != 0.0f)
                return true;
    return false;
}

// updateVar - 0x81FDB60, 130 B: ramp `value` across `range`, clamped to [0,1]; a descending range
// (x >= y) inverts the ramp. Branch structure transcribed exactly (the comparisons' orientation
// is what decides the NaN edge cases).
float EffectProperties::updateVar(float value, const Vec2& range)
{
    if (range.y > range.x)
    {
        if (value >= range.y)
            return 1.0f;
        if (range.x > value)
            return 0.0f;
        const float t = (value - range.x) / (range.y - range.x);
        if (t < 0.0f)
            return 0.0f;
        if (t > 1.0f)
            return 1.0f;
        return t;
    }
    if (value >= range.x)
        return 0.0f;
    if (range.y > value)
        return 1.0f;
    const float t = (value - range.y) / (range.x - range.y);
    float clamped = 0.0f;
    if (t >= 0.0f)
        clamped = (t <= 1.0f) ? t : 1.0f;
    return 1.0f - clamped;
}

// each - raw table index, NO bounds check, as shipped.
const char* EffectProperties::getEffectPropertyName(EffectProperty p) { return EffectPropertyNames[p]; }
const char* EffectProperties::getEffectAffectorName(EffectAffector a) { return EffectAffectorNames[a]; }

// the six generic cores
void EffectProperties::setPropertyRange(EffectProperty p, Vec2 v)        { m_range[p] = v; }
Vec2 EffectProperties::getPropertyRange(EffectProperty p) const          { return m_range[p]; }
void EffectProperties::setPropertyUsed(EffectProperty p, bool used)      { m_used[p] = used; }
bool EffectProperties::getPropertyUsed(EffectProperty p) const           { return m_used[p] != 0; }
void EffectProperties::setProperty(EffectProperty p, EffectAffector a, Vec2 v) { m_property[p][a] = v; }
Vec2 EffectProperties::getProperty(EffectProperty p, EffectAffector a) const   { return m_property[p][a]; }

// the 96 wrappers: one 16-method block per property
#define BF2_EP_WRAPPERS(Name, Prop)                                                               \
    void EffectProperties::set##Name##Range(Vec2 v)  { setPropertyRange(Prop, v); }               \
    Vec2 EffectProperties::get##Name##Range() const  { return getPropertyRange(Prop); }           \
    void EffectProperties::set##Name##Used(bool v)   { setPropertyUsed(Prop, v); }                \
    bool EffectProperties::get##Name##Used() const   { return getPropertyUsed(Prop); }            \
    void EffectProperties::set##Name##FromSpeed(Vec2 v)           { setProperty(Prop, EA_Speed, v); }            \
    Vec2 EffectProperties::get##Name##FromSpeed() const           { return getProperty(Prop, EA_Speed); }        \
    void EffectProperties::set##Name##FromRotationalSpeed(Vec2 v) { setProperty(Prop, EA_RotationalSpeed, v); }  \
    Vec2 EffectProperties::get##Name##FromRotationalSpeed() const { return getProperty(Prop, EA_RotationalSpeed); } \
    void EffectProperties::set##Name##FromAcceleration(Vec2 v)    { setProperty(Prop, EA_Acceleration, v); }     \
    Vec2 EffectProperties::get##Name##FromAcceleration() const    { return getProperty(Prop, EA_Acceleration); } \
    void EffectProperties::set##Name##FromAltitude(Vec2 v)        { setProperty(Prop, EA_Altitude, v); }         \
    Vec2 EffectProperties::get##Name##FromAltitude() const        { return getProperty(Prop, EA_Altitude); }     \
    void EffectProperties::set##Name##FromEngine(Vec2 v)          { setProperty(Prop, EA_Engine, v); }           \
    Vec2 EffectProperties::get##Name##FromEngine() const          { return getProperty(Prop, EA_Engine); }       \
    void EffectProperties::set##Name##FromSpeedInDof(Vec2 v)      { setProperty(Prop, EA_SpeedInDof, v); }       \
    Vec2 EffectProperties::get##Name##FromSpeedInDof() const      { return getProperty(Prop, EA_SpeedInDof); }

BF2_EP_WRAPPERS(AlphaIntensity,  EP_AlphaIntensity)
BF2_EP_WRAPPERS(EmitScaleFactor, EP_EmitScaleFactor)
BF2_EP_WRAPPERS(EmitSpeedScale,  EP_EmitSpeedScale)
BF2_EP_WRAPPERS(AgeScaleFactor,  EP_AgeScaleFactor)
BF2_EP_WRAPPERS(StartAge,        EP_StartAge)
BF2_EP_WRAPPERS(SoundIntensity,  EP_SoundIntensity)

#undef BF2_EP_WRAPPERS

// ELF 81FDC80 / dedicated PE 628D90. Ranges and used flags alone do not enable
// the block until an affector or a forced-used flag is present. An empty block still writes.
bool EffectProperties::makeScript(io::IStream* stream)
{
    std::ostringstream out;
    bool enabled = effectPropertiesUsed();
    if (!enabled)
        for (unsigned int property = 0; property < 6; ++property)
            if (m_used[property]) { enabled = true; break; }
    if (enabled)
    {
        out << "rem begin Auto-generated effectProperties-part." << std::endl;
        for (unsigned int property = 0; property < 6; ++property)
        {
            out << "ObjectTemplate." << EffectPropertyNames[property] << "Range "
                << m_range[property].x << '/' << m_range[property].y << std::endl;
            out << "ObjectTemplate." << EffectPropertyNames[property] << "Used "
                << (m_used[property] != 0) << std::endl;
            for (unsigned int affector = 0; affector < 6; ++affector)
            {
                const Vec2& value = m_property[property][affector];
                if (value.x != 0.0f || value.y != 0.0f)
                    out << "ObjectTemplate." << EffectPropertyNames[property] << "From"
                        << EffectAffectorNames[affector] << ' ' << value.x << '/' << value.y << std::endl;
            }
        }
        out << "rem end Auto-generated effectProperties-part." << std::endl;
    }
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace world
} // namespace hfe
} // namespace dice
