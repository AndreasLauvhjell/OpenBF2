// Stream conversion for player and game input names. [INFERRED] Source placement.

#include "Input.h"
#include "../../Core/StringUtil.h"

#include <istream>
#include <ostream>
#include <string>
#include <sstream>
#include <cstdio>

namespace dice {
namespace hfe {
namespace io {

namespace {

// One row per enumerator, in enum order - the value is the index, but the table keeps both so a
// hole in the enum could never silently shift the names.
struct PlayerInputName { PlayerInputMap value; const char* name; };
const PlayerInputName kPlayerInputNames[] = {
    { PIYaw, "PIYaw" }, { PIPitch, "PIPitch" }, { PIRoll, "PIRoll" }, { PIThrottle, "PIThrottle" },
    { PIMouseLookX, "PIMouseLookX" }, { PIMouseLookY, "PIMouseLookY" },
    { PICameraX, "PICameraX" }, { PICameraY, "PICameraY" },
    { PIFire, "PIFire" }, { PIAction, "PIAction" }, { PIUse, "PIUse" }, { PIMouseLook, "PIMouseLook" },
    { PIAltSprint, "PIAltSprint" }, { PISprint, "PISprint" },
    { PIWeaponSelect1, "PIWeaponSelect1" }, { PIWeaponSelect2, "PIWeaponSelect2" },
    { PIWeaponSelect3, "PIWeaponSelect3" }, { PIWeaponSelect4, "PIWeaponSelect4" },
    { PIWeaponSelect5, "PIWeaponSelect5" }, { PIWeaponSelect6, "PIWeaponSelect6" },
    { PIWeaponSelect7, "PIWeaponSelect7" }, { PIWeaponSelect8, "PIWeaponSelect8" },
    { PIWeaponSelect9, "PIWeaponSelect9" },
    { PIPositionSelect1, "PIPositionSelect1" }, { PIPositionSelect2, "PIPositionSelect2" },
    { PIPositionSelect3, "PIPositionSelect3" }, { PIPositionSelect4, "PIPositionSelect4" },
    { PIPositionSelect5, "PIPositionSelect5" }, { PIPositionSelect6, "PIPositionSelect6" },
    { PIPositionSelect7, "PIPositionSelect7" }, { PIPositionSelect8, "PIPositionSelect8" },
    { PIAltFire, "PIAltFire" }, { PIReload, "PIReload" }, { PISelectFunc, "PISelectFunc" },
    { PIDrop, "PIDrop" }, { PIToggleCameraMode, "PIToggleCameraMode" },
    { PIToggleCamera, "PIToggleCamera" }, { PILie, "PILie" }, { PICrouch, "PICrouch" },
    { PICameraMode1, "PICameraMode1" }, { PICameraMode2, "PICameraMode2" },
    { PICameraMode3, "PICameraMode3" }, { PICameraMode4, "PICameraMode4" },
    { PISelectPrimWeapon, "PISelectPrimWeapon" }, { PISelectSecWeapon, "PISelectSecWeapon" },
    { PIToggleWeapon, "PIToggleWeapon" }, { PIToggleFireRate, "PIToggleFireRate" },
    { PIFlareFire, "PIFlareFire" },
    { PIRadio1, "PIRadio1" }, { PIRadio2, "PIRadio2" }, { PIRadio3, "PIRadio3" },
    { PIRadio4, "PIRadio4" }, { PIRadio5, "PIRadio5" }, { PIRadio6, "PIRadio6" },
    { PIRadio7, "PIRadio7" }, { PIRadio8, "PIRadio8" },
    { PIScreenShot, "PIScreenShot" }, { PIToolTip, "PIToolTip" },
    { PISayAll, "PISayAll" }, { PISayTeam, "PISayTeam" },
    { PINextItem, "PINextItem" }, { PIPrevItem, "PIPrevItem" },
    { PICommunication, "PICommunication" }, { PIShowScoreBoard, "PIShowScoreBoard" },
    { PINone, "PINone" },
};
const int kPlayerInputNameCount = sizeof(kPlayerInputNames) / sizeof(kPlayerInputNames[0]);

struct GameInputName { GameInputMap value; const char* name; };
const GameInputName kGameInputNames[] = {
    { GIMenu, "GIMenu" },
    { GIToggleConsole, "GIToggleConsole" },
    { GIEscape, "GIEscape" },
    { GIUp, "GIUp" },
    { GIDown, "GIDown" },
    { GILeft, "GILeft" },
    { GIRight, "GIRight" },
    { GIPageUp, "GIPageUp" },
    { GIPageDown, "GIPageDown" },
    { GIRightShift, "GIRightShift" },
    { GILeftShift, "GILeftShift" },
    { GILeftCtrl, "GILeftCtrl" },
    { GIRightCtrl, "GIRightCtrl" },
    { GILeftAlt, "GILeftAlt" },
    { GIRightAlt, "GIRightAlt" },
    { GIOk, "GIOk" },
    { GIAltOk, "GIAltOk" },
    { GICancel, "GICancel" },
    { GIScreenShot, "GIScreenShot" },
    { GITogglePause, "GITogglePause" },
    { GIMouseLookX, "GIMouseLookX" },
    { GIMouseLookY, "GIMouseLookY" },
    { GISayAll, "GISayAll" },
    { GISayTeam, "GISayTeam" },
    { GISaySquad, "GISaySquad" },
    { GIMouseWheelUp, "GIMouseWheelUp" },
    { GIMouseWheelDown, "GIMouseWheelDown" },
    { GIQuit, "GIQuit" },
    { GITab, "GITab" },
    { GIEnter, "GIEnter" },
    { GIDelete, "GIDelete" },
    { GIBack, "GIBack" },
    { GIAltEnter, "GIAltEnter" },
    { GITacticalComm, "GITacticalComm" },
    { GIRadioComm, "GIRadioComm" },
    { GIMapSize, "GIMapSize" },
    { GIMapZoom, "GIMapZoom" },
    { GIMenuSelect0, "GIMenuSelect0" },
    { GIMenuSelect1, "GIMenuSelect1" },
    { GIMenuSelect2, "GIMenuSelect2" },
    { GIMenuSelect3, "GIMenuSelect3" },
    { GIMenuSelect4, "GIMenuSelect4" },
    { GIMenuSelect5, "GIMenuSelect5" },
    { GIMenuSelect6, "GIMenuSelect6" },
    { GIMenuSelect7, "GIMenuSelect7" },
    { GIMenuSelect8, "GIMenuSelect8" },
    { GIMenuSelect9, "GIMenuSelect9" },
    { GIYes, "GIYes" },
    { GINo, "GINo" },
    { GIAutoAccept, "GIAutoAccept" },
    { GICreateSquad, "GICreateSquad" },
    { GIJoinSquad, "GIJoinSquad" },
    { GISelectSquad, "GISelectSquad" },
    { GIToggleFreeCamera, "GIToggleFreeCamera" },
    { GIHotRankUp, "GIHotRankUp" },
    { GIShowScoreboard, "GIShowScoreboard" },
    { GIVoipPushToTalk, "GIVoipPushToTalk" },
    { GIVoipUseLeaderChannel, "GIVoipUseLeaderChannel" },
    { GI3dMap, "GI3dMap" },
    { GIScript1, "GIScript1" },
    { GIScript2, "GIScript2" },
    { GIScript3, "GIScript3" },
    { GIScript4, "GIScript4" },
    { GIScript5, "GIScript5" },
    { GIScript6, "GIScript6" },
    { GIScript7, "GIScript7" },
    { GIScript8, "GIScript8" },
    { GIScript9, "GIScript9" },
    { GIScript10, "GIScript10" },
    { GIScript11, "GIScript11" },
    { GIScript12, "GIScript12" },
    { GISelectAll, "GISelectAll" },
    { GIDeselectAll, "GIDeselectAll" },
    { GILeader, "GILeader" },
    { GINone, "GINone" },
};


} // namespace

// the switch-to-literal writer.
std::ostream& operator<<(std::ostream& stream, PlayerInputMap value)
{
    for (int i = 0; i < kPlayerInputNameCount; ++i)
    {
        if (kPlayerInputNames[i].value == value)
            return stream << kPlayerInputNames[i].name;
    }
    return stream;                                  // out-of-range writes nothing
}

std::ostream& operator<<(std::ostream& stream, GameInputMap value)
{
    for (const auto& row : kGameInputNames)
    {
        if (row.value == value)
            return stream << row.name;
    }
    return stream;
}

namespace {
// Source-level factoring of the identical unrolled comparison chains. Preserve
// sscanf's partial-token acceptance and the two-character length restriction.
template<class Enum, class Row, std::size_t N>
void parseInputName(const std::string& token, Enum& value,
                    const Row (&rows)[N], Enum sentinel)
{
    std::ostringstream numeric;
    if (token.size() <= 2)
    {
        int input = -1;
        if (std::sscanf(token.c_str(), "%d", &input) == 1)
            numeric << static_cast<Enum>(input);
    }
    for (const auto& row : rows)
    {
        if (token == row.name || numeric.str() == row.name)
        {
            value = row.value;
            return;
        }
    }
    // PINumInput/PINotInUse/PINone and GINumInput/GINone all reach the
    // same explicit sentinel store as an unknown spelling in both images.
    value = sentinel;
}
}

std::istream& operator>>(std::istream& stream, PlayerInputMap& value)
{
    std::string token;
    stream >> token;
    parseInputName(token, value, kPlayerInputNames, PINone);
    return stream;
}

std::istream& operator>>(std::istream& stream, GameInputMap& value)
{
    std::string token;
    stream >> token;
    parseInputName(token, value, kGameInputNames, GINone);
    return stream;
}

std::wistream& operator>>(std::wistream& stream, PlayerInputMap& value)
{
    std::wstring token;
    stream >> token;
    parseInputName(hfe::makeNarrow(token), value, kPlayerInputNames, PINone);
    return stream;
}

std::wistream& operator>>(std::wistream& stream, GameInputMap& value)
{
    std::wstring token;
    stream >> token;
    parseInputName(hfe::makeNarrow(token), value, kGameInputNames, GINone);
    return stream;
}

} // namespace io
} // namespace hfe
} // namespace dice
