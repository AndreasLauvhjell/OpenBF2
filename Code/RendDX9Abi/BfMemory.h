// Shared native CRT allocation adapter, [CHOSEN, not recovered].
// Object and vector allocations cross image boundaries and must be freed by the same allocator.
// Retail executable compatibility is retired by https://github.com/B2BF/OpenBF2/issues/7.

#ifndef RENDDX9ABI_BFMEMORY_H
#define RENDDX9ABI_BFMEMORY_H

#include "PluginAbi.h"

namespace renddx9abi {

// Allocates and frees through the shared /MD CRT. Pair these operations across the boundary.
void* engineAlloc(unsigned int size);
void  engineFree(void* p);

} // namespace renddx9abi

#endif // RENDDX9ABI_BFMEMORY_H
