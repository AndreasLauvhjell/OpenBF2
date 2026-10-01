// One tick of player controls, mapping flags and state. [INFERRED] Source placement.

#ifndef IO_INPUT_PLAYERINPUT_H     // [INFERRED] guard style
#define IO_INPUT_PLAYERINPUT_H

#include "Input.h"      // the template and the PlayerInputMap enum

namespace dice {
namespace hfe {
namespace io {

// Distinct nonpolymorphic type over the shared input template. Native size follows compiler alignment.
class PlayerInput : public Input<PlayerInputMap, 64>
{
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_INPUT_PLAYERINPUT_H
