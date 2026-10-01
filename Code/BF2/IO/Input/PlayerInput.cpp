// Input template definitions kept out of the widely included header to isolate Debug dependencies.
// [INFERRED] File placement; explicit instantiations provide the required specializations.

#include "PlayerInput.h"
#include "../../Core/Debug.h"

namespace dice {
namespace hfe {
namespace io {

// IO/Input/Input.h:191, message prefix "1 " - the overload number.
template <class T, unsigned int N>
void Input<T, N>::setInput(T index, float value)
{
    // [PLATFORM] Client4FD960 diagnoses negatives as well as the upper bound;
    // dedicated ELF80F9AA0 has only the original one-sided diagnostic.
    if (static_cast<int>(index) > static_cast<int>(N - 1)
#if !DEDICATED
        || (N == 64 && static_cast<int>(index) < 0)
#endif
        )
    {
        Debug(DEBUGTYPE_6, "IO/Input/Input.h", 191, __FILE__)
            << "1 setInput out of range: " << static_cast<int>(index) << ". Max is: " << N;
    }
    if (static_cast<int>(index) <= static_cast<int>(N - 1))
    {
        if ((mappingFlags >> (static_cast<unsigned int>(index) & 0x3F)) & 1ULL)
            inputs[index] = value;
    }
}

// IO/Input/Input.h:203, message prefix "2 ".
// the bool is converted to A float and stored in the same array (cvtsi2ss) - there is no separate
// digital state. "Pressed" is 1.0f in the same slot an axis writes, which is why every reader in
// this tree tests `> 0.5f`.
template <class T, unsigned int N>
void Input<T, N>::setInput(T index, bool value)
{
    // [PLATFORM] Client4FD880 also diagnoses negative indices.
    if (static_cast<int>(index) > static_cast<int>(N - 1)
#if !DEDICATED
        || (N == 64 && static_cast<int>(index) < 0)
#endif
        )
    {
        Debug(DEBUGTYPE_6, "IO/Input/Input.h", 203, __FILE__)
            << "2 setInput out of range: " << static_cast<int>(index) << ". Max is: " << N;
    }
    if (static_cast<int>(index) <= static_cast<int>(N - 1))
    {
        if ((mappingFlags >> (static_cast<unsigned int>(index) & 0x3F)) & 1ULL)
            inputs[index] = value ? 1.0f : 0.0f;
    }
}

// the two instantiations the binary has, and no others.
template class Input<PlayerInputMap, 64>;
template class Input<GameInputMap, 74>;

} // namespace io
} // namespace hfe
} // namespace dice
