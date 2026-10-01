// Fixed-size pool allocation and overflow-chain management. [INFERRED] Source placement.

#include "MemoryPool.h"
#include "Malloc.h"
#include "ClassManager.h"                 // createMemoryPool goes through it
#include "Ids.h"                          // [INFERRED] header name; the constants are attested
#include <new>                            // placement new - see alloc()

namespace dice {
namespace hfe {

// Pool storage shares the Malloc heap contract. Class factories are accessed through the engine registry.
extern ClassManager* classManager;

// Starts with one reference and no allocated pool chain. Call init before allocating from the pool.
MemoryPool::MemoryPool()
    : m_refCount(1)
    , m_flag11(false)       // 0x83C5C03  (byte store)
    , m_next(0)
    , m_prev(0)
    , m_buffer(0)
    , m_freeList(0)
    , m_unknown2C(1)
{
    // The vptr store at 0x83C5BF6 is the compiler's, not a statement.
}

// Initializes only the reference count/flag and delegates to init.
// This retail constructor leaves pointer members uninitialized, so allocating/freeing through such
// an instance is unsafe. Preserve the omission; normal factory paths use the default constructor.
MemoryPool::MemoryPool(int elementSize, int poolSize, bool flag, bool flag11)
    : m_refCount(1)
    , m_flag11(flag11)
{
    init(elementSize, poolSize, flag);   // 0x83C5C94, a tail jump - a direct call, not virtual
}

// Releases the overflow chain and deletes the backing array.
// The original alloc/free paths cross allocator boundaries. The RW-100 chosen CRT backend shares
// one heap but preserves this destruction sequence; it does not reproduce the original memory DLL.
MemoryPool::~MemoryPool()
{
    if (m_next)
        m_next->release();      // 0x83C5CE9, virtual - vtable +0x08

    delete [] m_buffer;         // 0x83C5CF9, operator delete[]
}

// Initializes element and pool sizes; slots must fit and align intrusive free-list pointers.
// [CHOSEN, not recovered] Void return type; observed callers discard the original result.
void MemoryPool::init(int elementSize, int poolSize, bool flag)
{
    // [CHOSEN, not recovered] Native port: each slot stores a native free-list
    // pointer. Keep the retail mask/rounding operation, using its actual width.
    // This remains exactly 4 on x86; no serialized size or allocator API changes.
    const int alignment = static_cast<int>(sizeof(void*));
    int aligned = elementSize & ~(alignment - 1);
    if (elementSize & (alignment - 1))
        aligned += alignment;

    m_elementSize = aligned;
    m_poolSize    = poolSize;
    m_used        = 0;
    m_flag10      = flag;
    m_maxCount    = poolSize / aligned;     // 0x83C5DCE idiv / 0x83C5DD0 store
}

// allocation

// Returns one fixed-size element; size is only forwarded when extending the overflow chain.
// Free elements store an intrusive next pointer. Allocated bytes use maxCount * elementSize,
// while tryFree bounds-checks poolSize. A failed overflow allocation still reaches init on null;
// preserve that failure path and the virtual init/alloc calls.
void* MemoryPool::alloc(unsigned int size)
{
    if (m_used < m_maxCount)                                    // 0x83C5DF4, signed compare
    {
        if (m_buffer == 0)
        {
            m_buffer = new char[m_maxCount * m_elementSize];    // 0x83C5E08, operator new[]

            // Thread every element to the next one. 0x83C5E20 - 0x83C5E3D.
            for (int i = 0; i < m_maxCount - 1; ++i)
                *reinterpret_cast<char**>(m_buffer + i * m_elementSize)
                    = m_buffer + (i + 1) * m_elementSize;

            // Terminate the last one. Written unconditionally after the loop - 0x83C5E4D.
            *reinterpret_cast<char**>(m_buffer + (m_maxCount - 1) * m_elementSize) = 0;

            m_freeList = m_buffer;
        }

        void* p = m_freeList;
        ++m_used;
        m_freeList = *reinterpret_cast<void**>(p);
        return p;
    }

    if (m_next == 0)
    {
        // 0x83C5E90 - 0x83C5ED7. sizeof(MemoryPool) is 0x30, which is the literal in the ELF.
        m_next = new (Malloc::malloc(sizeof(MemoryPool))) MemoryPool;

        m_next->init(m_elementSize, m_poolSize, m_flag10);      // 0x83C5EC4, virtual - same geometry
        m_next->m_prev = this;
    }

    return m_next->alloc(size);                                 // 0x83C5E88, virtual
}

// Forwards size through the allocation interface; file and line are ignored.
void* MemoryPool::alloc(unsigned int size, const char* file, unsigned int line)
{
    (void)file;
    (void)line;
    return alloc(size);                     // 0x83C6005, call [ecx+10h]
}

// Returns p to its owning pool in the chain, or passes it to Malloc::free when no pool owns it.
void MemoryPool::free(void* p)
{
    if (!tryFree(p))
        Malloc::free(p);
}

// Frees p through the virtual allocator interface. File and line are ignored.
void MemoryPool::free(void* p, const char* file, unsigned int line)
{
    (void)file;
    (void)line;
    free(p);                    // 0x83C6025, call [ecx+14h]
}

// Returns true after returning p to this pool or an overflow pool.
// The range check intentionally uses poolSize, including a possible unallocated remainder past the
// last whole element. Callers must supply valid pool allocations.
bool MemoryPool::tryFree(void* p)
{
    char* q = static_cast<char*>(p);

    if (q >= m_buffer && q < m_buffer + m_poolSize)
    {
        *reinterpret_cast<void**>(q) = m_freeList;
        m_freeList = q;
        --m_used;
        return true;
    }

    if (m_next == 0)
        return false;

    return m_next->tryFree(p);                          // 0x83C5F64, direct call
}

// the IBase quartet

// MemoryPool::addRef - Vtable +0x00.
void MemoryPool::addRef()
{
    ++m_refCount;
}

// MemoryPool::getRef - Vtable +0x04.
int MemoryPool::getRef()
{
    return m_refCount;
}

// Releases one reference, deleting the pool at zero. Returns the remaining count.
int MemoryPool::release()
{
    const int refCount = --m_refCount; // ELF keeps EBX; PE keeps ESI across deletion.

    if (refCount == 0)
        delete this;                // 0x83C5FB0, call [eax+28h]

    return refCount;                // ELF 0x83C5FC0 / PE 0x63F3B3; never reread freed storage.
}

// Returns this object for IBase or IMemoryPool, and null for other interface IDs.
void* MemoryPool::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<MemoryPool*>(this);

