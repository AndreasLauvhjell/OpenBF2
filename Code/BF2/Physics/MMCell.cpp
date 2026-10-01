// Material-pair effects, decals and sounds; source path [INFERRED].
// [PLATFORM] Sound-template dispatch uses different base views across ports; typed calls preserve
// the common overriders. Sound instances release through ISound, not IBase.

#include "MMCell.h"

#include "../Sound/SoundObjectTemplate.h"      // the Sound template: setIsUsed / impl (its second base)
#include "../Sound/BfSound.h"                  // g_bfSound->createSound
#include "../Sound/NullSound.h"                // ISound::release

namespace dice {
namespace hfe {
namespace world {

namespace {
// Binding marks the sound template used. Instances are created lazily and released through ISound.
// The dedicated null-sound backend can return a shared nondeleting instance.
inline void soundTemplateOnBound(IObjectTemplate* t)
{
    static_cast<SoundObjectTemplate*>(t)->setIsUsed();                              // ISoundObjectTemplate +0x10
}
inline void* soundInstanceCreate(IObjectTemplate* t)
{
    return g_bfSound->createSound(static_cast<SoundObjectTemplate*>(t)->impl(), 0);   // +0x78, then IBfSound
}
inline void soundInstanceRelease(void* sound)
{
    static_cast<ISound*>(sound)->release();                                          // ISound +0x04
}
} // namespace

// three empty maps and the damage mod at -1.0f - "no damage mod set" (MaterialManager::
// getDamageMod treats a negative cell value as 'use the default damage').
MMCell::MMCell()
    : m_damageMod(-1.0f)
{
}

// every bound sound instance is released, then the three maps go (the decal and
// effect maps hold bare template pointers and own nothing).
MMCell::~MMCell()
{
    for (std::map<unsigned int, CellSound>::iterator it = m_soundTemplates.begin();
         it != m_soundTemplates.end(); ++it)
    {
        if (it->second.sound)
            soundInstanceRelease(it->second.sound);
    }
}

void  MMCell::setDamageMod(float mod) { m_damageMod = mod; }
float MMCell::getDamageMod()          { return m_damageMod; }

// The one-argument forms are index 0 through the two-argument virtuals (PE 0x5EE020 / 0x5EE040 /
// 0x5EE060: `push t; push 0; call [vtbl + slot]`).
void MMCell::setEffectTemplate(IObjectTemplate* t) { setEffectTemplate(0, t); }
void MMCell::setDecalTemplate(IObjectTemplate* t)  { setDecalTemplate(0, t); }
void MMCell::setSoundTemplate(IObjectTemplate* t)  { setSoundTemplate(0, t); }

// find, overwrite the mapped pointer; else insert. (No null test on t - a null is
// stored like any other value.)
void MMCell::setEffectTemplate(int index, IObjectTemplate* t)
{
    std::map<unsigned int, IObjectTemplate*>::iterator it = m_effectTemplates.find(static_cast<unsigned int>(index));
    if (it != m_effectTemplates.end())
        it->second = t;
    else
        m_effectTemplates.insert(std::make_pair(static_cast<unsigned int>(index), t));
}

// the same on the decal map.
void MMCell::setDecalTemplate(int index, IObjectTemplate* t)
{
    std::map<unsigned int, IObjectTemplate*>::iterator it = m_decalTemplates.find(static_cast<unsigned int>(index));
    if (it != m_decalTemplates.end())
        it->second = t;
    else
        m_decalTemplates.insert(std::make_pair(static_cast<unsigned int>(index), t));
}

// the template at `index` if it is mapped and non-null; else, with `fallback`, the
// template at 0 (returned even when that one is null); else null.
IObjectTemplate* MMCell::getEffectTemplate(int index, bool fallback)
{
    std::map<unsigned int, IObjectTemplate*>::iterator it = m_effectTemplates.find(static_cast<unsigned int>(index));
    if (it != m_effectTemplates.end() && it->second)
        return it->second;
    if (!fallback)
        return 0;
    it = m_effectTemplates.find(0u);
    if (it == m_effectTemplates.end())
        return 0;
    return it->second;
}

// the same shape on the decal map.
IObjectTemplate* MMCell::getDecalTemplate(int index, bool fallback)
{
    std::map<unsigned int, IObjectTemplate*>::iterator it = m_decalTemplates.find(static_cast<unsigned int>(index));
    if (it != m_decalTemplates.end() && it->second)
        return it->second;
    if (!fallback)
        return 0;
    it = m_decalTemplates.find(0u);
    if (it == m_decalTemplates.end())
        return 0;
    return it->second;
}

// A null template only clears an existing entry's template (the instance, if any,
// stays). A non-null one overwrites an existing entry - releasing and dropping its instance - or
// inserts {t, 0} and then tells the template it was bound (the ghost hook).
void MMCell::setSoundTemplate(int index, IObjectTemplate* t)
{
    const unsigned int key = static_cast<unsigned int>(index);
    std::map<unsigned int, CellSound>::iterator it = m_soundTemplates.find(key);
    if (!t)
    {
        if (it != m_soundTemplates.end())
            it->second.soundTemplate = 0;
        return;
    }
    if (it != m_soundTemplates.end())
    {
        it->second.soundTemplate = t;
        if (it->second.sound)
            soundInstanceRelease(it->second.sound);
        it->second.sound = 0;
        return;
    }
    CellSound cs;
    cs.soundTemplate = t;
    cs.sound = 0;
    m_soundTemplates.insert(std::make_pair(key, cs));
    soundTemplateOnBound(t);                                                // PE 0x5ECA74 [FRONTIER]
}

// the entry at `index` if it is mapped with a non-null template; else, with
// `fallback`, the entry at 0 (whatever its template); else null.
MMCell::CellSound* MMCell::getCellSound(int index, bool fallback)
{
    std::map<unsigned int, CellSound>::iterator it = m_soundTemplates.find(static_cast<unsigned int>(index));
    if (it != m_soundTemplates.end() && it->second.soundTemplate)
        return &it->second;
    if (!fallback)
        return 0;
    it = m_soundTemplates.find(0u);
    if (it == m_soundTemplates.end())
        return 0;
    return &it->second;
}


IObjectTemplate* MMCell::getSoundTemplate(int index, bool fallback)
{
    CellSound* cs = getCellSound(index, fallback);
    return cs ? cs->soundTemplate : 0;
}

// the instance, manufactured on first use from the template (the ghost hook).
void* MMCell::getSound(int index, bool fallback)
{
    CellSound* cs = getCellSound(index, fallback);
    if (!cs)
        return 0;
    if (!cs->sound && cs->soundTemplate)
        cs->sound = soundInstanceCreate(cs->soundTemplate);                  // PE 0x5ECAD8 [FRONTIER]
    return cs->sound;
}

} // namespace world
} // namespace hfe
} // namespace dice
