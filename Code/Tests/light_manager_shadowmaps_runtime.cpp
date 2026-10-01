// [TEMP] [CHOSEN, not recovered] PE-layout dependency fixture for the real +0x2EC loader.
// Reuse the existing deterministic device/heap providers; no production LightManager body or
// table is replaced. The additional records model only settings +40, class manager +48,
// HeightmapCluster +2C, file manager +18, and stream read/release consumed by this loader.
#include <cstdlib>
#include <new>
static void* watchedIOAllocation;
static unsigned watchedIODeletes;
// [TEMP] Observe the real vector release without changing any production storage operation.
void* operator new(std::size_t size)
{
    void* result = std::malloc(size ? size : 1);
    if (!result) throw std::bad_alloc();
    return result;
}
void operator delete(void* value) noexcept
{
    if (value && value == watchedIOAllocation) ++watchedIODeletes;
    std::free(value);
}
void operator delete(void* value, std::size_t) noexcept { ::operator delete(value); }

#define main lightManagerGeneralFixtureMain
#include "light_manager_runtime.cpp"
#undef main

namespace {

struct ShadowService { void** table; };
void* g_shadowClassTable[0x4C / 4] = {};
void* g_shadowHeightTable[0x30 / 4] = {};
void* g_shadowSettingsTable[0x44 / 4] = {};
void* g_shadowFileTable[0x1C / 4] = {};
void* g_shadowStreamTable[0x14 / 4] = {};
ShadowService g_shadowClass = {g_shadowClassTable};
ShadowService g_shadowHeight = {g_shadowHeightTable};
ShadowService g_shadowSettings = {g_shadowSettingsTable};
ShadowService g_shadowFiles = {g_shadowFileTable};

struct ShadowStream
{
    void** table;
    std::vector<unsigned char> bytes;
    const char* label;
};
ShadowStream g_shadowMain = {g_shadowStreamTable, {}, "main"};
ShadowStream g_shadowIO = {g_shadowStreamTable, {}, "IO"};
bool g_shadowMainExists;
bool g_shadowIOExists;
int g_shadowHeightmapSize;
renddx9abi::HostString g_shadowLevel("Fixture");
renddx9abi::HostString g_shadowDirectory("Levels/");
std::vector<std::string> g_shadowCalls;
std::vector<std::string> g_shadowPaths;
int g_shadowReadCalls;
int g_shadowReleases;
bool g_shadowReadSizesCorrect;
bool g_shadowOpenFlagsCorrect;
bool g_shadowFallbacksEmpty;

void* __fastcall shadowSingleton(void*, void*, const renddx9abi::HostString& name)
{
    g_shadowCalls.push_back(std::string("singleton:") + name.c_str());
    return std::strcmp(name.c_str(), "HeightmapCluster") == 0 ? &g_shadowHeight : 0;
}

int __fastcall shadowHeightmapSize(void*, void*)
{
    g_shadowCalls.push_back("heightmapSize");
    return g_shadowHeightmapSize;
}

const renddx9abi::HostString& __fastcall shadowStringGet(
    void*, void*, const renddx9abi::HostString& key, const renddx9abi::HostString& fallback)
{
    g_shadowCalls.push_back(std::string("setting:") + key.c_str());
    g_shadowFallbacksEmpty = g_shadowFallbacksEmpty && fallback.size() == 0;
    if (std::strcmp(key.c_str(), "GSLoadLevel") == 0) return g_shadowLevel;
    if (std::strcmp(key.c_str(), "GSLevelDirectory") == 0) return g_shadowDirectory;
    return fallback;
}

renddx9abi::HostStream* __fastcall shadowOpen(
    void*, void*, const renddx9abi::HostString& path,
    renddx9abi::HostFileMode mode, renddx9abi::HostFileAccess access, unsigned int flags)
{
    g_shadowPaths.push_back(path.c_str());
    g_shadowOpenFlagsCorrect = g_shadowOpenFlagsCorrect && mode == 2 && access == 0 && flags == 0;
    const bool io = std::strstr(path.c_str(), "SimpleShadowmapIO.raw") != 0;
    g_shadowCalls.push_back(io ? "open:IO" : "open:main");
    if (io ? !g_shadowIOExists : !g_shadowMainExists) return 0;
    return reinterpret_cast<renddx9abi::HostStream*>(io ? &g_shadowIO : &g_shadowMain);
}

unsigned int __fastcall shadowRead(void* self, void*, void* destination, unsigned int size)
{
    ShadowStream& stream = *static_cast<ShadowStream*>(self);
    ++g_shadowReadCalls;
    g_shadowCalls.push_back(std::string("read:") + stream.label);
    g_shadowReadSizesCorrect = g_shadowReadSizesCorrect && size == stream.bytes.size();
    if (size) std::memcpy(destination, stream.bytes.data(), size);
    // The fixture fills every byte but deliberately reports zero: retail discards read's return.
    return 0;
}

int __fastcall shadowRelease(void* self, void*)
{
    ShadowStream& stream = *static_cast<ShadowStream*>(self);
    ++g_shadowReleases;
    g_shadowCalls.push_back(std::string("release:") + stream.label);
    return 0;
}

void initializeShadowProviders()
{
    g_shadowClassTable[0x48 / 4] = reinterpret_cast<void*>(&shadowSingleton);
    g_shadowHeightTable[0x2C / 4] = reinterpret_cast<void*>(&shadowHeightmapSize);
    g_shadowSettingsTable[0x40 / 4] = reinterpret_cast<void*>(&shadowStringGet);
    g_shadowFileTable[0x18 / 4] = reinterpret_cast<void*>(&shadowOpen);
    g_shadowStreamTable[0x10 / 4] = reinterpret_cast<void*>(&shadowRead);
    g_shadowStreamTable[0x08 / 4] = reinterpret_cast<void*>(&shadowRelease);
    dice::hfe::classManager = reinterpret_cast<dice::hfe::IClassManager*>(&g_shadowClass);
    dice::hfe::settings = reinterpret_cast<IBase*>(&g_shadowSettings);
    dice::hfe::fileManager = reinterpret_cast<renddx9abi::HostFileManager*>(&g_shadowFiles);
    g_shadowReadSizesCorrect = g_shadowOpenFlagsCorrect = g_shadowFallbacksEmpty = true;
}

void configureShadowLoad(int heightmapSize, bool mainExists, bool ioExists)
{
    g_shadowHeightmapSize = heightmapSize;
    g_shadowMainExists = mainExists;
    g_shadowIOExists = ioExists;
    const int size = heightmapSize / 2;
    g_shadowMain.bytes.resize(size * size);
    g_shadowIO.bytes.resize(size * size);
    for (unsigned int i = 0; i < g_shadowMain.bytes.size(); ++i)
    {
        g_shadowMain.bytes[i] = static_cast<unsigned char>(i);
        const unsigned char choices[] = {0, 127, 128, 255};
        g_shadowIO.bytes[i] = choices[i % 4];
    }
    g_shadowCalls.clear();
    g_shadowPaths.clear();
}

bool allPackedDefault(LightManager* manager)
{
    const unsigned char* data = rawField<unsigned char*>(manager, 0x190);
    const int bytes = rawField<int>(manager, 0x1A0) * rawField<int>(manager, 0x1A8);
    for (int i = 0; i < bytes; ++i) if (data[i] != 255) return false;
    return true;
}

bool ioBit(LightManager* manager, unsigned int index)
{
    return (rawField<unsigned int*>(manager, 0x184)[index >> 5] & (1u << (index & 31))) != 0;
}

bool allIOTrue(LightManager* manager)
{
    for (unsigned int i = 0; i < rawField<unsigned int>(manager, 0x17C); ++i)
        if (!ioBit(manager, i)) return false;
    return true;
}

bool dimensionsEqual(LightManager* manager, int size, int bitWidth, int rows,
                     int bitX, int originY, int byteWidth, int byteX)
{
    return rawField<float>(manager, 0x194) == size && rawField<int>(manager, 0x198) == bitWidth
        && rawField<int>(manager, 0x19C) == bitX && rawField<int>(manager, 0x1A0) == rows
        && rawField<int>(manager, 0x1A4) == originY && rawField<int>(manager, 0x1A8) == byteWidth
        && rawField<int>(manager, 0x1AC) == byteX && rawField<int>(manager, 0x1B0) == originY;
}

void testSimpleShadowLoader()
{
    LightManager* manager = createManager();
    check(manager != 0, "real LightManager factory constructs the loader's storage");

    configureShadowLoad(8, false, true);
    manager->loadSimpleShadowMaps();
    check(dimensionsEqual(manager, 4, 12, 6, 6, 3, 3, 1),
          "loader derives every packed and bit dimension from getHeightmapSize / 2");
    check(allPackedDefault(manager), "missing main file leaves packed bytes at FF");
    check(rawField<unsigned int>(manager, 0x17C) == 0 && rawField<void*>(manager, 0x184) == 0,
          "initial missing main file preserves the empty IO vector");
    const char* absentCalls[] = {"singleton:HeightmapCluster", "heightmapSize",
        "setting:GSLoadLevel", "setting:GSLevelDirectory", "open:main"};
    check(g_shadowCalls == std::vector<std::string>(absentCalls, absentCalls + 5),
          "missing main file returns before IO lookup and preserves singleton/settings order");
    check(g_shadowPaths.size() == 1 && g_shadowPaths[0] == "Levels/Fixture/SimpleShadowmap.raw",
          "path concatenates GSLevelDirectory then GSLoadLevel without adding separators");

    configureShadowLoad(32, true, true);
    manager->loadSimpleShadowMaps();
    check(dimensionsEqual(manager, 16, 24, 18, 12, 9, 6, 3),
          "present 16x16 image publishes both coordinate origins and row strides");
    check(rawField<unsigned int>(manager, 0x17C) == 432,
          "successful main load sizes the IO vector to padded bit width times rows");
    bool mainPixels = true, ioPixels = true, mainPadding = true, ioPadding = true;
    const unsigned char* packed = rawField<unsigned char*>(manager, 0x190);
    for (int y = 0; y < 18; ++y)
    {
        for (int x = 0; x < 24; ++x)
        {
            const bool inside = y > 0 && y < 17 && x >= 4 && x < 20;
            const int pixel = (packed[y * 6 + x / 4] >> (2 * (x % 4))) & 3;
            if (inside)
            {
                const int source = (16 - y) * 16 + x - 4;
                const int expected = source <= 42 ? 0 : source <= 127 ? 1 : source <= 212 ? 2 : 3;
                mainPixels = mainPixels && pixel == expected;
                ioPixels = ioPixels && ioBit(manager, y * 24 + x) == (source % 4 >= 2);
            }
            else
            {
                mainPadding = mainPadding && pixel == 3;
                ioPadding = ioPadding && ioBit(manager, y * 24 + x);
            }
        }
    }
    check(mainPixels, "all 256 byte values quantize at 43/128/213 and vertically flip into two-bit texels");
    check(ioPixels, "IO threshold distinguishes 127 from 128 and vertically flips rows");
    check(mainPadding && ioPadding, "one row and four column borders retain their shipped defaults");
    check(rawField<unsigned int*>(manager, 0x184)[13] == 0xFFFFu,
          "IO fill leaves high bits above the declared count at zero");
    const char* presentCalls[] = {"singleton:HeightmapCluster", "heightmapSize",
        "setting:GSLoadLevel", "setting:GSLevelDirectory", "open:main", "read:main",
        "release:main", "open:IO", "read:IO", "release:IO"};
    check(g_shadowCalls == std::vector<std::string>(presentCalls, presentCalls + 10),
          "each stream is read and released before the next file opens");
    check(g_shadowPaths.size() == 2 && g_shadowPaths[1] == "Levels/Fixture/SimpleShadowmapIO.raw",
          "IO file uses the same composed level path");
    check(manager->sampleSimpleShadowMapTexel(-8, -8) == 1.0f &&
          manager->sampleSimpleShadowMapTexel(-8, 7) == 0.0f &&
          manager->sampleSimpleShadowMapIOTexel(-8, -8) == 0.0f &&
          manager->sampleSimpleShadowMapIOTexel(-6, -8) == 1.0f,
          "real texel samplers consume the loader's packed arrays and centered origins");

    vc71::vector_layout& vector = rawField<vc71::vector_layout>(manager, 0x180);
    char* savedFirst = vector.first;
    char* savedLast = vector.last;
    char* savedEnd = vector.end;
    const std::vector<unsigned char> savedBytes(vector.first, vector.last);
    configureShadowLoad(12, false, true);
    manager->loadSimpleShadowMaps();
    check(dimensionsEqual(manager, 6, 14, 8, 7, 4, 3, 1) && allPackedDefault(manager),
          "reload absent main replaces packed storage and dimensions with defaults");
    check(rawField<unsigned int>(manager, 0x17C) == 432 && vector.first == savedFirst &&
          vector.last == savedLast && vector.end == savedEnd &&
          std::memcmp(vector.first, savedBytes.data(), savedBytes.size()) == 0,
          "reload absent main preserves the previous IO vector including contents and capacity");
    check(g_shadowPaths.size() == 1, "reload absent main still skips IO even when that file exists");

    configureShadowLoad(12, true, false);
    manager->loadSimpleShadowMaps();
    check(rawField<unsigned int>(manager, 0x17C) == 112 && allIOTrue(manager),
          "missing IO after a present main file resets every in-range bit to true");
    check(vector.first == savedFirst && vector.end == savedEnd && vector.last == vector.first + 16 &&
          rawField<unsigned int*>(manager, 0x184)[3] == 0xFFFFu,
          "IO vector shrink retains capacity and masks the high tail bits");

    configureShadowLoad(40, true, false);
    watchedIOAllocation = vector.first;
    watchedIODeletes = 0;
    manager->loadSimpleShadowMaps();
    check(rawField<unsigned int>(manager, 0x17C) == 616 && allIOTrue(manager),
          "larger reload grows IO storage and initializes all bits");
    check(vector.last - vector.first == 80 && vector.end - vector.first == 84,
          "VC7 word storage grows prior capacity 14 by one half to 21 words");
    check(watchedIODeletes == 1, "IO vector growth releases its previous word allocation exactly once");
    watchedIOAllocation = 0;

    configureShadowLoad(7, true, false);
    manager->loadSimpleShadowMaps();
    check(dimensionsEqual(manager, 3, 11, 5, 5, 2, 2, 1) &&
          rawField<unsigned int>(manager, 0x17C) == 55 && allIOTrue(manager),
          "odd heightmap size retains shipped integer truncation in all dimensions");
    check(g_shadowReadSizesCorrect && g_shadowReadCalls == g_shadowReleases,
          "every N-squared read has a matching release, even with a zero reported read count");
    check(g_shadowOpenFlagsCorrect && g_shadowFallbacksEmpty,
          "both opens use mode 2/access 0/flags 0 and both settings use empty defaults");
    watchedIOAllocation = vector.first;
    watchedIODeletes = 0;
    check(releaseManager(manager) == 0, "real destructor releases main and IO allocations after reloads");
    check(watchedIODeletes == 1, "real destructor frees the final IO word allocation exactly once");
    watchedIOAllocation = 0;
}

} // anonymous namespace

int main()
{
    static_assert(sizeof(void*) == 4, "shadow map fixture is x86-only");
    initializeFakeCom();
    initializeRendererBase();
    initializeShadowProviders();
    testSimpleShadowLoader();
    if (g_rendererBaseObject.primaryRenderTarget)
    {
        g_rendererBaseObject.primaryRenderTarget->Release();
        g_rendererBaseObject.primaryRenderTarget = 0;
    }
    check(g_frontierCalls == 0, "loader fixture reaches no production frontier");
    check(g_textureStates.empty() && g_surfaceCreates == g_surfaceDestroys,
          "all isolated renderer fixture resources are released");
    if (g_engineAllocations != g_engineFrees)
        std::printf("engine allocation counts: %d allocated, %d freed\n", g_engineAllocations, g_engineFrees);
    check(g_engineAllocations == g_engineFrees, "loader and manager engine allocations balance");
    std::printf("light manager shadow map runtime: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
