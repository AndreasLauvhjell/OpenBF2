// Default ammo component adds reference counting and a second typed template pointer to AmmoComp.
// Both template fields hold the same value; all ammo behavior is inherited. Header placement is [INFERRED].

#ifndef GAME_OBJECTS_WEAPONS_AMMOCOMPONENTS_DEFAULTAMMOCOMP_H
#define GAME_OBJECTS_WEAPONS_AMMOCOMPONENTS_DEFAULTAMMOCOMP_H

#include "AmmoComp.h"

namespace dice {
namespace hfe {
namespace world {

class DefaultAmmoCompTemplate;

class DefaultAmmoComp : public AmmoComp
{
public:
    explicit DefaultAmmoComp(DefaultAmmoCompTemplate* objectTemplate);
    virtual ~DefaultAmmoComp();

    // The exact quartet is present in both its primary and adjusted IWeaponAmmoComp tables.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

private:
    /* +0x5C */ int m_refCount;
    /* +0x60 */ DefaultAmmoCompTemplate* m_defaultTemplate;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_AMMOCOMPONENTS_DEFAULTAMMOCOMP_H
