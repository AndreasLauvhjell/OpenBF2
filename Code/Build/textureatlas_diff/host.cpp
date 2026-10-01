// [TEMP] [CHOSEN, not recovered] TextureAtlasBuilder differential host.
// Loads one DLL by full path and compares its callbacks, results, and output files
// against another run of the same scenario. See Code\Build\textureatlas_diff.cmd.
// Usage: host.exe <dll> <game dir|-> <inputs dir> <out dir> <scenario|list> <strings: vc71|native> <device: hal|none>
// The game directory supplies dependent DLLs; - leaves the search path unchanged.
// Choose vc71 strings for the retail DLL and native strings for the current compiler.
// hal requires a desktop GPU; none uses zero-sized pages and tests parsing and header output only.
// Inputs come from make_inputs.py; relative source paths resolve from the input directory.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

#include "../../BF2/RendDX9/TextureAtlas/TextureAtlasBuilder/ITextureAtlas.h"

using textureatlas::ITextureAtlas;
using textureatlas::ITextureAtlasConfiguration;

namespace {

typedef ITextureAtlasConfiguration* (__fastcall* CreateConfigurationFn)();
typedef ITextureAtlas* (__fastcall* CreateAtlasFn)(IDirect3DDevice9* device);

CreateConfigurationFn g_createConfiguration = 0;
CreateAtlasFn         g_createAtlas = 0;
IDirect3DDevice9*     g_device = 0;
bool                  g_vc71Strings = false;
std::string           g_outDir;

// the callbacks: one pair per string layout
std::string readVc71String(const void* object)
{
    const unsigned char* const raw = static_cast<const unsigned char*>(object);
    unsigned int size = 0, res = 0;
    std::memcpy(&size, raw + 20, 4);
    std::memcpy(&res, raw + 24, 4);
    const char* chars = 0;
    if (res < 16)
        chars = reinterpret_cast<const char*>(raw + 4);
    else
        std::memcpy(&chars, raw + 4, 4);
    return std::string(chars, size);
}

void report(const char* kind, const std::string& text)
{
    std::printf("  %s: %s\n", kind, text.c_str());
}

void __fastcall onErrorNative(const std::string& message)   { report("ERROR", message); }
void __fastcall onWarningNative(const std::string& message) { report("WARNING", message); }
void __fastcall onErrorVc71(const std::string& message)     { report("ERROR", readVc71String(&message)); }
void __fastcall onWarningVc71(const std::string& message)   { report("WARNING", readVc71String(&message)); }

textureatlas::MessageFn errorSink()   { return g_vc71Strings ? onErrorVc71 : onErrorNative; }
textureatlas::MessageFn warningSink() { return g_vc71Strings ? onWarningVc71 : onWarningNative; }

// the device
LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    return DefWindowProcA(hwnd, msg, wp, lp);
}

bool createDevice()
{
    WNDCLASSA wc;
    std::memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = windowProc;
    wc.hInstance = GetModuleHandleA(0);
    wc.lpszClassName = "TextureAtlasDiffHost";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("TextureAtlasDiffHost", "TextureAtlasDiffHost", WS_OVERLAPPEDWINDOW,
                              0, 0, 64, 64, 0, 0, wc.hInstance, 0);       // never shown
    if (!hwnd)
        return false;
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
        return false;
    D3DPRESENT_PARAMETERS pp;
    std::memset(&pp, 0, sizeof(pp));
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_UNKNOWN;
    pp.BackBufferWidth = 64;
    pp.BackBufferHeight = 64;
    pp.hDeviceWindow = hwnd;
    // No D3DCREATE_FPU_PRESERVE: like the tool and the game, the device leaves the x87 unit in single
    // precision - identical for both DLLs, which share this process and its d3dx9_25.dll.
    const HRESULT hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                         D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &g_device);
    if (FAILED(hr))
    {
        std::printf("  [host] CreateDevice failed 0x%08lX\n", static_cast<unsigned long>(hr));
        return false;
    }
    return true;
}

// helpers
std::string outPath(const char* scenario)
{
    std::string dir = g_outDir + "\\" + scenario;
    CreateDirectoryA(dir.c_str(), 0);
    return dir + "\\";
}

ITextureAtlasConfiguration* newConfiguration()
{
    ITextureAtlasConfiguration* const configuration = g_createConfiguration();
    configuration->setErrorCallback(errorSink());
    configuration->setWarningCallback(warningSink());
    return configuration;
}

ITextureAtlas* newAtlas()
{
    ITextureAtlas* const atlas = g_createAtlas(g_device);
    atlas->setErrorCallback(errorSink());
    atlas->setWarningCallback(warningSink());
    return atlas;
}

void addTexture(ITextureAtlasConfiguration* configuration, const char* name, bool border)
{
    const bool added = configuration->addTexture(name, border) != 0;
    std::printf("  addTexture(%s, %d) -> %s\n", name, border ? 1 : 0, added ? "record" : "null");
}

