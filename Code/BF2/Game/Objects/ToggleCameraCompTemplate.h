// Camera-toggle settings for remote-controlled and optic weapons.
// Stores camera id and view mode for both initial and toggled states. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_TOGGLECAMERACOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_TOGGLECAMERACOMPTEMPLATE_H

#include "ZoomCompTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class ToggleCameraCompTemplate : public ZoomCompTemplate
{
public:
    ToggleCameraCompTemplate();

    // Leaf interface methods; this inheritance chain has no virtual destructor.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int);
    virtual bool  makeScript(io::IStream*);

    void setChangeCameraId(int value);
    int getChangeCameraId() const { return m_changeCameraId; }
    void setChangeCameraViewMode(int value) { m_changeCameraViewMode = value; }
    int getChangeCameraViewMode() const { return m_changeCameraViewMode; }
    void setStartCameraId(int value) { m_startCameraId = value; }
    int getStartCameraId() const { return m_startCameraId; }
    void setStartCameraViewMode(int value) { m_startCameraViewMode = value; }
    int getStartCameraViewMode() const { return m_startCameraViewMode; }
    void setUseProjectileCamera(bool value) { m_useProjectileCamera = value; }
    bool getUseProjectileCamera() const { return m_useProjectileCamera; }
    void setAllowUntoggledFire(bool value) { m_allowUntoggledFire = value; }
    bool getAllowUntoggledFire() const { return m_allowUntoggledFire; }

protected:
    // layout, offsets 16..39. ZoomCompTemplate occupies 0..15.
    /* 16 */ int  m_refCount;

    /* 20 */ int  m_changeCameraId;
    /* 24 */ int  m_changeCameraViewMode;
    /* 28 */ int  m_startCameraId;
    /* 32 */ int  m_startCameraViewMode;

    /* 36 */ bool m_useProjectileCamera;
    /* 37 */ bool m_allowUntoggledFire;
                 // +0x26..+0x27 is implicit tail padding, untouched by both retail constructors.
};

} // namespace world
} // namespace hfe
} // namespace dice

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(dice::hfe::world::ToggleCameraCompTemplate) == 0x28,
              "ToggleCameraCompTemplate x86 ABI drift");
#endif

#endif // GAME_OBJECTS_TOGGLECAMERACOMPTEMPLATE_H
