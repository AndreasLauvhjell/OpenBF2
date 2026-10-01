// Path:     [INFERRED] beside the attested DeviationComponents family.
// Contents: complete SimpleDeviationComp runtime/template pair, recovered from the symbolised
// BF2 1.50 Linux ELF and cross-checked against the Win32 dedicated-server image.

#ifndef GAME_OBJECTS_WEAPONS_DEVIATIONCOMPONENTS_SIMPLEDEVIATIONCOMP_H
#define GAME_OBJECTS_WEAPONS_DEVIATIONCOMPONENTS_SIMPLEDEVIATIONCOMP_H

#include "DeviationComp.h"

namespace dice {
namespace hfe {
namespace world {

class SimpleDeviationCompTemplate;

class SimpleDeviationComp : public DeviationComp
{
public:
    explicit SimpleDeviationComp(SimpleDeviationCompTemplate* objectTemplate);
    virtual ~SimpleDeviationComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void updateDeviation(io::PlayerInput input, bool movementState, bool firing);
    virtual float getMaxDeviation() const;
    virtual void setAIDeviation(float deviation);

private:
    /* +0x2C */ int m_simpleRefCount;
    /* +0x30 */ Random m_simpleRandom; // Constructed by retail even though this leaf never samples it.
    /* +0x34 */ float m_aiDeviation;
    /* +0x38 */ SimpleDeviationCompTemplate* m_simpleTemplate;
};

class SimpleDeviationCompTemplate : public DeviationCompTemplate
{
public:
    SimpleDeviationCompTemplate();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();
    virtual void objectCreated(IObject* object);
    virtual void objectDestroyed(IObject* object);
    virtual void setTemplate(IObjectTemplate* objectTemplate);
    virtual void* queryImplementation(unsigned int classId);
    virtual ~SimpleDeviationCompTemplate();
    virtual bool makeScript(io::IStream* stream);

private:
    /* +0x1C */ int m_simpleRefCount;
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(SimpleDeviationComp) == 0x3C,
              "SimpleDeviationComp must match the shipped x86 layout");
static_assert(sizeof(SimpleDeviationCompTemplate) == 0x20,
              "SimpleDeviationCompTemplate must match the shipped x86 layout");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_DEVIATIONCOMPONENTS_SIMPLEDEVIATIONCOMP_H
