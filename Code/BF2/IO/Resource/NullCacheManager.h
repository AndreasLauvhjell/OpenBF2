// Null cache service: create/open return null, isCached returns false, and mutations do nothing.
// [INFERRED] Header placement. The interface has no virtual destructor.

#ifndef IO_RESOURCE_NULLCACHEMANAGER_H          // [INFERRED] guard style, per Core/IBase.h
#define IO_RESOURCE_NULLCACHEMANAGER_H

#include <string>

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace io {

// IFileSystem - the real interface lives in IO/File/IFileSystem.h, which belongs to another owner.
// Forward-declared rather than included so this header does not couple to that file.
class IFileSystem;

// IStream - IID_IStream = 1662503166 (Core/Ids.h); IO/Stream/IStream.h exists. Forward-declared for
// the same reason: only pointers cross this interface.
class IStream;

// Cache interface. [CHOSEN, not recovered] Interface boundary and pointer return types for create/open.
// Preserve virtual declaration order.
class ICacheManager : public IBase
{
public:
    virtual IStream* create(const std::string& name)      = 0;   // +0x10  not the class factory
    virtual IStream* open(const std::string& name)        = 0;   // +0x14
    virtual bool     isCached(const std::string& name)    = 0;   // +0x18
    virtual void     invalidate(const std::string& name)  = 0;   // +0x1C
    virtual void     invalidateAll()                      = 0;   // +0x20
    virtual void     setFileSystem(IFileSystem* fs)       = 0;   // +0x24

    // no virtual destructor - see the vtable block above. This is deliberate.
};

// Reference-counted cache-manager service that performs no caching.
class NullCacheManager : public ICacheManager
{
public:
    // Starts with one reference.
    NullCacheManager();                                     // [inlined in ELF] - see create()

    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;   // 0x8435BC0  - _ZNK..., const

    // ICacheManager, +0x10..+0x24
    virtual IStream* create(const std::string& name);       // 0x8435B10  virtual, not the factory
    virtual IStream* open(const std::string& name);
    virtual bool     isCached(const std::string& name);
    virtual void     invalidate(const std::string& name);
    virtual void     invalidateAll();
    virtual void     setFileSystem(IFileSystem* fs);

    // The class factory. Overloads with the virtual create() above - that collision is dice's and
    // is preserved. _ZN4dice3hfe2io16NullCacheManager6createEjPNS0_5IBaseE.
    static IBase* create(unsigned int iid, IBase* outer);

private:
    /* 0x04 */ int m_refCount;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_RESOURCE_NULLCACHEMANAGER_H
