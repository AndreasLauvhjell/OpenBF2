// Shared zoom-component settings. The console surface is split between DefaultZoom and ToggleCamera subclasses.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_ZOOMCOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_ZOOMCOMPTEMPLATE_H

#include "CompTemplateInterfaces.h"
#include "../../IO/Console/IConsoleSaveable.h"
#include "../../IO/Input/Input.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;

// Abstract zoom configuration; leaves implement reference counting and component creation.
// This base has no virtual destructor.
class ZoomCompTemplate : public IWeaponCompTemplate, public io::IConsoleSaveable
{
public:
    ZoomCompTemplate();

    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent() = 0;  // pure here, unlike Fire/Ammo/Deviation
    virtual void  objectCreated(IObject*);
    virtual void  objectDestroyed(IObject*);
    virtual void  setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual bool  makeScript(io::IStream*);           // +0x30

    void setCameraInput(io::PlayerInputMap input) { m_cameraInput = input; }
    io::PlayerInputMap getCameraInput() const { return m_cameraInput; }
    void setOnlyZoomWhenProne(bool value) { m_onlyZoomWhenProne = value; }
    bool getOnlyZoomWhenProne() const { return m_onlyZoomWhenProne; }

protected:
    /*  8 */ io::PlayerInputMap m_cameraInput;
    /* 12 */ bool  m_onlyZoomWhenProne;
    /* 13 */ bool  _byteD;                            // [UNRESOLVED]
    /* 14 */ bool  _byteE;                            // [UNRESOLVED]
    /* 15 */ bool  _byteF;                            // [UNRESOLVED]
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(ZoomCompTemplate) == 0x10, "ZoomCompTemplate x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ZOOMCOMPTEMPLATE_H
