// Projectile interface face with a weak-control block; destruction clears its target before dropping the reference.
// The original x86 face is eight bytes. Full virtual declarations are intentionally deferred:
// declare the complete interface and all overrides atomically before dispatching through this pointer.
// Until then use the primary table, typed owner/lifetime seams or a named [FRONTIER].
// Preserve the concrete fatal set/getLauncherWeapon defaults when restoring that interface.
// [PLATFORM] PE has one destructor slot versus ELF's pair. Header placement is [INFERRED].

#ifndef GAME_OBJECTS_PROJECTILES_IPROJECTILEOBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_PROJECTILES_IPROJECTILEOBJECT_H

#include "../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {
namespace world {

class IProjectileObject
{
public:
    // 0x83294xx, trivial: vptr + null block.
    IProjectileObject()
        : m_weakControl(0)
    {
    }

    // 58-ish B: null the target out of every live weak pointer, drop the block.
    virtual ~IProjectileObject()
    {
        WeakPtr<IProjectileObject>::Ctrl* control = m_weakControl;
        if (control)
        {
            control->target = 0;
            if (control->refCount-- == 1)
                ::operator delete(control);
        }
    }

    /* +0x04 */ WeakPtr<IProjectileObject>::Ctrl* m_weakControl;   // ctor 0
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_IPROJECTILEOBJECT_H
