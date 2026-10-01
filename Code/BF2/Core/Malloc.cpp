// [INFERRED] Core/Malloc.cpp; see Malloc.h for placement evidence.
#include "Malloc.h"
#include <cstdlib>

namespace dice { namespace hfe {

// Allocates and frees pool storage through the shared CRT heap.
// [CHOSEN, not recovered] [RETIRE-WHEN: aidll-00-heap] Replace only when the original allocation
// backend and all cross-module new/delete operations use the same validated heap contract.
void* Malloc::malloc(unsigned int size)
{
    return std::malloc(size);
}

void Malloc::free(void* pointer)
{
    std::free(pointer);
}

} }
