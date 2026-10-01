// Descriptor-attested at835D70/literal94E11C. Path/member names [INFERRED].
// Complete28-slot Data family; metadata94E0B4 explicitly selects Data as parent.
#ifndef BFMEME_TOGGLEDATA_H
#define BFMEME_TOGGLEDATA_H
#include "Data.h"
#if !DEDICATED
namespace dice { namespace meme {
class ToggleData : public Data
{
public:
    ToggleData();                                       // factory835CC0 inlines construction
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool setFloat(System*, float) override;
    bool setString(System*, const char*) override;
    float getFloat(System*) override;
    int getCharacter(System*, int character) override;
    static TypeDescriptor* getClassType();
protected:
    Data* m_toggle;                                      // +08, borrowed selector
    Data* m_first;                                       // +0C, borrowed Data 1
    Data* m_second;                                      // +10, borrowed Data 2
};
} }
#endif
#endif
