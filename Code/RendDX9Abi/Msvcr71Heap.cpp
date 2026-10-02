// Path:     [CHOSEN, not recovered] - see Msvcr71Heap.h.

#include "Msvcr71Heap.h"
#include "PluginAbi.h"

#include <cstdlib>

namespace renddx9abi {

void* crtMalloc(size_t size)
{
    return std::malloc(size);
}

void crtFree(void* p)
{
    std::free(p);
}

} // namespace renddx9abi
