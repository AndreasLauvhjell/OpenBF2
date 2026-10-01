// [TEMP] [CHOSEN, not recovered] Bounded localization fixture. Only file discovery
// and open are replaced; File, MemStream, ExcelLexicon, Registry, settings and
// BF2Engine::initLocalization run their real bodies. This is not engine-init reach.
#include "../BF2/Game/Main/BF2Engine.h"
#include "../BF2/Game/Main/Game.h"
#include "../BF2/Core/System.h"
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/Core/Registry.h"
#include "../BF2/Core/StringUtil.h"
#include "../BF2/Core/Debug.h"
#include "../BF2/IO/File/File.h"
#include "../BF2/IO/File/FileManager.h"
#include "../BF2/IO/File/Win32FileSystem.h"
#include "../BF2/IO/Stream/MemStream.h"
#include "../BF2/IO/Localization/ExcelLexicon.h"
#include <cstdio>
#include <cstring>
#include <clocale>
#include <fstream>
#include <iterator>
#include <map>
#include <vector>

namespace dice { namespace hfe {
class ISettingsRepostitory { public: static void createSingleton(); };
} }

using namespace dice::hfe;
using namespace dice::hfe::io;

namespace {
unsigned checks = 0, failures = 0;
void check(bool condition, const char* name)
{
    ++checks;
    if (!condition) { ++failures; std::printf("FAIL: %s\n", name); }
}

std::string wideBytes(const std::wstring& text)
{
    return std::string(reinterpret_cast<const char*>(text.data()), text.size() * sizeof(wchar_t));
}
std::string table(const std::wstring& rows) { return std::string("\xff\xfe", 2) + wideBytes(rows); }
std::wstring row(const std::wstring& key, const std::wstring& value)
{
    return key + L"\x1b\x1b" + value + L"\x1b\x1b\r\n";
}

class FixtureFiles : public FileManager
{
public:
    std::map<std::string, std::string> contents;
    std::map<std::string, std::list<FileInfo>> listings;
    std::vector<std::string> searches, opens;
    bool argumentsMatch = true;
    IStream* open(const std::string& name, FileMode mode, FileAccess access, unsigned flags) override
    {
        opens.push_back(name);
        argumentsMatch &= mode == FILEMODE_READ && access == 0 && flags == 0;
        const auto found = contents.find(name);
        if (found == contents.end()) return 0;
        MemStream* stream = new MemStream;
        stream->setSize(static_cast<unsigned>(found->second.size()));
        if (!found->second.empty())
            std::memcpy(stream->getPtr(), found->second.data(), found->second.size());
        return static_cast<IMemStream*>(stream);
    }
    int findFiles(const std::string& directory, bool recursive, std::list<FileInfo>& out) override
    {
        searches.push_back(directory);
        argumentsMatch &= recursive;
        out = listings[directory];
        return -1; // Specific fixture: retail ignores the discovery return value.
    }
};

FileInfo entry(const std::string& name, bool directory = false)
{
    FileInfo result = {};
    result.name = name;
    result.isDirectory = directory;
    return result;
}
}

int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4 && sizeof(wchar_t) == 2, "Win32 client fixture");
    std::setlocale(LC_ALL, "C");
    // Exercise return paths after retail diagnostics using the real debug filter.
    // Default production Error diagnostics still abort; this is fixture-only.
    turnOffAllDebug(DebugTypeError);
    check(makeNarrow(L"key") == "key", "narrow ASCII conversion");
    check(makeNarrow(L"").empty(), "narrow empty conversion");
    check(makeNarrow(std::wstring(L"ab\0cd", 5)) == "ab", "narrow embedded NUL truncation");
    check(makeNarrow(L"\u4e00") == "no_conversion_exists", "C-locale conversion failure sentinel");
    check(registry->getString(RegKeyCurrentUser, "missing", "fallback",
                             "BF2RebuiltFixture_7e6c4c79_missing") == "fallback", "missing registry default");

    // FileManager teardown is still declaration-only. Keep this pre-init fixture
    // process-owned, like the Engine/System below; do not supply a fake destructor.
    FixtureFiles& files = *new FixtureFiles;
    fileManager = &files;
    files.contents["tokens"] = wideBytes(std::wstring(L"ab\0cd", 5) + L"|tail");
    {
        File file;
        check(file.open("tokens", FILEMODE_READ, static_cast<FileAccess>(0), 0), "token file open");
        std::wstring token;
        check(file.read(token, L'|') && token == L"ab" && file.getPosition() == 12,
              "wide reader consumes delimiter past embedded NUL");
        check(file.read(token, L'|') && token == L"tail", "wide reader accepts missing delimiter at EOF");
        check(file.read(token, L'|') && token.empty(), "wide reader returns true at empty EOF");
    }
    files.contents["bom"] = table(L"");
    files.contents["empty"] = "";
    files.contents["badbom"] = "XX";
    ExcelLexicon loaded;
    check(!loaded.load("missing"), "missing lexicon rejected");
    check(loaded.load("bom"), "BOM-only lexicon accepted");
    check(!loaded.load("empty"), "empty lexicon diagnostic and false with errors filtered");
    check(!loaded.load("badbom"), "bad fingerprint diagnostic and false with errors filtered");
    files.contents["rows"] = table(row(L"first padded", L"First") + row(L"FIRST", L"Second")
        + row(L"empty", L"") + row(L"wide", L"\u00e9\u4e00") + row(L"last", L"Last"));
    check(loaded.load("rows"), "multi-row table loads");
    check(loaded.getString("fIrSt") == L"First", "space trimming, case folding and first definition wins");
    check(loaded.getString("empty") == L"empty", "empty value omitted");
    check(loaded.getString("wide") == L"\u00e9\u4e00", "wide values retained without narrowing");
    check(loaded.getString("last") == L"Last", "whitespace scan preserves next key");
    files.contents["fallback"] = table(row(L"first", L"Stock") + row(L"fallback", L"Stock only"));
    check(loaded.load("fallback") && loaded.getString("first") == L"First"
          && loaded.getString("fallback") == L"Stock only", "cross-file first definition wins");
    files.contents["failure"] = table(row(L"before_failure", L"kept") + row(L"no_conversion_exists", L"bad"));
    check(!loaded.load("failure") && loaded.getString("before_failure") == L"kept", "failed import retains earlier rows");
    files.contents["truncated"] = table(L"truncated\x1b");
    check(!loaded.load("truncated"), "failed post-key seek rejected");
    files.contents["lastunit"] = table(row(L"tail", L"ok") + L"X");
    check(loaded.load("lastunit"), "retail whitespace scan consumes final nonspace at EOF");

    g_system = new System;
    BF2Engine* engine = new BF2Engine(0);
    ISettingsRepostitory::createSingleton();
    engine->parseParameters("+multi 1 +noSound 1 +fullscreen 0 +modPath mods/bf2");
    lexicon = 0;
    check(!engine->initLocalization(), "client initLocalization rejects missing lexicon");
    ExcelLexicon engineLexicon;
    lexicon = &engineLexicon;
    const std::string language = registry->getString(RegKeyLocalMachine, "Language", "English", "");
    const std::string mod = "localization/" + language + "/";
    const std::string stock = "../../mods/bf2/localization/" + language + "/";
    files.contents[mod + "mod.data"] = table(row(L"priority", L"mod"));
    files.contents[stock + "stock.utxt"] = table(row(L"priority", L"stock") + row(L"stock_only", L"yes"));
    files.listings[mod] = {entry(""), entry("nested", true), entry("mod.data"), entry("missing.utxt")};
    files.listings[stock] = {entry("stock.utxt")};
    files.searches.clear(); files.opens.clear();
    settings->stringSet("GSModDirectory", "mods/example");
    check(engine->initLocalization(), "client localization initializes despite missing file/find result");
    check(settings->stringGet("GSLanguage", "") == language, "language setting comes from registry/default");
    check(files.searches == std::vector<std::string>({mod, stock}), "mod directory precedes stock fallback");
    check(files.opens == std::vector<std::string>({mod + "mod.data", mod + "missing.utxt", stock + "stock.utxt"}),
          "empty/directory entries skipped; all file extensions loaded");
    check(engineLexicon.getString("priority") == L"mod" && engineLexicon.getString("stock_only") == L"yes",
          "initLocalization loads real lexicon with fallback precedence");
    files.searches.clear();
    settings->stringSet("GSModDirectory", "mods/bf2");
    check(engine->initLocalization() && files.searches == std::vector<std::string>({mod}), "stock mod has no redundant fallback");
    files.searches.clear();
    settings->stringSet("GSModDirectory", "mods/BF2");
    check(engine->initLocalization() && files.searches.size() == 2, "mod comparison is case sensitive");
    files.searches.clear(); files.opens.clear();
    engine->parseParameters("+dedicated 1"); lexicon = 0;
    settings->stringSet("GSLanguage", "unchanged");
    check(engine->initLocalization() && files.searches.empty() && files.opens.empty()
          && settings->stringGet("GSLanguage", "") == "unchanged", "runtime dedicated branch bypasses localization");
    check(files.argumentsMatch, "retail recursive/read/access/flags arguments");

    if (argc > 1)
    {
        std::ifstream input(argv[1], std::ios::binary);
        check(input.good(), "stock English.utxt opened read-only");
        files.contents["retail"] = std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        ExcelLexicon retail;
        check(retail.load("retail"), "stock English.utxt parsed");
        check(retail.getString("id_language") == L"English", "stock language key");
        check(retail.getString("HUD_INGAME_QUIT") == L"Quit", "stock HUD quit label");
    }
    if (argc > 2)
    {
        // Separate filesystem integration pass: no open/findFiles overrides. The
        // current Win32 backend still omits retail buffering; see its source.
        FileManager* disk = new FileManager;
        Win32FileSystem* filesystem = new Win32FileSystem;
        disk->setBaseSystem(filesystem);
        filesystem->release();
        disk->addPath(argv[2]);
        fileManager = disk;
        ExcelLexicon* diskLexicon = new ExcelLexicon;
        lexicon = diskLexicon;
        engine->parseParameters("+multi 1 +noSound 1 +fullscreen 0 +modPath mods/bf2");
        check(engine->initLocalization(), "real filesystem client localization initializes");
        check(diskLexicon->getString("ID_LANGUAGE") == makeWide(language), "real directory enumeration loads language");
        check(diskLexicon->getString("HUD_INGAME_QUIT") != L"HUD_INGAME_QUIT", "real filesystem loads HUD labels");
        lexicon = 0;
        delete diskLexicon;
        // FileManager destruction is still unresolved; disk remains process-owned.
    }
    fileManager = 0;
    // Engine/System pre-init objects have no valid full shutdown yet; process exit owns them.
    std::printf("Client localization: %u/%u checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
