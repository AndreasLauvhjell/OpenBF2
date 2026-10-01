// [TEMP] [CHOSEN, not recovered] Device-free atlas packing and .tai writer checks.
// Uses a table-backed configuration and the base 2048x2048 atlas, checking placement,
// overlap, page sizing, and miplevels. This verifies invariants, not retail byte parity.
// Usage: selftest.exe <out dir>; zero exit means all checks passed.
#include "../../BF2/RendDX9/TextureAtlas/TextureAtlasBuilder/TextureAtlas.h"
#include "../../BF2/RendDX9/TextureAtlas/TextureAtlasBuilder/TextureAtlasConfiguration.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace textureatlas;

namespace {

int g_failures = 0;
int g_checks = 0;
std::vector<std::string> g_messages;

void check(bool ok, const char* what)
{
    ++g_checks;
    if (!ok)
    {
        ++g_failures;
        std::printf("  FAIL %s\n", what);
    }
}

void __fastcall collect(const std::string& message)
{
    g_messages.push_back(message);
}

struct Spec { int w, h; const char* format; int miplevels; };

// The configuration leaf of the test: image facts from a table, not from D3DX.
class TableConfiguration : public TextureAtlasConfiguration
{
public:
    std::map<std::string, Spec> table;
    virtual bool getTextureInfo(const std::string& inputDirectory, const std::string& name, TextureInfo& info)
    {
        info.path = inputDirectory + name;
        info.name = name;
        std::map<std::string, Spec>::const_iterator it = table.find(name);
        if (it == table.end())
            return false;
        info.rect.w = it->second.w;
        info.rect.h = it->second.h;
        info.format = it->second.format;
        info.miplevels = it->second.miplevels;
        return true;
    }
    int groupMiplevels(const char* name)
    {
        for (GroupIter g = groupsBegin(); g != groupsEnd(); ++g)
            if (g->name == name)
                return g->miplevels;
        return -999;
    }
};

// The base atlas, with its pages visible to the checks.
class InspectableAtlas : public TextureAtlas
{
public:
    const std::vector<Group>& pages() const { return m_pages; }
};

bool isPow2(int v) { return v > 0 && (v & (v - 1)) == 0; }

// The packing invariants for one atlas: every page a power of two within 2048, every texture inside
// its page, no two textures of a page overlapping, the page names <group><index> in order.
void checkPages(const InspectableAtlas& atlas, size_t expectedTextures, const char* label)
{
    char what[256];
    size_t placed = 0;
    for (size_t p = 0; p < atlas.pages().size(); ++p)
    {
        const Group& page = atlas.pages()[p];
        std::snprintf(what, sizeof(what), "%s: page %s is %dx%d, a power of two <= 2048", label,
                      page.name.c_str(), page.width, page.height);
        check(isPow2(page.width) && isPow2(page.height) && page.width <= 2048 && page.height <= 2048, what);
        for (size_t i = 0; i < page.textures.size(); ++i)
        {
            const Rect& a = page.textures[i].rect;
            std::snprintf(what, sizeof(what), "%s: %s inside its page", label, page.textures[i].name.c_str());
            check(a.x >= 0 && a.y >= 0 && a.x + a.w <= page.width && a.y + a.h <= page.height, what);
            for (size_t j = i + 1; j < page.textures.size(); ++j)
            {
                const Rect& b = page.textures[j].rect;
                const bool overlap = a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
                std::snprintf(what, sizeof(what), "%s: %s and %s do not overlap", label,
                              page.textures[i].name.c_str(), page.textures[j].name.c_str());
                check(!overlap, what);
            }
        }
        placed += page.textures.size();
    }
    std::snprintf(what, sizeof(what), "%s: %u textures placed (expected %u)", label,
                  static_cast<unsigned>(placed), static_cast<unsigned>(expectedTextures));
    check(placed == expectedTextures, what);
}

std::string readFile(const std::string& path)
{
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f)
        return std::string();
    std::string data;
    char buffer[4096];
    size_t n;
    while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0)
        data.append(buffer, n);
    std::fclose(f);
    return data;
}

void addAll(TableConfiguration& config, const char* prefix, int count, Spec spec)
{
    char name[64];
    for (int i = 0; i < count; ++i)
    {
        std::snprintf(name, sizeof(name), "%s%02d.dds", prefix, i);
        config.table[name] = spec;
        config.addTexture(name, false);
    }
}

} // anonymous namespace

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::printf("usage: selftest.exe <out dir>\n");
        return 2;
    }
    const std::string out = std::string(argv[1]) + "\\";

    // 1. 48 equal 64x64 textures (the tie case). The split rule fills the top row first (32 across),
    //    then the 33rd splits the remaining 2048x1984 with dw > dh, so the rest go down the left
    //    column: the 48th lands at y = 1024 and the page rounds to 2048x2048.
    {
        TableConfiguration config;
        config.setWarningCallback(collect);
        config.setErrorCallback(collect);
        config.setOutputPath(out.c_str());
        config.addGroup("Ties", 0, 0, "", 0, 1);
        const Spec s = { 64, 64, "A8R8G8B8", 1 };
        addAll(config, "ties/t", 48, s);
        InspectableAtlas atlas;
        atlas.setWarningCallback(collect);
        g_messages.clear();
        atlas.build(&config);
        checkPages(atlas, 48, "ties");
        for (size_t p = 0; p < atlas.pages().size(); ++p)
            std::printf("  ties: page %s %dx%d, %u textures\n", atlas.pages()[p].name.c_str(),
                        atlas.pages()[p].width, atlas.pages()[p].height,
                        static_cast<unsigned>(atlas.pages()[p].textures.size()));
        check(atlas.pages().size() == 1 && atlas.pages()[0].width == 2048 && atlas.pages()[0].height == 2048,
              "ties: one 2048x2048 page");
        check(g_messages.empty(), "ties: no warning");
        check(atlas.save((out + "Ties.tai").c_str()), "ties: save() true");
        const std::string tai = readFile(out + "Ties.tai");
        check(tai.find("# " + out + "Ties.tai\r\n# \r\n# \r\n") == 0, "ties: .tai header line 1-3, CRLF");
        check(tai.find("ties/t00.dds\t\tTies0.dds, 0, ") != std::string::npos, "ties: a .tai line for t00");
        check(config.groupMiplevels("Ties") == 1, "ties: the configuration's miplevels rewritten to 1");
    }

    // 2. Six 1024x1024 sources: two 2048x2048 pages (4 + 2), indices 0 and 1.
    {
        TableConfiguration config;
        config.setOutputPath(out.c_str());
        config.addGroup("Pages", 0, 0, "DXT1", 0, 1);
        const Spec s = { 1024, 1024, "DXT1", 11 };
        addAll(config, "pages/big", 6, s);
        InspectableAtlas atlas;
        atlas.build(&config);
        checkPages(atlas, 6, "pages");
        check(atlas.pages().size() == 2, "pages: two pages");
        if (atlas.pages().size() == 2)
        {
            check(atlas.pages()[0].name == "Pages0" && atlas.pages()[0].index == 0, "pages: Pages0 index 0");
            check(atlas.pages()[1].name == "Pages1" && atlas.pages()[1].index == 1, "pages: Pages1 index 1");
            check(atlas.pages()[0].width == 2048 && atlas.pages()[0].height == 2048, "pages: the first page is full");
            check(atlas.pages()[1].width == 2048 && atlas.pages()[1].height == 1024, "pages: the second page rounds to 2048x1024");
            check(atlas.pages()[0].miplevels == 11, "pages: page miplevels = the group's maximum (11)");
        }
        atlas.save((out + "Pages.tai").c_str());
    }

    // 3. Split mode (format ""): three formats, one page per format run, descending format names.
    {
        TableConfiguration config;
        config.setOutputPath(out.c_str());
        config.addGroup("Mixed", 0, 0, "", 0, 1);
        const Spec a = { 64, 64, "A8R8G8B8", 1 };
        const Spec d1 = { 64, 64, "DXT1", 1 };
        const Spec d5 = { 64, 64, "DXT5", 1 };
        addAll(config, "mixed/a", 3, a);
        addAll(config, "mixed/d", 3, d1);
        addAll(config, "mixed/e", 3, d5);
        InspectableAtlas atlas;
        atlas.build(&config);
        checkPages(atlas, 9, "mixed");
        check(atlas.pages().size() == 3, "mixed: three pages");
        if (atlas.pages().size() == 3)
        {
            check(atlas.pages()[0].format == "DXT5" && atlas.pages()[1].format == "DXT1" &&
                  atlas.pages()[2].format == "A8R8G8B8", "mixed: pages DXT5, DXT1, A8R8G8B8 (descending)");
        }
        atlas.save((out + "Mixed.tai").c_str());
    }

    // 4. Too large and the group limits: a 4096 square is dropped with the warning; a 300x40 group
    //    limit packs as 300 wide (not a power of two until the page rounds).
    {
        TableConfiguration config;
        config.setOutputPath(out.c_str());
        config.addGroup("Limits", 300, 40, "", 0, 1);
        const Spec big = { 4096, 4096, "DXT1", 1 };
        const Spec small = { 100, 40, "A8R8G8B8", 1 };
        config.table["huge.dds"] = big;
        config.addTexture("huge.dds", false);
        addAll(config, "small/s", 4, small);
        InspectableAtlas atlas;
        atlas.setWarningCallback(collect);
        g_messages.clear();
        atlas.build(&config);
        check(g_messages.size() == 1 &&
              g_messages[0] == "Texture size larger than maximum atlas size: huge.dds", "limits: one warning for huge.dds");
        checkPages(atlas, 4, "limits");
        check(atlas.pages().size() == 2, "limits: 3 then 1 sources on 300x40 pages");
        if (atlas.pages().size() == 2)
            check(atlas.pages()[0].width == 512 && atlas.pages()[0].height == 64, "limits: page 0 rounds 300x40 to 512x64");
        atlas.save((out + "Limits.tai").c_str());
    }

    // 5. Borders: a bordered texture grows by 2 x border in the configuration, and the .tai insets it.
    {
        TableConfiguration config;
        config.setOutputPath(out.c_str());
        config.addGroup("Border", 2048, 2048, "", 1, 2);
        const Spec s = { 60, 60, "A8R8G8B8", 1 };
        config.table["b.dds"] = s;
        config.table["n.dds"] = s;
        config.addTexture("b.dds", true);
        config.addTexture("n.dds", false);
        InspectableAtlas atlas;
        atlas.build(&config);
        checkPages(atlas, 2, "border");
        atlas.save((out + "Border.tai").c_str());
        const std::string tai = readFile(out + "Border.tai");
        // b.dds is 64x64 after the border (60 + 2 x 2), placed first (larger): page 128x64.
        check(tai.find("b.dds\t\tBorder0.dds, 0, 0.015625, 0.03125, 0.46875, 0.9375\r\n") != std::string::npos,
              "border: inset .tai line for b.dds (2/128, 2/64, 60/128, 60/64)");
    }

    // 6. Sort mode 1 (non-strict comparator) on 40 equal textures: no crash, everything placed.
    {
        TableConfiguration config;
        config.setOutputPath(out.c_str());
        config.addGroup("Size", 0, 0, "", 0, 1);
        const Spec s = { 32, 32, "A8R8G8B8", 1 };
        addAll(config, "size/s", 40, s);
        InspectableAtlas atlas;
        atlas.setSortMode(1);
        atlas.build(&config);
        checkPages(atlas, 40, "sortmode1");
    }

    std::printf("%d/%d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures ? 1 : 0;
}
