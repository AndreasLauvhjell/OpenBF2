// Path:     attested - the virtual-template diagnostic names
// Game/Objects/Weapons/DeviationComponents/DeviationComp.h in both shipped images.
// Contents: common weapon-deviation runtime recovered from the BF2 1.50 Linux ELF and Win32 PE.

#ifndef GAME_OBJECTS_WEAPONS_DEVIATIONCOMPONENTS_DEVIATIONCOMP_H
#define GAME_OBJECTS_WEAPONS_DEVIATIONCOMPONENTS_DEVIATIONCOMP_H

#include <cstddef>

#include "../FireComponents/FireComp.h"
#include "DeviationCompTemplate.h"
#include "../../../../Core/Random.h"

namespace dice {
namespace hfe {
namespace world {

class GenericFireArm;

// The shipped x86 base is 0x2c bytes.  It is abstract only because the three concrete deviation
// strategies supply setAIDeviation().  All other permissive component operations have real bodies.
class DeviationComp : public WeaponComp, public IWeaponDeviationComp
{
public:
    explicit DeviationComp(DeviationCompTemplate* objectTemplate);
    virtual ~DeviationComp();

    virtual unsigned int getInterfaceID() const;

    virtual void initComponent(WeaponInitEvent&);
    virtual bool preUpdate();
    virtual void updateComponent(float, unsigned int);
    virtual bool isSelectable() const;
    virtual void fireStop(bool);
    virtual bool requestFire(bool, bool);
    virtual void postFire(WeaponFireEvent&);
    virtual void fireComplete(unsigned int);
    virtual void postSwitchFireRate(FireRate);
    virtual bool requestReload(bool);
    virtual void reload(ReloadState);
    virtual void postReload();
    virtual void outOfAmmo(bool);
    virtual void stoppedTracking();
    virtual void postChangeTarget(IObject*);
    virtual void projectileWasReset(IObject*);
    virtual void enable();
    virtual void disable();
    virtual void stop();
    virtual bool busy();
    virtual void exit();
    virtual void destroy();
    virtual void pickup();
    virtual void reset();

    virtual void updateDeviation(io::PlayerInput, bool, bool);
    virtual void setZoom(int);
    virtual Vec3 deviatePosition(Vec3&, int);
    virtual void setProjectileDeviationFixed(Vec3&, int);
    virtual float getMinDeviation() const;
    virtual float getMaxDeviation() const;
    virtual float getDeviation() const;
    virtual void setAIDeviation(float) = 0;

protected:
    GenericFireArm* firearm() const
    {
        return reinterpret_cast<GenericFireArm*>(m_initObject);
    }

    /* +0x14 */ float m_deviation;
    /* +0x18 */ float m_fireDeviation;
    /* +0x1C */ float m_cachedDeviationX;
    /* +0x20 */ float m_cachedDeviationY;
    /* +0x24 */ Random m_random;
    /* +0x28 */ DeviationCompTemplate* m_template;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(DeviationComp) == 0x2C, "DeviationComp x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_DEVIATIONCOMPONENTS_DEVIATIONCOMP_H
