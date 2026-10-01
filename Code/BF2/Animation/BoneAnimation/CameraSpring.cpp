// Camera-spring configuration, merging, and serialization.

#include "CameraSpring.h"
#include <cmath>
#include <cfloat>
#include <sstream>
#include "../../IO/Stream/IStream.h"

namespace dice {
namespace anim {


// Initializes all camera-spring coefficients and state to zero.

CameraSpring::CameraSpring()
{
    amplitude.x   = 0.0f; amplitude.y   = 0.0f; amplitude.z   = 0.0f;   // +0x00 +0x04 +0x08
    zoomDamping.x = 0.0f; zoomDamping.y = 0.0f; zoomDamping.z = 0.0f;   // +0x0C +0x10 +0x14
    stiffness.x   = 0.0f; stiffness.y   = 0.0f; stiffness.z   = 0.0f;   // +0x18 +0x1C +0x20
    move.x        = 0.0f; move.y        = 0.0f; move.z        = 0.0f;   // +0x24 +0x28 +0x2C
    hit.x         = 0.0f; hit.y         = 0.0f; hit.z         = 0.0f;   // +0x30 +0x34 +0x38
    fire.x        = 0.0f; fire.y        = 0.0f; fire.z        = 0.0f;   // +0x3C +0x40 +0x44
    look.x        = 0.0f; look.y        = 0.0f;                         // +0x48 +0x4C
}

namespace {
bool springVectorIsZero(const hfe::Vec3& value)
{
    const hfe::Vec3& zero = hfe::Vec3::getZero();
    return std::fabs(value.x - zero.x) <= FLT_EPSILON
        && std::fabs(value.y - zero.y) <= FLT_EPSILON
        && std::fabs(value.z - zero.z) <= FLT_EPSILON;
}
}

// Nonzero vectors replace whole fields, including zero components; look tests summed absolute components.
// Preserve field order: look precedes hit/fire.
void CameraSpring::merge(const CameraSpring* other)
{
    if (!springVectorIsZero(other->amplitude))   amplitude = other->amplitude;
    if (!springVectorIsZero(other->zoomDamping)) zoomDamping = other->zoomDamping;
    if (!springVectorIsZero(other->stiffness))   stiffness = other->stiffness;
    if (!springVectorIsZero(other->move))        move = other->move;
    if (!(std::fabs(other->look.x) + std::fabs(other->look.y) <= FLT_EPSILON))
        look = other->look;
    if (!springVectorIsZero(other->hit))         hit = other->hit;
    if (!springVectorIsZero(other->fire))        fire = other->fire;
}

// Writes use unconditionally and other fields when nonzero under their epsilon tests.
// Preserve unordered-value behavior and look-before-hit/fire order.
void CameraSpring::makeScript(hfe::io::IStream* stream)
{
    std::ostringstream output;
    output << "animationSystem.cameraSpring.use 1" << std::endl;
    if (!springVectorIsZero(amplitude))
        output << "animationSystem.cameraSpring.amplitude " << amplitude.x << '/' << amplitude.y << '/' << amplitude.z << std::endl;
    if (!springVectorIsZero(zoomDamping))
        output << "animationSystem.cameraSpring.zoomDamping " << zoomDamping.x << '/' << zoomDamping.y << '/' << zoomDamping.z << std::endl;
    if (!springVectorIsZero(stiffness))
        output << "animationSystem.cameraSpring.stiffness " << stiffness.x << '/' << stiffness.y << '/' << stiffness.z << std::endl;
    if (!springVectorIsZero(move))
        output << "animationSystem.cameraSpring.move " << move.x << '/' << move.y << '/' << move.z << std::endl;
    if (!(std::fabs(look.x) + std::fabs(look.y) <= FLT_EPSILON))
        output << "animationSystem.cameraSpring.look " << look.x << '/' << look.y << std::endl;
    if (!springVectorIsZero(hit))
        output << "animationSystem.cameraSpring.hit " << hit.x << '/' << hit.y << '/' << hit.z << std::endl;
    if (!springVectorIsZero(fire))
        output << "animationSystem.cameraSpring.fire " << fire.x << '/' << fire.y << '/' << fire.z << std::endl;
    const std::string text = output.str();
    stream->write(text.c_str(), static_cast<unsigned int>(text.size()));
}

} // namespace anim
} // namespace dice
