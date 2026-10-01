// Descriptor-attested dice::meme::AndData at PE82EF80; path [INFERRED].
// Parent LogicalData follows metadata94B63C+04 ->82EE00. Full Data28 family.
#ifndef BFMEME_ANDDATA_H
#define BFMEME_ANDDATA_H
#include "LogicalData.h"
#if !DEDICATED
namespace dice { namespace meme {
class AndData : public LogicalData
{
public:
    explicit AndData(const char* name = nullptr);          // 794DD0, factory82EC70
    TypeDescriptor* getType() override;
    float getFloat(System*) override;
    static TypeDescriptor* getClassType();                // metadata94B63C
};
} }
#endif
#endif
