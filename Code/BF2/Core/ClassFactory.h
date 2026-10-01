// Path: [INFERRED] beside ClassManager.h; bodies recovered from the symbolized ELF.
// Factories refuse aggregation, construct T, query the requested interface and release on mismatch.
// Keep the distinct hfe, world and ai templates; their outer parameter is always hfe::IBase*.
// [CHOSEN, not recovered] A shared macro spells the three recovered template bodies.

#ifndef CORE_CLASSFACTORY_H     // [INFERRED] guard style
#define CORE_CLASSFACTORY_H

#include "IBase.h"

// The factory takes an interface ID and optional aggregation owner. Aggregation is refused.
#define DICE_HFE_CLASSFACTORY_TEMPLATE                                          \
    template <typename T>                                                       \
    struct ClassFactory                                                         \
    {                                                                           \
        static ::dice::hfe::IBase* create(unsigned int iid,                     \
                                          ::dice::hfe::IBase* outer)            \
        {                                                                       \
            if (outer)                     /* aggregation is refused engine-wide */ \
                return 0;                                                       \
                                                                                \
            T* object = new T;                                                  \
                                                                                \
            ::dice::hfe::IBase* result =                                        \
                (::dice::hfe::IBase*)object->queryInterface(iid);  /* vtable +0x0C */ \
            if (!result)                                                        \
                object->release();                                 /* vtable +0x08 */ \
                                                                                \
            return result;                                                      \
        }                                                                       \
    }

namespace dice {
namespace hfe {

// 1 instantiation: SoundObjectTemplate.
DICE_HFE_CLASSFACTORY_TEMPLATE;

// 122 instantiations — every ObjectTemplate, TemplateComponent and world manager.
namespace world { DICE_HFE_CLASSFACTORY_TEMPLATE; }

// 1 instantiation: ai::AIConnection.
namespace ai    { DICE_HFE_CLASSFACTORY_TEMPLATE; }

} // namespace hfe
} // namespace dice

#endif // CORE_CLASSFACTORY_H