// RendDX9's recipe (0x10015EA0 steps 6-8): one group (name, 0, 0, format, 0, 1), the names added
// with border = false in stricmp map order (the caller sorts), build, save <out><name>.tai.
void renddx9Recipe(const char* scenario, const char* inputDir, const char* name, const char* format,
                   std::vector<std::string> names, int sortMode)
{
    std::sort(names.begin(), names.end(),
              [](const std::string& a, const std::string& b) { return _stricmp(a.c_str(), b.c_str()) < 0; });
    const std::string out = outPath(scenario);
    ITextureAtlasConfiguration* const configuration = newConfiguration();
    configuration->setInputPath(inputDir);
    configuration->setOutputPath(out.c_str());
    configuration->addGroup(name, 0, 0, format, 0, 1);
    for (size_t i = 0; i < names.size(); ++i)
        addTexture(configuration, names[i].c_str(), false);
    std::printf("  inputPath \"%s\", outputPath \"%s\"\n", configuration->getInputPath(), configuration->getOutputPath());

    ITextureAtlas* const atlas = newAtlas();
    if (sortMode >= 0)
        atlas->setSortMode(sortMode);
    atlas->build(configuration);
    const std::string tai = out + name + ".tai";
    const bool saved = atlas->save(tai.c_str());
    std::printf("  save(%s) -> %d\n", tai.c_str(), saved ? 1 : 0);
    delete atlas;
    delete configuration;
}

std::vector<std::string> numbered(const char* prefix, int count, const char* suffix)
{
    std::vector<std::string> names;
    char buffer[64];
    for (int i = 0; i < count; ++i)
    {
        std::sprintf(buffer, "%s%02d%s", prefix, i, suffix);
        names.push_back(buffer);
    }
    return names;
}

// the scenarios
void s1_overgrowth()
{
    // Overgrowth.generateAtlas: outDir = inDir, "<level>OvergrowthAtlas", DXT5.
    std::vector<std::string> names;
    names.push_back("og/Bush_A.dds");
    names.push_back("og/bush_b.dds");
    names.push_back("og/Tree_Pine.dds");
    names.push_back("og/tree_leaves.dds");
    names.push_back("og/Stump.dds");
    names.push_back("og/grass_rgba.dds");
    renddx9Recipe("S1_overgrowth", "", "OvergrowthAtlas", "DXT5", names, -1);
}

void s2_undergrowth()
{
    // The undergrowth rebuild: input directory "", CWD-relative names, DXT5.
    renddx9Recipe("S2_undergrowth", "", "UndergrowthAtlas", "DXT5", numbered("ug/grass", 5, ".dds"), -1);
}

void s3_decal()
{
    // DecalManager.generateTexture: format "" - one page run per source format.
    std::vector<std::string> names = numbered("decal/hole", 6, ".dds");
    names.push_back("decal/scorch_big.dds");
    renddx9Recipe("S3_decal", "decal_dir", "DecalAtlas", "", names, -1);
}

void s4_particle()
{
    renddx9Recipe("S4_particle", "", "ParticleAtlas", "", numbered("particle/fx", 9, ".dds"), -1);
}

void s5_lightmap()
{
    // loadAllLightMapFilesDLL: format "", sets of equal-size lightmaps (the tie case).
    renddx9Recipe("S5_lightmap", "lightmaps", "LightmapAtlas", "", numbered("lm", 20, ".dds"), -1);
}

void s6_ties()
{
    // 48 equal-area textures: more than _ISORT_MAX (32) and the ninther threshold (40), so the VC7.1
    // partition, median-of-nine and the insertion sort all decide the placement.
    renddx9Recipe("S6_ties", "", "TieAtlas", "", numbered("ties/t", 48, ".dds"), -1);
}

void s7_multipage()
{
    renddx9Recipe("S7_multipage", "", "PageAtlas", "", numbered("pages/big", 6, ".dds"), -1);
}

void s8_mixed()
{
    renddx9Recipe("S8_mixed", "", "MixedAtlas", "", numbered("mixed/m", 9, ".dds"), -1);
}

void s9_tac()
{
    // A MemeAtlas-shaped .tac: load, save it back, build and save the .tai.
    const std::string out = outPath("S9_tac");
    ITextureAtlasConfiguration* const configuration = newConfiguration();
    configuration->setInputPath("");
    configuration->setOutputPath(out.c_str());
    const bool loaded = configuration->load("tac/meme.tac");
    std::printf("  load(tac/meme.tac) -> %d\n", loaded ? 1 : 0);
    const std::string tac = out + "meme_roundtrip.tac";
    configuration->save(tac.c_str());
    std::printf("  save(%s)\n", tac.c_str());
    std::printf("  selectGroup(MemeAtlas) -> %s\n", configuration->selectGroup("MemeAtlas") ? "group" : "null");
    std::printf("  selectGroup(nothing) -> %s\n", configuration->selectGroup("nothing") ? "group" : "null");

    ITextureAtlas* const atlas = newAtlas();
    atlas->build(configuration);
    const std::string tai = out + "MemeAtlas.tai";
    const bool saved = atlas->save(tai.c_str());
    std::printf("  save(%s) -> %d\n", tai.c_str(), saved ? 1 : 0);
    // build() rewrote the group's miplevels when they were <= 0 - visible in a second .tac.
    const std::string tac2 = out + "meme_after_build.tac";
    configuration->save(tac2.c_str());
    delete atlas;
    delete configuration;
}

