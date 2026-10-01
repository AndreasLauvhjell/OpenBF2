// Descriptor-attested dice::meme::OrData at PE82EF40; path [INFERRED].
// Metadata94B61C+04 ->82EE00 proves LogicalData ancestry. Complete Data28.
#ifndef BFMEME_ORDATA_H
#define BFMEME_ORDATA_H
#include "LogicalData.h"
#if !DEDICATED
namespace dice { namespace meme {
class OrData : public LogicalData
{
public:
    explicit OrData(const char* name = nullptr);           // 794DA0, factory82EC00
    TypeDescriptor* getType() override;
    float getFloat(System*) override;
    static TypeDescriptor* getClassType();                // metadata94B61C
};
} }
#endif
#endif
