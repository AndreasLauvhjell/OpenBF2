
// Ambience zones ordered so the listener selects the smallest enclosing sphere.
// An explicitly selected trigger overrides the spatial search.


#ifndef SOUND_SOUNDENGINE_SOUNDTRIGGER_H
#define SOUND_SOUNDENGINE_SOUNDTRIGGER_H

#include "../../Math/Vec3.h"

#include <vector>

namespace dice {
namespace hfe {

class Sound;
class ISoundTemplate;

class SoundTriggerManager
{
public:
    SoundTriggerManager();
    ~SoundTriggerManager();

    void   addTrigger(ISoundTemplate* t);           // a 2D, dry Sound of the template
    void   setTrigger(Sound* s);                    // the client's explicit choice
    Sound* getTrigger() const { return m_current; }
    int    numTriggers() const { return (int) m_triggers.size(); }
    void   removeTrigger(ISoundTemplate* t);
    void   removeAll();
    void   sort();                                  // by min distance, largest first
    void   update(const Vec3* listenerPosition);    // drive the ambient player

    /* +0x00 */ // the vector's allocator word
    /* +0x04 */ std::vector<Sound*> m_triggers;
    /* +0x10 */ Sound* m_current;
    /* +0x14 */ bool   m_changed;                   // an explicit trigger was cleared: next fade is forced
};

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_SOUNDTRIGGER_H
