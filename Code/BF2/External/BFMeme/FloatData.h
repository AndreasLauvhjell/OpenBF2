// Descriptor-attested dice::meme::FloatData, PE82E380/literal94B338.
// Path [INFERRED] beside the attested BFMeme cluster. Complete Data28 family.
#ifndef BFMEME_FLOATDATA_H
#define BFMEME_FLOATDATA_H
#include "Data.h"
#if !DEDICATED
namespace dice { namespace meme {
class FloatData : public Data
{
public:
    explicit FloatData(float value = 0.0f, const char* name = nullptr);
    ~FloatData() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool setFloat(System*, float) override;
    bool setString(System*, const char*) override;
    float getFloat(System*) override;
    int getCharacter(System*, int character) override;
    static TypeDescriptor* getClassType();                  // metadata94B2F8
private:
    // Member names [INFERRED] from the complete read/write/formatting bodies.
    float m_value;                                         // +08, serialized
    float m_cachedValue;                                   // +0C, last formatted value
    char* m_text;                                          // +10, owned scalar allocation
};
} }
#endif
#endif
