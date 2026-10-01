// Path:     [INFERRED] - beside its header. Contents: recovered from bf2.i64.
// when the deep drive reached "AIR_F35B_RGear" (CID 38062).

#include "LandingGearTemplate.h"
#include "../../IO/Stream/IStream.h"
#include <sstream>

#include "../../Core/Ids.h"
#include "Game/LandingGear.h"

namespace dice {
namespace hfe {
namespace world {

bool LandingGearTemplate::m_forceGearUp = false;               // .bss zero-init

// RotationalBundleTemplate base, the three vptr stores, then seven fields. No updateFlags.
LandingGearTemplate::LandingGearTemplate()
{
    m_gearDownHeight = 30.0f;
    m_gearUpHeight = 20.0f;
    m_gearDownSpeed = 60.0f;
    m_gearUpSpeed = 70.0f;
    m_gearUpDelay = 0.0f;
    m_gearDownDelay = 0.0f;
    m_invertGear = false;
}

// (D1): vptr resets + the base chain.
LandingGearTemplate::~LandingGearTemplate()
{
}

unsigned int LandingGearTemplate::getClassID() const
{
    return CID_LandingGearTemplate;
}

// the standard one-CID hop into the base's chain (a literal tail-jump in the ELF).
void* LandingGearTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_LandingGearTemplate)
        return this;
    return RotationalBundleTemplate::queryImplementation(classId);
}

// operator new(0x1CC), then LandingGear(this). The PE equivalent at 0x60B7F0
// allocates 0x1E4 because its base layout differs. Typed new keeps this leaf coupled to the active
// hierarchy instead of copying either reference binary's absolute allocation size.
IObject* LandingGearTemplate::createObject() const
{
    return new LandingGear(this);
}

// ELF 82A8E40 / dedicated PE 60B820: construct the stream before calling the base.
bool LandingGearTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream out;
    if (!RotationalBundleTemplate::makeScript(stream)) return false;
    if (m_gearDownHeight != 30.0f) out << "ObjectTemplate.setGearDownHeight " << m_gearDownHeight << std::endl;
    if (m_gearUpHeight != 20.0f) out << "ObjectTemplate.setGearUpHeight " << m_gearUpHeight << std::endl;
    if (m_gearDownSpeed != 60.0f) out << "ObjectTemplate.setGearDownSpeed " << m_gearDownSpeed << std::endl;
    if (m_gearUpSpeed != 70.0f) out << "ObjectTemplate.setGearUpSpeed " << m_gearUpSpeed << std::endl;
    if (m_gearUpDelay != 0.0f) out << "ObjectTemplate.setGearUpDelay " << m_gearUpDelay << std::endl;
    if (m_gearDownDelay != 0.0f) out << "ObjectTemplate.setGearDownDelay " << m_gearDownDelay << std::endl;
    if (m_invertGear) out << "ObjectTemplate.invertGear " << m_invertGear << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace world
} // namespace hfe
} // namespace dice
