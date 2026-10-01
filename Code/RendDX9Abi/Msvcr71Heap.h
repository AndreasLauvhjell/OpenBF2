// MSVCR71 heap access for strings the host destroys through MSVCP71.
// [CHOSEN, not recovered] adapter: resolve malloc/free dynamically to avoid collisions with this
// image's CRT symbols. The host loads MSVCR71 before loading the plugin.
// Do not exchange these allocations with Memory.dll's object/vector allocator.

#ifndef RENDDX9ABI_MSVCR71HEAP_H
#define RENDDX9ABI_MSVCR71HEAP_H

#include <stddef.h>

namespace renddx9abi {

// Both return null if msvcr71.dll is not resident - which, in the process this DLL is designed to
// be loaded into, cannot happen. Callers still check, because a null here means the ABI assumption
// has failed and that is worth failing loudly rather than corrupting a heap.
void* crtMalloc(size_t size);
void  crtFree(void* p);

// True once msvcr71.dll has been found and both entry points resolved. Checked by the plugin entry
// point so an ABI failure is reported at load time instead of at the first cross-image free.
bool  crtHeapAvailable();

} // namespace renddx9abi

#endif // RENDDX9ABI_MSVCR71HEAP_H
