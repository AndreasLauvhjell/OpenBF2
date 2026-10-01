// Animation selection and trigger messages for animated objects. [INFERRED] Source placement.
// AnimatedBundle selects first- or third-person animation by index; PlayerControlObject exposes one system.

#ifndef GAME_OBJECTS_IANIMATEDBUNDLE_H     // [INFERRED] guard style
#define GAME_OBJECTS_IANIMATEDBUNDLE_H

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world {

class IAnimatedBundle : public IBase
{
public:
    virtual void  setTriggerMessage(int message) {}          // +0x10 (default)
    virtual void  clearTriggerMessage(int message) {}        // +0x14 (default)
    virtual void  setActiveAnimationSystem(int index) = 0;   // +0x18
    virtual void* getActiveAnimationSystem() = 0;            // +0x1C

protected:
    ~IAnimatedBundle() {}
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IANIMATEDBUNDLE_H
