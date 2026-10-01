// Rotational floater backed by PhysicsFloatingBundle.
// Original source placement is Game/Objects/Vehicle/Floatingbundle.cpp.

#ifndef GAME_OBJECTS_GAME_FLOATINGBUNDLE_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_FLOATINGBUNDLE_H

#include "RotationalBundle.h"

namespace dice {
namespace hfe {
namespace world {

class FloatingBundleTemplate;

class FloatingBundle : public RotationalBundle
{
public:
    explicit FloatingBundle(const FloatingBundleTemplate* objectTemplate);
    virtual ~FloatingBundle();

    virtual void init();
    virtual void disableCachedObject();
    virtual void enableCachedObject();
    virtual void handleUpdate(float, unsigned int);
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);

protected:
    /* 0x1B0 */ float m_floaterAngle;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_FLOATINGBUNDLE_H
