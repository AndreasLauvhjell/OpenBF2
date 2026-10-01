// [CHOSEN, not recovered] Selects host/plugin carriers and factory conventions. Retail
// uses VC7.1 layouts and fastcall factories; native mode shares the current STL/CRT.
// Renderer-internal VC7.1 containers remain unchanged.

#ifndef RENDDX9ABI_PLUGINABI_H
#define RENDDX9ABI_PLUGINABI_H

#ifndef BF2_PLUGIN_ABI_NATIVE
#define BF2_PLUGIN_ABI_NATIVE 0
#endif

#if BF2_PLUGIN_ABI_NATIVE != 0 && BF2_PLUGIN_ABI_NATIVE != 1
#error BF2_PLUGIN_ABI_NATIVE must be 0 (retail VC7.1 ABI) or 1 (native rebuilt-client ABI)
#endif

#include "Vc71String.h"
#include "Vc71Vector.h"

#if BF2_PLUGIN_ABI_NATIVE
#include <list>
#include <string>
#include <vector>
#include "../BF2/Core/ClassManager.h"

#if !defined(_MSC_VER) || !defined(_MT) || !defined(_DLL)
#error BF2_PLUGIN_ABI_NATIVE requires the supported MSVC dynamic-CRT (/MD) build
#endif
#if !defined(_ITERATOR_DEBUG_LEVEL) || _ITERATOR_DEBUG_LEVEL != 0
#error BF2_PLUGIN_ABI_NATIVE requires _ITERATOR_DEBUG_LEVEL=0 in both host and plugin images
#endif
#endif

namespace dice { namespace hfe { class IBase; } }

namespace renddx9abi {

#if BF2_PLUGIN_ABI_NATIVE
typedef std::string HostString;
typedef std::list<HostString> HostStringList;

template <typename T>
class HostVectorWriter
{
public:
    explicit HostVectorWriter(std::vector<T>& hostVector) : m_vector(hostVector) {}
    void push_back(const T& value) { m_vector.push_back(value); }
    // Returns the current registration count for accurate diagnostics.
    unsigned int size() const { return static_cast<unsigned int>(m_vector.size()); }

private:
    std::vector<T>& m_vector;
};
#else
typedef vc71::string HostString;

// VC7.1 std::list<std::string>, as read by RendDX9.dll sub_10017AB0: the list object's +4 is the
// self-linked head node and each element string starts at node+8. This is a host-boundary view,
// never a replacement for RendDX9's own recovered list layouts.
struct HostStringListNode
{
    HostStringListNode* next;
    HostStringListNode* prev;
    HostString          value;
};

struct HostStringList
{
    void*               allocator;
    HostStringListNode* head;
    unsigned int        size;
};

#if defined(_M_IX86)
static_assert(sizeof(HostStringList) == 12,
              "the retail host std::list view must retain its VC7.1 x86 size");
#endif

template <typename T>
class HostVectorWriter
{
public:
    explicit HostVectorWriter(void* hostVector) : m_vector(hostVector) {}
    void push_back(const T& value) { m_vector.push_back(value); }
    // The twin of the native accessor above; `vectorRef` already derives this from the host's
    // (_Last - _First) span, so it is the same number the retail host would compute.
    unsigned int size() const { return m_vector.size(); }

private:
    vc71::vectorRef<T> m_vector;
};
#endif

// Host-callable slots use HostString, converting once at this boundary (RW-129).
// Native host strings differ from the internal 28-byte vc71::string; compatibility conversions are
// identity references. Passing the wrong layout corrupts the caller's storage.
#if BF2_PLUGIN_ABI_NATIVE
inline HostString toHostString(const vc71::string& value)
{
    return HostString(value.c_str(), value.size());
}

inline vc71::string toVc71String(const HostString& value)
{
    return vc71::string(value.c_str(), static_cast<unsigned int>(value.size()));
}
#else
inline const HostString&   toHostString(const vc71::string& value)  { return value; }
inline const vc71::string& toVc71String(const HostString& value)    { return value; }
#endif

inline unsigned int hostStringListSize(const HostStringList* values)
{
#if BF2_PLUGIN_ABI_NATIVE
    return values ? static_cast<unsigned int>(values->size()) : 0;
#else
    return values ? values->size : 0;
#endif
}

template <typename Visitor>
inline void visitHostStringList(const HostStringList* values, Visitor visitor)
{
    if (!values)
        return;

#if BF2_PLUGIN_ABI_NATIVE
    for (HostStringList::const_iterator it = values->begin(); it != values->end(); ++it)
        visitor(*it);
#else
    if (!values->head)
        return;
    for (HostStringListNode* node = values->head->next;
         node != values->head;
         node = node->next)
        visitor(node->value);
#endif
}

inline HostString makeHostString(const vc71::string& value)
{
    return HostString(value.c_str());
}

inline void assignFromHostString(vc71::string& destination, const HostString& value)
{
    destination = value.c_str();
}

// The recovered PE factories remain explicitly __fastcall.  Core/ClassManager.h's native record
// holds an ordinary free-function pointer (__cdecl under the supported build), so native mode
// publishes one stateless thunk per recovered factory instead of changing the internal functions.
typedef dice::hfe::IBase* (__fastcall *RetailFastcallFactory)(unsigned int,
                                                               dice::hfe::IBase*);

#if BF2_PLUGIN_ABI_NATIVE
template <RetailFastcallFactory Factory>
struct NativeFactoryThunk
{
    static dice::hfe::IBase* __cdecl invoke(unsigned int iid, dice::hfe::IBase* outer)
    {
        return Factory(iid, outer);
    }
};
#endif

} // namespace renddx9abi

#if BF2_PLUGIN_ABI_NATIVE
#define RD9_HOST_VECTOR_PARAMETER(Type, name) std::vector<Type>& name
#define RD9_CLASS_FACTORY(factory) \
    (&::renddx9abi::NativeFactoryThunk<factory>::invoke)
#else
#define RD9_HOST_VECTOR_PARAMETER(Type, name) void* name
#define RD9_CLASS_FACTORY(factory) (factory)
#endif

#endif // RENDDX9ABI_PLUGINABI_H
