// [INFERRED] Core/Malloc.h: ELF 0x83C4EF0..0x83C4F19 precedes the MD5 TU;
// no source-path literal attests this class's file.
#ifndef CORE_MALLOC_H
#define CORE_MALLOC_H

namespace dice { namespace hfe {
class Malloc
{
public:
    static void* malloc(unsigned int size);
    static void free(void* pointer);
};
} }

#endif
