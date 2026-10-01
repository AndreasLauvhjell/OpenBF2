// Path:     attested. Game/Objects/Weapons/SoundComponents/DefaultSoundComp.cpp occurs in the
// retail source-path map; this sibling header is [INFERRED].
// Contents: complete DefaultSoundComp runtime and template contracts recovered from bf2.i64 and
// cross-checked against the Win32 dedicated-server component tables.
// The dedicated image still constructs this component even though SimpleObjectTemplate's sound
// creation arm returns null there. Keeping the full state machine matters: stock weapon templates
// must load, attach the correct IWeaponSoundComp face, and take the shipped null-sound branches.

#ifndef GAME_OBJECTS_WEAPONS_SOUNDCOMPONENTS_DEFAULTSOUNDCOMP_H
#define GAME_OBJECTS_WEAPONS_SOUNDCOMPONENTS_DEFAULTSOUNDCOMP_H

#include "../FireComponents/FireComp.h"
#include "../../CompTemplateInterfaces.h"
#include "../../../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {

class ISound;
class ObjectSoundNode;

namespace io { class IStream; }

namespace world {

class GenericFireArm;
class IObject;
class IObjectComponent;
class IObjectTemplate;
class SimpleObjectTemplate;

class DefaultSoundCompTemplate;

class DefaultSoundComp : public WeaponComp, public IWeaponSoundComp
{
public:
    explicit DefaultSoundComp(DefaultSoundCompTemplate* objectTemplate);
    virtual ~DefaultSoundComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual void initComponent(WeaponInitEvent& event);
    virtual bool preUpdate();
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual bool isSelectable() const;
    virtual void fireStop(bool alternate);
    virtual bool requestFire(bool primary, bool alternate);
    virtual void postFire(WeaponFireEvent& event);
    virtual void fireComplete(unsigned int);
    virtual void postSwitchFireRate(FireRate fireRate);
    virtual bool requestReload(bool);
    virtual void reload(ReloadState state);
    virtual void postReload();
    virtual void outOfAmmo(bool triggerClick);
    virtual void stoppedTracking();
    virtual void postChangeTarget(IObject*);
    virtual void projectileWasReset(IObject*);
    virtual void enable();
    virtual void disable();
    virtual void stop();
    virtual bool busy();
    virtual void exit();
    virtual void destroy();
    virtual void pickup();
    virtual void reset();

    virtual void toggleCamera();
    virtual void setAlternateParent(GenericFireArm* firearm);
    virtual bool getNoisy() const;
    virtual int getMaxSoundsPerBurst() const;

    // [CHOSEN] source names for the recovered contiguous +0x20 sound-pointer array.
    enum SoundIndex
    {
        Fire1P = 0,
        Fire1POutdoor,
        Fire3P,
        BoltClick,
        TriggerClick,
        SwitchFireRate,
        Reload1P,
        Reload3P,
        Deploy1P,
        Deploy3P,
        Zoom,
        SoundCount
    };

private:
    GenericFireArm* firearm() const;
    bool is1P();
    bool isBackstepping() const;

    /* +0x14 */ int m_refCount;
    /* +0x18 */ DefaultSoundCompTemplate* m_template;
    /* +0x1C */ GenericFireArm* m_alternateParent;
    /* +0x20 */ ISound* m_sounds[SoundCount];
    /* +0x4C */ ObjectSoundNode* m_soundNode;
    /* +0x50 */ int m_soundsUntilNextPlay;
    /* +0x54 */ unsigned char m_reloadSampleState;
    /* +0x55 */ bool m_playBoltClick;
    /* +0x56 */ bool m_lowRoundsPerMinute;
    /* +0x57 */ bool m_continuousFirePlaying;
    /* +0x58 */ bool m_hadFourSampleStart;
    /* +0x59 */ bool m_enabled;
    /* +0x5C */ int m_lastEnableTick;
};

class DefaultSoundCompTemplate : public IWeaponCompTemplate, public io::IConsoleSaveable
{
public:
    DefaultSoundCompTemplate();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();
    virtual void objectCreated(IObject*);
    virtual void objectDestroyed(IObject*);
    virtual void setTemplate(IObjectTemplate* objectTemplate);
    virtual void* queryImplementation(unsigned int classId);
    virtual bool makeScript(io::IStream* stream);

    void initTemplates(SimpleObjectTemplate* objectTemplate, unsigned int flags);

    void setNoisy(bool value) { m_noisy = value; }
    bool getNoisy() const { return m_noisy; }
    void setMaxSoundsPerBurst(int value) { m_maxSoundsPerBurst = value; }
    int getMaxSoundsPerBurst() const { return m_maxSoundsPerBurst; }

private:
    /* +0x08 */ int m_refCount;
    /* +0x0C */ bool m_noisy;
    /* +0x10 */ int m_maxSoundsPerBurst;
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(DefaultSoundComp) == 0x60,
              "DefaultSoundComp must match the shipped x86 layout");
static_assert(sizeof(DefaultSoundCompTemplate) == 0x14,
              "DefaultSoundCompTemplate must match the shipped x86 layout");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_SOUNDCOMPONENTS_DEFAULTSOUNDCOMP_H