    if (iid == IID_IMemoryPool)                                 // 0x83C5FE5, cmovz
        return const_cast<MemoryPool*>(this);

    return 0;
}

// Factory definitions are grouped here; the original image emitted them with the registering module.

// Creates a one-reference pool, rejects aggregation and releases on IID mismatch.
// Does not initialize pool geometry; the caller must call init before allocation.
IBase* MemoryPool::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    MemoryPool* self = new MemoryPool();                // 0x84FA407, operator new(0x30u)
    void* itf = self->queryInterface(iid);              // 0x84FA422, virtual
    if (itf == 0)
        self->release();                                // 0x84FA434, virtual
    return static_cast<IBase*>(itf);
}

// Creates and initializes a pool. Zero classId uses the default implementation for IID_IMemoryPool;
// nonzero selects that class explicitly. Returns null when the factory cannot supply the interface.
IMemoryPool* createMemoryPool(int elementSize, int poolSize, bool flag, unsigned int classId)
{
    IBase* base;

    if (classId != 0)
        base = classManager->createInstance(classId, IID_IMemoryPool, 0);   // vtable +0x34
    else
        base = classManager->createInstance(IID_IMemoryPool, 0);            // vtable +0x30

    IMemoryPool* pool = static_cast<IMemoryPool*>(base);

    if (pool)
        pool->init(elementSize, poolSize, flag);                            // 0x84FA02E, virtual

    return pool;
}

} // namespace hfe
} // namespace dice
