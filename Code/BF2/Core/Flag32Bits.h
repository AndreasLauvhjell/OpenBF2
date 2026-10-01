// Typed set/clear masks over a 32-bit flag word; header path [INFERRED].
// Each template type has a separate used-bit registry. Registering a bit twice deliberately faults.
// Updates apply (word | set) & ~clear; tests require every set bit and exclude every clear bit.

#ifndef CORE_FLAG32BITS_H     // [INFERRED] guard style
#define CORE_FLAG32BITS_H

namespace dice {
namespace hfe {

template <typename T>
struct Flag32Bits
{
    unsigned int set;                                // +0x00 - the bits to set
    unsigned int clear;                              // +0x04 - the bits to clear

    Flag32Bits() : set(0), clear(0) {}

    // (the IPlayer instantiation): registers the bit; a repeat is the null write.
    explicit Flag32Bits(int bit)
    {
        const unsigned int mask = 1u << bit;
        set = 0;
        clear = 0;
        if (usedFlags() & mask)
        {
            *static_cast<volatile int*>(0) = 0;      // the shipped assert: `mov ds:0, 0`
            return;
        }
        set = mask;
        usedFlags() |= mask;
    }

    // The readers, as Player has them (Player::updateFlags / testFlags).
    unsigned int apply(unsigned int word) const     { return (word | set) & ~clear; }
    bool test(unsigned int word) const               { return (set & word) == set && (clear & ~word) == clear; }

private:
    static unsigned int& usedFlags()                 // the per-T registry (a function-local static
    {                                                //   in the ELF: Flag32Bits(int)::usedFlags)
        static unsigned int s_usedFlags = 0;
        return s_usedFlags;
    }
};

} // namespace hfe
} // namespace dice

#endif // CORE_FLAG32BITS_H
