// [CHOSEN, not recovered] Shared engine-heap wrappers and modern-MSVC allocation adapters.

#include "BfMemory.h"

#include <cstdlib>
#include <new>

namespace renddx9abi {

void* engineAlloc(unsigned int size)
{
    return std::malloc(size);
}

void engineFree(void* p)
{
    std::free(p);
}

} // namespace renddx9abi

// Route scalar, array and nothrow allocations through the same heap used by engineFree.
// All allocations use the shared CRT heap.
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

// C++14 sized deallocation uses the same shared-CRT path as the unsized forms.
void operator delete(void* p, size_t) throw()
{
    ::operator delete(p);
}

void operator delete[](void* p, size_t) throw()
{
    ::operator delete(p);
}
