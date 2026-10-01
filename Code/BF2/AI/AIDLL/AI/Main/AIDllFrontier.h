// [CHOSEN, not recovered] Reports a missing AI DLL method to stderr, flushes, and aborts.
// The argument is its fully qualified name as a string literal. This macro never returns.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

#ifndef AI_AIDLL_AI_MAIN_AIDLLFRONTIER_H
#define AI_AIDLL_AI_MAIN_AIDLLFRONTIER_H

#include <cstdio>
#include <cstdlib>

#define BF2_AI_FRONTIER(qualifiedName)                                                             \
    do                                                                                             \
    {                                                                                              \
        std::fprintf(stderr, "[FRONTIER] " qualifiedName " reached\n");                            \
        std::fflush(stderr);                                                                       \
        std::abort();                                                                              \
    }                                                                                              \
    while (false)

#endif // AI_AIDLL_AI_MAIN_AIDLLFRONTIER_H
