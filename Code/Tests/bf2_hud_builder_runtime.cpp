// [TEMP] Native one-slot builder lifetime/binding fixture. Production bodies
// are linked directly; no HUD/System descriptor or global HUD is published.
#include <string>
#define private public
#include "../BF2/Menu/Bf2HudBuilder.h"
#undef private
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <type_traits>
#include <cstddef>
using dice::hfe::hud::Bf2HudBuilder;
namespace {
int checks = 0, failures = 0;
const void* watched[4] = {};
int deletionOrder[4] = {}, deletionCount = 0;
void check(bool value, const char* label)
{
    ++checks;
    if (!value) { ++failures; std::printf("FAIL: %s\n", label); }
}
void recordDelete(void* pointer)
{
    for (int i = 0; i != 4; ++i)
        if (pointer && pointer == watched[i]) {
            if (deletionCount < 4) deletionOrder[deletionCount] = i;
            ++deletionCount;
        }
}
}
// [TEMP] Observe ordinary native string allocations freed by the real dtor.
void* operator new(std::size_t size)
{
    if (void* result = std::malloc(size ? size : 1)) return result;
    throw std::bad_alloc();
}
void operator delete(void* pointer) noexcept { recordDelete(pointer); std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { recordDelete(pointer); std::free(pointer); }

int main()
{
    check(sizeof(void*) == 4 && sizeof(std::string) == 24, "native x86 string ABI");
    check(sizeof(Bf2HudBuilder) == 140, "native builder accounts for four smaller strings");
    check(std::is_polymorphic<Bf2HudBuilder>::value && !std::has_virtual_destructor<Bf2HudBuilder>::value,
          "builder has virtual binding and a nonvirtual destructor");
    check(offsetof(Bf2HudBuilder, m_system) == 4 && offsetof(Bf2HudBuilder, m_field018) == 24,
          "native prefix offsets");
    check(offsetof(Bf2HudBuilder, m_field088) == 120 && offsetof(Bf2HudBuilder, m_buttonTexture) == 124,
          "flag and unknown tail follow native strings");
    alignas(Bf2HudBuilder) unsigned char storage[sizeof(Bf2HudBuilder)];
    std::memset(storage, 0xa5, sizeof(storage));
    Bf2HudBuilder* builder = new (storage) Bf2HudBuilder;
    check(!builder->m_system && !builder->m_field008 && !builder->m_field00c &&
          !builder->m_field010 && !builder->m_field014, "constructor clears five observed words");
    check(builder->m_field018 == "NULL" && builder->m_field034 == "NULL" &&
          builder->m_field050 == "NULL" && builder->m_field06c == "NULL", "four strings retain exact uppercase sentinel");
    check(builder->m_field088, "constructor enables observed flag");
    bool untouched = true;
    for (std::size_t i = offsetof(Bf2HudBuilder, m_field088) + 1; i != sizeof(storage); ++i)
        untouched = untouched && storage[i] == 0xa5;
    check(untouched, "constructor preserves alignment and trailing payload bytes");

    // Binding stores opaque identities only; neither token is represented as a
    // constructed System object, and the real binding never dereferences them.
    int firstToken = 0, secondToken = 0;
    dice::meme::System* first = reinterpret_cast<dice::meme::System*>(&firstToken);
    dice::meme::System* second = reinterpret_cast<dice::meme::System*>(&secondToken);
    Bf2HudBuilder* volatile dispatch = builder;
    dispatch->setSystem(first);
    check(builder->m_system == first, "virtual binding stores supplied pointer");
    dispatch->setSystem(second);
    check(builder->m_system == second, "virtual binding replaces old pointer without ownership");
    dispatch->setSystem(nullptr);
    check(!builder->m_system, "virtual binding accepts null");
    check(builder->m_field018 == "NULL" && builder->m_field088 && firstToken == 0 && secondToken == 0,
          "binding does not change other fields or pointee storage");

    builder->m_field018.assign(80, 'a');
    builder->m_field034.assign(81, 'b');
    builder->m_field050.assign(82, 'c');
    builder->m_field06c.assign(83, 'd');
    watched[0] = builder->m_field018.data(); watched[1] = builder->m_field034.data();
    watched[2] = builder->m_field050.data(); watched[3] = builder->m_field06c.data();
    builder->m_system = first;
    // Identity tokens only; the real destructor never dereferences these
    // borrowed handles. Keep each assignment typed after handle recovery.
    builder->m_field008 = reinterpret_cast<dice::meme::Bf2MenuMapNode*>(&firstToken);
    builder->m_field00c = reinterpret_cast<dice::meme::Object*>(&firstToken);
    builder->m_field010 = reinterpret_cast<dice::meme::Object*>(&secondToken);
    builder->m_field014 = reinterpret_cast<dice::meme::Object*>(&firstToken);
    builder->~Bf2HudBuilder();
    check(deletionCount == 4 && deletionOrder[0] == 3 && deletionOrder[1] == 2 &&
          deletionOrder[2] == 1 && deletionOrder[3] == 0, "real destructor releases four strings in reverse order");
    check(firstToken == 0 && secondToken == 0, "destructor does not dereference or own System");
    bool retained = true;
    for (std::size_t i = offsetof(Bf2HudBuilder, m_field088) + 1; i != sizeof(storage); ++i)
        retained = retained && storage[i] == 0xa5;
    check(retained, "destructor leaves unspecified tail untouched");
    std::printf("Bf2HudBuilder: %d/%d checks passed (strict native x86 link).\n", checks-failures, checks);
    return failures ? 1 : 0;
}
