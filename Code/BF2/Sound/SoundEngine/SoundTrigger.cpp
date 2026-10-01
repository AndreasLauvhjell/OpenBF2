// BF2Audio sound triggers; excluded from host images.
#if defined(BF2_IMAGE_BF2AUDIO)
#include "SoundTrigger.h"
#include "Sound.h"
#include "BfSoundEngine.h"

#include <algorithm>
#include <cmath>

namespace dice {
namespace hfe {

namespace {
// Orders sounds by decreasing minimum distance.
struct GreaterMinDistance
{
    bool operator()(Sound* a, Sound* b) const
    {
        return b->getTemplateImpl()->getMinDistance() < a->getTemplateImpl()->getMinDistance();
    }
};
}

SoundTriggerManager::SoundTriggerManager()
    : m_current(0), m_changed(false)
{
}

SoundTriggerManager::~SoundTriggerManager()
{
    removeAll();
}

// The template is made 2D and dry before its Sound is created.
void SoundTriggerManager::addTrigger(ISoundTemplate* t)
{
    if (t)
    {
        t->setIs3dSound(false);
        t->setReverbLevel(0.0f);
        Sound* s = static_cast<Sound*>(t->createSound());
        m_triggers.push_back(s);
    }
}

void SoundTriggerManager::setTrigger(Sound* s)
{
    if (s != m_current)
    {
        if (s)
            s->addRef();
        if (m_current)
        {
            m_current->release();
            m_current = 0;
            m_changed = true;
        }
        if (s)
            m_current = s;
    }
}

// the first trigger made from `t`.
void SoundTriggerManager::removeTrigger(ISoundTemplate* t)
{
    for (std::vector<Sound*>::iterator it = m_triggers.begin(); it != m_triggers.end(); ++it)
    {
        if ((*it)->getTemplate() == t)
        {
            (*it)->release();
            m_triggers.erase(it);
            return;
        }
    }
}

void SoundTriggerManager::removeAll()
{
    for (unsigned int i = 0; i < m_triggers.size(); ++i)
        m_triggers[i]->release();
    std::vector<Sound*>().swap(m_triggers);
    if (m_current)
    {
        m_current->release();
        m_current = 0;
    }
}

void SoundTriggerManager::sort()
{
    std::sort(m_triggers.begin(), m_triggers.end(), GreaterMinDistance());
}

// With no explicit trigger, scan from the smallest zone up: the listener is
// inside a zone when it is within the template's min distance of its position and, if the
// template has a radius, within that radius vertically. While the engine is interactive the scan
// also bubbles a zone up past a smaller neighbour so the order survives template edits. The
// result - the explicit trigger, the zone, or null - goes to the ambient player with the normal
// fade, or the forced fade right after an explicit trigger was cleared.
void SoundTriggerManager::update(const Vec3* listenerPosition)
{
    Sound* found = m_current;
    if (listenerPosition && !m_current)
    {
        int i = (int) m_triggers.size() - 1;
        while (i >= 0)
        {
            Sound* s = m_triggers[i];
            SoundTemplate* t = s->getTemplateImpl();
            const Vec3& p = t->getPosition();
            const float dx = listenerPosition->x - p.x;
            const float dy = listenerPosition->y - p.y;
            const float dz = listenerPosition->z - p.z;
            const float r  = t->getMinDistance();
            if (r * r > dz * dz + dy * dy + dx * dx)
            {
                if (0.0f == t->getRadius())
                {
                    found = s;
                    break;
                }
                const float vertical = std::fabs(t->getPosition().y - listenerPosition->y);
                if (t->getRadius() >= vertical)
                {
                    found = s;
                    break;
                }
            }
            if (g_soundEngineInstance->getInteractiveMode() && i > 0)
            {
                const float below = m_triggers[i - 1]->getTemplateImpl()->getMinDistance();
                if (m_triggers[i]->getTemplateImpl()->getMinDistance() > below)
                {
                    Sound* tmp = m_triggers[i];
                    m_triggers[i] = m_triggers[i - 1];
                    m_triggers[i - 1] = tmp;
                }
            }
            --i;
        }
    }
    float fade;
    if (found == m_current || m_changed)
    {
        fade = g_soundProperties->get(SoundProperties::AmbientForcedFadeTime);
        m_changed = false;
    }
    else
    {
        fade = g_soundProperties->get(SoundProperties::AmbientFadeTime);
    }
    g_soundEngineInstance->playAmbient(found, fade);
}

} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_BF2AUDIO
