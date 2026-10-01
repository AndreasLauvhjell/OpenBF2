// First-person camera shake configuration. [INFERRED] header placement.
// Six Vec3 values and one Vec2 begin at zero; configure them before enabling the spring.
// The object has no virtual interface or owned resources.

#ifndef ANIMATION_BONEANIMATION_CAMERASPRING_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_CAMERASPRING_H

// Vec2 and Vec3 both live in Vec3.h in this tree, and both are typedefs onto dice::hfe::Math -
// forward-declaring either as a struct is a hard error. Include the header.
#include "../../Math/Vec3.h"

namespace dice {

namespace hfe { namespace io { class IStream; } }

namespace anim {

class CameraSpring
{
public:
    CameraSpring();

    // makeScript is what AnimationSystemTemplate::save calls before bundles/triggers.
    void makeScript(hfe::io::IStream* stream);
    void merge(const CameraSpring* other);

    // Console bindings read and write these fields directly.
    /* +0x00 */ hfe::Vec3 amplitude;      // cc162
    /* +0x0C */ hfe::Vec3 zoomDamping;    // cc163
    /* +0x18 */ hfe::Vec3 stiffness;      // cc164
    /* +0x24 */ hfe::Vec3 move;           // cc166
    /* +0x30 */ hfe::Vec3 hit;            // cc167  bound, but no shipped .inc sets it
    /* +0x3C */ hfe::Vec3 fire;           // cc168
    /* +0x48 */ hfe::Vec2 look;           // cc165  - the only Vec2, and the last eight bytes
};

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_CAMERASPRING_H
