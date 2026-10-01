// Descriptor-attested dice::meme::EqualData at PE82EE40; path [INFERRED].
// Metadata94B438+04 ->82EE00 proves LogicalData ancestry. Complete Data28.
#ifndef BFMEME_EQUALDATA_H
#define BFMEME_EQUALDATA_H
#include "LogicalData.h"
#if !DEDICATED
namespace dice { namespace meme {
class EqualData : public LogicalData
{
public:
    explicit EqualData(const char* name = nullptr);        // 794D70, factory82EA40
    TypeDescriptor* getType() override;
    float getFloat(System*) override;
    static TypeDescriptor* getClassType();                // metadata94B438
};
} }
#endif
#endif
