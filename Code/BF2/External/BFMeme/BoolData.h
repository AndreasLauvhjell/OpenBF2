// Descriptor-attested dice::meme::BoolData, getter82DF80/literal94B1FC; path [INFERRED]
// beside the attested BFMeme cluster. Complete inherited28-slot Data table.
#ifndef BFMEME_BOOLDATA_H
#define BFMEME_BOOLDATA_H
#include "Data.h"
#if !DEDICATED
namespace dice { namespace meme {
class BoolData : public Data
{
public:
    BoolData();                                           // inlined in82DF00
    ~BoolData() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool setFloat(System*, float) override;
    bool setString(System*, const char*) override;
    float getFloat(System*) override;
    int getCharacter(System*, int character) override;
    static TypeDescriptor* getClassType();                  // metadata94B1DC
private:
    bool m_value;                                         // +08, serialized value
    unsigned char m_field09;                              // +09 [UNRESOLVED] role; ctor clears
};
} }
#endif
#endif
