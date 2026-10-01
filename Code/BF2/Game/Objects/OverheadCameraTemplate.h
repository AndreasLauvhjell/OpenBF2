// Template for the commander's overhead camera.

#ifndef GAME_OBJECTS_OVERHEADCAMERATEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_OVERHEADCAMERATEMPLATE_H

#include "Base/BundleTemplate.h"

namespace dice {
namespace hfe {
namespace hud { class BF2HudManager; }
namespace world {

struct BaseTemplateConsoleAccess;

// Overhead-camera template. Interface lookup follows BundleTemplate's inherited subobjects.
class OverheadCameraTemplate : public BundleTemplate
{
public:
    OverheadCameraTemplate();

    virtual void  addRef();                          // refcount +0x17C
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;   // the table quoted above
    virtual unsigned int getClassID() const;
    virtual ~OverheadCameraTemplate();               // (D1) / 65 B (D0)
    virtual void* queryImplementation(unsigned int); // one-cid hop -> Bundle
    virtual IObject* createObject() const;           // new(0x210) OverheadCamera,
    virtual bool  makeScript(io::IStream*);          // ELF 829 B / PE 124 B - complete empty writer

protected:
    // The object half reads FOV, speed and height directly - the sibling arrangement.
    friend class OverheadCamera;
    friend class ::dice::hfe::hud::BF2HudManager; // PE7513E0 reads height after the template CID check
    friend struct BaseTemplateConsoleAccess;       // recovered ObjectTemplate.* binding executors

    /* 380 */ int   m_refCount3;                     // ctor 1
    /* 384 */ float m_activeFOV;                     // ctor 2.3f  (0x40133333)
    // Named from the console properties that write them. Note +384's
    // .con property is `fOV` while the member is `m_activeFOV` - the two naming domains again.
    /* 388 */ float m_speed;                         // was `_dword184`; .con `speed`;  ctor 10.0f
    /* 392 */ float m_height;                        // was `_float188`; .con `height`; ctor 160.0f
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_OVERHEADCAMERATEMPLATE_H
