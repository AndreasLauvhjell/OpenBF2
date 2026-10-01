// Sampled curve with authored keys and linear interpolation. Header path [INFERRED].
// Set sample values and their authored-mask bits before generateDistribution(). It fills only
// gaps between keys; samples outside the authored interval retain their existing values.
// The x86 layout is 0x1AC bytes: one leading word, 102 samples indexed 0..101, then a 128-bit mask.

#ifndef CORE_OVERTIMEDISTRIBUTION_H     // [INFERRED] guard style
#define CORE_OVERTIMEDISTRIBUTION_H

#include <iosfwd>

namespace dice {
namespace hfe {

class OverTimeDistribution
{
public:
    // Inlined in ELF ConsoleClass124; PE 0x4F6970 takes the default sample value.
    explicit OverTimeDistribution(float defaultValue = 1.0f);


    void generateDistribution();                    // fill unauthored samples between authored keys

    // Convenience for owners, matching what every recovered caller does by hand: write a key and
    // mark it authored. [CHOSEN] helper - the ELF inlines both stores at each call site.
    void setKey(int index, float value)
    {
        m_samples[index] = value;
        m_authored[index >> 5] |= (1u << (index & 31));
    }

    int          m_field00;                         // +0x000 default float bits, retained as int
                                                    //   for existing owners; PE ctor 0x4F6970
    float        m_samples[102];                    // +0x004 .. +0x19B  (indices 0..101)
    unsigned int m_authored[4];                     // +0x19C .. +0x1AB - one bit per sample
};                                                  // sizeof 0x1AC

std::ostream& operator<<(std::ostream&, const OverTimeDistribution&);
std::istream& operator>>(std::istream&, OverTimeDistribution&);

} // namespace hfe
} // namespace dice

#endif
