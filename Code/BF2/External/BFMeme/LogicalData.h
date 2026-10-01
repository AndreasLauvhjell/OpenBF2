// Descriptor-attested dice::meme::LogicalData, PE82EE00; path [INFERRED]
// beside the attested BFMeme cluster. Complete inherited28-slot Data family.
#ifndef BFMEME_LOGICALDATA_H
#define BFMEME_LOGICALDATA_H
#include "Data.h"
#if !DEDICATED
namespace dice { namespace meme {
class LogicalData : public Data
{
public:
    // Factory82E9D0 supplies null; named derived constructors794DD0/794E00
    // preserve the Data name argument. Shared constructor spelling [INFERRED].
    explicit LogicalData(const char* name = nullptr);
    ~LogicalData() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    int getCharacter(System*, int character) override;
    // +70, the family's only added slot. Primary932848 has29 entries, one more
    // than Data's28 (949710 +70 is already string data), and the Equal/Or/And/
    // Not tables all inherit794D50 unchanged. Name [INFERRED] from the "Data 1"
    // / "Data 2" stream labels; the body returns nothing and guards nothing.
    virtual void setData(Data* first, Data* second);
    static TypeDescriptor* getClassType();                // metadata94B418
protected:
    // Names [INFERRED]; Data 1/Data 2 labels attest the two operand roles.
    Data* m_first;                                       // +08, borrowed
    Data* m_second;                                      // +0C, borrowed
};
} }
#endif
#endif
