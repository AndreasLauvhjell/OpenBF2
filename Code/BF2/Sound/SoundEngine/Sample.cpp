// BF2Audio sample scheduling state; excluded from host images.
#if defined(BF2_IMAGE_BF2AUDIO)
#include "Sample.h"
#include "Sound.h"
#include "SoundFile.h"
#include "BfSoundEngine.h"

namespace dice {
namespace hfe {

int g_sampleCount = 0;      // ds:10048DC4

Sample::Sample()
    : m_sound(0), m_startFrame(0), m_readyFrame(0), m_priority(0.0f),
      m_pitchSeed(0), m_volumeSeed(0), m_index(0), m_started(false), m_pending(false),
      m_progress(0.0f)
{
    ++g_sampleCount;
}

Sample::~Sample()
{
    if (m_sound)
        m_sound->detachSample(this);
    m_sound = 0;
    --g_sampleCount;
}

void Sample::detachFromSound()
{
    if (m_sound)
        m_sound->detachSample(this);
    m_sound = 0;
}

// `m_sound->getTemplateImpl()->getIs3dSound()` - reads the sound without a null test.
bool Sample::is3D() const
{
    return m_sound->getTemplateImpl()->getIs3dSound();
}

// Requires an owning Sound; forwards to its template.
int Sample::getLengthFrames() const
{
    return m_sound->getTemplateImpl()->getLengthFrames(m_index);
}

SoundFile* Sample::getSoundFile() const
{
    return m_sound->getTemplateImpl()->getSoundFile(m_index);
}

// `blockStart` is the sample position of the block the voice is currently in
// (OpenALSample slot +0x28 answers it: 0 while an intro plays, the loop start while looping, the
// loop end once the outro is queued); `elapsedFrames` is (engine frame - m_startFrame). Before the
// loop block the answer is 0, past it 1, inside it the position within the loop as 0..1 - the
// elapsed frames become samples through the engine's rate over its frames-per-second constant
// (ds:1003F300 = 30) and wrap with a modulo. A wrap (the value falling below the remembered one)
// ticks both loop counters.
float Sample::loopProgress(int blockStart, int elapsedFrames)
{
    SoundFile* file = m_sound->getTemplateImpl()->getSoundFile(m_index);
    float progress;
    if (blockStart < file->m_loopEnd)
    {
        if (blockStart < file->m_loopStart)
        {
            progress = 0.0f;
        }
        else
        {
            const int loopLength = file->m_loopEnd - file->m_loopStart;
            const int samples    = (int) (elapsedFrames * (g_soundEngineInstance->m_sampleRate / (unsigned int) g_framesPerSecond)
                                          - file->m_loopStart);
            progress = (float) (samples % loopLength) / (float) loopLength;
        }
    }
    else
    {
        progress = 1.0f;
    }
    if (progress < m_progress)
    {
        ++m_pitchSeed;
        ++m_volumeSeed;
    }
    m_progress = progress;
    return progress;
}

} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_BF2AUDIO
