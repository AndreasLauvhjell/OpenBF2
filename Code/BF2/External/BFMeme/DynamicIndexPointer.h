// Path [INFERRED] beside the attested BFMeme cluster. PE837A80 attests the
// descriptor name dice::meme::DynamicIndexPointer.
#ifndef BFMEME_DYNAMICINDEXPOINTER_H
#define BFMEME_DYNAMICINDEXPOINTER_H
#include "Pointer.h"
#if !DEDICATED
#include <cstdint>
namespace dice { namespace meme {

// Complete23-slot table94E6C0. No additional virtuals follow Pointer+58.
class DynamicIndexPointer : public Pointer
{
public:
    DynamicIndexPointer();                                // inlined factory837940
    ~DynamicIndexPointer() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool visit0x3c(System*, Stream*) override;
    bool update(System*, float delta) override;
    bool event1(System*, int first, int index, int second) override;
    bool event2(System*, int first, int index) override;
    static TypeDescriptor* getClassType();                 // 837A80, complete factory837940

protected:
    // Names [INFERRED]; "Is locked" and "Index order" are visitor literals.
    bool m_locked;                                        // +14, padding untouched
    int m_indexOrder[4];                                   // +18..24
    int m_assignedCount;                                   // +28
    int m_lastInputIndex;                                  // +2C, zero-based except input0 ->0
    std::uint32_t m_field030, m_field034;                   // [UNRESOLVED] zeroed words
    float m_timer;                                        // +38, only positive values decremented
};
} }
#endif
#endif
