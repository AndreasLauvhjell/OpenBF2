// Checks allocation results and reports a failed allocation through the engine diagnostic path.
#ifndef AI_AIDLL_AI_AIUTILS_CHECKPOINTER_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AIUTILS_CHECKPOINTER_H

#include "../../../../Core/Debug.h"

namespace dice {
namespace hfe {
namespace ai {

template <class T>
inline T* checkPointer(T* p)
{
    if (!p)
        Debug(DebugTypeError, __FILE__, __LINE__, "") << "AI::Allocation failed";

    return p;
}

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIUTILS_CHECKPOINTER_H
