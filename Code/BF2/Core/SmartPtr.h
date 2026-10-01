// Intrusive smart pointer holding one pointer; header path [INFERRED].
// Ownership resides in the pointee's IBase count. Assignment releases the old value and retains
// the incoming pointer rather than adopting it.

#ifndef CORE_SMARTPTR_H     // [INFERRED] guard style
#define CORE_SMARTPTR_H

namespace dice {
namespace hfe {

template <typename T>
class SmartPtr
{
public:
    SmartPtr() : m_ptr(0) {}

    SmartPtr(T* p) : m_ptr(p)
    {
        if (m_ptr) m_ptr->addRef();
    }

    SmartPtr(const SmartPtr& o) : m_ptr(o.m_ptr)
    {
        if (m_ptr) m_ptr->addRef();
    }

    ~SmartPtr()
    {
        if (m_ptr) m_ptr->release();
    }

    SmartPtr& operator=(const SmartPtr& o)
    {
        if (this != &o)
        {
            if (m_ptr) m_ptr->release();     // recovered ordinary-assignment order
            m_ptr = o.m_ptr;
            if (m_ptr) m_ptr->addRef();
        }
        return *this;
    }

    // BF2's component maps sometimes store an interface pointer returned by queryInterface while
    // retaining through the original object pointer.  With multiple inheritance those pointers
    // need not have the same address, so an ordinary SmartPtr assignment cannot express the
    // operation.  The shipped order is release old -> store queried interface -> addRef owner.
    template <typename Owner>
    void assignQueriedInterface(T* storedInterface, Owner* originalOwner)
    {
        if (m_ptr) m_ptr->release();
        m_ptr = storedInterface;
        if (originalOwner) originalOwner->addRef();
    }

    T* get() const        { return m_ptr; }
    T* operator->() const { return m_ptr; }
    operator T*() const   { return m_ptr; }

private:
    T* m_ptr;      // the only member - sizeof(SmartPtr<T>) == 4
};

} // namespace hfe
} // namespace dice

#endif // CORE_SMARTPTR_H
