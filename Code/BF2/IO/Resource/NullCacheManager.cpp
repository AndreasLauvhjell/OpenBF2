// Null cache-manager implementation. [INFERRED] Source placement.

#include "NullCacheManager.h"
#include "../../Core/Ids.h"                // [INFERRED] header name; the constants are attested

namespace dice {
namespace hfe {
namespace io {

// Initializes the null cache manager with one owned reference.
NullCacheManager::NullCacheManager()
    : m_refCount(1)             // 0x850BC05, inlined into create
{
}

// IBase quartet

// Core/IBase.h declares addRef() void - see the note in
// NullResourceManager.cpp about the materialised eax.
void NullCacheManager::addRef()
{
    ++m_refCount;
}


int NullCacheManager::getRef()
{
    return m_refCount;
}

// `operator delete(this)` directly at 0x8435BB0 - no virtual destructor
// exists in this hierarchy. Returns the new count.
int NullCacheManager::release()
{
    const int count = m_refCount - 1;
    const bool dead = (m_refCount == 1);
    m_refCount = count;

    if (dead)
        delete this;            // non-virtual

    return count;
}

// The uniform two-id shape.
void* NullCacheManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<NullCacheManager*>(this);
    if (iid == IID_ICacheManager)
        return const_cast<NullCacheManager*>(this);
    return 0;
}

// ICacheManager - six slots, all of them stubs. Three `xor eax,eax / ret` (7 bytes) and three empty
// (5 bytes). These are written-and-empty bodies, not missing ones.

// - `return 0`.
// this is the virtual create, not the class factory. See the header.
IStream* NullCacheManager::create(const std::string& /*name*/)
{
    return 0;
}

// - `return 0`.
IStream* NullCacheManager::open(const std::string& /*name*/)
{
    return 0;
}

// - `return 0`.
bool NullCacheManager::isCached(const std::string& /*name*/)
{
    return false;
}

// - empty.
void NullCacheManager::invalidate(const std::string& /*name*/)
{
}

// - empty.
void NullCacheManager::invalidateAll()
{
}

// - empty. The file system handed in is dropped, which is what makes this
// a null cache rather than a pass-through one: it cannot fall back to the real file system because
// it never keeps the pointer.
void NullCacheManager::setFileSystem(IFileSystem* /*fs*/)
{
}

// Creates the requested cache interface with one reference. Rejects aggregation and unsupported IDs.
IBase* NullCacheManager::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    NullCacheManager* self = new NullCacheManager();        // 0x850BBF0, operator new(8u)
    void* itf = self->queryInterface(iid);                  // 0x850BC0F, direct (devirtualised)
    if (itf == 0)
        self->release();                                    // virtual, slot +0x08
    return static_cast<IBase*>(itf);
}

// The registry pointer selects the static factory overload, distinct from virtual stream creation.

} // namespace io
} // namespace hfe
} // namespace dice
