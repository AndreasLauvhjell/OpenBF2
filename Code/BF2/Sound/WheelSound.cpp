// Wheel rotation sound setup and updates.

#include "WheelSound.h"

#include <cmath>
#include <string>

#include "NullSound.h"                                   // ISound
#include "ObjectSoundNode.h"                             // ObjectSoundNodeFixed
#include "SoundObjectTemplate.h"                         // initTemplates' addSoundTemplate product
#include "../Game/Objects/Game/Spring.h"                 // m_spring's +0x164 rotation speed
#include "../Game/Objects/IObject.h"                     // getAbsoluteTransformation (the node)
#include "../Game/Objects/TemplateLeaves.h"              // SpringTemplate - createSoundFromTemplate

namespace dice {
namespace hfe {

const Mat4& ObjectSoundNodeFixed::getTransform()
{
    static const Mat4 s_zero = Mat4();                                                // [CHOSEN] for the ELF's null reference
    if (m_object)
        return m_object->getAbsoluteTransformation();                                 // IObject +0x84
    return s_zero;
}
const Math::Vec3* ObjectSoundNodeFixed::getVelocity()   { return 0; }

// Creates a Rotation sound parented to the spring's fixed node; pitch and volume share one value.
WheelSound::WheelSound(const world::SpringTemplate* springTemplate, world::Spring* spring)
{
    m_spring = spring;                                                                // +0x04
    m_node = new ObjectSoundNodeFixed(spring);                                        // +0x08 (operator new(8))
    m_value = 0.0f;                                                                   // +0x0C
    m_sound = static_cast<ISound*>(springTemplate->createSoundFromTemplate(std::string("Rotation"), spring));   // +0x00
    if (m_sound)
    {
        m_sound->setParent(m_node);                                                   // +0x3C
        m_sound->setVolumeEvaluator(0, &m_value);                                     // +0x44
        m_sound->setPitchEvaluator(0, &m_value);                                      // +0x4C
    }
}

WheelSound::~WheelSound()
{
    if (m_sound)
        m_sound->release();                                                           // +0x04
    delete m_node;                                                                    // operator delete only - no dtor slot
}

// (the argument is not read).
void WheelSound::update(float)
{
    if (!m_sound)
        return;
    float v = std::fabs(0.057295777f * m_spring->m_rotationSpeed);                    // spring +0x164
    if (v < 0.0f)      v = 0.0f;
    else if (v > 1.0f) v = 1.0f;
    m_value = v;
    if (m_sound->isPlaying())                                                         // +0x38
    {
        if (m_value < 0.02f)
            m_sound->stop();                                                          // +0x30
    }
    else if (m_value > 0.050000001f)
    {
        m_sound->play();                                                              // +0x10
    }
}

// Adds a non-looping 3D Rotation sound template. The second argument is unused.
void WheelSound::initTemplates(world::SpringTemplate* t, unsigned int)
{
    SoundObjectTemplate* s =
        static_cast<SoundObjectTemplate*>(t->addSoundTemplate("Rotation"));
    s->impl()->setIs3dSound(true);      // impl() +0x150; setIs3dSound +0x68
    s->impl()->setLoopCount(0);         // setLoopCount   +0x18
}

} // namespace hfe
} // namespace dice
