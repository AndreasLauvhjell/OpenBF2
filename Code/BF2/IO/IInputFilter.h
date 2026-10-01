// Input filter interface. Returning true continues the filter chain; false stops it.
// Preserve destructor, game-filter and player-filter declaration order. [INFERRED] Source placement.

#ifndef IO_IINPUTFILTER_H           // [INFERRED] guard style
#define IO_IINPUTFILTER_H

namespace dice {
namespace hfe {
namespace io {

class GameInput;      // the two slot parameters, by reference only - forward-declared here so the
class PlayerInput;    // header stands alone (BF2Engine.h declares them after including this file)

// sizeof is 4 (vptr only), which is established.
class IInputFilter
{
public:
    virtual ~IInputFilter() {}

    // Filters a game sample in place; true lets subsequent filters process it.
    virtual bool filterGameInput(GameInput& input) = 0;       // slot 1 (PE +0x04, ELF +0x08)
    virtual bool filterPlayerInput(PlayerInput& input) = 0;   // slot 2 (PE +0x08, ELF +0x0C)
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif
