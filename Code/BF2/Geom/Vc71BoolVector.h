// Packed Boolean vector with the 20-byte VC7.1 header: bit count followed by word storage.
// Used for road visibility and texture-mirroring flags.


#ifdef BF2_IMAGE_RENDDX9

#ifndef BF2_GEOM_VC71BOOLVECTOR_H
#define BF2_GEOM_VC71BOOLVECTOR_H

#include "UndergrowthSystem.h"       // Vc71Vec<T> - the tree's owning 16-byte VC7.1 vector

namespace dice {
namespace hfe {
namespace geom {

struct Vc71BoolVector
{
    unsigned int           size;     // +0x00  _Mysize - the destructor zeroes it before the words go
    Vc71Vec<unsigned int>  words;    // +0x04  _Myvec

    Vc71BoolVector() : size(0) {}
    ~Vc71BoolVector() { size = 0; }  // then the word vector's own destructor frees it

    void push_back(bool value)
    {
        words.resize((size + 1u + 31u) >> 5, 0u);
        ++size;
        set(size - 1u, value);
    }
    bool test(unsigned int index) const
    {
        return (words[index >> 5] & (1u << (index & 31u))) != 0;
    }
    void set(unsigned int index, bool value)
    {
        unsigned int& word = words[index >> 5];
        const unsigned int mask = 1u << (index & 31u);
        if (value)
            word |= mask;
        else
            word &= ~mask;
    }

private:
    Vc71BoolVector(const Vc71BoolVector&);
    Vc71BoolVector& operator=(const Vc71BoolVector&);
};

#if defined(_M_IX86)
static_assert(sizeof(Vc71BoolVector) == 20, "the VC7.1 vector<bool> is 20 bytes");
#endif

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_GEOM_VC71BOOLVECTOR_H
#endif // BF2_IMAGE_RENDDX9
