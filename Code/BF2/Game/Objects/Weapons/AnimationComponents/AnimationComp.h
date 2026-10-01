// Path:     [INFERRED] beside the attested DefaultAnimationComp.cpp translation unit.
// Contents: the shared weapon-animation template/runtime contracts recovered from the symbolised
// BF2 1.50 Linux ELF and cross-checked against the Win32 PE call slots.
// The IWeaponAnimationComp tail differs by one slot between the shipped ABIs because Itanium emits
// two destructor entries and MSVC emits one.  Keep every caller typed; raw +0x8C/+0x88 dispatch is
// not portable between the two retail images.

#ifndef GAME_OBJECTS_WEAPONS_ANIMATIONCOMPONENTS_ANIMATIONCOMP_H
#define GAME_OBJECTS_WEAPONS_ANIMATIONCOMPONENTS_ANIMATIONCOMP_H

#include "../FireComponents/FireComp.h"
#include "../../CompTemplateInterfaces.h"
#include "../../WeaponComp.h"
#include "../../../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace anim { class IAnimationSystem; }
namespace hfe {
namespace io { class IStream; }
namespace world {

class IObject;
class IObjectTemplate;

class AnimationCompTemplate : public IWeaponCompTemplate, public io::IConsoleSaveable
{
public:
    AnimationCompTemplate();

    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent() = 0;
    virtual void objectCreated(IObject*);
    virtual void objectDestroyed(IObject*);
    virtual void setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int classId);
    virtual bool makeScript(io::IStream* stream);
    virtual void setAnimateOnZoom(bool value);
    virtual bool getAnimateOnZoom();

private:
    /* ELF +0x08 */ bool m_animateOnZoom;
};

class IWeaponAnimationComp : public IWeaponComp
{
public:
    virtual ~IWeaponAnimationComp();                                  // ELF +0x84/+0x88
    virtual void setActiveAnimationSystem(anim::IAnimationSystem*) = 0;// ELF +0x8C
    virtual void updateOnZooming(int zoom) = 0;                        // ELF +0x90
    virtual void updateOnFiring(FireState state) = 0;                  // ELF +0x94
    virtual void setAmmoLeft(int rounds) = 0;                          // ELF +0x98
    virtual bool getSignalDelayShiftStarted() const = 0;               // ELF +0x9C
};

// Shipped x86 size: 0x1C.  Like FireComp and ZoomComp this base remains abstract: each leaf owns
// the effective IBase quartet/refcount and supplies the two animation-tail methods left pure here.
class AnimationComp : public WeaponComp, public IWeaponAnimationComp
{
public:
    explicit AnimationComp(AnimationCompTemplate* objectTemplate);
    virtual ~AnimationComp();

    virtual unsigned int getInterfaceID() const;
    virtual void initComponent(WeaponInitEvent&);
    virtual bool preUpdate();
    virtual void updateComponent(float, unsigned int);
    virtual bool isSelectable() const;
    virtual void fireStop(bool);
    virtual bool requestFire(bool, bool);
    virtual void postFire(WeaponFireEvent&);
    virtual void fireComplete(unsigned int);
    virtual void postSwitchFireRate(FireRate);
    virtual bool requestReload(bool);
    virtual void reload(ReloadState);
    virtual void postReload();
    virtual void outOfAmmo(bool);
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

    virtual void setActiveAnimationSystem(anim::IAnimationSystem* system);
    virtual void updateOnZooming(int zoom);
    virtual void updateOnFiring(FireState state);

protected:
    /* +0x14 */ anim::IAnimationSystem* m_activeAnimationSystem;
    /* +0x18 */ AnimationCompTemplate* m_animationTemplate;

    GenericFireArm* firearm() const
    {
        return reinterpret_cast<GenericFireArm*>(m_initObject);
    }
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(AnimationCompTemplate) == 0x0C,
              "AnimationCompTemplate x86 ABI drift");
static_assert(sizeof(AnimationComp) == 0x1C, "AnimationComp x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_ANIMATIONCOMPONENTS_ANIMATIONCOMP_H
