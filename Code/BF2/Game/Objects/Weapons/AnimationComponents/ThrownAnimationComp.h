// Path:     [INFERRED] from the contiguous AnimationComponents family in the symbolised ELF.
// Contents: complete ThrownAnimationComp template/runtime declarations recovered from both
// shipped x86 images.

#ifndef GAME_OBJECTS_WEAPONS_ANIMATIONCOMPONENTS_THROWNANIMATIONCOMP_H
#define GAME_OBJECTS_WEAPONS_ANIMATIONCOMPONENTS_THROWNANIMATIONCOMP_H

#include "AnimationComp.h"

namespace dice {
namespace hfe {
namespace world {

class ThrownAnimationComp;

class ThrownAnimationCompTemplate : public AnimationCompTemplate
{
public:
    ThrownAnimationCompTemplate();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int classId);
    virtual bool makeScript(io::IStream* stream);

private:
    /* ELF +0x0C */ int m_refCount;
};

class ThrownAnimationComp : public AnimationComp
{
public:
    explicit ThrownAnimationComp(ThrownAnimationCompTemplate* objectTemplate);
    virtual ~ThrownAnimationComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual void updateOnFiring(FireState state);
    virtual void postFire(WeaponFireEvent& event);
    virtual void reload(ReloadState state);
    virtual void enable();
    virtual void disable();
    virtual void updateOnZooming(int zoom);
    virtual void setAmmoLeft(int rounds);
    virtual bool getSignalDelayShiftStarted() const;

private:
    /* +0x1C */ int m_thrownRefCount;
    /* +0x20 */ bool m_throwStarted;
    /* +0x24 */ ThrownAnimationCompTemplate* m_thrownTemplate;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(ThrownAnimationCompTemplate) == 0x10,
              "ThrownAnimationCompTemplate x86 ABI drift");
static_assert(sizeof(ThrownAnimationComp) == 0x28,
              "ThrownAnimationComp x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_ANIMATIONCOMPONENTS_THROWNANIMATIONCOMP_H
