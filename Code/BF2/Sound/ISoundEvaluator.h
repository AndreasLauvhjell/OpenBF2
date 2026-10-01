// Sound evaluator interface. Preserve virtual declaration order.


#ifndef SOUND_ISOUNDEVALUATOR_H     // [INFERRED] guard style
#define SOUND_ISOUNDEVALUATOR_H

namespace dice {
namespace hfe {

class ISound;

class ISoundEvaluator
{
public:
    virtual float getValue(float value, int sample) = 0;     // +0x00

protected:
    ~ISoundEvaluator() {}
};

class ScaleEvaluator : public ISoundEvaluator
{
public:
    ScaleEvaluator() : m_sound(0), m_scale(1.0f) {}
    virtual float getValue(float value, int sample);

    /* 0x04 */ ISound* m_sound;
    /* 0x08 */ float   m_scale;
};

// Pass-through volume evaluator; returns its input unchanged.
class FixedVolumeEvaluator : public ISoundEvaluator
{
public:
    virtual float getValue(float value, int) { return value; }
};

} // namespace hfe
} // namespace dice

#endif // SOUND_ISOUNDEVALUATOR_H
