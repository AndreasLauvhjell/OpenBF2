// Null resource service. Only the memory-limit property retains state; instant loading is false.
// [INFERRED] Header placement. release deletes the concrete object without a virtual destructor.

#ifndef IO_RESOURCE_NULLRESOURCEMANAGER_H       // [INFERRED] guard style, per Core/IBase.h
#define IO_RESOURCE_NULLRESOURCEMANAGER_H

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace io {

// Opaque resource interface. [CHOSEN, not recovered] ResourceType enumerators remain unidentified.
class IResource;

enum ResourceType
{
    RT_UNKNOWN = 0          // [CHOSEN, not recovered] - the real enumerator set is unknown
};

// Resource-priority interface.
// [CHOSEN, not recovered] Interface boundary inferred from its sole implementation; virtual order is preserved.
class IResourceManager : public IBase
{
public:
    virtual void         addPriority(IResource* resource, float priority)        = 0;  // +0x10
    virtual void         update(float deltaTime)                                 = 0;  // +0x14
    virtual void         registerResource(IResource* resource, ResourceType type) = 0; // +0x18
    virtual void         unregisterResource(IResource* resource)                 = 0;  // +0x1C
    virtual void         unloadAll()                                             = 0;  // +0x20
    virtual void         setMemoryLimit(unsigned int limit)                      = 0;  // +0x24
    virtual unsigned int getMemoryLimit()                                        = 0;  // +0x28
    virtual void         tempPrintPriorityList()                                 = 0;  // +0x2C
    virtual void         setInstantLoading(bool instant)                         = 0;  // +0x30
    virtual bool         getInstantLoading()                                     = 0;  // +0x34

    // no virtual destructor - see the vtable block above. This is deliberate.
};

// Null resource manager retaining the configurable memory-limit setting.
class NullResourceManager : public IResourceManager
{
public:
    NullResourceManager();

    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;           // 0x8435CE0  - _ZNK..., const

    // IResourceManager, +0x10..+0x34
    virtual void         addPriority(IResource* resource, float priority);
    virtual void         update(float deltaTime);
    virtual void         registerResource(IResource* resource, ResourceType type);
    virtual void         unregisterResource(IResource* resource);
    virtual void         unloadAll();
    virtual void         setMemoryLimit(unsigned int limit);
    virtual unsigned int getMemoryLimit();
    virtual void         tempPrintPriorityList();
    virtual void         setInstantLoading(bool instant);
    virtual bool         getInstantLoading();

    // The factory ClassManager::createInstance calls. Signature fixed by ClassRegInfo::create and by
    // the mangled name _ZN4dice3hfe2io19NullResourceManager6createEjPNS0_5IBaseE.
    static IBase* create(unsigned int iid, IBase* outer);

private:
    /* 0x04 */ int          m_refCount;
    /* 0x08 */ unsigned int m_memoryLimit;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_RESOURCE_NULLRESOURCEMANAGER_H
