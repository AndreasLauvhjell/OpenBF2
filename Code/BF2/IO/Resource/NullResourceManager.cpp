// Null resource-manager implementation. [INFERRED] Source placement.

#include "NullResourceManager.h"
#include "../../Core/Ids.h"                // [INFERRED] header name; the constants are attested

namespace dice {
namespace hfe {
namespace io {

// constructor

// Initializes one owned reference and the default memory limit of 50000.
NullResourceManager::NullResourceManager()
    : m_refCount(1)
    , m_memoryLimit(50000)
{
}

// Adds one reference.
void NullResourceManager::addRef()
{
    ++m_refCount;
}

// getRef -
int NullResourceManager::getRef()
{
    return m_refCount;
}

// Releases one reference and deletes this concrete object at zero. Returns the remaining count.
int NullResourceManager::release()
{
    const int count = m_refCount - 1;
    const bool dead = (m_refCount == 1);
    m_refCount = count;

    if (dead)
        delete this;            // non-virtual: operator delete(this)

    return count;
}

// Returns this object for IBase or IResourceManager, and null for other interface IDs.
void* NullResourceManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<NullResourceManager*>(this);
    if (iid == IID_IResourceManager)
        return const_cast<NullResourceManager*>(this);
    return 0;
}

// Disabled resource operations intentionally perform no work.
void NullResourceManager::addPriority(IResource* /*resource*/, float /*priority*/)
{
}

// - empty.
void NullResourceManager::update(float /*deltaTime*/)
{
}

// - empty.
void NullResourceManager::registerResource(IResource* /*resource*/, ResourceType /*type*/)
{
}

// - empty.
void NullResourceManager::unregisterResource(IResource* /*resource*/)
{
}

// - empty.
void NullResourceManager::unloadAll()
{
}

// real - see the header block on why a null object keeps this one.
void NullResourceManager::setMemoryLimit(unsigned int limit)
{
    m_memoryLimit = limit;
}

// real.
unsigned int NullResourceManager::getMemoryLimit()
{
    return m_memoryLimit;
}

// - empty. (The "temp" in the name is dice's; kept per the fidelity rule.)
void NullResourceManager::tempPrintPriorityList()
{
}

// - empty. asymmetric with setMemoryLimit on purpose: the setter discards
// its argument and the getter below hardwires false, so instant loading is a behaviour this class
// does not have, where the memory limit is a property it must still carry.
void NullResourceManager::setInstantLoading(bool /*instant*/)
{
}

// - `xor eax,eax / ret`.
bool NullResourceManager::getInstantLoading()
{
    return false;
}

// [CHOSEN] Factory placement beside the concrete resource manager.

// Creates the requested resource-manager interface with one owned reference; rejects aggregation.
IBase* NullResourceManager::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    NullResourceManager* self = new NullResourceManager();      // 0x850BBA0, operator new(0xCu)
    void* itf = self->queryInterface(iid);                      // virtual, slot +0x0C
    if (itf == 0)
        self->release();                                        // virtual, slot +0x08
    return static_cast<IBase*>(itf);
}

} // namespace io
} // namespace hfe
} // namespace dice
