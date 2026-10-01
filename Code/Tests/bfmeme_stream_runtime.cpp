// [TEMP] Transport and object-lifetime fixtures for real recovered BFMeme bodies.
// /FORCE leaves descriptor, sound, collection and class-reader frontiers uncalled.
#include "../BF2/External/BFMeme/OStream.h"
#include "../BF2/External/BFMeme/IStream.h"
#include "../BF2/External/BFMeme/FileIStream.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/External/BFMeme/Data.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using namespace dice::meme;
static int checks, failures;
#define CHECK(expression) do { ++checks; if (!(expression)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #expression); } } while (0)

// Transport-only overrides model an exact byte sink/source and selected short I/O.
struct BufferOutput : OStream {
    std::vector<unsigned char> bytes;
    int position = 0, calls = 0, shortCall = -1, shortCount = 0, seekCalls = 0;
    int write(const void* data, int count) override {
        ++calls;
        if (calls == shortCall) count = (std::min)(count, shortCount);
        if (position + count > static_cast<int>(bytes.size())) bytes.resize(position + count);
        if (count) std::memcpy(bytes.data() + position, data, count);
        position += count;
        return count;
    }
    int getPosition() override { return position; }
    bool setPosition(int p) override { ++seekCalls; position = p; return false; }
};
struct BufferInput : IStream {
    std::vector<unsigned char> bytes;
    int position = 0, calls = 0, shortCall = -1, shortCount = 0;
    explicit BufferInput(const std::vector<unsigned char>& data) : bytes(data) {}
    int read(void* data, int count) override {
        ++calls;
        count = (std::min)(count, static_cast<int>(bytes.size()) - position);
        if (calls == shortCall) count = (std::min)(count, shortCount);
        if (count) std::memcpy(data, bytes.data() + position, count);
        position += count;
        return count;
    }
    int getPosition() override { return position; }
};
struct ModeStream : Stream {
    int calls = 0;
    bool isPassive() override { ++calls; return true; }
};
static int enumCalls;
static int __fastcall enumNext(int value) { ++enumCalls; return value + 1; }
static const char* __fastcall enumName(int) { ++enumCalls; return "fixture"; }

