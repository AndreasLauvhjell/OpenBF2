// Exposes the template's ability payload without mutable ability state of its own.
// init/setObject must run before reading m_object; the constructor leaves it uninitialized.
// Preserve declaration order: non-const hasUpdate is appended after the destructor, not the const override.
// [PLATFORM] Destructor slot count differs between PE and ELF. Source placement is [INFERRED].

#ifndef GAME_OBJECTS_GAMELOGICCOMP_ABILITYGLCOMP_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAMELOGICCOMP_ABILITYGLCOMP_H

#include "IAbilityGLComp.h"

namespace dice {
namespace hfe {
namespace world {

class AbilityGLCompTemplate;

class AbilityGLComp : public IAbilityGLComp
{
public:
    explicit AbilityGLComp(AbilityGLCompTemplate* objectTemplate);

    // IObjectComponent overrides
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;                    // IID_IAbilityGLComp
    virtual void  init(IObject* object);                            // stores m_object
    virtual void  setObject(IObject* object);
    virtual IObject* getObject() const;
    virtual void  handleUpdate(float deltaTime);                    // empty

    // IAbilityGLComp
    virtual AbilityGLInfo* getGLInfo();                             // template payload
    virtual ~AbilityGLComp();                                       // (D1/D2), 14 B - vptr store only

    // appended after the destructor pair
    virtual bool  hasUpdate();                                      // +0x44 false (NON-const NEW slot)

private:
    int                    m_refCount;                              // +0x04
    AbilityGLCompTemplate* m_template;                              // +0x08
    IObject*               m_object;                                // +0x0C see the header note
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAMELOGICCOMP_ABILITYGLCOMP_H
