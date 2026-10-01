// PhysicsType stream conversion. [INFERRED] Source path follows the original translation-unit order.

#include "PhysicsType.h"

#include <cstdio>
#include <sstream>
#include <string>

namespace dice {
namespace hfe {
namespace world {

// The default arm emits None, including for out-of-range values.
std::ostream& operator<<(std::ostream& stream, PhysicsType type)
{
    switch (type)
    {
    case PhysicsType_Point:           stream << "Point";           break;
    case PhysicsType_RotationalPoint: stream << "RotationalPoint"; break;
    case PhysicsType_Mesh:            stream << "Mesh";            break;
    case PhysicsType_Platform:        stream << "Platform";        break;
    default:                          stream << "None";             break;
    }
    return stream;
}

// Retail attempts `%d` only for tokens no longer than three characters, serializes that enum value
// back through operator<<, then compares both original and normalized tokens case-sensitively.
// Unknown spellings become None without manufacturing failbit.
std::istream& operator>>(std::istream& stream, PhysicsType& type)
{
    std::string token;
    stream >> token;

    std::stringstream normalized;
    int numeric = -1;
    if (token.length() <= 3 && std::sscanf(token.c_str(), "%d", &numeric) == 1)
        normalized << static_cast<PhysicsType>(numeric);

    const std::string alias = normalized.str();
    if (token == "None" || alias == "None")
        type = PhysicsType_None;
    else if (token == "Point" || alias == "Point")
        type = PhysicsType_Point;
    else if (token == "RotationalPoint" || alias == "RotationalPoint")
        type = PhysicsType_RotationalPoint;
    else if (token == "Mesh" || alias == "Mesh")
        type = PhysicsType_Mesh;
    else if (token == "Platform" || alias == "Platform")
        type = PhysicsType_Platform;
    else
        type = PhysicsType_None;
    return stream;
}

} // namespace world
} // namespace hfe
} // namespace dice
