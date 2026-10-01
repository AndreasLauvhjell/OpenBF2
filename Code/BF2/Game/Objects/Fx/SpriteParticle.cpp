// Path: [INFERRED], see SpriteParticle.h.
// Own bodies: ELF 0x82117D0..0x821205F; dedicated PE 0x6032A0..0x603509.
// This older particle type has a real dedicated object with intentionally empty update/emission.
#include "SpriteParticle.h"
#include "../../../Core/Ids.h"
#include "../../../IO/Stream/IStream.h"
#include <sstream>

namespace dice { namespace hfe { namespace world {

SpriteParticleTemplate::SpriteParticleTemplate() : m_refCount(1)
{
    updateFlags(0x40, 0);
    m_particleSystem = 0;
}
SpriteParticleTemplate::~SpriteParticleTemplate() {}
void SpriteParticleTemplate::addRef() { ++m_refCount; }
int SpriteParticleTemplate::getRef() { return m_refCount; }
int SpriteParticleTemplate::release()
{
    const int count = --m_refCount;
    if (!count) delete this;
    return count;
}
void* SpriteParticleTemplate::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectTemplate)
        return const_cast<SpriteParticleTemplate*>(this);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<SpriteParticleTemplate*>(this));
    return 0;
}
unsigned int SpriteParticleTemplate::getClassID() const { return CID_SpriteParticleTemplate; }
void* SpriteParticleTemplate::queryImplementation(unsigned int cid)
{
    return cid == CID_SpriteParticleTemplate || cid == CID_ObjectTemplate ? this : 0;
}
IObject* SpriteParticleTemplate::createObject() const { return new SpriteParticle(this); }

// / PE secondary IConsoleSaveable +0x10 -> 0x603340.
// Preserve the extra zero-length write after the base script and its checked result.
bool SpriteParticleTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (!ObjectTemplate::makeScript(stream)) return false;
    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

SpriteParticle::SpriteParticle(const SpriteParticleTemplate* objectTemplate)
    : Object(objectTemplate), m_particleSystem(0), m_visualBoundingRadius(1.0f),
      m_removeParticleSystem(false)
{
    updateFlags(8, 0);
}
SpriteParticle::~SpriteParticle() {}
// /0x82118C0 and the dedicated PE contain no simulation/emission work.
void SpriteParticle::handleUpdate(float, unsigned int) {}
void SpriteParticle::addParticle(const Vec3&, const Vec3&, float) {}
void SpriteParticle::removeParticleSystem() { m_removeParticleSystem = true; }
float SpriteParticle::getVisualBoundingRadius() const { return m_visualBoundingRadius; }

} } }
