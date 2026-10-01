// [TEMP] Native x86 StringData fixture. No production method is replaced.
// Typed setters/visitors observe only the callbacks each test requires.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <float.h>
#include <new>
#define private public
#include "../BF2/External/BFMeme/Data.h"
#undef private
#include "../BF2/External/BFMeme/Stream.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
using namespace dice::meme;
static int checks, failures;
static void check(bool good, const char* label)
{ ++checks; if (!good) { ++failures; std::printf("FAIL %s\n", label); } }
static StringData* watchedObject;
static void* watchedValue;
static void* watchedName;
static int valueDeletes, nameDeletes;
static bool allocationObserver;
void* operator new(std::size_t bytes)
{
    if (allocationObserver) {
        check(watchedObject->m_value == nullptr, "setter clears old value before clone allocation");
        allocationObserver = false;
    }
    void* result = std::malloc(bytes ? bytes : 1);
    if (!result) throw std::bad_alloc();
    return result;
}
void operator delete(void* value) noexcept
{
    if (watchedObject && value == watchedValue) {
        ++valueDeletes;
        check(watchedObject->m_value == watchedValue, "value pointer remains live during delete callback");
        check(watchedObject->getName() == watchedName, "name remains live during value deletion");
    }
    if (watchedObject && value == watchedName) {
        ++nameDeletes;
        check(watchedObject->m_value == nullptr, "value is cleared before base name deletion");
    }
    std::free(value);
}
void operator delete(void* value, std::size_t) noexcept { ::operator delete(value); }
static float fromBits(std::uint32_t bits)
{ float value; std::memcpy(&value, &bits, 4); return value; }
static bool textIs(StringData& data, const char* expected)
{
    char text[128]; data.getString(nullptr, text, sizeof(text));
    const bool same = std::strcmp(text, expected) == 0;
    if (!same) std::printf("text expected=[%s] actual=[%s]\n", expected, text);
    return same;
}
struct SetterObserver : StringData
{
    int calls = 0;
    System* system = nullptr;
    char text[64] = {};
    bool setString(System* value, const char* string) override {
        ++calls; system = value;
        std::strcpy(text, string);
        return false;
    }
};
struct StringVisitor : Stream
{
    char** expected = nullptr;
    int calls = 0;
    bool result = false;
    bool replace = false;
    bool streamString(char** field, const char* label) override {
        ++calls;
        check(field == expected, "stream receives actual owned field address");
        check(std::strcmp(label, "String <do not edit>") == 0, "stream receives exact label");
        if (replace) {
            const char text[] = "visitor value";
            char* replacement = static_cast<char*>(::operator new(sizeof(text)));
            std::memcpy(replacement, text, sizeof(text));
            ::operator delete(*field);
            *field = replacement;
        }
        return result;
    }
};
int main()
{
    static_assert(sizeof(void*) == 4, "x86 fixture");
    static_assert(sizeof(StringData) == 12, "retail StringData layout");
    StringData empty;
    check(empty.m_value == nullptr && empty.getName() == nullptr, "null constructor preserves both null pointers");
    check(empty.getFloat(nullptr) == 0 && empty.getCharacter(nullptr, -200) == 0,
        "null value returns zero before character indexing");
    check(empty.getObjectSize() == 12 && empty.getElementCount(nullptr) == 1, "size and inherited scalar count");
    check(empty.getElement(0) == &empty && empty.getElement(-1) == nullptr && empty.getElement(1) == nullptr,
        "inherited element accessor accepts only zero");
    check(empty.setString(nullptr, "") && empty.m_value != nullptr && empty.m_value[0] == 0,
        "empty text allocates distinct owned clone");
    check(empty.setString(nullptr, nullptr) && empty.m_value == nullptr, "null setter frees empty clone");
    char initial[] = "123.5tail";
    char name[] = "NamedValue";
    StringData value(initial, name);
    check(value.m_value != initial && value.getName() != name, "constructor clones name and value independently");
    initial[0] = '9'; name[0] = 'X';
    check(textIs(value, "123.5tail") && std::strcmp(value.getName(), "NamedValue") == 0,
        "source mutation leaves owned clones unchanged");
    check(value.getFloat(nullptr) == 123.5f, "atof prefix parsing");
    check(value.setString(nullptr, "nonnumeric") && value.getFloat(nullptr) == 0, "atof invalid text returns zero");
    check(value.setString(nullptr, "  -1.25e2suffix") && value.getFloat(nullptr) == -125, "atof whitespace sign exponent");
    const char highBytes[] = {static_cast<char>(0x80), static_cast<char>(0xFF), 0};
    value.setString(nullptr, highBytes);
    check(value.getCharacter(nullptr, 0) == 128 && value.getCharacter(nullptr, 1) == 255 && value.getCharacter(nullptr, 2) == 0,
        "character getter zero extends unsigned bytes");
    check(value.getCharacterElement(nullptr, -20, 1) == 255, "element wrapper ignores element index");
    wchar_t wide[8];
    check(value.getWideString(nullptr, wide, 8) == 2 && wide[0] == 128 && wide[1] == 255,
        "wide extraction preserves unsigned narrow values");
    check(value.setWideString(nullptr, L"A\x0123") && textIs(value, "A#"), "inherited wide setter narrows low bytes");

    struct Golden { std::uint32_t bits; const char* text; };
    const Golden goldens[] = {
        {0x00000000,"0.0000000"}, {0x80000000,"0.0000000"},
        {0x3DCCCCCD,"0.1000000"}, {0xBDCCCCCD,"-0.1000000"},
        {0x3F9E0651,"1.2345677"}, {0xBF9E0651,"-1.2345677"},
        {0x411FFFFF,"9.9999990"}, {0x40490FDB,"3.1415927"},
        {0x3F7FFFFF,"0.9999999"}, {0x33D6BF94,"0.0000000"},
        {0x3F800001,"1.0000001"}, {0x4B800000,"16777216.0000000"},
        {0x47F12065,"123456.7890625"}, {0x4F800000,"0.0000000"},
        {0x4F000000,"./,),(-*,(.0000000"}
    };
    for (const Golden& golden : goldens) {
        unsigned short controlBefore, controlAfter, statusBefore, statusAfter;
        __asm { fnstcw controlBefore }
        __asm { fnstsw statusBefore }
        check(value.setFloat(nullptr, fromBits(golden.bits)), "setFloat returns true");
        __asm { fnstsw statusAfter }
        __asm { fnstcw controlAfter }
        check(textIs(value, golden.text), golden.text);
        check(controlBefore == controlAfter && (statusBefore & 0x3800) == (statusAfter & 0x3800),
            "float formatter restores x87 control and stack depth");
    }
    unsigned int savedControl = 0;
    _controlfp_s(&savedControl, 0, 0);
    const unsigned int roundingModes[] = {_RC_NEAR, _RC_DOWN, _RC_UP, _RC_CHOP};
    for (unsigned int mode : roundingModes) {
        unsigned int activeControl = 0;
        _controlfp_s(&activeControl, mode, _MCW_RC);
        check(value.setFloat(nullptr, 2.5f) && textIs(value, "2.5000000"),
            "exact finite conversion under each caller rounding mode");
        _controlfp_s(&activeControl, 0, 0);
        check((activeControl & _MCW_RC) == mode, "caller controlfp rounding mode restored");
    }
    _controlfp_s(&savedControl, savedControl, _MCW_RC);
    SetterObserver setter;
    System* identity = reinterpret_cast<System*>(0x12340000); // [TEMP] never dereferenced
    check(setter.setFloat(identity, fromBits(0x3F9E0651)), "setFloat discards false virtual setter result");
    check(setter.calls == 1 && setter.system == identity && std::strcmp(setter.text,"1.2345677") == 0,
        "setFloat calls override once with same System and formatted temporary");
    check(setter.m_value == nullptr, "virtual observer does not invoke concrete string setter");
    check(setter.setFloatElement(identity, 500, 2.5f) && setter.calls == 2 && std::strcmp(setter.text,"2.5000000") == 0,
        "element setter dispatches through real scalar formatter and ignores index");

    StringVisitor visitor;
    visitor.expected = &value.m_value;
    check(!value.streamProperties(identity, &visitor) && visitor.calls == 1, "property visitor false propagated");
    visitor.result = true; visitor.replace = true;
    check(value.streamProperties(identity, &visitor) && textIs(value,"visitor value"),
        "property visitor true and owned-field replacement visible");
    visitor.replace = false;
    check(value.visit0x2c(identity,&visitor) && value.visit0x34(identity,&visitor)
        && value.visit0x38(identity,&visitor) && value.visit0x3c(identity,&visitor) && visitor.calls == 6,
        "all inherited visitor wrappers dispatch to StringData property body");

    TypeDescriptor* descriptor = StringData::getClassType();
    check(value.getType() == descriptor && descriptor->getInstanceSize() == 12, "concrete descriptor and native size");
    check(std::strcmp(descriptor->getName(), "dice::meme::StringData") == 0 && descriptor->getDescription() == nullptr,
        "descriptor name and inherited null description");
    check(descriptor->getBaseType() == Data::getClassType() && descriptor->isDerivedFrom(Object::getClassType())
        && descriptor->isDerivedFrom(RuntimeObject::getClassType()), "descriptor complete ancestry");
    check(TypeDescriptor::find("dice::meme::StringData") == descriptor, "descriptor actual registry lookup");
    const int before = descriptor->getInstanceCount();
    RuntimeObject* created = descriptor->create();
    check(created && created->getType() == descriptor && static_cast<StringData*>(created)->m_value == nullptr
        && created->getObjectSize() == 12, "factory constructs complete default StringData");
    check(descriptor->getInstanceCount() == before, "direct factory does not modify lifecycle counter");
    delete created;
    created = nullptr;
    check(createRuntimeObject(created, descriptor) && descriptor->getInstanceCount() == before + 1,
        "actual runtime creation increments concrete counter");
    check(createRuntimeObject(created, descriptor) && descriptor->getInstanceCount() == before + 1,
        "actual runtime replacement balances old/new counts");
    check(destroyRuntimeObject(created) && !created && descriptor->getInstanceCount() == before,
        "actual runtime destruction balances concrete counter");

    StringData* watched = new StringData("old text", "owned name");
    watchedObject = watched; watchedValue = watched->m_value; watchedName = const_cast<char*>(watched->getName());
    allocationObserver = true;
    check(watched->setString(nullptr,"new text") && valueDeletes == 1 && !allocationObserver,
        "setter performs observed delete-clear-clone sequence");
    watchedValue = watched->m_value;
    delete watched;
    watchedObject = nullptr;
    check(valueDeletes == 2 && nameDeletes == 1, "destructor frees value then name exactly once");
    std::printf("StringData native diagnostic: %d/%d checks passed\n", checks-failures, checks);
    return failures ? 1 : 0;
}
