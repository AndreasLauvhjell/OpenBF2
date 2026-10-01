// [INFERRED] Source placement.

#include "InformationType.h"

#include <istream>
#include <ostream>
#include <string>

namespace dice {
namespace hfe {
namespace ai {

// Writes the name of a recognized information type; unknown values produce no text.
std::ostream& operator<<(std::ostream& stream, InformationType type)
{
    switch (type)
    {
    case ITProduction:     stream << "ITProduction"; break;
    case ITAmmunition:     stream << "ITAmmunition"; break;
    case ITRepair:         stream << "ITRepair"; break;
    case ITUnit:           stream << "ITUnit"; break;
    case ITAir:            stream << "ITAir"; break;
    case ITHelicopter:     stream << "ITHelicopter"; break;
    case ITGround:         stream << "ITGround"; break;
    case ITNaval:          stream << "ITNaval"; break;
    case ITControlpoint:   stream << "ITControlpoint"; break;
    case ITRadar:          stream << "ITRadar"; break;
    case ITAirfield:       stream << "ITAirfield"; break;
    case ITTransportation: stream << "ITTransportation"; break;
    case ITArtillery:      stream << "ITArtillery"; break;
    case ITFixed:          stream << "ITFixed"; break;
    case ITCover:          stream << "ITCover"; break;
    case ITMobile:         stream << "ITMobile"; break;
    case ITUnmanned:       stream << "ITUnmanned"; break;
    case ITLowPriority:    stream << "ITLowPriority"; break;
    case ITStructure:      stream << "ITStructure"; break;
    case ITBiological:     stream << "ITBiological"; break;
    case ITVegetation:     stream << "ITVegetation"; break;
    case ITSoldier:        stream << "ITSoldier"; break;
    case ITObstructedView: stream << "ITObstructedView"; break;
    case ITNoTemperature:  stream << "ITNoTemperature"; break;
    case ITHasWreck:       stream << "ITHasWreck"; break;
    case ITObstacle:       stream << "ITObstacle"; break;
    case ITHasCarHorn:     stream << "ITHasCarHorn"; break;
    default:               break;
    }
    return stream;
}

// Reads a case-sensitive information-type name in the original comparison order.
// Stores -1 when extraction fails or the token is unknown.
std::istream& operator>>(std::istream& stream, InformationType& type)
{
    std::string name;
    stream >> name;

    if (name == "ITProduction")          type = ITProduction;
    else if (name == "ITAmmunition")     type = ITAmmunition;
    else if (name == "ITRepair")         type = ITRepair;
    else if (name == "ITUnit")           type = ITUnit;
    else if (name == "ITAir")            type = ITAir;
    else if (name == "ITGround")         type = ITGround;
    else if (name == "ITNaval")          type = ITNaval;
    else if (name == "ITControlpoint")   type = ITControlpoint;
    else if (name == "ITRadar")          type = ITRadar;
    else if (name == "ITAirfield")       type = ITAirfield;
    else if (name == "ITTransportation") type = ITTransportation;
    else if (name == "ITArtillery")      type = ITArtillery;
    else if (name == "ITFixed")          type = ITFixed;
    else if (name == "ITCover")          type = ITCover;
    else if (name == "ITMobile")         type = ITMobile;
    else if (name == "ITHasWreck")       type = ITHasWreck;
    else if (name == "ITObstacle")       type = ITObstacle;
    else if (name == "ITHasCarHorn")     type = ITHasCarHorn;
    else if (name == "ITUnmanned")       type = ITUnmanned;
    else if (name == "ITLowPriority")    type = ITLowPriority;
    else if (name == "ITStructure")      type = ITStructure;
    else if (name == "ITBiological")     type = ITBiological;
    else if (name == "ITVegetation")     type = ITVegetation;
    else if (name == "ITSoldier")        type = ITSoldier;
    else if (name == "ITObstructedView") type = ITObstructedView;
    else if (name == "ITNoTemperature")  type = ITNoTemperature;
    else type = (name == "ITHelicopter") ? ITHelicopter : static_cast<InformationType>(-1);

    return stream;
}

} // namespace ai
} // namespace hfe
} // namespace dice
