// Path:     [INFERRED] beside the attested DeviationComponents family.
// Contents: complete TargetDeviationComp runtime/template pair, recovered from both BF2 1.50
// shipped x86 images.  This is the deterministic artillery target-spread strategy.

#ifndef GAME_OBJECTS_WEAPONS_DEVIATIONCOMPONENTS_TARGETDEVIATIONCOMP_H
#define GAME_OBJECTS_WEAPONS_DEVIATIONCOMPONENTS_TARGETDEVIATIONCOMP_H

#include <map>

#include "DeviationComp.h"

namespace dice {
namespace hfe {
namespace world {

class TargetDeviationCompTemplate;

class TargetDeviationComp : public DeviationComp
{
public:
    explicit TargetDeviationComp(TargetDeviationCompTemplate* objectTemplate);
    virtual ~TargetDeviationComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getInterfaceID() const;

    virtual void updateComponent(float, unsigned int);
    virtual void updateDeviation(io::PlayerInput, bool, bool);
    virtual Vec3 deviatePosition(Vec3&, int);
    virtual void setProjectileDeviationFixed(Vec3&, int);
    virtual void postFire(WeaponFireEvent&);
    virtual float getMaxDeviation() const;
    virtual void setAIDeviation(float);

private:
    /* ELF +0x2C */ int m_targetRefCount;
    /* ELF +0x30 */ Random m_targetRandom;
    /* ELF +0x34 */ float m_aiDeviation;
    // Retail libstdc++ places this tree at +0x38 and the template at +0x44.  The rebuilt MSVC STL
    // representation is intentionally allowed to differ; no serialized or external offset uses it.
    std::map<int, Vec3> m_fixedDeviations;
    TargetDeviationCompTemplate* m_targetTemplate;
};

class TargetDeviationCompTemplate : public DeviationCompTemplate
{
public:
    TargetDeviationCompTemplate();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();
    virtual void objectCreated(IObject*);
    virtual void objectDestroyed(IObject*);
    virtual void setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual ~TargetDeviationCompTemplate();
    virtual bool makeScript(io::IStream*);

    void setRadius(float value) { m_radius = value; }
    float getRadius() const { return m_radius; }

private:
    /* +0x1C */ int m_targetRefCount;
    /* +0x20 */ float m_radius;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(TargetDeviationCompTemplate) == 0x24,
              "TargetDeviationCompTemplate x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_DEVIATIONCOMPONENTS_TARGETDEVIATIONCOMP_H
