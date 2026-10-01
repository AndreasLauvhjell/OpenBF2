// Path: [INFERRED] in Game/Objects; no __FILE__ attests this header's placement.
// Weapon component base, recovered from the symbolized ELF (table 0x88B52C0).
// Retail x86 size is 0x10. Keep m_object and m_initObject separate.
// Update and frame-update are distinct channels; release returns the new count and
// self-deletes at zero. Reference-count operations are non-atomic.

#ifndef GAME_OBJECTS_WEAPONCOMP_H     // [INFERRED] guard style
#define GAME_OBJECTS_WEAPONCOMP_H

#include "IObjectComponent.h"

namespace dice {
namespace hfe {
namespace world {

class IObject;

// sizeof == 0x10 (16). ProjectileComp is byte-for-byte the same shape.
// Call setObject before init: derived components may cache addresses in the attached object.
// init's hierarchy pointer remains separate from the object returned by getObject.
class WeaponComp : public IObjectComponent
{
public:
    virtual void  addRef();                             // +0x00
    virtual int   getRef();                             // +0x04
    virtual int   release();                            // vptr +0x08, 0x82590C0 - returns new count
    virtual void* queryInterface(unsigned int) const;   // +0x0C

    virtual void  init(IObject*);                       // vptr +0x14, 0x8259130 -> m_initObject
    virtual void  setObject(IObject*);                  // vptr +0x1C, 0x8259140 -> m_object
    virtual IObject* getObject() const;                 // vptr +0x20, 0x8259150 <- m_object

    virtual bool  hasUpdate() const;                    // vptr +0x24 - base returns false
    virtual void  handleUpdate(float);                  // vptr +0x28 - base is empty

    virtual ~WeaponComp();                              // vptr +0x38 / +0x3C

protected:
    /* +0x04 */ int      m_refCount;
    /* +0x08 */ IObject* m_object;       // acted on; init() caches interior pointers into it.
                                         // must be set (setObject) before init() runs.
    /* +0x0C */ IObject* m_initObject;   // handed to world::getRootParent - hierarchy traversal.
                                         // Distinct member from m_object; do not merge them.
};

// Base projectile component with object/template pointers and reference counting.
// Concrete leaves expose their projectile interfaces and own separate live reference counts.
class ProjectileComp : public IObjectComponent
{
public:
    virtual void  addRef();                             // +0x00
    virtual int   getRef();                             // +0x04
    virtual int   release();                            // vptr +0x08, 0x8289350 - returns new count
    virtual void* queryInterface(unsigned int) const;   // +0x0C

    virtual void  init(IObject*);                       // vptr +0x14, 0x82893C0 -> m_initObject
    virtual void  setObject(IObject*);                  // vptr +0x1C, 0x82893D0 -> m_object
    virtual IObject* getObject() const;                 // vptr +0x20, 0x82893E0 <- m_object

    virtual bool  hasUpdate() const;                    // vptr +0x24 - base returns false
    virtual void  handleUpdate(float);                  // vptr +0x28, 0x8289400 - base is empty

    virtual ~ProjectileComp();

protected:
    /* +0x04 */ int      m_refCount;
    /* +0x08 */ IObject* m_object;
    /* +0x0C */ IObject* m_initObject;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif
