// Shared null sound and template instances are constructed before main.

#include "NullSound.h"

#include "../Math/Vec3.h"

namespace dice {
namespace hfe {

// The null engine's zero vector backs all listener getters.
NullSoundTemplate::NullSoundTemplate()  { m_refCount = 0; m_field08 = 0; m_field0C = 0; }
#if !defined(BF2_IMAGE_BF2AUDIO)
NullSoundEngine::NullSoundEngine()      { m_zeroVector.x = 0.0f; m_zeroVector.y = 0.0f; m_zeroVector.z = 0.0f; }
#endif

// 10-byte refcount pair on the shared singleton - release decrements and never deletes (10 bytes
// holds no delete path; the object is the process-lifetime shared impl).
void NullSoundTemplate::addRef()  { ++m_refCount; }
void NullSoundTemplate::release() { --m_refCount; }

// The 10/20-byte string readers return empties.
const char*  NullSoundTemplate::getFilename() const        { return ""; }
const char** NullSoundTemplate::getFilenames(int* n) const { if (n) *n = 0; return 0; }

// 11-byte statics: a zero vector each.
const Vec3& NullSoundTemplate::getPosition() const  { static Vec3 z; return z; }
const Vec3& NullSoundTemplate::getDirection() const { static Vec3 z; return z; }

// 0x83DC3xx, 10 B: `mov eax, ds:8AC0DB0h; ret` - every caller shares the one null template.
#if !defined(BF2_IMAGE_BF2AUDIO)
ISoundTemplate* NullSoundEngine::createTemplate() { return g_nullSoundTemplate; }
#endif

// Returns the owning image's shared null sound/template pair.
ISound*         NullSoundTemplate::createSound() { return g_nullSound; }
ISoundTemplate* NullSound::getTemplate() const   { return g_nullSoundTemplate; }
// Shared fallback instances for images without an audio implementation.
#if !defined(BF2_IMAGE_BF2AUDIO)
ISound*         g_nullSound         = new NullSound;
ISoundTemplate* g_nullSoundTemplate = new NullSoundTemplate;
ISoundEngine*   g_nullSoundEngine   = new NullSoundEngine;
#endif

} // namespace hfe
} // namespace dice
