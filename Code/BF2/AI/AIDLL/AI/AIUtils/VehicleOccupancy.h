// [INFERRED] path/name: the separate ELF translation unit is anchored by
// _GLOBAL__I__ZN4dice3hfe2ai15vehicleOccupiedEji , without a filename.
// AIUtils is an attested directory; this filename is chosen for the recovered free function.
#ifndef AI_AIDLL_AI_AIUTILS_VEHICLEOCCUPANCY_H
#define AI_AIDLL_AI_AIUTILS_VEHICLEOCCUPANCY_H

namespace dice { namespace hfe { namespace ai {

// Parameter types/namespace from the ELF mangled name; Boolean materialized in both ports.
bool vehicleOccupied(unsigned int handle, int side);

} } }
#endif
