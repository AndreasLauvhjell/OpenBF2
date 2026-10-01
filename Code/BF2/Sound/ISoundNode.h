// Sound source node interface.


#ifndef SOUND_ISOUNDNODE_H     // [INFERRED] guard style
#define SOUND_ISOUNDNODE_H

namespace dice {
namespace hfe {

struct Mat4;
namespace Math { struct Vec3; }

class ISoundNode
{
public:
    virtual const Mat4& getTransform() = 0;                  // +0x00
    virtual const Math::Vec3* getVelocity() = 0;             // +0x04 [CHOSEN return type]

protected:
    ~ISoundNode() {}
};

} // namespace hfe
} // namespace dice

#endif // SOUND_ISOUNDNODE_H
