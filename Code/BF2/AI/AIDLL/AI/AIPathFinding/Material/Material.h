// Navigation material properties and script conversion.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIPATHFINDING_MATERIAL_MATERIAL_H
#define AI_AIDLL_AI_AIPATHFINDING_MATERIAL_MATERIAL_H

#include <iosfwd>

namespace dice {
namespace hfe {
namespace ai {

// The typedef spelling is observable in generated console metadata, while the underlying byte is
// fixed by both the Itanium `Rh` mangling and the MSVC receiver call.
typedef unsigned char materialId;

std::istream& operator>>(std::istream&, materialId&);

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPATHFINDING_MATERIAL_MATERIAL_H
