// Interface holder with create-and-replace behavior. Header path [INFERRED].
// create releases the old interface before asking ClassManager; failure leaves the holder null.
// There is no destructor release. Callers retaining a returned interface must manage its reference.

#ifndef CORE_SMARTITF_H     // [INFERRED] guard style
#define CORE_SMARTITF_H

#include "ClassManager.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;      // Core/Globals.cpp; ELF 0x8AC85A0

template <typename T, const unsigned int* IID>
class SmartItf
{
public:
    SmartItf() : m_p(0) {}

    bool create(unsigned int classId)
    {
        if (m_p)
            m_p->release();                                         // IBase vtable +0x08

        m_p = static_cast<T*>(classManager->createInstance(classId, *IID, 0));   // vtable +0x34
        return m_p != 0;
    }

    operator T*() const { return m_p; }
    T* operator->() const { return m_p; }

private:
    T* m_p;
};

} // namespace hfe
} // namespace dice

#endif // CORE_SMARTITF_H
