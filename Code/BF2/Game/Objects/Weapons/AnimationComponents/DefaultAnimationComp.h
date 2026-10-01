// Path:     attested by the Win32 Debug __FILE__ literal in DefaultAnimationComp::reload.
// Contents: complete DefaultAnimationComp template/runtime declarations recovered from both
// shipped x86 images.

#ifndef GAME_OBJECTS_WEAPONS_ANIMATIONCOMPONENTS_DEFAULTANIMATIONCOMP_H
#define GAME_OBJECTS_WEAPONS_ANIMATIONCOMPONENTS_DEFAULTANIMATIONCOMP_H

#include "AnimationComp.h"

namespace dice {
namespace hfe {
namespace world {

class DefaultAnimationComp;

class DefaultAnimationCompTemplate : public AnimationCompTemplate
{
public:
    DefaultAnimationCompTemplate();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int classId);
    virtual bool makeScript(io::IStream* stream);

    virtual void setUseShiftAnimation(bool value);
    virtual bool getUseShiftAnimation();
    virtual void setLoopingFire(bool value);
    virtual bool getLoopingFire();
    virtual void setShiftDelay(float seconds);
    virtual float getShiftDelay() const;

    int getShiftDelayInTicks() const;

private:
    /* ELF +0x0C */ int m_refCount;
    /* ELF +0x10 */ bool m_useShiftAnimation;
    /* ELF +0x14 */ float m_shiftDelay;
    /* ELF +0x18 */ int m_shiftDelayInTicks;
    /* ELF +0x1C */ bool m_loopingFire;
};

class DefaultAnimationComp : public AnimationComp
{
public:
    explicit DefaultAnimationComp(DefaultAnimationCompTemplate* objectTemplate);
    virtual ~DefaultAnimationComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool requestFire(bool, bool);
    virtual void updateComponent(float, unsigned int);
    virtual void postFire(WeaponFireEvent& event);
    virtual void fireStop(bool alternate);
    virtual void fireComplete(unsigned int atTick);
    virtual void reload(ReloadState state);
    virtual bool busy();
    virtual void disable();
    virtual void shift();
    virtual void setAmmoLeft(int rounds);
    virtual bool getSignalDelayShiftStarted() const;

private:
    /* +0x1C */ int m_defaultRefCount;
    /* +0x20 */ DefaultAnimationCompTemplate* m_defaultTemplate;
    /* +0x24 */ bool m_shiftPending;
    /* +0x28 */ unsigned int m_shiftDelayStartedAtTick;
    /* +0x2C */ mutable bool m_signalDelayShiftStarted;
    /* +0x30 */ int m_ammoLeft;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(DefaultAnimationCompTemplate) == 0x20,
              "DefaultAnimationCompTemplate x86 ABI drift");
static_assert(sizeof(DefaultAnimationComp) == 0x34,
              "DefaultAnimationComp x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_ANIMATIONCOMPONENTS_DEFAULTANIMATIONCOMP_H
