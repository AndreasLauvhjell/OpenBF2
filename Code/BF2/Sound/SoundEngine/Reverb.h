
// Maintains preset and active EAX listener reverb settings.
// Listener level adjusts the preset room level; a dirty flag defers device updates.


#ifndef SOUND_SOUNDENGINE_REVERB_H
#define SOUND_SOUNDENGINE_REVERB_H

#include "BfOpenAL.h"

namespace dice {
namespace hfe {

class ISoundProvider;

class Reverb
{
public:
    Reverb();                                       // both blocks zeroed, dirty
    bool load(const char* name);                    // false = not found / too big
    void setRoomLevel(float volume);                // listener reverb 0..1 into lRoom
    void apply(ISoundProvider* provider);           // EAXSet when dirty
    void markDirty() { m_dirty = true; }

    EAXLISTENERPROPERTIES m_preset;                 // +0x00
    EAXLISTENERPROPERTIES m_current;                // +0x70
    bool                  m_dirty;                  // +0xE0
};

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_REVERB_H
