// Owns the projectile's named Looping sound; absent sound templates leave the component inert.
// Binds the projectile's ISoundNode so the sound engine tracks transform/velocity; component updates are empty.
// [PLATFORM] Use typed base conversion because rebuilt ISoundNode offsets differ. Placement is [INFERRED].

#ifndef GAME_OBJECTS_PROJECTILES_SOUNDCOMPONENTS_DEFAULTPROJECTILESOUNDCOMP_H
#define GAME_OBJECTS_PROJECTILES_SOUNDCOMPONENTS_DEFAULTPROJECTILESOUNDCOMP_H

#include "../GenericProjectile.h"      // IProjectileSoundComp
#include "../../WeaponComp.h"          // ProjectileComp

namespace dice {
namespace hfe {

class ISound;

namespace world {

class DefaultProjectileSoundCompTemplate;
class SimpleObjectTemplate;

class DefaultProjectileSoundComp : public ProjectileComp, public IProjectileSoundComp
{
public:
    explicit DefaultProjectileSoundComp(
        DefaultProjectileSoundCompTemplate* objectTemplate);
    virtual ~DefaultProjectileSoundComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool hasUpdate() const;
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void activate();

    virtual void initSoundComp(const SimpleObjectTemplate* soundTemplate);
    virtual void reset();

private:
    /* +0x14 */ int m_refCount;
    // The one sound. NullSound.h now owns the complete 30-slot ISound interface, so this stays a
    // typed lifetime/parenting edge in both ports rather than repeating raw vtable offsets here.
    /* +0x18 */ ISound* m_sound;
    /* +0x1C */ DefaultProjectileSoundCompTemplate* m_template;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_SOUNDCOMPONENTS_DEFAULTPROJECTILESOUNDCOMP_H
