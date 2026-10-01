// Stream conversion for input enums. [INFERRED] Source placement.
// Unknown tokens retain a default without setting failbit: keys use 255, other enums use zero.
// Key readers also accept lowercase function-key names; unnamed values need not round-trip.

#include "Input.h"

#include <istream>
#include <ostream>
#include <string>

namespace dice {
namespace hfe {
namespace io {

// operator<<(InputDeviceTypes). IDTUndefined is both case 0 and the fallback.
std::ostream& operator<<(std::ostream& stream, InputDeviceTypes value)
{
    switch (value)
    {
    case IDTUndefined:        return stream << "IDTUndefined";
    case IDTMouse:            return stream << "IDTMouse";
    case IDTGameController:   return stream << "IDTGameController";
    case IDTKeyboard:         return stream << "IDTKeyboard";
    default:  break;
    }
    return stream << "IDTUndefined";
}

// operator>>(InputDeviceTypes).
std::istream& operator>>(std::istream& stream, InputDeviceTypes& value)
{
    std::string token;
    stream >> token;
    value = IDTUndefined;                 // stored before the chain, and kept on no match
    if (token.compare("IDTUndefined") == 0) value = IDTUndefined;
    else if (token.compare("IDTMouse") == 0) value = IDTMouse;
    else if (token.compare("IDTGameController") == 0) value = IDTGameController;
    else if (token.compare("IDTKeyboard") == 0) value = IDTKeyboard;
    return stream;
}

// operator<<(InputDeviceFlags). A bitmask, but the printer only names exact single values.
std::ostream& operator<<(std::ostream& stream, InputDeviceFlags value)
{
    switch (value)
    {
    case IDFNone:             return stream << "IDFNone";
    case IDFMouse:            return stream << "IDFMouse";
    case IDFKeyboard:         return stream << "IDFKeyboard";
    case IDFGameController_0: return stream << "IDFGameController_0";
    case IDFGameController_1: return stream << "IDFGameController_1";
    case IDFGameController_2: return stream << "IDFGameController_2";
    case IDFGameController_3: return stream << "IDFGameController_3";
    case IDFGameController_4: return stream << "IDFGameController_4";
    case IDFGameController_5: return stream << "IDFGameController_5";
    case IDFGameController_6: return stream << "IDFGameController_6";
    case IDFGameController_7: return stream << "IDFGameController_7";
    case IDFFalcon:           return stream << "IDFFalcon";
    case IDFAll:              return stream << "IDFAll";
    default:  break;
    }
    return stream << "IDFNone";
}

// operator>>(InputDeviceFlags).
std::istream& operator>>(std::istream& stream, InputDeviceFlags& value)
{
    std::string token;
    stream >> token;
    value = IDFNone;                 // stored before the chain, and kept on no match
    if (token.compare("IDFNone") == 0) value = IDFNone;
    else if (token.compare("IDFMouse") == 0) value = IDFMouse;
    else if (token.compare("IDFKeyboard") == 0) value = IDFKeyboard;
    else if (token.compare("IDFGameController_0") == 0) value = IDFGameController_0;
    else if (token.compare("IDFGameController_1") == 0) value = IDFGameController_1;
    else if (token.compare("IDFGameController_2") == 0) value = IDFGameController_2;
    else if (token.compare("IDFGameController_3") == 0) value = IDFGameController_3;
    else if (token.compare("IDFGameController_4") == 0) value = IDFGameController_4;
    else if (token.compare("IDFGameController_5") == 0) value = IDFGameController_5;
    else if (token.compare("IDFGameController_6") == 0) value = IDFGameController_6;
    else if (token.compare("IDFGameController_7") == 0) value = IDFGameController_7;
    else if (token.compare("IDFFalcon") == 0) value = IDFFalcon;
    else if (token.compare("IDFAll") == 0) value = IDFAll;
    return stream;
}

// operator<<(InputDeviceAxes). A 16-case jump table at ds:0x88DF9C8.
std::ostream& operator<<(std::ostream& stream, InputDeviceAxes value)
{
    switch (value)
    {
    case IDAxis_0:            return stream << "IDAxis_0";
    case IDAxis_1:            return stream << "IDAxis_1";
    case IDAxis_2:            return stream << "IDAxis_2";
    case IDAxis_3:            return stream << "IDAxis_3";
    case IDAxis_4:            return stream << "IDAxis_4";
    case IDAxis_5:            return stream << "IDAxis_5";
    case IDAxis_6:            return stream << "IDAxis_6";
    case IDAxis_7:            return stream << "IDAxis_7";
    case IDAxis_8:            return stream << "IDAxis_8";
    case IDAxis_9:            return stream << "IDAxis_9";
    case IDAxis_10:           return stream << "IDAxis_10";
    case IDAxis_11:           return stream << "IDAxis_11";
    case IDAxis_12:           return stream << "IDAxis_12";
    case IDAxis_13:           return stream << "IDAxis_13";
    case IDAxis_14:           return stream << "IDAxis_14";
    case IDAxis_None:         return stream << "IDAxis_None";
    default:  break;
    }
    return stream << "IDAxis_None";
}

// operator>>(InputDeviceAxes).
// the default is IDAxis_0, not IDAxis_None - an unknown axis name reads as axis 0.
std::istream& operator>>(std::istream& stream, InputDeviceAxes& value)
{
    std::string token;
    stream >> token;
    value = IDAxis_0;                 // stored before the chain, and kept on no match
    if (token.compare("IDAxis_0") == 0) value = IDAxis_0;
    else if (token.compare("IDAxis_1") == 0) value = IDAxis_1;
    else if (token.compare("IDAxis_2") == 0) value = IDAxis_2;
    else if (token.compare("IDAxis_3") == 0) value = IDAxis_3;
    else if (token.compare("IDAxis_4") == 0) value = IDAxis_4;
    else if (token.compare("IDAxis_5") == 0) value = IDAxis_5;
    else if (token.compare("IDAxis_6") == 0) value = IDAxis_6;
    else if (token.compare("IDAxis_7") == 0) value = IDAxis_7;
    else if (token.compare("IDAxis_8") == 0) value = IDAxis_8;
    else if (token.compare("IDAxis_9") == 0) value = IDAxis_9;
    else if (token.compare("IDAxis_10") == 0) value = IDAxis_10;
    else if (token.compare("IDAxis_11") == 0) value = IDAxis_11;
    else if (token.compare("IDAxis_12") == 0) value = IDAxis_12;
    else if (token.compare("IDAxis_13") == 0) value = IDAxis_13;
    else if (token.compare("IDAxis_14") == 0) value = IDAxis_14;
    else if (token.compare("IDAxis_None") == 0) value = IDAxis_None;
    return stream;
}

// operator<<(InputDeviceButtons).
// the fallback label is the bare word "None", which no reader accepts.
std::ostream& operator<<(std::ostream& stream, InputDeviceButtons value)
{
    switch (value)
    {
    case IDButton_0:          return stream << "IDButton_0";
    case IDButton_1:          return stream << "IDButton_1";
    case IDButton_2:          return stream << "IDButton_2";
    case IDButton_3:          return stream << "IDButton_3";
    case IDButton_4:          return stream << "IDButton_4";
    case IDButton_5:          return stream << "IDButton_5";
    case IDButton_6:          return stream << "IDButton_6";
    case IDButton_7:          return stream << "IDButton_7";
    case IDButton_8:          return stream << "IDButton_8";
    case IDButton_9:          return stream << "IDButton_9";
    case IDButton_10:         return stream << "IDButton_10";
    case IDButton_11:         return stream << "IDButton_11";
    case IDButton_12:         return stream << "IDButton_12";
    case IDButton_13:         return stream << "IDButton_13";
    case IDButton_14:         return stream << "IDButton_14";
    case IDButton_15:         return stream << "IDButton_15";
    case IDButton_16:         return stream << "IDButton_16";
    case IDButton_17:         return stream << "IDButton_17";
    case IDButton_18:         return stream << "IDButton_18";
    case IDButton_19:         return stream << "IDButton_19";
    case IDButton_20:         return stream << "IDButton_20";
    case IDButton_21:         return stream << "IDButton_21";
    case IDButton_22:         return stream << "IDButton_22";
    case IDButton_23:         return stream << "IDButton_23";
    case IDButton_24:         return stream << "IDButton_24";
    case IDButton_25:         return stream << "IDButton_25";
    case IDButton_26:         return stream << "IDButton_26";
    case IDButton_27:         return stream << "IDButton_27";
    case IDButton_28:         return stream << "IDButton_28";
    case IDButton_29:         return stream << "IDButton_29";
    case IDButton_30:         return stream << "IDButton_30";
    case IDButton_31:         return stream << "IDButton_31";
    default:  break;
    }
    return stream << "None";
}

// operator>>(InputDeviceButtons).
// the default is a plain 0 - not a named enumerator; no button bit is set.
std::istream& operator>>(std::istream& stream, InputDeviceButtons& value)
{
    std::string token;
    stream >> token;
    value = static_cast<InputDeviceButtons>(0);                 // stored before the chain, and kept on no match
    if (token.compare("IDButton_0") == 0) value = IDButton_0;
    else if (token.compare("IDButton_1") == 0) value = IDButton_1;
    else if (token.compare("IDButton_2") == 0) value = IDButton_2;
    else if (token.compare("IDButton_3") == 0) value = IDButton_3;
    else if (token.compare("IDButton_4") == 0) value = IDButton_4;
    else if (token.compare("IDButton_5") == 0) value = IDButton_5;
    else if (token.compare("IDButton_6") == 0) value = IDButton_6;
    else if (token.compare("IDButton_7") == 0) value = IDButton_7;
    else if (token.compare("IDButton_8") == 0) value = IDButton_8;
    else if (token.compare("IDButton_9") == 0) value = IDButton_9;
    else if (token.compare("IDButton_10") == 0) value = IDButton_10;
    else if (token.compare("IDButton_11") == 0) value = IDButton_11;
    else if (token.compare("IDButton_12") == 0) value = IDButton_12;
    else if (token.compare("IDButton_13") == 0) value = IDButton_13;
    else if (token.compare("IDButton_14") == 0) value = IDButton_14;
    else if (token.compare("IDButton_15") == 0) value = IDButton_15;
    else if (token.compare("IDButton_16") == 0) value = IDButton_16;
    else if (token.compare("IDButton_17") == 0) value = IDButton_17;
    else if (token.compare("IDButton_18") == 0) value = IDButton_18;
    else if (token.compare("IDButton_19") == 0) value = IDButton_19;
    else if (token.compare("IDButton_20") == 0) value = IDButton_20;
    else if (token.compare("IDButton_21") == 0) value = IDButton_21;
    else if (token.compare("IDButton_22") == 0) value = IDButton_22;
    else if (token.compare("IDButton_23") == 0) value = IDButton_23;
    else if (token.compare("IDButton_24") == 0) value = IDButton_24;
    else if (token.compare("IDButton_25") == 0) value = IDButton_25;
    else if (token.compare("IDButton_26") == 0) value = IDButton_26;
    else if (token.compare("IDButton_27") == 0) value = IDButton_27;
    else if (token.compare("IDButton_28") == 0) value = IDButton_28;
    else if (token.compare("IDButton_29") == 0) value = IDButton_29;
    else if (token.compare("IDButton_30") == 0) value = IDButton_30;
    else if (token.compare("IDButton_31") == 0) value = IDButton_31;
    return stream;
}

// operator<<(InputDeviceKeys).
// A 256-case jump table at ds:0x88DFA08; every unnamed scan code falls to the bare prefix.
std::ostream& operator<<(std::ostream& stream, InputDeviceKeys value)
{
    switch (value)
    {
    case IDKey_Escape:        return stream << "IDKey_Escape";
    case IDKey_1:             return stream << "IDKey_1";
    case IDKey_2:             return stream << "IDKey_2";
    case IDKey_3:             return stream << "IDKey_3";
    case IDKey_4:             return stream << "IDKey_4";
    case IDKey_5:             return stream << "IDKey_5";
    case IDKey_6:             return stream << "IDKey_6";
    case IDKey_7:             return stream << "IDKey_7";
    case IDKey_8:             return stream << "IDKey_8";
    case IDKey_9:             return stream << "IDKey_9";
    case IDKey_0:             return stream << "IDKey_0";
    case IDKey_Minus:         return stream << "IDKey_Minus";
    case IDKey_Equals:        return stream << "IDKey_Equals";
    case IDKey_Backspace:     return stream << "IDKey_Backspace";
    case IDKey_Tab:           return stream << "IDKey_Tab";
    case IDKey_Q:             return stream << "IDKey_Q";
    case IDKey_W:             return stream << "IDKey_W";
    case IDKey_E:             return stream << "IDKey_E";
    case IDKey_R:             return stream << "IDKey_R";
    case IDKey_T:             return stream << "IDKey_T";
    case IDKey_Y:             return stream << "IDKey_Y";
    case IDKey_U:             return stream << "IDKey_U";
    case IDKey_I:             return stream << "IDKey_I";
    case IDKey_O:             return stream << "IDKey_O";
    case IDKey_P:             return stream << "IDKey_P";
    case IDKey_LeftBracket:   return stream << "IDKey_LeftBracket";
    case IDKey_RightBracket:  return stream << "IDKey_RightBracket";
    case IDKey_Enter:         return stream << "IDKey_Enter";
    case IDKey_LeftCtrl:      return stream << "IDKey_LeftCtrl";
    case IDKey_A:             return stream << "IDKey_A";
    case IDKey_S:             return stream << "IDKey_S";
    case IDKey_D:             return stream << "IDKey_D";
    case IDKey_F:             return stream << "IDKey_F";
    case IDKey_G:             return stream << "IDKey_G";
    case IDKey_H:             return stream << "IDKey_H";
    case IDKey_J:             return stream << "IDKey_J";
    case IDKey_K:             return stream << "IDKey_K";
    case IDKey_L:             return stream << "IDKey_L";
    case IDKey_Semicolon:     return stream << "IDKey_Semicolon";
    case IDKey_Apostrophe:    return stream << "IDKey_Apostrophe";
    case IDKey_Grave:         return stream << "IDKey_Grave";
    case IDKey_LeftShift:     return stream << "IDKey_LeftShift";
    case IDKey_Backslash:     return stream << "IDKey_Backslash";
    case IDKey_Z:             return stream << "IDKey_Z";
    case IDKey_X:             return stream << "IDKey_X";
    case IDKey_C:             return stream << "IDKey_C";
    case IDKey_V:             return stream << "IDKey_V";
    case IDKey_B:             return stream << "IDKey_B";
    case IDKey_N:             return stream << "IDKey_N";
    case IDKey_M:             return stream << "IDKey_M";
    case IDKey_Comma:         return stream << "IDKey_Comma";
    case IDKey_Period:        return stream << "IDKey_Period";
    case IDKey_Slash:         return stream << "IDKey_Slash";
    case IDKey_RightShift:    return stream << "IDKey_RightShift";
    case IDKey_Multiply:      return stream << "IDKey_Multiply";
    case IDKey_LeftAlt:       return stream << "IDKey_LeftAlt";
    case IDKey_Space:         return stream << "IDKey_Space";
    case IDKey_Capital:       return stream << "IDKey_Capital";
    case IDKey_F1:            return stream << "IDKey_F1";
    case IDKey_F2:            return stream << "IDKey_F2";
    case IDKey_F3:            return stream << "IDKey_F3";
    case IDKey_F4:            return stream << "IDKey_F4";
    case IDKey_F5:            return stream << "IDKey_F5";
    case IDKey_F6:            return stream << "IDKey_F6";
    case IDKey_F7:            return stream << "IDKey_F7";
    case IDKey_F8:            return stream << "IDKey_F8";
    case IDKey_F9:            return stream << "IDKey_F9";
    case IDKey_F10:           return stream << "IDKey_F10";
    case IDKey_Numlock:       return stream << "IDKey_Numlock";
    case IDKey_ScrollLock:    return stream << "IDKey_ScrollLock";
    case IDKey_Numpad7:       return stream << "IDKey_Numpad7";
    case IDKey_Numpad8:       return stream << "IDKey_Numpad8";
    case IDKey_Numpad9:       return stream << "IDKey_Numpad9";
    case IDKey_Subtract:      return stream << "IDKey_Subtract";
    case IDKey_Numpad4:       return stream << "IDKey_Numpad4";
    case IDKey_Numpad5:       return stream << "IDKey_Numpad5";
    case IDKey_Numpad6:       return stream << "IDKey_Numpad6";
    case IDKey_Add:           return stream << "IDKey_Add";
    case IDKey_Numpad1:       return stream << "IDKey_Numpad1";
    case IDKey_Numpad2:       return stream << "IDKey_Numpad2";
    case IDKey_Numpad3:       return stream << "IDKey_Numpad3";
    case IDKey_Numpad0:       return stream << "IDKey_Numpad0";
    case IDKey_Decimal:       return stream << "IDKey_Decimal";
    case IDKey_OEM_102:       return stream << "IDKey_OEM_102";
    case IDKey_F11:           return stream << "IDKey_F11";
    case IDKey_F12:           return stream << "IDKey_F12";
    case IDKey_F13:           return stream << "IDKey_F13";
    case IDKey_F14:           return stream << "IDKey_F14";
    case IDKey_F15:           return stream << "IDKey_F15";
    case IDKey_Kana:          return stream << "IDKey_Kana";
    case IDKey_Convert:       return stream << "IDKey_Convert";
    case IDKey_NoConvert:     return stream << "IDKey_NoConvert";
    case IDKey_Yen:           return stream << "IDKey_Yen";
    case IDKey_NumpadEquals:  return stream << "IDKey_NumpadEquals";
    case IDKey_PrevTrack:     return stream << "IDKey_PrevTrack";
    case IDKey_At:            return stream << "IDKey_At";
    case IDKey_Colon:         return stream << "IDKey_Colon";
    case IDKey_Underline:     return stream << "IDKey_Underline";
    case IDKey_Kanji:         return stream << "IDKey_Kanji";
    case IDKey_Stop:          return stream << "IDKey_Stop";
    case IDKey_Ax:            return stream << "IDKey_Ax";
    case IDKey_Unlabeled:     return stream << "IDKey_Unlabeled";
    case IDKey_NextTrack:     return stream << "IDKey_NextTrack";
    case IDKey_NumpadEnter:   return stream << "IDKey_NumpadEnter";
    case IDKey_RightCtrl:     return stream << "IDKey_RightCtrl";
    case IDKey_Mute:          return stream << "IDKey_Mute";
    case IDKey_Calculator:    return stream << "IDKey_Calculator";
    case IDKey_PlayPause:     return stream << "IDKey_PlayPause";
    case IDKey_MediaStop:     return stream << "IDKey_MediaStop";
    case IDKey_VolumeDown:    return stream << "IDKey_VolumeDown";
    case IDKey_VolumeUp:      return stream << "IDKey_VolumeUp";
    case IDKey_WebHome:       return stream << "IDKey_WebHome";
    case IDKey_NumpadComma:   return stream << "IDKey_NumpadComma";
    case IDKey_Divide:        return stream << "IDKey_Divide";
    case IDKey_PrintScreen:   return stream << "IDKey_PrintScreen";
    case IDKey_RightAlt:      return stream << "IDKey_RightAlt";
    case IDKey_Pause:         return stream << "IDKey_Pause";
    case IDKey_Home:          return stream << "IDKey_Home";
    case IDKey_ArrowUp:       return stream << "IDKey_ArrowUp";
    case IDKey_PageUp:        return stream << "IDKey_PageUp";
    case IDKey_ArrowLeft:     return stream << "IDKey_ArrowLeft";
    case IDKey_ArrowRight:    return stream << "IDKey_ArrowRight";
    case IDKey_End:           return stream << "IDKey_End";
    case IDKey_ArrowDown:     return stream << "IDKey_ArrowDown";
    case IDKey_PageDown:      return stream << "IDKey_PageDown";
    case IDKey_Insert:        return stream << "IDKey_Insert";
    case IDKey_Delete:        return stream << "IDKey_Delete";
    case IDKey_LeftWin:       return stream << "IDKey_LeftWin";
    case IDKey_RightWin:      return stream << "IDKey_RightWin";
    case IDKey_AppMenu:       return stream << "IDKey_AppMenu";
    case IDKey_Power:         return stream << "IDKey_Power";
    case IDKey_Sleep:         return stream << "IDKey_Sleep";
    case IDKey_Wake:          return stream << "IDKey_Wake";
    case IDKey_WebSearch:     return stream << "IDKey_WebSearch";
    case IDKey_WebFavorites:  return stream << "IDKey_WebFavorites";
    case IDKey_WebRefresh:    return stream << "IDKey_WebRefresh";
    case IDKey_WebStop:       return stream << "IDKey_WebStop";
    case IDKey_WebForward:    return stream << "IDKey_WebForward";
    case IDKey_WebBack:       return stream << "IDKey_WebBack";
    case IDKey_MyComputer:    return stream << "IDKey_MyComputer";
    case IDKey_Mail:          return stream << "IDKey_Mail";
    case IDKey_MediaSelect:   return stream << "IDKey_MediaSelect";
    default:  break;
    }
    return stream << "IDKey_";
}

// operator>>(InputDeviceKeys).
// the default is 255 (IDKey_None), and the fifteen lowercase IDKey_f* aliases are real.
std::istream& operator>>(std::istream& stream, InputDeviceKeys& value)
{
    // A table preserves first-match order without exceeding MSVC's nested-block limit.
    static const struct { const char* name; InputDeviceKeys value; } kNames[] =
    {
    { "IDKey_Escape", IDKey_Escape },
    { "IDKey_1", IDKey_1 },
    { "IDKey_2", IDKey_2 },
    { "IDKey_3", IDKey_3 },
    { "IDKey_4", IDKey_4 },
    { "IDKey_5", IDKey_5 },
    { "IDKey_6", IDKey_6 },
    { "IDKey_7", IDKey_7 },
    { "IDKey_8", IDKey_8 },
    { "IDKey_9", IDKey_9 },
    { "IDKey_0", IDKey_0 },
    { "IDKey_Minus", IDKey_Minus },
    { "IDKey_Equals", IDKey_Equals },
    { "IDKey_Backspace", IDKey_Backspace },
    { "IDKey_Tab", IDKey_Tab },
    { "IDKey_Q", IDKey_Q },
    { "IDKey_W", IDKey_W },
    { "IDKey_E", IDKey_E },
    { "IDKey_R", IDKey_R },
    { "IDKey_T", IDKey_T },
    { "IDKey_Y", IDKey_Y },
    { "IDKey_U", IDKey_U },
    { "IDKey_I", IDKey_I },
    { "IDKey_O", IDKey_O },
    { "IDKey_P", IDKey_P },
    { "IDKey_LeftBracket", IDKey_LeftBracket },
    { "IDKey_RightBracket", IDKey_RightBracket },
    { "IDKey_Enter", IDKey_Enter },
    { "IDKey_LeftCtrl", IDKey_LeftCtrl },
    { "IDKey_A", IDKey_A },
    { "IDKey_S", IDKey_S },
    { "IDKey_D", IDKey_D },
    { "IDKey_F", IDKey_F },
    { "IDKey_G", IDKey_G },
    { "IDKey_H", IDKey_H },
    { "IDKey_J", IDKey_J },
    { "IDKey_K", IDKey_K },
    { "IDKey_L", IDKey_L },
    { "IDKey_Semicolon", IDKey_Semicolon },
    { "IDKey_Apostrophe", IDKey_Apostrophe },
    { "IDKey_Grave", IDKey_Grave },
    { "IDKey_LeftShift", IDKey_LeftShift },
    { "IDKey_Backslash", IDKey_Backslash },
    { "IDKey_Z", IDKey_Z },
    { "IDKey_X", IDKey_X },
    { "IDKey_C", IDKey_C },
    { "IDKey_V", IDKey_V },
    { "IDKey_B", IDKey_B },
    { "IDKey_N", IDKey_N },
    { "IDKey_M", IDKey_M },
    { "IDKey_Comma", IDKey_Comma },
    { "IDKey_Period", IDKey_Period },
    { "IDKey_Slash", IDKey_Slash },
    { "IDKey_RightShift", IDKey_RightShift },
    { "IDKey_Multiply", IDKey_Multiply },
    { "IDKey_LeftAlt", IDKey_LeftAlt },
    { "IDKey_Space", IDKey_Space },
    { "IDKey_Capital", IDKey_Capital },
    { "IDKey_F1", IDKey_F1 },
    { "IDKey_f1", IDKey_F1 },   // lowercase alias, reader only
    { "IDKey_F2", IDKey_F2 },
    { "IDKey_f2", IDKey_F2 },   // lowercase alias, reader only
    { "IDKey_F3", IDKey_F3 },
    { "IDKey_f3", IDKey_F3 },   // lowercase alias, reader only
    { "IDKey_F4", IDKey_F4 },
    { "IDKey_f4", IDKey_F4 },   // lowercase alias, reader only
    { "IDKey_F5", IDKey_F5 },
    { "IDKey_f5", IDKey_F5 },   // lowercase alias, reader only
    { "IDKey_F6", IDKey_F6 },
    { "IDKey_f6", IDKey_F6 },   // lowercase alias, reader only
    { "IDKey_F7", IDKey_F7 },
    { "IDKey_f7", IDKey_F7 },   // lowercase alias, reader only
    { "IDKey_F8", IDKey_F8 },
    { "IDKey_f8", IDKey_F8 },   // lowercase alias, reader only
    { "IDKey_F9", IDKey_F9 },
    { "IDKey_f9", IDKey_F9 },   // lowercase alias, reader only
    { "IDKey_F10", IDKey_F10 },
    { "IDKey_f10", IDKey_F10 },   // lowercase alias, reader only
    { "IDKey_Numlock", IDKey_Numlock },
    { "IDKey_ScrollLock", IDKey_ScrollLock },
    { "IDKey_Numpad7", IDKey_Numpad7 },
    { "IDKey_Numpad8", IDKey_Numpad8 },
    { "IDKey_Numpad9", IDKey_Numpad9 },
    { "IDKey_Subtract", IDKey_Subtract },
    { "IDKey_Numpad4", IDKey_Numpad4 },
    { "IDKey_Numpad5", IDKey_Numpad5 },
    { "IDKey_Numpad6", IDKey_Numpad6 },
    { "IDKey_Add", IDKey_Add },
    { "IDKey_Numpad1", IDKey_Numpad1 },
    { "IDKey_Numpad2", IDKey_Numpad2 },
    { "IDKey_Numpad3", IDKey_Numpad3 },
    { "IDKey_Numpad0", IDKey_Numpad0 },
    { "IDKey_Decimal", IDKey_Decimal },
    { "IDKey_OEM_102", IDKey_OEM_102 },
    { "IDKey_F11", IDKey_F11 },
    { "IDKey_f11", IDKey_F11 },   // lowercase alias, reader only
    { "IDKey_F12", IDKey_F12 },
    { "IDKey_f12", IDKey_F12 },   // lowercase alias, reader only
    { "IDKey_F13", IDKey_F13 },
    { "IDKey_f13", IDKey_F13 },   // lowercase alias, reader only
    { "IDKey_F14", IDKey_F14 },
    { "IDKey_f14", IDKey_F14 },   // lowercase alias, reader only
    { "IDKey_F15", IDKey_F15 },
    { "IDKey_f15", IDKey_F15 },   // lowercase alias, reader only
    { "IDKey_Kana", IDKey_Kana },
    { "IDKey_Convert", IDKey_Convert },
    { "IDKey_NoConvert", IDKey_NoConvert },
    { "IDKey_Yen", IDKey_Yen },
    { "IDKey_NumpadEquals", IDKey_NumpadEquals },
    { "IDKey_PrevTrack", IDKey_PrevTrack },
    { "IDKey_At", IDKey_At },
    { "IDKey_Colon", IDKey_Colon },
    { "IDKey_Underline", IDKey_Underline },
    { "IDKey_Kanji", IDKey_Kanji },
    { "IDKey_Stop", IDKey_Stop },
    { "IDKey_Ax", IDKey_Ax },
    { "IDKey_Unlabeled", IDKey_Unlabeled },
    { "IDKey_NextTrack", IDKey_NextTrack },
    { "IDKey_NumpadEnter", IDKey_NumpadEnter },
    { "IDKey_RightCtrl", IDKey_RightCtrl },
    { "IDKey_Mute", IDKey_Mute },
    { "IDKey_Calculator", IDKey_Calculator },
    { "IDKey_PlayPause", IDKey_PlayPause },
    { "IDKey_MediaStop", IDKey_MediaStop },
    { "IDKey_VolumeDown", IDKey_VolumeDown },
    { "IDKey_VolumeUp", IDKey_VolumeUp },
    { "IDKey_WebHome", IDKey_WebHome },
    { "IDKey_NumpadComma", IDKey_NumpadComma },
    { "IDKey_Divide", IDKey_Divide },
    { "IDKey_PrintScreen", IDKey_PrintScreen },
    { "IDKey_RightAlt", IDKey_RightAlt },
    { "IDKey_Pause", IDKey_Pause },
    { "IDKey_Home", IDKey_Home },
    { "IDKey_ArrowUp", IDKey_ArrowUp },
    { "IDKey_PageUp", IDKey_PageUp },
    { "IDKey_ArrowLeft", IDKey_ArrowLeft },
    { "IDKey_ArrowRight", IDKey_ArrowRight },
    { "IDKey_End", IDKey_End },
    { "IDKey_ArrowDown", IDKey_ArrowDown },
    { "IDKey_PageDown", IDKey_PageDown },
    { "IDKey_Insert", IDKey_Insert },
    { "IDKey_Delete", IDKey_Delete },
    { "IDKey_LeftWin", IDKey_LeftWin },
    { "IDKey_RightWin", IDKey_RightWin },
    { "IDKey_AppMenu", IDKey_AppMenu },
    { "IDKey_Power", IDKey_Power },
    { "IDKey_Sleep", IDKey_Sleep },
    { "IDKey_Wake", IDKey_Wake },
    { "IDKey_WebSearch", IDKey_WebSearch },
    { "IDKey_WebFavorites", IDKey_WebFavorites },
    { "IDKey_WebRefresh", IDKey_WebRefresh },
    { "IDKey_WebStop", IDKey_WebStop },
    { "IDKey_WebForward", IDKey_WebForward },
    { "IDKey_WebBack", IDKey_WebBack },
    { "IDKey_MyComputer", IDKey_MyComputer },
    { "IDKey_Mail", IDKey_Mail },
    { "IDKey_MediaSelect", IDKey_MediaSelect },
    };

    std::string token;
    stream >> token;
    value = IDKey_None;                 // stored before the chain, kept on no match
    for (unsigned int i = 0; i < sizeof(kNames) / sizeof(kNames[0]); ++i)
    {
        if (token.compare(kNames[i].name) == 0)
        {
            value = kNames[i].value;
            break;
        }
    }
    return stream;
}

// operator<<(InputDevicePOVs).
// the fallback label is the bare word "None", which the reader does not accept.
std::ostream& operator<<(std::ostream& stream, InputDevicePOVs value)
{
    switch (value)
    {
    case IDPOV_0:             return stream << "IDPOV_0";
    case IDPOV_1:             return stream << "IDPOV_1";
    case IDPOV_2:             return stream << "IDPOV_2";
    default:  break;
    }
    return stream << "None";
}

// operator>>(InputDevicePOVs).
std::istream& operator>>(std::istream& stream, InputDevicePOVs& value)
{
    std::string token;
    stream >> token;
    value = IDPOV_0;                 // stored before the chain, and kept on no match
    if (token.compare("IDPOV_0") == 0) value = IDPOV_0;
    else if (token.compare("IDPOV_1") == 0) value = IDPOV_1;
    else if (token.compare("IDPOV_2") == 0) value = IDPOV_2;
    return stream;
}

} // namespace io
} // namespace hfe
} // namespace dice
