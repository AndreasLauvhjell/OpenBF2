// [INFERRED] Source placement.

#include "SoldierPose.h"

#include <istream>
#include <ostream>
#include <string>

namespace dice {
namespace hfe {
namespace ai {

// One token; each pair tested prefixed name first; a miss
// stores SPNotAvailable, exactly as the first pair's hit does.
std::istream& operator>>(std::istream& stream, SoldierPose& pose)
{
    std::string name;
    stream >> name;

    if (name == "SPNotAvailable" || name == "NotAvailable")
        pose = SPNotAvailable;
    else if (name == "SPStanding" || name == "Standing")
        pose = SPStanding;
    else if (name == "SPCrouching" || name == "Crouching")
        pose = SPCrouching;
    else if (name == "SPLying" || name == "Lying")
        pose = SPLying;
    else if (name == "SPSwimming" || name == "Swimming")
        pose = SPSwimming;
    else
        pose = SPNotAvailable;

    return stream;
}

// An out-of-range value writes nothing.
std::ostream& operator<<(std::ostream& stream, SoldierPose pose)
{
    switch (pose)
    {
    case SPNotAvailable: stream << "SPNotAvailable"; break;
    case SPStanding:     stream << "SPStanding"; break;
    case SPCrouching:    stream << "SPCrouching"; break;
    case SPLying:        stream << "SPLying"; break;
    case SPSwimming:     stream << "SPSwimming"; break;
    default:             break;
    }
    return stream;
}

} // namespace ai
} // namespace hfe
} // namespace dice
