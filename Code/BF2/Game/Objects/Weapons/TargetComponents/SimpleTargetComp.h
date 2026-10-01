// Path:     [INFERRED] beside the attested LPTargetComp.cpp.
// Contents: direct-target projectile component and its dynamic-velocity template contract.

#ifndef GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_SIMPLETARGETCOMP_H
#define GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_SIMPLETARGETCOMP_H

#include "TargetComp.h"

namespace dice {
namespace hfe {
namespace world {

class SimpleTargetCompTemplate : public TargetCompTemplate
{
public:
    SimpleTargetCompTemplate();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int classId);
    virtual bool makeScript(io::IStream* stream);

    void setUseDynamicVelocity(bool value) { m_useDynamicVelocity = value; }
    bool getUseDynamicVelocity() const { return m_useDynamicVelocity; }
    void setMinVelocity(float value) { m_minVelocity = value; }
    float getMinVelocity() const { return m_minVelocity; }
    void setMaxVelocity(float value) { m_maxVelocity = value; }
    float getMaxVelocity() const { return m_maxVelocity; }
    void setVelocityModifier(float value) { m_velocityModifier = value; }
    float getVelocityModifier() const { return m_velocityModifier; }

private:
    // TargetCompTemplate contains retail-STL strings, so its leaf starts at ELF +0x3C but PE
    // +0x84. Reach these fields by name; do not project either port's STL layout onto the other.
    /* ELF +0x3C / PE +0x84 */ int m_leafRefCount;
    /* ELF +0x40 / PE +0x88 */ bool m_useDynamicVelocity;
                                      // ELF +0x41..+0x43 / PE +0x89..+0x8B padding.
    /* ELF +0x44 / PE +0x8C */ float m_minVelocity;
    /* ELF +0x48 / PE +0x90 */ float m_maxVelocity;
    /* ELF +0x4C / PE +0x94 */ float m_velocityModifier;
};

class SimpleTargetComp : public TargetComp
{
public:
    explicit SimpleTargetComp(SimpleTargetCompTemplate* objectTemplate);
    virtual ~SimpleTargetComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;
    virtual void postFire(WeaponFireEvent& event);
    virtual float getDynamicVelocity() const;
    virtual bool busy();

    // This one-argument overload is the leaf's final primary-vtable slot (ELF slot 59; the PE
    // SimpleTarget primary table appends it after the two TargetComp-only slots).
    virtual void setTargetObject(IObject* target);

private:
    /* +0x4C */ int m_leafRefCount;
    /* +0x50 */ SimpleTargetCompTemplate* m_simpleTemplate;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(SimpleTargetComp) == 0x54, "SimpleTargetComp x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_SIMPLETARGETCOMP_H
