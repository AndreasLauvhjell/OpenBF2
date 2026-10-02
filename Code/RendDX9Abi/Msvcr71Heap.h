// Shared CRT allocation for the retained VC7.1-shaped internal string storage.
// [CHOSEN, not recovered] Retail CRT resolution is retired by
// https://github.com/B2BF/OpenBF2/issues/7; the existing helper names remain internal.

#ifndef RENDDX9ABI_MSVCR71HEAP_H
#define RENDDX9ABI_MSVCR71HEAP_H

#include <stddef.h>

namespace renddx9abi {

// Allocates and frees through the shared native CRT; allocation failure returns null.
void* crtMalloc(size_t size);
void  crtFree(void* p);

} // namespace renddx9abi

#endif // RENDDX9ABI_MSVCR71HEAP_H
