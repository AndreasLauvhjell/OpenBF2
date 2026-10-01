// BF2Audio image only: its engine, exports, and null instances must not be linked into the host.
// Build with Code/Build/bf2audio.cmd.
#if defined(BF2_IMAGE_BF2AUDIO)
#include "Ambient.h"
#include "Sound.h"
#include "SoundEvaluator.h"
#include "BfSoundEngine.h"

#include <cmath>

namespace dice {
namespace hfe {

void AmbientSmoother::update(int steps)
{
    if (!target)
        return;
    const float gap = *target - value;
    if (std::fabs(gap) < 0.0000099999997f)
        return;
    float move = (float) steps * speed * gap;
    if (gap <= 0.0f)
    {
        if (gap >= 0.0f)
            return;
        if (!(gap < move))
            move = gap;
    }
    else
    {
        if (!(gap >= move))
            move = gap;
    }
    value += move;
    if (std::fabs(value) < 0.001f)
        value = 0.0f;
}

// The two built-in envelopes: fade-in (0,0)->(1,1) and fade-out (0,1)->(1,0),
// both over x 0..1, y 0..1, not randomised.
AmbientPlayer::AmbientPlayer()
    : m_current(0), m_next(0), m_pending(0), m_fadeStart(0), m_fadeEnd(-1), m_pendingFade(0.0f),
      m_t(0.0f), m_obstructionTarget(1.0f), m_fadeIn(0), m_fadeOut(0)
{
    m_smoother.target = &m_obstructionTarget;
    m_smoother.value  = m_obstructionTarget;
    m_smoother.speed  = g_soundProperties->get(SoundProperties::AmbientIndoorObstructionSpeed);
    EnvelopePoint fadeIn[2]  = { { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } };
    assignEvaluator(m_fadeIn,  new SoundEvaluator(0.0f, 1.0f, 0.0f, 1.0f, false, 2, fadeIn));
    EnvelopePoint fadeOut[2] = { { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } };
    assignEvaluator(m_fadeOut, new SoundEvaluator(0.0f, 1.0f, 0.0f, 1.0f, false, 2, fadeOut));
}

AmbientPlayer::~AmbientPlayer()
{
    if (m_current) m_current->release();
    if (m_next)    m_next->release();
    if (m_pending) m_pending->release();
    deleteEvaluator(m_fadeOut);
    deleteEvaluator(m_fadeIn);
}

// once a fade is over, a sound driven by one of the built-in envelopes gets its
// evaluator cleared again.
void AmbientPlayer::finishFade(ISound* sound)
{
    ISoundEvaluator* e = sound->getVolumeEvaluator();
    if (e == m_fadeIn || e == m_fadeOut)
        sound->setVolumeEvaluator(0, 0);
}

void AmbientPlayer::stopSlot(ISound** slot)
{
    ISound* s = *slot;
    if (!s)
        return;
    s->stop();
    ISoundEvaluator* e = s->getVolumeEvaluator();
    if (e == m_fadeIn || e == m_fadeOut)
        s->setVolumeEvaluator(0, 0);
    s->setObstructionEvaluator(0);
    s->release();
    *slot = 0;
}

// Start fading from m_current to `sound`: nothing when it is the current sound
// or plays the same file; the smoother's speed is refreshed from the property; a sound already
// driven by a foreign evaluator keeps it and just switches. The new sound's first file is preloaded
// through checkSampleAvailable(0) before play(). A null `sound` stops both slots.
void AmbientPlayer::crossfadeTo(ISound* sound, float fadeSeconds)
{
    if (sound)
    {
        if (sound == m_current)
            return;
        if (m_current)
        {
            ISoundTemplate* tn = sound->getTemplate();
            ISoundTemplate* tc = m_current->getTemplate();
            if (tn->checkSampleAvailable(0) == tc->checkSampleAvailable(0))
                return;
        }
        m_smoother.speed = g_soundProperties->get(SoundProperties::AmbientIndoorObstructionSpeed);
        if (m_current)
        {
            if (fadeSeconds > 0.0f)
            {
                ISoundEvaluator* e = m_current->getVolumeEvaluator();
                if (!e || e == m_fadeIn || e == m_fadeOut)
                {
                    m_current->setVolumeEvaluator(m_fadeOut, &m_t);
                    m_current->setObstructionEvaluator(this);
                }
            }
        }
        m_next = sound;
        sound->addRef();
        if (fadeSeconds > 0.0f)
        {
            ISoundEvaluator* e = m_next->getVolumeEvaluator();
            if (!e || e == m_fadeIn || e == m_fadeOut)
            {
                m_next->setVolumeEvaluator(m_fadeIn, &m_t);
                m_next->setObstructionEvaluator(this);
            }
        }
        m_next->getTemplate()->checkSampleAvailable(0);
        m_next->play();
    }
    else
    {
        stopSlot(&m_current);
        stopSlot(&m_next);
    }
    if (m_current || m_next)
    {
        m_fadeStart = g_soundEngineInstance->m_frame;
        m_t = 0.0f;
        m_fadeEnd = m_fadeStart - (int) (fadeSeconds * -30.0f);
    }
}

// A null sound means the engine's null sound (silence). While a fade is running
// the request waits behind it - only the latest one.
void AmbientPlayer::play(ISound* sound, float fadeSeconds)
{
    if (!sound)
        sound = g_nullSound;
    if (m_next)
    {
        if (sound != m_next)
        {
            sound->addRef();
            if (m_pending)
                m_pending->release();
            m_pending = sound;
            m_pendingFade = fadeSeconds;
        }
    }
    else
    {
        crossfadeTo(sound, fadeSeconds);
    }
}

void AmbientPlayer::stopAll(float /*fadeSeconds - unused, as shipped*/)
{
    stopSlot(&m_current);
    stopSlot(&m_next);
    stopSlot(&m_pending);
}

// Advance the fade parameter; when the window ends, retire the old sound,
// promote the new one, and start the queued request if it differs. Then the indoor obstruction:
// zero for the current trigger zone or outdoors, the property's amount otherwise, smoothed.
void AmbientPlayer::update()
{
    const int frame = g_soundEngineInstance->m_frame;
    if (m_fadeEnd >= 0)
    {
        if (frame < m_fadeEnd)
        {
            if (frame >= m_fadeStart)
                m_t = (float) (frame - m_fadeStart) / (float) (m_fadeEnd - m_fadeStart);
        }
        else
        {
            if (m_current)
                stopSlot(&m_current);
            if (m_next)
            {
                ISound* n = m_next;
                m_current = n;
                m_next = 0;
                finishFade(n);
            }
            m_fadeEnd = -1;
            m_t = 0.0f;
            if (m_pending)
            {
                if (m_pending != m_current)
                    crossfadeTo(m_pending, m_pendingFade);
                m_pending->release();
                m_pending = 0;
            }
        }
    }
    const bool indoors = g_soundEngineInstance->getListenerReverb() > 0.0f
                      || g_soundProperties->get(SoundProperties::Listener3dObstruction) > 0.0f;
    if ((ISound*) g_soundEngineInstance->m_triggers.getTrigger() == m_current || !indoors)
    {
        m_obstructionTarget = 0.0f;
        m_smoother.update(1);
    }
    else
    {
        m_obstructionTarget = g_soundProperties->get(SoundProperties::AmbientIndoorObstruction);
        m_smoother.update(1);
    }
}

} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_BF2AUDIO
