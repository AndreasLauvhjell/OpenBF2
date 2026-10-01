// PhysicsType names and stream conversion. [INFERRED] Source path.

#ifndef PHYSICS_PHYSICSTYPE_H
#define PHYSICS_PHYSICSTYPE_H

#include <iosfwd>

namespace dice {
namespace hfe {
namespace world {

// Values and serialized tokens are recovered. Enumerator identifiers are [CHOSEN] because the
// Itanium symbols expose the PhysicsType name but not its source-level enumerator identifiers.
enum PhysicsType
{
    PhysicsType_None = 0,
    PhysicsType_Point = 1,
    PhysicsType_RotationalPoint = 2,
    PhysicsType_Mesh = 3,
    PhysicsType_Platform = 4
};

std::ostream& operator<<(std::ostream& stream, PhysicsType type);
std::istream& operator>>(std::istream& stream, PhysicsType& type);

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_PHYSICSTYPE_H
