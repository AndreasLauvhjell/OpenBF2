// Data is descriptor-attested at PE 0x9497DC. Placement and method names inferred.
// Complete 28-slot Data table 0x949710.
#ifndef BFMEME_DATA_H
#define BFMEME_DATA_H
#include "Object.h"
#if !DEDICATED
namespace dice { namespace meme {
class Data : public Object {
public:
    explicit Data(const char* name = 0);
    ~Data() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    const char* getName() override;
    bool setName(const char*) override;
    bool slot_0x20() override;
    bool slot_0x24() override;
    bool onCreate(System*) override;
    bool streamProperties(System*, Stream*) override;
    // [INFERRED names/signedness] Indexed data access. Base wrappers ignore the index and dispatch scalar virtuals.
    virtual bool setFloatElement(System*, int index, float);          // +40
    virtual bool setWideStringElement(System*, int index, const wchar_t*); // +44
    virtual bool setStringElement(System*, int index, const char*);    // +48
    virtual bool setFloat(System*, float);                            // +4C
    virtual bool setWideString(System*, const wchar_t*);               // +50
    virtual bool setString(System*, const char*);                      // +54
    virtual float getFloatElement(System*, int index);                // +58
    virtual float getFloat(System*);                                  // +5C
    // Full signed-int return: Style consumes EAX and BfListBoxData 0x7C4830
    // sign-extends narrow bytes into EAX. Extraction helpers narrow separately.
    virtual int getCharacterElement(System*, int index, int character); // +60
    virtual int getCharacter(System*, int character);                 // +64
    virtual int getElementCount(System*);                             // +68
    virtual Data* getElement(int index);                              // +6C
    int getString(System*, char* buffer, int capacity);
    int getWideString(System*, wchar_t* buffer, int capacity);
    int getStringLength(System*);                                    // 0x829BA0, full int characters
    static TypeDescriptor* getClassType();
private:
    char* m_name; // PE +04, owned clone; null is distinct from empty text
};
// Descriptor-attested dice::meme::StringData, 8298A0. Complete inherited
// 28-slot table949438; owned narrow string at +08, no additional virtuals.
class StringData : public Data {
public:
    explicit StringData(const char* value = 0, const char* name = 0);
    ~StringData() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool setFloat(System*, float) override;
    bool setString(System*, const char*) override;
    float getFloat(System*) override;
    int getCharacter(System*, int character) override; // 829290, unsigned byte, no bounds check
    static TypeDescriptor* getClassType();             // eight-slot descriptor949628
private:
    char* m_value;
};
// Descriptor-attested FloatRefData. PE82E0D0/94B288; complete inherited28 slots.
// External value pointer, cached value and scalar-allocated formatting buffer.
class FloatRefData : public Data {
public:
    explicit FloatRefData(float* value = nullptr, const char* name = nullptr);
    ~FloatRefData() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool setFloat(System*, float) override;
    bool setString(System*, const char*) override;
    float getFloat(System*) override;
    int getCharacter(System*, int) override;         // 82E170, unchecked signed byte
    static TypeDescriptor* getClassType();           // eight-slot descriptor94B318
private:
    float* m_value;
    float m_cachedValue;
    char* m_text;
};
// [INFERRED name] Bound Boolean value. Retains Data's descriptor and size getter despite larger concrete storage.
class BoolRefData : public Data {
public:
    BoolRefData(bool* value, const char* name);
    bool setFloat(System*, float) override;
    bool setString(System*, const char*) override;
    float getFloat(System*) override;
    int getCharacter(System*, int character) override;
    virtual bool bind(System*, bool* value);                          // +70
private:
    bool* m_value;
    bool m_fallback;
};
// [INFERRED name] Bound integer value. Retains Data's descriptor, size getter, and streamProperties.
class IntRefData : public Data {
public:
    IntRefData(int* value, const char* name);
    ~IntRefData() override;
    bool setFloat(System*, float) override;
    bool setString(System*, const char*) override;  // 82E110, shared with the float pair
    float getFloat(System*) override;
    int getCharacter(System*, int character) override;
private:
    int* m_value;      // +08; aliases m_cachedValue when no external value exists
    int m_cachedValue; // +0C, doubles as the fallback storage
    char* m_text;      // +10, owned scalar allocation
};
// String reference bound to external storage, including HUD information-layer slots.
class StringRefData : public Data {
public:
    StringRefData(const char** value = nullptr, const char* name = nullptr);
    ~StringRefData() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool setFloat(System*, float) override;
    bool setString(System*, const char*) override;
    float getFloat(System*) override;
    int getCharacter(System*, int character) override;
    static TypeDescriptor* getClassType();
private:
    const char** m_value;    // +08, aimed at m_fallback when no slot is supplied
    const char* m_fallback;  // +0C
};
// Wide-string reference. The narrow-string setter widens into a new allocation.
class WstringRefData : public Data {
public:
    WstringRefData(const wchar_t** value = nullptr, const char* name = nullptr);
    ~WstringRefData() override;                     // 829830, owns nothing
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool setFloat(System*, float) override;         // 829360, shared with the narrow class
    bool setWideString(System*, const wchar_t*) override;
    bool setString(System*, const char*) override;
    float getFloat(System*) override;
    int getCharacter(System*, int character) override;
    static TypeDescriptor* getClassType();
private:
    const wchar_t** m_value;   // +08, aimed at m_fallback when no slot is supplied
    const wchar_t* m_fallback; // +0C
};
// Owned wide text used by text nodes.
class WstringData : public Data {
public:
    WstringData(const wchar_t* value = nullptr, const char* name = nullptr);
    ~WstringData() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool setFloat(System*, float) override;
    bool setWideString(System*, const wchar_t*) override;
    bool setString(System*, const char*) override;
    float getFloat(System*) override;
    int getCharacter(System*, int character) override;
    static TypeDescriptor* getClassType();
private:
    wchar_t* m_value; // +08, owned clone
};
} }
#endif
#endif