static void codecTests()
{
    CHECK(sizeof(Stream) == 4); CHECK(sizeof(OStream) == 4); CHECK(sizeof(IStream) == 8);
    CHECK(sizeof(FileIStream) == 12); CHECK(sizeof(DeletionStream) == 8);
    Stream base; int scalar = 7;
    CHECK(!base.streamInt(&scalar, "value")); CHECK(scalar == 7);
    ModeStream mode;
    CHECK(mode.streamInt(&scalar, "value")); CHECK(mode.calls == 1);
    CHECK(mode.streamFont(nullptr, &scalar, "font")); CHECK(mode.calls == 2);

    BufferOutput output;
    unsigned char ub = 0xEF; char sb = -2; unsigned short us = 0xABCD; short ss = -1234;
    unsigned long ul = 0x89ABCDEF; long sl = -1234567; float f = 1.25f; bool b = true;
    int integer = -100003; wchar_t character = 0xFFFE;
    CHECK(output.streamUByte(&ub, "")); CHECK(output.streamByte(&sb, ""));
    CHECK(output.streamUShort(&us, "")); CHECK(output.streamShort(&ss, ""));
    CHECK(output.streamULong(&ul, "")); CHECK(output.streamLong(&sl, ""));
    CHECK(output.streamFloat(&f, "")); CHECK(output.streamBool(&b, ""));
    CHECK(output.streamInt(&integer, "")); CHECK(output.streamCharacter(&character, ""));
    CHECK(output.bytes.size() == 27);
    CHECK(output.bytes[2] == 0xCD && output.bytes[3] == 0xAB);
    CHECK(output.bytes[23] == 0xFE && output.bytes[24] == 0xFF && output.bytes[25] == 0 && output.bytes[26] == 0);
    BufferInput input(output.bytes); IStream middle(&input), outer(&middle);
    CHECK(outer.getSource() == &input);
    unsigned char ub2 = 0; char sb2 = 0; unsigned short us2 = 0; short ss2 = 0;
    unsigned long ul2 = 0; long sl2 = 0; float f2 = 0; bool b2 = false; int i2 = 0; wchar_t c2 = 0;
    CHECK(outer.streamUByte(&ub2, "") && ub2 == ub); CHECK(outer.streamByte(&sb2, "") && sb2 == sb);
    CHECK(outer.streamUShort(&us2, "") && us2 == us); CHECK(outer.streamShort(&ss2, "") && ss2 == ss);
    CHECK(outer.streamULong(&ul2, "") && ul2 == ul); CHECK(outer.streamLong(&sl2, "") && sl2 == sl);
    CHECK(outer.streamFloat(&f2, "") && f2 == f); CHECK(outer.streamBool(&b2, "") && b2);
    CHECK(outer.streamInt(&i2, "") && i2 == integer); CHECK(outer.streamCharacter(&c2, "") && c2 == character);
    b2 = true; CHECK(!outer.streamBool(&b2, "") && !b2); // clears before failure
    i2 = 43; CHECK(!outer.streamInt(&i2, "") && i2 == 43);
    BufferInput noncanonical({ 0xFE }); CHECK(noncanonical.streamBool(&b2, "") && b2);
    bool raw = false; *reinterpret_cast<unsigned char*>(&raw) = 0xFE;
    BufferOutput normalized; CHECK(normalized.streamBool(&raw, "") && normalized.bytes[0] == 1);

    char narrowStorage[] = "abc"; char* narrow = narrowStorage;
    BufferOutput strings; CHECK(strings.streamString(&narrow, "ignored"));
    CHECK(strings.bytes == std::vector<unsigned char>({3,0,0,0,'a','b','c'}));
    BufferInput readString(strings.bytes); char* owned = static_cast<char*>(::operator new(8));
    std::strcpy(owned, "old"); CHECK(readString.streamString(&owned, "ignored"));
    CHECK(std::strcmp(owned, "abc") == 0); ::operator delete(owned);
    BufferOutput shortWrite; shortWrite.shortCall = 2; shortWrite.shortCount = 1;
    CHECK(shortWrite.streamString(&narrow, "")); CHECK(shortWrite.bytes.size() == 5); // retail defect
    BufferInput shortRead(strings.bytes); shortRead.shortCall = 2; shortRead.shortCount = 1; owned = nullptr;
    CHECK(shortRead.streamString(&owned, "")); CHECK(owned[0] == 'a' && owned[3] == 0); ::operator delete(owned);
    BufferInput failedString({}); owned = static_cast<char*>(::operator new(2));
    CHECK(!failedString.streamString(&owned, "") && owned == nullptr);
    BufferInput zeroString({0,0,0,0}); owned = static_cast<char*>(::operator new(2));
    CHECK(zeroString.streamString(&owned, "") && owned == nullptr);
    char emptyStorage[] = ""; char* empty = emptyStorage; BufferOutput emptyOutput;
    CHECK(emptyOutput.streamString(&empty, "")); CHECK(emptyOutput.calls == 2);
    char oneStorage[] = "x"; char* one = oneStorage; BufferOutput failedOne;
    failedOne.shortCall = 2; CHECK(!failedOne.streamString(&one, ""));

    wchar_t wideStorage[] = L"AZ"; wchar_t* wide = wideStorage; BufferOutput wideOutput;
    CHECK(wideOutput.streamWideString(&wide, "")); CHECK(wideOutput.bytes.size() == 8);
    BufferInput wideInput(wideOutput.bytes); wchar_t* ownedWide = nullptr;
    CHECK(wideInput.streamWideString(&ownedWide, "")); CHECK(std::wcscmp(ownedWide,L"AZ") == 0); ::operator delete(ownedWide);
    BufferOutput wideShort; wideShort.shortCall = 2; wideShort.shortCount = 1;
    CHECK(!wideShort.streamWideString(&wide, ""));

    BufferOutput shortStrings; std::string longName(258, 'q');
    CHECK(shortStrings.writeShortString(nullptr, longName.c_str(), ""));
    CHECK(shortStrings.bytes == std::vector<unsigned char>({2,'q','q'}));
    BufferOutput wrapped; std::string wrapName(256, 'z');
    CHECK(wrapped.writeShortString(nullptr, wrapName.c_str(), "") && wrapped.calls == 1);
    char buffer[4] = {'?', '?', '?', '?'}; BufferInput bounded({3,'a','b','c'});
    CHECK(bounded.readShortString(nullptr, buffer, 3, "")); CHECK(std::strcmp(buffer,"abc") == 0); // equality accepted
    BufferInput oversized({3,'a','b','c'}); buffer[0] = '?';
    CHECK(!oversized.readShortString(nullptr, buffer, 2, "") && oversized.position == 1 && buffer[0] == '?');
    BufferInput zeroShort({0}); CHECK(zeroShort.readShortString(nullptr, buffer, -1, "") && buffer[0] == 0);
    BufferOutput enumeration; int e = 17;
    CHECK(enumeration.streamEnum(&e, enumNext, enumName, "enum"));
    BufferInput enumInput(enumeration.bytes); e = 0;
    CHECK(enumInput.streamEnum(&e, enumNext, enumName, "enum") && e == 17 && enumCalls == 0);

    Object* nullObject = nullptr; BufferOutput nullFrame;
    CHECK(writeObject(nullptr, nullObject, &nullFrame));
    CHECK(nullFrame.bytes == std::vector<unsigned char>({6,0,0,0,0,0}));
    CHECK(nullFrame.position == 6 && nullFrame.seekCalls == 2); // false seek returns ignored
    BufferOutput failedBackpatch; failedBackpatch.shortCall = 4;
    CHECK(!writeObject(nullptr, nullObject, &failedBackpatch));
    CHECK(failedBackpatch.position == 0 && failedBackpatch.seekCalls == 1);
}

