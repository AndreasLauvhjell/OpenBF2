// Sound node attached to an engine object.


#ifndef SOUND_OBJECTSOUNDNODE_H     // [INFERRED] guard style
#define SOUND_OBJECTSOUNDNODE_H

#include "ISoundNode.h"

namespace dice {
namespace hfe {
namespace world { class IObject; }

class ObjectSoundNodeFixed : public ISoundNode
{
public:
    explicit ObjectSoundNodeFixed(world::IObject* object) : m_object(object) {}
    virtual const Mat4& getTransform();
    virtual const Math::Vec3* getVelocity();                                      // 0

    /* 0x04 */ world::IObject* m_object;
};

class ObjectSoundNode : public ISoundNode
{
public:
    explicit ObjectSoundNode(world::IObject* object) : m_object(object) {}
    virtual const Mat4& getTransform();
    virtual const Math::Vec3* getVelocity();

    /* 0x04 */ world::IObject* m_object;
};

} // namespace hfe
} // namespace dice

#endif // SOUND_OBJECTSOUNDNODE_H
