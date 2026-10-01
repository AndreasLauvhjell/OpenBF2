// Descriptor-attested dice::meme::NotData at PE82F000; path [INFERRED].
// Parent LogicalData follows metadata94B6F4+04 ->82EE00. Full Data28 family.
#ifndef BFMEME_NOTDATA_H
#define BFMEME_NOTDATA_H
#include "LogicalData.h"
#if !DEDICATED
namespace dice { namespace meme {
class NotData : public LogicalData
{
public:
    explicit NotData(const char* name = nullptr);          // 794E00, factory82ED50
    TypeDescriptor* getType() override;
    bool streamProperties(System*, Stream*) override;
    float getFloat(System*) override;
    static TypeDescriptor* getClassType();                // metadata94B6F4
};
} }
#endif
#endif
