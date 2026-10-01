// [TEMP] [CHOSEN, not recovered] Focused PE-layout constructor/lifecycle fixture.
// Uses production OverGrowth methods, authored untouched-byte sentinels and process-local heaps.
#include "../BF2/Geom/Overgrowth/OverGrowth.h"
#include "../RendDX9Abi/Vc71String.h"
#include "../RendDX9Abi/Vc71Vector.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {
int checks;
int failures;
int frees;
int nullFrees;
template<class T> T& field(void* p, unsigned int off)
{ return *reinterpret_cast<T*>(static_cast<unsigned char*>(p) + off); }
void check(bool value, const char* name)
{
    ++checks;
    if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", name); }
}
}
namespace renddx9abi {
void* engineAlloc(unsigned int size)
{
    void* p = std::malloc(size);
    if (p) std::memset(p, 0xCC, size);
    return p;
}
void engineFree(void* p) { if (p) ++frees; else ++nullFrees; std::free(p); }
void* crtMalloc(size_t size) { return std::malloc(size); }
void crtFree(void* p) { std::free(p); }
}
namespace dice { namespace hfe { namespace dx9rend {
void* g_OverGrowth;
void overGrowthPreLoadResources();
void overGrowthPostLoadResources();
} } }

int main(int argc, char** argv)
{
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    using dice::hfe::geom::OverGrowth;
    using namespace dice::hfe::dx9rend;
    OverGrowth* p = dice::hfe::geom::createOverGrowth();
    g_OverGrowth = p;
    if (argc > 1 && std::strcmp(argv[1], "nonempty") == 0)
    {
        field<vc71::string>(p, 0x88) = "authored/nonempty";
        overGrowthPreLoadResources(); // Must diagnose and abort; never return fabricated success.
        return 99;
    }
    check(p && sizeof(*p) == 0x154, "340-byte owner allocated");
    check(field<vc71::string>(p, 0x88).empty() && field<unsigned int>(p, 0x9C) == 0,
          "real constructed empty path has zero size at +9C");
    check(field<unsigned int>(p, 0x18) == 0x759DC150 && field<int>(p, 0x10) == 10
              && field<float>(p, 0x24) == 500.0f && field<float>(p, 0xF4) == 1.0f,
          "base and derived scalar defaults match the PE");
    check(field<unsigned int>(p, 0x64) == 0xCCCCCCCC
              && field<unsigned int>(p, 0xD8) == 0xCCCCCCCC
              && field<unsigned int>(p, 0x104) == 0xCCCCCCCC,
          "constructor preserves unwritten coordinates, grid and tail");
    void** table = *reinterpret_cast<void***>(p);
    typedef float (__thiscall* Getter)(void*);
    typedef void (__thiscall* Draw)(void*, void*, float);
    typedef void (__thiscall* Update)(void*, void*);
    check(reinterpret_cast<Getter>(table[0x10 / 4])(p) == 500.0f,
          "actual primary +10 getter returns its float through ST0");
    overGrowthPreLoadResources();
    overGrowthPostLoadResources();
    reinterpret_cast<Draw>(table[0x34 / 4])(p, 0, 12.0f);
    reinterpret_cast<Update>(table[0x1C / 4])(p, 0);
    check(field<unsigned int>(p, 0xD8) == 0xCCCCCCCC && field<void*>(p, 0xE0) == 0,
          "empty direct lifecycle and actual draw slots touch no resource/view fields");
    {
        typedef void (__thiscall* ShadowQuery)(void*, void*, void*, void*, void*);
        const ShadowQuery shadowQuery = reinterpret_cast<ShadowQuery>(table[0x38 / 4]);
        vc71::vector_layout records = { 0, 0, 0, 0 };
        vc71::vector_layout geometry = { 0, 0, 0, 0 };
        alignas(4) unsigned char oneRecord[0x498];
        unsigned char patchSentinel = 0;

        // The rebuilt client's OverGrowth: never loaded, patch array +0xE0 still the ctor's null.
        shadowQuery(p, &records, 0, 0, &geometry);
        check(geometry.first == 0 && geometry.last == 0 && field<void*>(p, 0xE0) == 0,
              "+38 on the never-loaded owner returns at the patch-array test, touching nothing");

        // Each later guard, one at a time.
        field<void*>(p, 0xE0) = &patchSentinel;
        field<unsigned char>(p, 0xD4) = 0;
        shadowQuery(p, &records, 0, 0, &geometry);
        check(geometry.first == 0, "+38 returns at the +D4 byte");
        field<unsigned char>(p, 0xD4) = 1;
        shadowQuery(p, &records, 0, 0, &geometry);
        check(geometry.first == 0, "+38 returns on a null records vector (100CE200)");
        records.first = records.last = reinterpret_cast<char*>(oneRecord);
        shadowQuery(p, &records, 0, 0, &geometry);
        check(geometry.first == 0, "+38 returns on an empty records vector (100CE200)");
        records.last = records.first + 0x498;
        const int gridSize = field<int>(p, 0x10);
        field<int>(p, 0x10) = 0;
        shadowQuery(p, &records, 0, 0, &geometry);
        check(geometry.first == 0, "+38 returns on a non-positive grid size (signed +10)");
        field<int>(p, 0x10) = gridSize;
        if (argc > 1 && std::strcmp(argv[1], "shadowwalk") == 0)
        {
            shadowQuery(p, &records, 0, 0, &geometry); // Must name the patch walk and abort.
            return 98;
        }
        field<void*>(p, 0xE0) = 0;                     // clear() must see the ctor's state again
    }
    const int oldFrees = frees;
    const int oldNullFrees = nullFrees;
    typedef void* (__thiscall* Delete)(void*, unsigned int);
    reinterpret_cast<Delete>(table[0])(p, 1);
    g_OverGrowth = 0;
    check(frees == oldFrees + 1 && nullFrees == oldNullFrees + 4,
          "scalar deleting slot frees owner after both ordered base cleanup passes");
    std::printf("overgrowth runtime: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
