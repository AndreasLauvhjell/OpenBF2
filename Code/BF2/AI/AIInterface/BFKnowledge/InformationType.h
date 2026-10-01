// AI template classification used by console scripts.
// Parsing is case-sensitive. An unknown or empty token produces -1, which addType ignores.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_INFORMATIONTYPE_H
#define AI_AIINTERFACE_BFKNOWLEDGE_INFORMATIONTYPE_H

#include <iosfwd>

namespace dice {
namespace hfe {
namespace ai {

// The explicit `int` base is this tree's (C++11): the parser stores -1 for an unknown name, which
// is outside an unfixed enum's value range. It changes neither the 4-byte size nor any mangling.
enum InformationType : int
{
    ITProduction     = 0,
    ITAmmunition     = 1,
    ITRepair         = 2,
    ITUnit           = 3,
    ITAir            = 4,
    ITHelicopter     = 5,
    ITGround         = 6,
    ITNaval          = 7,
    ITControlpoint   = 8,
    ITRadar          = 9,
    ITAirfield       = 10,
    ITTransportation = 11,
    ITArtillery      = 12,
    ITFixed          = 13,
    ITCover          = 14,
    ITMobile         = 15,
    ITUnmanned       = 16,
    ITLowPriority    = 17,
    ITStructure      = 18,
    ITBiological     = 19,
    ITVegetation     = 20,
    ITSoldier        = 21,   // AITemplate::initFromObject sets this bit for soldiers
    ITObstructedView = 22,
    ITNoTemperature  = 23,
    ITHasWreck       = 24,   // AITemplate::createWreckTemplate builds a wreck only with this bit
    ITObstacle       = 25,   // and gives the wreck this one
    ITHasCarHorn     = 26
};

std::ostream& operator<<(std::ostream& stream, InformationType type);
std::istream& operator>>(std::istream& stream, InformationType& type);

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_INFORMATIONTYPE_H
