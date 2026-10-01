// Path:     [INFERRED] beside the attested LPTargetComp.cpp.
// Contents: heat/laser lock-on weapon target component and its concrete template.

#ifndef GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_MULTIFIREARMTARGETCOMP_H
#define GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_MULTIFIREARMTARGETCOMP_H

#include "TargetComp.h"

namespace dice {
namespace hfe {
namespace world {

class MultiFireArmTargetCompTemplate : public TargetCompTemplate
{
public:
    MultiFireArmTargetCompTemplate();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int classId);
    virtual bool makeScript(io::IStream* stream);

    void setLockDelay(float seconds);
    float getLockDelay() const { return m_lockDelay; }
    int getLockDelayTicks() const { return m_lockDelayTicks; }
    void setLoseLockDelay(float seconds);
    float getLoseLockDelay() const { return m_loseLockDelay; }
    int getLoseLockDelayTicks() const { return m_loseLockDelayTicks; }
    void setLockAngle(float degrees) { m_lockAngle = degrees; }
    float getLockAngle() const { return m_lockAngle; }

private:
    /* ELF +0x3C / PE +0x84 */ int m_leafRefCount;
    /* ELF +0x40 / PE +0x88 */ float m_lockAngle;
    /* ELF +0x44 / PE +0x8C */ float m_lockDelay;
    /* ELF +0x48 / PE +0x90 */ int m_lockDelayTicks;
    /* ELF +0x4C / PE +0x94 */ float m_loseLockDelay;
    /* ELF +0x50 / PE +0x98 */ int m_loseLockDelayTicks;
};

class MultiFireArmTargetComp : public TargetComp
{
public:
    explicit MultiFireArmTargetComp(MultiFireArmTargetCompTemplate* objectTemplate);
    virtual ~MultiFireArmTargetComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;
    virtual bool busy();

    virtual void updateComponent(float dt, unsigned int tick);
    virtual void setTargetObject(IObject* target, bool locked, IObject* lockVehicle);
    virtual bool requestFire(bool released, bool alternate);
    virtual void postFire(WeaponFireEvent& event);
    virtual void disable();

private:
    /* +0x4C */ int m_leafRefCount;
    /* +0x50 */ bool m_acquiredFeedbackPending;
    /* +0x51 */ bool m_losingLock;
                 // +0x52..+0x53 padding.
    /* +0x54 */ unsigned int m_transitionTick;
    /* +0x58 */ int m_targetChangeTick;
    /* +0x5C */ MultiFireArmTargetCompTemplate* m_multiTemplate;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(MultiFireArmTargetComp) == 0x60,
              "MultiFireArmTargetComp x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_MULTIFIREARMTARGETCOMP_H
