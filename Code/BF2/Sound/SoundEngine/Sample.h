// [INFERRED] Class name and source placement.

// One playing voice. A game-facing Sound may own two simultaneous sample instances.


#ifndef SOUND_SOUNDENGINE_SAMPLE_H
#define SOUND_SOUNDENGINE_SAMPLE_H

#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class Sound;
class SoundFile;

// Scheduled sound sample.
class Sample
{
public:
    Sample();
    virtual ~Sample();

    virtual void  releaseSource() = 0;                              // +0x04
    virtual int   getState() = 0;                                   // +0x08
    virtual const Vec3* getPosition(Vec3* out) = 0;                 // +0x0C
    virtual const Vec3* getPositionRef() = 0;                       // +0x10
    virtual int   start(int startSample, int loops) = 0;            // +0x14
    virtual void  update(int steps) = 0;                            // +0x18
    virtual bool  stop(bool force) = 0;                             // +0x1C
    virtual bool  isStopped() = 0;                                  // +0x20
    virtual bool  unresolvedSlot9(int) { return false; }            // +0x24 [UNRESOLVED]

    // tell the Sound this sample is gone (Sound::detachSample) and forget it.
    void       detachFromSound();
    // Reports missing ownership and the owner's 3D setting.
    bool       hasNoSound() const { return m_sound == 0; }
    bool       is3D() const;
    // Returns the selected file and its length through the owning template.
    int        getLengthFrames() const;
    SoundFile* getSoundFile() const;
    // where inside the file's loop block the sample is, 0..1, given the block
    // that is playing and the frames elapsed since start; 1.0 past the loop end, 0.0 before the
    // loop start. Each time the value wraps (a loop completed) both envelope-row seeds below tick,
    // so every pass through the loop draws a fresh randomised envelope row.
    float      loopProgress(int blockStart, int elapsedFrames);

    Sound*         m_sound;          // +0x04 the owner, or 0 once detached
    int            m_startFrame;     // +0x08 engine frame the sample was (re)scheduled at
    int            m_readyFrame;     // +0x0C engine frame it may start at (the delay heap's key)
    float          m_priority;       // +0x10 BfSoundEngine::computePriorities writes it
    // Seeded together by schedule and advanced on loop wrap; low six bits select pitch/volume rows.
    // [INFERRED] Field names.
    unsigned char  m_pitchSeed;      // +0x14
    unsigned char  m_volumeSeed;     // +0x15
    short          m_index;          // +0x16 which of the template's files this plays
    bool           m_started;        // +0x18 was ever handed a voice
    bool           m_pending;        // +0x19 waiting for a retrigger slot decision
    float          m_progress;       // +0x1C last loopProgress answer
};

// ds:live Sample objects. The stats block reports it.
extern int g_sampleCount;

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_SAMPLE_H
