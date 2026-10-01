// Soldier stance values used by AI templates and script conversion.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_SOLDIERPOSE_H
#define AI_AIINTERFACE_BFKNOWLEDGE_SOLDIERPOSE_H

#include <iosfwd>

namespace dice {
namespace hfe {
namespace ai {

enum SoldierPose : int
{
    SPNotAvailable = 0,
    SPStanding     = 1,     // WeaponTemplate's constructor default
    SPCrouching    = 2,
    SPLying        = 3,
    SPSwimming     = 4
};

std::istream& operator>>(std::istream& stream, SoldierPose& pose);
std::ostream& operator<<(std::ostream& stream, SoldierPose pose);

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_SOLDIERPOSE_H
