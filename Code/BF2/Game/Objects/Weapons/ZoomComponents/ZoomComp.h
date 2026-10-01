// Path:     [INFERRED] beside the attested weapon-component families.
// Contents: typed zoom-component interface and recovered ZoomComp base, cross-read from the
// BF2 1.50 Linux ELF and Win32 PE.  Typed calls are mandatory here: Itanium emits two
// interface-destructor entries while MSVC emits one, shifting every zoom-specific slot.

#ifndef GAME_OBJECTS_WEAPONS_ZOOMCOMPONENTS_ZOOMCOMP_H
#define GAME_OBJECTS_WEAPONS_ZOOMCOMPONENTS_ZOOMCOMP_H

#include "../FireComponents/FireComp.h"
#include "../../ZoomCompTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class GenericFireArm;

class IWeaponZoomComp : public IWeaponComp
{
public:
    virtual ~IWeaponZoomComp();

    virtual void toggle(TemplateMessage) = 0;
    virtual void changeViewMode() = 0;
    virtual bool getSignalToggle() = 0;
    virtual int getPrevCameraId() const = 0;
    virtual void setCameraNr(int) = 0;
    virtual int getCameraNr() const = 0;
    virtual bool allowTargetSeek() const = 0;
    virtual io::PlayerInputMap getInput() const = 0;
    virtual int getChangeCameraId() const = 0;
    virtual bool canZoom() = 0;
    virtual bool allowEffects() const = 0;
    virtual float getMouseSpeedMod() { return 1.0f; }
    virtual float getZoomFactor(unsigned int) const { return -1.0f; }
};

// Shipped x86 size: 0x18.  The base remains abstract because its two IBase faces are completed by
// concrete leaves, which own their effective reference count and queryInterface implementation.
class ZoomComp : public WeaponComp, public IWeaponZoomComp
{
public:
    explicit ZoomComp(ZoomCompTemplate* objectTemplate);
    virtual ~ZoomComp();

    virtual unsigned int getInterfaceID() const;

    // Declaration order is the recovered primary ZoomComp vtable order.  Several members also
    // satisfy the IWeaponComp/IWeaponZoomComp secondary face; do not reorder them.
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
    virtual int getPrevCameraId() const;
    virtual int getChangeCameraId() const;
    virtual bool getSignalToggle();
    virtual void toggle(TemplateMessage);
    virtual void changeTarget(int);
    virtual void setCameraNr(int);
    virtual int getCameraNr() const;
    virtual bool allowTargetSeek() const;
    virtual void changeViewMode();
    virtual float getZoomFactor(int) const;
    virtual bool allowEffects() const;
    virtual io::PlayerInputMap getInput() const;
    virtual bool canZoom();

protected:
    bool is1P() const;
    GenericFireArm* firearm() const
    {
        return reinterpret_cast<GenericFireArm*>(m_initObject);
    }

    /* +0x14 */ ZoomCompTemplate* m_template;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(ZoomComp) == 0x18, "ZoomComp x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_ZOOMCOMPONENTS_ZOOMCOMP_H
