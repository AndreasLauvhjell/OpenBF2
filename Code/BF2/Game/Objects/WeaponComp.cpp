// Path:     [INFERRED] beside WeaponComp.h — placement is a guess, see that header.
// Contents: implementation. All eleven members read end to end in bf2.i64; addresses cite the ELF.
// `ProjectileComp` is byte-for-byte the same class with a different name and a different subclass
// set — same members, same sizes, same offsets. Everything below applies to it unchanged.

#include "WeaponComp.h"

#include "../../Core/Ids.h"

namespace dice {
namespace hfe {
namespace world {

// 15 bytes. Plain increment — NO `lock` prefix, so these components are single-threaded
// by construction. That is a constraint a rebuild must not quietly relax by using an atomic.
void WeaponComp::addRef()
{
    ++m_refCount;               // +0x04
}

int WeaponComp::getRef()
{
    return m_refCount;
}

// Decrements and returns the reference count, deleting through the virtual destructor at zero.
int WeaponComp::release()
{
    int remaining = --m_refCount;

    if (remaining == 0 && this != 0)
    {
        // Dispatched through vtable slot +0x3C, the deleting destructor. In C++ source that is
        // `delete this`; the compiler emits the same slot.
        delete this;
    }

    return remaining;
}

// 32 bytes. WeaponComp exposes only its IBase/IObjectComponent face; specialized
// components add their own IID in their override.
void* WeaponComp::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return const_cast<WeaponComp*>(this);
    return 0;
}

// 14 bytes. Stores its argument at +0x0C.
// this IS NOT `setObject`. The two take the same parameter type and are easy to conflate, but
// they write different fields: init -> +0x0C, setObject -> +0x08. `ArmorGLComp::init` shows what
// each is for — it caches interior pointers off m_object (+0x08) and hands m_initObject (+0x0C) to
// `world::getRootParent` for hierarchy traversal. Collapsing them into one member compiles, runs,
// and is subtly wrong.
void WeaponComp::init(IObject* object)
{
    m_initObject = object;      // +0x0C
}

// setObject must precede init because initialization caches pointers derived from the object.
void WeaponComp::setObject(IObject* object)
{
    m_object = object;          // +0x08
}

IObject* WeaponComp::getObject() const
{
    return m_object;
}

// / 0x8259170 — `xor eax,eax / retn` and an empty body. Defaults for subclasses; a
// component that does not override these is simply never ticked.
bool WeaponComp::hasUpdate() const
{
    return false;
}

void WeaponComp::handleUpdate(float)
{
}

// / 0x8259120, 21 and 14 bytes.
WeaponComp::~WeaponComp()
{
}


// Shared projectile-component lifecycle. setObject must precede init; they populate different fields.
// release returns the remaining count and deletes at zero. Derived interfaces use their own reference counters.
void ProjectileComp::addRef()
{
    ++m_refCount;
}

int ProjectileComp::getRef()
{
    return m_refCount;
}

int ProjectileComp::release()
{
    int remaining = --m_refCount;

    if (remaining == 0 && this != 0)
        delete this;

    return remaining;
}

void* ProjectileComp::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return const_cast<ProjectileComp*>(this);
    return 0;
}

void ProjectileComp::init(IObject* object)           // > m_initObject
{
    m_initObject = object;
}

void ProjectileComp::setObject(IObject* object)      // > m_object
{
    m_object = object;
}

IObject* ProjectileComp::getObject() const
{
    return m_object;
}

bool ProjectileComp::hasUpdate() const
{
    return false;
}

void ProjectileComp::handleUpdate(float)
{
}

ProjectileComp::~ProjectileComp()
{
}

} // namespace world
} // namespace hfe
} // namespace dice
