// Section-sign colour escapes for HUD and menu text.
//
// [CHOSEN, not recovered] Retail reads the four characters that follow a section-sign 'C' toggle
// and never uses them: the toggle always switches to one fixed colour per renderer. This extension
// reads those four characters as hexadecimal digits R, G, B and A (each 0-F, scaled to 0..1) and
// uses that colour while the toggle is on. Shipped strings only ever use the payload "1001", and a
// payload that is not four hexadecimal digits cannot name a colour, so both keep the retail colour
// and unmodified content renders exactly as retail. The closing toggle's payload is ignored, as in
// retail. Local experiment: it needs a linked issue with the approach discussed before submission.
#ifndef MENU_COLORESCAPE_H
#define MENU_COLORESCAPE_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include "../External/BFMeme/Quad.h"
namespace dice { namespace meme {

// Returns the value of one hexadecimal digit, or -1 when the character is not one.
inline int colorEscapeDigit(int character)
{
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    return -1;
}

// Reads the four payload characters through readCharacter(0..3) and stores the colour they name.
// Stops at the first character that is not a hexadecimal digit, so a payload cut short by the end
// of the string is never read past its terminator. Returns false, leaving color untouched, when the
// payload does not name a colour or is the retail "1001".
template <typename ReadCharacter>
bool readColorEscape(ReadCharacter readCharacter, Color* color)
{
    int digits[4];
    for (int i = 0; i != 4; ++i) {
        digits[i] = colorEscapeDigit(readCharacter(i));
        if (digits[i] < 0) return false;
    }
    if (digits[0] == 1 && digits[1] == 0 && digits[2] == 0 && digits[3] == 1) return false;
    color->r = static_cast<float>(digits[0]) / 15.0f;
    color->g = static_cast<float>(digits[1]) / 15.0f;
    color->b = static_cast<float>(digits[2]) / 15.0f;
    color->a = static_cast<float>(digits[3]) / 15.0f;
    return true;
}

} }
#endif
#endif
