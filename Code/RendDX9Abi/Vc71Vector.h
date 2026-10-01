// Non-owning VC7.1 vector view, [CHOSEN, not recovered] source.
// The 16-byte layout has allocator padding followed by begin/end/capacity pointers at +4/+8/+12.
// The host constructs and destroys the vector; this adapter may grow its buffer through Memory.dll
// so the host can free it safely. Class and singleton registration elements occupy 36 and 44 bytes.

#ifndef RENDDX9ABI_VC71VECTOR_H
#define RENDDX9ABI_VC71VECTOR_H

#include <new>

// declared, not included. BfMemory.h includes PluginAbi.h, which includes this header, so
// including BfMemory.h back from here is a cycle: when BfMemory.cpp is the translation unit the
// guard is already set and the declarations below would not have arrived yet. Two prototypes cost
// nothing and break the loop; they must stay identical to BfMemory.h's.
namespace renddx9abi {
void* engineAlloc(unsigned int size);
void  engineFree(void* p);
}

namespace vc71 {

// The host's object, laid out exactly. Only ever seen through a pointer the host gave us.
struct vector_layout
{
    void* alloc;   // +0   allocator<T>, empty, padded
    char* first;   // +4   _First
    char* last;    // +8   _Last
    char* end;     // +12  _End
};

static_assert(sizeof(vector_layout) == 16, "the host's std::vector is 16 bytes on VC7.1");

template <typename T>
class vectorRef
{
public:
    explicit vectorRef(void* hostVector)
        : m_v(static_cast<vector_layout*>(hostVector))
    {
    }

    unsigned int size() const
    {
        return m_v->first ? static_cast<unsigned int>((m_v->last - m_v->first) / sizeof(T)) : 0u;
    }

    unsigned int capacity() const
    {
        return m_v->first ? static_cast<unsigned int>((m_v->end - m_v->first) / sizeof(T)) : 0u;
    }

    // 0x1005F270 / 0x1005F1E0, with the grow path at 0x1005EE20 folded in for the n == 1 case.
    void push_back(const T& value)
    {
        const unsigned int n = size();
        if (!m_v->first || n >= capacity())
            grow(n + 1);

        // Placement-construct in place, then publish. Publishing last matters: if the copy
        // constructor throws, the host must not see a half-built record.
        new (m_v->last) T(value);
        m_v->last += sizeof(T);
    }

private:
    void grow(unsigned int required)
    {
        const unsigned int oldSize = size();
        const unsigned int oldCap  = capacity();

        // VC7.1: half again, then clamped up to whatever was actually asked for.
        // 0x1005EE20 renders this as `v9 = (v6 >> 1) + v6; if (v9 < a3 + size) v9 = a3 + size;`
        unsigned int newCap = oldCap + (oldCap >> 1);
        if (newCap < required)
            newCap = required;

        // `operator new`, not crtMalloc. A vector's buffer belongs to the Memory.dll heap - see
        // the header note. Getting these two the wrong way round is the single most damaging
        // mistake available in this file, and it would not fault until the host freed the block.
        char* buffer = static_cast<char*>(::operator new(newCap * sizeof(T)));

        T* src = reinterpret_cast<T*>(m_v->first);
        T* dst = reinterpret_cast<T*>(buffer);
        for (unsigned int i = 0; i < oldSize; ++i)
        {
            new (dst + i) T(src[i]);
            src[i].~T();
        }

        if (m_v->first)
            ::operator delete(m_v->first);

        m_v->first = buffer;
        m_v->last  = buffer + oldSize * sizeof(T);
        m_v->end   = buffer + newCap  * sizeof(T);
    }

    vector_layout* m_v;
};


// Element-size-aware operations on a VC7.1 vector. Preserve element bytes, pointer
// records, and engine-heap ownership. [CHOSEN, not recovered] Capacity growth policy.
// [RETIRE-WHEN: RW-74] Replace when the original growth policy is established.
inline unsigned int vectorSizeIn(const vector_layout& v, unsigned int elemSize)
{
    return v.first ? static_cast<unsigned int>(v.last - v.first) / elemSize : 0u;
}

inline bool vectorReserveIn(vector_layout& v, unsigned int elemSize, unsigned int needed)
{
    const unsigned int capacity =
        v.first ? static_cast<unsigned int>(v.end - v.first) / elemSize : 0u;
    if (needed <= capacity)
        return true;

    unsigned int grown = capacity ? capacity * 2u : 8u;
    if (grown < needed)
        grown = needed;

    char* const block = static_cast<char*>(::renddx9abi::engineAlloc(grown * elemSize));
    if (!block)
        return false;

    const unsigned int used = vectorSizeIn(v, elemSize) * elemSize;
    for (unsigned int i = 0; i < used; ++i)
        block[i] = v.first[i];

    if (v.first)
        ::renddx9abi::engineFree(v.first);

    v.first = block;
    v.last  = block + used;
    v.end   = block + grown * elemSize;
    return true;
}

// A byte copy, deliberately: every element this is used for is trivially copyable, and the
// 36-byte per-part record is moved this way by the shipped code too - it pushes the record and
// then frees the source's blocks, which only works because the copy is bitwise.
inline bool vectorPushBackIn(vector_layout& v, unsigned int elemSize, const void* element)
{
    const unsigned int size = vectorSizeIn(v, elemSize);
    if (!vectorReserveIn(v, elemSize, size + 1u))
        return false;

    char* const dst = v.first + size * elemSize;
    const char* const src = static_cast<const char*>(element);
    for (unsigned int i = 0; i < elemSize; ++i)
        dst[i] = src[i];

    v.last = dst + elemSize;
    return true;
}

} // namespace vc71

#endif // RENDDX9ABI_VC71VECTOR_H
