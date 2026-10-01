// [INFERRED] Class name and source placement.

// Cross-fades ambient sounds, retaining the current, incoming and pending voices.


#ifndef SOUND_SOUNDENGINE_AMBIENT_H
#define SOUND_SOUNDENGINE_AMBIENT_H

#include "../NullSound.h"
#include "../ISoundObstructionEvaluator.h"

namespace dice {
namespace hfe {

class SoundEvaluator;

// Approaches target without overshooting and snaps to zero below 0.001.
struct AmbientSmoother
{
    const float* target;    // +0x00
    float        value;     // +0x04
    float        speed;     // +0x08
    void update(int steps);
};

class AmbientPlayer : public ISoundObstructionEvaluator
{
public:
    AmbientPlayer();
    virtual float evaluate(ISound*, const Math::Vec3*) { return m_smoother.value; }
    virtual ~AmbientPlayer();

    void play(ISound* sound, float fadeSeconds);                        // ISoundEngine::playAmbient
    void stopAll(float fadeSeconds);                                    // ISoundEngine::stopAmbient;
                                                                        // the fade is taken and ignored (every
                                                                        // slot stops at once, as shipped)
    void update();                                                      // once per engine update

private:
    void crossfadeTo(ISound* sound, float fadeSeconds);
    void stopSlot(ISound** slot);
    void finishFade(ISound* sound);

    ISound*         m_current;              // +0x04
    ISound*         m_next;                 // +0x08
    ISound*         m_pending;              // +0x0C queued behind m_next
    int             m_fadeStart;            // +0x10 engine frames
    int             m_fadeEnd;              // +0x14 1 while no fade runs
    float           m_pendingFade;          // +0x18
    float           m_t;                    // +0x1C the fade parameter both envelopes read
    float           m_obstructionTarget;    // +0x20
    AmbientSmoother m_smoother;             // +0x24
    SoundEvaluator* m_fadeIn;               // +0x30 0->1 over x 0..1
    SoundEvaluator* m_fadeOut;              // +0x34 1->0 over x 0..1
};

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_AMBIENT_H
