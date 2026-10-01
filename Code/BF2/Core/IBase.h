// Reference-counted engine interface root. Header path [INFERRED].
// Preserve the addRef/getRef/release/queryInterface virtual order and nonvirtual destructor.
// CIDs select concrete classes; IIDs select interfaces. queryImplementation is class-specific
// and does not belong to this four-slot interface.

#ifndef CORE_IBASE_H     // [INFERRED] guard style
#define CORE_IBASE_H

namespace dice {
namespace hfe {

// queryInterface is const; release returns the new reference count.
// These signatures must match every override to preserve dispatch and avoid extra virtual slots.

#ifndef BF2_HFE_IBASE_DEFINED
#define BF2_HFE_IBASE_DEFINED
class IBase
{
public:
    virtual void  addRef()                                = 0;   // vtable +0x00
    virtual int   getRef()                                = 0;   // vtable +0x04
    virtual int   release()                               = 0;   // vtable +0x08  - see above
    virtual void* queryInterface(unsigned int iid) const  = 0;   // vtable +0x0C  - see above

protected:
    // Non-virtual and protected: no IBase* is ever deleted directly - release() is. Every concrete
    // class dumped so far places its destructor pair well after these four slots (ClassManager at
    // +0x6C/+0x70, ObjectTemplateManager at +0x88/+0x8C, FileManager at +0xB8/+0xBC), which is only
    // consistent with the destructor not being part of this interface.
    ~IBase() {}
};
#endif

} // namespace hfe
} // namespace dice

#endif // CORE_IBASE_H
