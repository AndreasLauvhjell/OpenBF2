// Integer-valued data with the inherited Data interface.
#ifndef BFMEME_INTDATA_H
#define BFMEME_INTDATA_H
#include "Data.h"
#if !DEDICATED
namespace dice { namespace meme {
class IntData : public Data
{
public:
    explicit IntData(int value = 0, const char* name = nullptr); // 82D8C0, factory82DB60
    ~IntData() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool setFloat(System*, float) override;
    bool setString(System*, const char*) override;           // 82E110, shared body
    float getFloat(System*) override;
    int getCharacter(System*, int character) override;
    static TypeDescriptor* getClassType();                  // metadata94B0C0
private:
    // Member names [INFERRED] from the complete read/write/formatting bodies.
    int m_value;         // +08, serialized "Value <do not edit>"
    int m_cachedValue;   // +0C, last formatted value
    char* m_text;        // +10, owned scalar allocation
};
} }
#endif
#endif
