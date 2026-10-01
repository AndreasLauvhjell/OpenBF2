// Fixed-size pooled allocator with linked overflow pools. Header path [INFERRED].
// MemoryPool derives from IMemoryPool, IAllocator and IBase. The interface slot boundary is
// [CHOSEN], but alloc/free, debug overloads and init must retain their declared virtual order.
// Only MemoryPool introduces a virtual destructor, after init; tryFree is nonvirtual.
// The retail x86 object is 48 bytes.

#ifndef CORE_MEMORYPOOL_H     // [INFERRED] guard style, per Core/IBase.h
#define CORE_MEMORYPOOL_H

#include "IBase.h"

namespace dice {
namespace hfe {

// Allocator base of IMemoryPool. The division of slots between the two is [CHOSEN, not recovered].
// No separate IAllocator interface ID is known; queryInterface exposes only IBase and IMemoryPool.
class IAllocator : public IBase
{
public:
    virtual void* alloc(unsigned int size)                                      = 0;  // +0x10
    virtual void  free(void* p)                                                 = 0;  // +0x14
    virtual void* alloc(unsigned int size, const char* file, unsigned int line) = 0;  // +0x18
    virtual void  free(void* p, const char* file, unsigned int line)            = 0;  // +0x1C
};

// IMemoryPool - RTTI-attested as MemoryPool's direct base (_ZTIN4dice3hfe11IMemoryPoolE @ 0x88D1258).
// IID_IMemoryPool = 40005, which is the value Core/Ids.h already carries.
class IMemoryPool : public IAllocator
{
public:
    // [CHOSEN] boundary - see the IAllocator block. Signature (int,int,bool) is attested:
    // _ZN4dice3hfe10MemoryPool4initEiib.
    virtual void init(int elementSize, int poolSize, bool flag) = 0;              // +0x20
};

// Fixed-size allocation pool with a linked overflow chain; exhausted pools add another pool.
// free walks the chain until a pool recognizes the address. Element size is four-byte aligned;
// poolSize is measured in bytes. The data buffer is allocated lazily.
// Keep the write-only fields and unknown flags: m_flag10 propagates into overflow initialization,
// while the meanings of m_flag11 and m_unknown2C remain [UNRESOLVED].
class MemoryPool : public IMemoryPool
{
public:
    MemoryPool();
    MemoryPool(int elementSize, int poolSize, bool flag, bool flag11);

    // IBase quartet, vtable +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;            // 0x83C5FD0  - _ZNK..., const

    // IAllocator, vtable +0x10..+0x1C
    virtual void* alloc(unsigned int size);
    virtual void  free(void* p);
    virtual void* alloc(unsigned int size, const char* file, unsigned int line);
    virtual void  free(void* p, const char* file, unsigned int line);

    // IMemoryPool, vtable +0x20
    virtual void  init(int elementSize, int poolSize, bool flag);

    // vtable +0x24 / +0x28. Declared last because it is last - see the vtable block above.
    virtual ~MemoryPool();

    // non-virtual
    bool tryFree(void* p);

    // The factory ClassManager::createInstance calls. Signature is fixed by ClassRegInfo::create in
    // Core/ClassManager.h and by the mangled name _ZN4dice3hfe10MemoryPool6createEjPNS0_5IBaseE.
    static IBase* create(unsigned int iid, IBase* outer);

private:
    /* 0x04 */ int         m_refCount;
    /* 0x08 */ int         m_elementSize;
    /* 0x0C */ int         m_poolSize;
    /* 0x10 */ bool        m_flag10;      // [UNRESOLVED] name and meaning; 0x83C5DC8 stores init's 3rd arg
    /* 0x11 */ bool        m_flag11;      // [UNRESOLVED] name and meaning; 0x83C5C03 / 0x83C5C86
    /* 0x14 */ MemoryPool* m_next;
    /* 0x18 */ MemoryPool* m_prev;        // [UNRESOLVED] purpose - written at 0x83C5ECA, never read
    /* 0x1C */ char*       m_buffer;
    /* 0x20 */ void*       m_freeList;
    /* 0x24 */ int         m_used;
    /* 0x28 */ int         m_maxCount;
    /* 0x2C */ int         m_unknown2C;   // [UNRESOLVED] - 0x83C5C23 sets 1; no instruction reads it
};

// Creates and initializes the requested pool class. [INFERRED] Declaration placement.
IMemoryPool* createMemoryPool(int elementSize, int poolSize, bool flag, unsigned int classId);

} // namespace hfe
} // namespace dice

#endif // CORE_MEMORYPOOL_H
