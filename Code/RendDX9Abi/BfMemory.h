// Shared Memory.dll allocator adapter, [CHOSEN, not recovered] source.
// Object and vector allocations cross image boundaries and must be freed by the same allocator.
// String buffers instead use MSVCR71 through Msvcr71Heap.h; the two heaps are not interchangeable.
// memoryName lazily registers the renderer bucket once; subsequent allocations use its ID.

#ifndef RENDDX9ABI_BFMEMORY_H
#define RENDDX9ABI_BFMEMORY_H

#include "PluginAbi.h"

#if !BF2_PLUGIN_ABI_NATIVE
namespace dice {
namespace hfe {
namespace memory {

// Declared, not defined - these resolve against the import library the build script generates from
// the shipped Memory.dll. Modern MSVC mangles all three to the byte-identical names listed above,
// which is what makes a plain declaration enough.
int   addMemoryName(const char* name);
void* bf_malloc(unsigned int size, int nameId);
void  bf_free(void* p);

} // namespace memory
} // namespace hfe
} // namespace dice
#endif

namespace renddx9abi {

// 0x10244A8C in the original. Zero until the first memoryName() call.
extern int g_memoryNameId;

// sub_1005F300. Idempotent; safe to call from a static constructor.
void memoryName(const char* name);

// Allocates and frees through the selected shared heap: engine heap for retail ABI,
// shared /MD CRT for native ABI. Pair these operations across the boundary.
void* engineAlloc(unsigned int size);
void  engineFree(void* p);

} // namespace renddx9abi

#endif // RENDDX9ABI_BFMEMORY_H
