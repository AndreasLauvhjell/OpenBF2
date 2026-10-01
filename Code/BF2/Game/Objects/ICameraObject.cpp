// Default camera-interface lighting, damage, and mode-cycle behavior.

#include "ICameraObject.h"

namespace dice {
namespace hfe {
namespace world {

bool  ICameraObject::getAllowDynamicLights() const     { return true; }
bool  ICameraObject::getAllowDynamicSun() const        { return true; }
float ICameraObject::getInsideDamageFrequency() const  { return 0.0f; }
float ICameraObject::getInsideDamageThreshold() const  { return 0.0f; }
bool  ICameraObject::allowCameraModeCycle()            { return true; }
bool  ICameraObject::isOverheadCamera() const          { return false; }

} // namespace world
} // namespace hfe
} // namespace dice
