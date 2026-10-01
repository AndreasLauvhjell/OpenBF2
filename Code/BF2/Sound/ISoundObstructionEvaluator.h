// Callback for evaluating obstruction between a sound and its listener.

#ifndef SOUND_ISOUNDOBSTRUCTIONEVALUATOR_H     // [INFERRED] guard style
#define SOUND_ISOUNDOBSTRUCTIONEVALUATOR_H

namespace dice {
namespace hfe {

class ISound;
namespace Math { struct Vec3; }

class ISoundObstructionEvaluator
{
public:
    virtual float evaluate(ISound* sound, const Math::Vec3* position) = 0;    // +0x00

protected:
    ~ISoundObstructionEvaluator() {}
};

} // namespace hfe
} // namespace dice

#endif // SOUND_ISOUNDOBSTRUCTIONEVALUATOR_H
