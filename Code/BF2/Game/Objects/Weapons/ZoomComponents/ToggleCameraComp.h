// Path:     [INFERRED] beside the recovered ZoomComp/DefaultZoomComp family.
// Contents: camera-switching zoom component recovered from the BF2 1.50 retail binaries.

#ifndef GAME_OBJECTS_WEAPONS_ZOOMCOMPONENTS_TOGGLECAMERACOMP_H
#define GAME_OBJECTS_WEAPONS_ZOOMCOMPONENTS_TOGGLECAMERACOMP_H

#include "ZoomComp.h"
#include "../../ToggleCameraCompTemplate.h"
#include "../../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {
namespace world {

class IProjectileObject;

class ToggleCameraComp : public ZoomComp
{
public:
    explicit ToggleCameraComp(ToggleCameraCompTemplate* objectTemplate);
    virtual ~ToggleCameraComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool requestFire(bool released, bool alternate);
    virtual void postFire(WeaponFireEvent& event);
    virtual void enable();
    virtual void reset();
    virtual int getPrevCameraId() const;
    virtual int getChangeCameraId() const;
    virtual void toggle(TemplateMessage message);
    virtual void changeTarget(int targetId);
    virtual int getCameraNr() const;
    virtual bool allowTargetSeek() const;
    virtual void changeViewMode();

    // A concrete helper in retail, absent from both the primary and secondary vtables.
    bool fireAllowed() const;

private:
    /* +0x18 */ int m_leafRefCount;
    /* +0x1C */ int m_prevCameraId;
    /* +0x20 */ WeakPtr<IProjectileObject> m_projectile;
    /* +0x24 */ ToggleCameraCompTemplate* m_toggleTemplate;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(ToggleCameraComp) == 0x28, "ToggleCameraComp x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_ZOOMCOMPONENTS_TOGGLECAMERACOMP_H
