// [CHOSEN, not recovered] Shared engine-heap wrappers and modern-MSVC allocation adapters.

#include "BfMemory.h"

#include <cstdlib>
#include <new>

namespace renddx9abi {

int g_memoryNameId = 0;

// Registers the allocation bucket once; later calls ignore a different supplied name.
void memoryName(const char* name)
{
#if BF2_PLUGIN_ABI_NATIVE
    // The native rebuilt-client ABI shares the current /MD CRT. Memory.dll is deliberately not an
    // import of this flavour, so its retail-only accounting bucket has no native counterpart.
    (void) name;
#else
    if (!g_memoryNameId)
        g_memoryNameId = dice::hfe::memory::addMemoryName(name);
#endif
}

// sub_1005F320, 0x1005F320. Two instructions in the original: push the cached bucket id, tail-call
// bf_malloc.
void* engineAlloc(unsigned int size)
{
#if BF2_PLUGIN_ABI_NATIVE
    return std::malloc(size);
#else
    return dice::hfe::memory::bf_malloc(size, g_memoryNameId);
#endif
}

// sub_1005F360, 0x1005F360.
void engineFree(void* p)
{
#if BF2_PLUGIN_ABI_NATIVE
    std::free(p);
#else
    dice::hfe::memory::bf_free(p);
#endif
}

} // namespace renddx9abi

// Route scalar, array and nothrow allocations through the same heap used by engineFree.
// Native mode uses the shared CRT heap; compatibility mode uses Memory.dll.
// Throwing new reports allocation failure with bad_alloc even though the original allocator can return null.

void* operator new(size_t size)
{
    void* p = renddx9abi::engineAlloc(static_cast<unsigned int>(size ? size : 1));
    if (!p)
        throw std::bad_alloc();
    return p;
}

void* operator new[](size_t size)
{
    return ::operator new(size);
}

void* operator new(size_t size, const std::nothrow_t&) throw()
{
    return renddx9abi::engineAlloc(static_cast<unsigned int>(size ? size : 1));
}

void* operator new[](size_t size, const std::nothrow_t&) throw()
{
    return renddx9abi::engineAlloc(static_cast<unsigned int>(size ? size : 1));
}

void operator delete(void* p) throw()
{
    if (p)
        renddx9abi::engineFree(p);
}

void operator delete[](void* p) throw()
{
    ::operator delete(p);
}

void operator delete(void* p, const std::nothrow_t&) throw()
{
    ::operator delete(p);
}

void operator delete[](void* p, const std::nothrow_t&) throw()
{
    ::operator delete(p);
}

// C++14 sized deallocation. The compiler will call these in preference to the unsized forms; both
// must land on bf_free or a block allocated above would be handed to the CRT heap.
void operator delete(void* p, size_t) throw()
{
    ::operator delete(p);
}

void operator delete[](void* p, size_t) throw()
{
    ::operator delete(p);
}