static int liveObjects, visits, destroyed, fixtureInstances;
static bool constructNamed, createResult = true, visitResult = true;
static Object* lastCreated;
static Object** observedSlot;
static bool sawClearedSlot;
static TypeDescriptor* fixtureType();
struct FixtureObject : Object {
    bool named;
    FixtureObject() : named(constructNamed) { ++liveObjects; lastCreated = this; }
    ~FixtureObject() override { --liveObjects; ++destroyed; }
    TypeDescriptor* getType() override { return fixtureType(); }
    const char* getName() override { return named ? "named" : nullptr; }
    bool onCreate(System*) override { return createResult; }
    bool streamProperties(System*, Stream* stream) override {
        ++visits; if (observedSlot && !*observedSlot) sawClearedSlot = true;
        return stream->isPassive() && visitResult;
    }
};
struct FixtureDescriptor : TypeDescriptor {
    FixtureDescriptor() : TypeDescriptor("fixture::StreamLifetimeObject") {}
    RuntimeObject* create() override { return new FixtureObject; }
    void addInstance() override { ++fixtureInstances; }
    void removeInstance() override { --fixtureInstances; }
};
static TypeDescriptor* fixtureType() { static FixtureDescriptor type; return &type; }
static void cleanup(Object*& object) { CHECK(destroyRuntimeObject(reinterpret_cast<RuntimeObject*&>(object))); }
static void lifetimeTests()
{
    Object* value = nullptr;
    CHECK(deleteObject(nullptr, value));
    CHECK(createObject(nullptr, value, fixtureType())); CHECK(value && fixtureInstances == 1);
    observedSlot = &value; CHECK(deleteObject(nullptr, value));
    CHECK(!value && fixtureInstances == 0 && liveObjects == 0 && visits == 1 && sawClearedSlot);
    constructNamed = true;
    CHECK(createObject(nullptr, value, fixtureType())); Object* retained = value;
    int previousVisits = visits; CHECK(deleteObject(nullptr, value));
    CHECK(!value && liveObjects == 1 && visits == previousVisits && fixtureInstances == 1); cleanup(retained);
    CHECK(createObject(nullptr, value, fixtureType())); retained = value;
    DeletionStream preserve(true); CHECK(preserve.streamObject(nullptr, value, "", nullptr));
    CHECK(value == retained && fixtureInstances == 1 && visits == previousVisits + 1); cleanup(value);
    CHECK(createObject(nullptr, value, fixtureType())); retained = value; visitResult = false;
    CHECK(!preserve.streamObject(nullptr, value, "", nullptr));
    CHECK(!value && liveObjects == 1 && fixtureInstances == 1); cleanup(retained);
    constructNamed = false;
    CHECK(createObject(nullptr, value, fixtureType())); retained = value;
    CHECK(!deleteObject(nullptr, value)); CHECK(!value && fixtureInstances == 1); cleanup(retained);
    visitResult = true; createResult = false;
    CHECK(!createObject(nullptr, value, fixtureType())); CHECK(!value && liveObjects == 0 && fixtureInstances == 0);
    constructNamed = true;
    CHECK(!createObject(nullptr, value, fixtureType()));
    CHECK(!value && liveObjects == 1 && fixtureInstances == 1); retained = lastCreated; cleanup(retained);
    createResult = true; constructNamed = false; observedSlot = nullptr;
    CHECK(liveObjects == 0 && fixtureInstances == 0);

    Object* first = nullptr; Object* second = nullptr;
    CHECK(createObject(nullptr, first, Data::getClassType())); CHECK(first->setName("first"));
    CHECK(createObject(nullptr, second, Data::getClassType())); CHECK(second->setName("second"));
    NameTreeRoot registry;
    CHECK(registry.insert(nullptr, first, "Top")); CHECK(registry.insert(nullptr, second, "Top"));
    CHECK(registry.preClose(nullptr));
    CHECK(registry.findObject(nullptr, "Top/first") == first && registry.findObject(nullptr, "Top/second") == second);
    CHECK(Data::getClassType()->getInstanceCount() == 2);
    ModeStream traversal; CHECK(registry.stream(nullptr, &traversal)); CHECK(traversal.calls == 2);
    CHECK(registry.close(nullptr)); CHECK(Data::getClassType()->getInstanceCount() == 0);
}
int main()
{
    codecTests(); lifetimeTests();
    std::printf("BFMeme stream runtime: %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
