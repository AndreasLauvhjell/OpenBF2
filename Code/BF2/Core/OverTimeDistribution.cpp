// Path:     [INFERRED] - beside its header. Contents: recovered from bf2.i64.

#include "OverTimeDistribution.h"
#include <ostream>
#include <istream>
#include <cstring>

namespace dice {
namespace hfe {

OverTimeDistribution::OverTimeDistribution(float defaultValue)
{
    std::memcpy(&m_field00, &defaultValue, sizeof(defaultValue));
    for (int i = 0; i != 4; ++i) m_authored[i] = 0;
    for (int i = 0; i <= 101; ++i) m_samples[i] = defaultValue;
}

// Reads authored distribution keys and consumes the terminating character.
// Separators, stream state and sample indices are not validated.
std::istream& operator>>(std::istream& stream, OverTimeDistribution& distribution)
{
    for (int i = 0; i != 4; ++i) distribution.m_authored[i] = 0;
    float defaultValue;
    std::memcpy(&defaultValue, &distribution.m_field00, sizeof(defaultValue));
    for (int i = 0; i <= 101; ++i) distribution.m_samples[i] = defaultValue;
    int index;
    float value;
    char separator;
    do
    {
        stream >> index >> separator >> value;
        distribution.setKey(index, value);
        distribution.generateDistribution();
    } while (stream.get() == '|');
    return stream;
}

// Writes authored samples in increasing index order; an untouched curve writes an empty string.
std::ostream& operator<<(std::ostream& stream, const OverTimeDistribution& distribution)
{
    bool first = true;
    for (int i = 0; i <= 101; ++i)
    {
        if ((distribution.m_authored[i >> 5] & (1u << (i & 31))) == 0) continue;
        if (!first) stream << '|';
        stream << i << '/' << distribution.m_samples[i];
        first = false;
    }
    return stream;
}

// Interpolates samples between authored keys, beginning the search at index zero.
// Samples beyond the last authored key are left unchanged.
void OverTimeDistribution::generateDistribution()
{
    int lo = 0;
    for (;;)
    {
        int hi = lo + 1;
        while (hi <= 101)
        {
            if (m_authored[hi >> 5] & (1u << (hi & 31)))
                break;
            ++hi;
        }
        if (hi == 102)                      // no further authored key - done
            break;

        const float span = float(hi - lo);
        for (int j = lo + 1; j < hi; ++j)
            m_samples[j] = (m_samples[lo] * float(hi - j) +
                            m_samples[hi] * float(j - lo)) / span;

        if (hi > 99)                        // the ELF's back-edge bound
            break;
        lo = hi;
    }
}

} // namespace hfe
} // namespace dice
