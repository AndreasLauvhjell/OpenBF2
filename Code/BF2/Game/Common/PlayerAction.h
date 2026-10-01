// One frame of player input: six 16-bit axes, low/high button masks, an index and a flag.
// ActionBuffer retains current and queued actions; the manager supplies their wire framing.

#ifndef GAME_COMMON_PLAYERACTION_H     // [INFERRED] guard style
#define GAME_COMMON_PLAYERACTION_H

namespace dice {
namespace hfe {

namespace io { class PlayerInput; class BitStream; }

// A free function IN this same translation unit (0x8087E80, immediately above setAxis), so it
// belongs to PlayerAction.cpp rather than to a maths header. It is the engine's axis quantiser -
// see the .cpp for what "internal resolution" turns out to mean.
float convertAxisToInternalResolution(float value);

class PlayerAction                           // 0x1C
{
public:
    PlayerAction() {}                        // [CHOSEN] - no default ctor exists in the binary,
                                             // Default construction leaves state untouched. The implicit copy constructor
                                             // copies members without initializing tail padding.

    void set(const io::PlayerInput& input);

    // the PlayerAction-to-PlayerAction copy the prediction replay uses.
    // NOT the compiler's operator=: it is written out field by field, six words from +0x04, the
    // two button masks, then m_inputIndex last and the byte at +0x18 - twenty-five of the
    // twenty-eight bytes, in an order no memcpy would produce. [INFERRED] name.
    void set(const PlayerAction& other);

    // returns bool, NOT void - when the body was read. The ELF ends with
    // `mov eax, 1` immediately before the epilogue, so it always answers true. Itanium mangling
    // does not encode return types, so nothing gave this away until the body was disassembled.
    // Every caller ignores it.
    bool get(io::PlayerInput& out) const;
    void print() const;                      //  [FRONTIER]

    // [CHOSEN] int represents the unresolved PlayerInputMap enum for axes 0..5.
    // Restore the typed parameter when that enum is recovered.
    void setAxis(const io::PlayerInput& input, int axis);

    // Uses the same unresolved PlayerInputMap-to-int parameter adaptation as setAxis.
    void getAxis(io::PlayerInput& out, int axis) const;

    // Wire codecs are defined with PlayerActionManager. Encode six sign/magnitude axes,
    // the low mask, nine high-mask bits, and one flag; inputIndex is not transmitted.
    // Both consume and clear the stream error latch, returning false on error.
    bool readFromStream(io::BitStream* stream);
    bool writeToStream(io::BitStream* stream);

    /* +0x00 */ int            m_inputIndex;
    /* +0x04 */ short          m_axis[6];    // verified - see the note above
    /* +0x10 */ unsigned int   m_buttonMaskLow;
    /* +0x14 */ unsigned int   m_buttonMaskHigh;
    /* +0x18 */ unsigned char  m_flag;
    /* +0x19 */ unsigned char  m_pad19[3];   // to 0x1C
};

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_PLAYERACTION_H