void s10_sortmode1()
{
    renddx9Recipe("S10_sortmode1", "", "SizeAtlas", "", numbered("sizes/s", 12, ".dds"), 1);
}

void s11_errors()
{
    const std::string out = outPath("S11_errors");
    {
        ITextureAtlasConfiguration* const configuration = newConfiguration();
        std::printf("  load(missing.tac) -> %d\n", configuration->load("tac/missing.tac") ? 1 : 0);
        std::printf("  load(flags.tac) -> %d\n", configuration->load("tac/flags.tac") ? 1 : 0);
        const std::string tac = out + "flags_roundtrip.tac";
        configuration->save(tac.c_str());
        delete configuration;
    }
    {
        ITextureAtlasConfiguration* const configuration = newConfiguration();
        std::printf("  load(noname.tac) -> %d\n", configuration->load("tac/noname.tac") ? 1 : 0);
        delete configuration;
    }
    {
        ITextureAtlasConfiguration* const configuration = newConfiguration();
        configuration->setOutputPath(out.c_str());
        configuration->addGroup("Errors", 0, 0, "", 0, 1);
        addTexture(configuration, "errors/missing_file.dds", false);
        addTexture(configuration, "errors/huge.dds", false);
        addTexture(configuration, "errors/small.dds", true);
        ITextureAtlas* const atlas = newAtlas();
        atlas->build(configuration);
        const std::string tai = out + "Errors.tai";
        std::printf("  save(%s) -> %d\n", tai.c_str(), atlas->save(tai.c_str()) ? 1 : 0);
        delete atlas;
        delete configuration;
    }
}

struct Scenario { const char* name; void (*run)(); };
const Scenario kScenarios[] =
{
    { "S1",  s1_overgrowth }, { "S2", s2_undergrowth }, { "S3", s3_decal }, { "S4", s4_particle },
    { "S5",  s5_lightmap },   { "S6", s6_ties },        { "S7", s7_multipage }, { "S8", s8_mixed },
    { "S9",  s9_tac },        { "S10", s10_sortmode1 }, { "S11", s11_errors },
};

} // anonymous namespace

int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "list") == 0)
    {
        for (size_t i = 0; i < sizeof(kScenarios) / sizeof(kScenarios[0]); ++i)
            std::printf("%s\n", kScenarios[i].name);
        return 0;
    }
    if (argc != 8)
    {
        std::fprintf(stderr, "usage: host.exe <dll> <game dir|-> <inputs dir> <out dir> <scenario> <vc71|native> <hal|none>\n");
        return 2;
    }
    const char* const dllPath = argv[1];
    const char* const gameDir = argv[2];
    const char* const inputsDir = argv[3];
    g_outDir = argv[4];
    const char* const scenario = argv[5];
    g_vc71Strings = std::strcmp(argv[6], "vc71") == 0;
    const bool wantDevice = std::strcmp(argv[7], "hal") == 0;

    if (std::strcmp(gameDir, "-") != 0)
        SetDllDirectoryA(gameDir);
    HMODULE dll = LoadLibraryA(dllPath);
    if (!dll)
    {
        std::printf("[host] LoadLibrary(%s) failed, GetLastError %lu\n", dllPath, GetLastError());
        return 3;
    }
    g_createConfiguration = reinterpret_cast<CreateConfigurationFn>(
        GetProcAddress(dll, "?create@ITextureAtlasConfiguration@textureatlas@@SIPAV12@XZ"));
    g_createAtlas = reinterpret_cast<CreateAtlasFn>(
        GetProcAddress(dll, "?create@ITextureAtlas@textureatlas@@SIPAV12@PAUIDirect3DDevice9@@@Z"));
    if (!g_createConfiguration || !g_createAtlas)
    {
        std::printf("[host] the two creates are not exported by %s\n", dllPath);
        return 3;
    }
    if (!SetCurrentDirectoryA(inputsDir))
    {
        std::printf("[host] SetCurrentDirectory(%s) failed\n", inputsDir);
        return 3;
    }
    CreateDirectoryA(g_outDir.c_str(), 0);
    if (wantDevice && !createDevice())
    {
        std::printf("[host] no Direct3D 9 HAL device - run this in a desktop session with a GPU, or use 'none'\n");
        return 4;
    }
    std::printf("[host] device: %s, strings: %s\n", g_device ? "HAL" : "none (0 x 0 pages)", g_vc71Strings ? "vc71" : "native");

    for (size_t i = 0; i < sizeof(kScenarios) / sizeof(kScenarios[0]); ++i)
    {
        if (std::strcmp(scenario, kScenarios[i].name) == 0)
        {
            std::printf("== %s\n", kScenarios[i].name);
            kScenarios[i].run();
            std::fflush(stdout);
            if (g_device)
                g_device->Release();
            return 0;
        }
    }
    std::printf("[host] unknown scenario %s\n", scenario);
    return 2;
}
