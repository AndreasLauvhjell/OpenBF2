// Piecewise-linear sound envelopes with optional randomized points.

// Piecewise-linear envelope with clamped, sorted control points and optional randomized rows.
// getValue selects one of 64 rows by sample and interpolates between neighboring points.


#ifndef SOUND_SOUNDENGINE_SOUNDEVALUATOR_H
#define SOUND_SOUNDENGINE_SOUNDEVALUATOR_H

#include "../ISoundEvaluator.h"

#include <vector>

namespace dice {
namespace hfe {

// The tree's Sound/SoundObjectTemplate.cpp owns the definition (3 floats); the DLL sees the same
// twelve bytes. Redeclared here under the DLL's own name to keep this image self-contained.
struct EnvelopePoint
{
    float x;
    float yMin;
    float yMax;
};

// 0x38 bytes: vptr, the four bounds, the flag, the points, the baked table.
class SoundEvaluator : public ISoundEvaluator
{
public:
    // Copies `count` points and bakes the table.
    SoundEvaluator(float xMin, float xMax, float yMin, float yMax, bool randomize,
                   unsigned int count, const EnvelopePoint* points);
    // Destruction is non-virtual; owners must delete through the concrete type.
    ~SoundEvaluator() {}

    virtual float getValue(float x, int sample);

    void rebuild();

    float                      m_xMin;          // +0x04
    float                      m_xMax;          // +0x08
    float                      m_yMin;          // +0x0C
    float                      m_yMax;          // +0x10
    bool                       m_randomize;     // +0x14
    std::vector<EnvelopePoint> m_points;        // +0x18 (VC7.1: pad, first, last, end)
    std::vector<float>         m_values;        // +0x28 rows * points.size()
};

// (in the Ambient TU, inlined into Sound.cpp's setters too): give `slot` a new
// evaluator, deleting the old one unless it is the same object.
inline void assignEvaluator(SoundEvaluator*& slot, SoundEvaluator* e)
{
    if (e == slot)
    {
        slot = e;
    }
    else
    {
        if (slot)
            delete slot;
        slot = e;
    }
}

// delete what `holder` holds - and leave the pointer as it was, as shipped.
inline void deleteEvaluator(SoundEvaluator*& holder)
{
    if (holder)
        delete holder;
}

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_SOUNDEVALUATOR_H
