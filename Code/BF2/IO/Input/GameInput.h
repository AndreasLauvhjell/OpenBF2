// One game-input sample. [INFERRED] Source placement.
// The empty derived type preserves its own API name; native size follows compiler alignment.

#ifndef IO_INPUT_GAMEINPUT_H
#define IO_INPUT_GAMEINPUT_H

#include "Input.h"

namespace dice {
namespace hfe {
namespace io {

class GameInput : public Input<GameInputMap, 74>
{
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_INPUT_GAMEINPUT_H
