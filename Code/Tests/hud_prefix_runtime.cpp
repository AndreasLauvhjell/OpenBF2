// [TEMP] [CHOSEN, not recovered] Isolated HUD prefix behavior fixture.
// Uses real HUD, lexicon and string bodies. No incomplete virtual is overridden
// or called. /FORCE permits the still-unwritten HUD entries to remain unresolved.
#include <cstdint>
#include <list>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <clocale>
#include <limits>
#define private public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/IO/Localization/ExcelLexicon.h"
#undef private

namespace dice { namespace hfe {
hud::BF2HudManager* hudManager = nullptr;
namespace io { IBase* lexicon = nullptr; }
} }

int main()
{
    using namespace dice::hfe;
    using namespace dice::hfe::hud;
    std::setlocale(LC_ALL, "C");
    unsigned checks = 0, failures = 0;
    auto check = [&](bool ok, const char* label) {
        ++checks;
        if (!ok) { ++failures; std::printf("FAIL: %s\n", label); }
    };
    void* storage = ::operator new(sizeof(BF2HudManager));
    std::memset(storage, 0x5a, sizeof(BF2HudManager));
    BF2HudManager* hud = new (storage) BF2HudManager;
    check(hud->refresh(), "retail refresh is always true");
    check(hud->getSystem() == nullptr, "root getter reads uninitialized lifecycle root");
    hud->setPaint(false);
    check(!hud->m_field248 && !hud->m_field249, "setPaint forwards to setUpdate");
    hud->setUpdate(true);
    check(!hud->m_field248 && hud->m_field249, "setUpdate does not change paint");
    hud->setPaint(true);
    check(hud->m_field248 && hud->m_field249, "paint enable updates both flags");
    check(hud->toWideString("ABC") == L"ABC", "narrow-to-wide ASCII");
    check(hud->toNarrowString(L"ABC") == "ABC", "wide-to-narrow ASCII");
    check(hud->toWideString("").empty() && hud->toNarrowString(L"").empty(), "empty conversions");
    check(hud->toWideString(std::string("a\0b", 3)) == L"a", "narrow conversion ends at embedded null");
    check(hud->toNarrowString(std::wstring(L"a\0b", 3)) == "a", "wide conversion ends at embedded null");
    check(hud->toNarrowString(std::wstring(1, static_cast<wchar_t>(0x100))).empty(), "unrepresentable wide input yields empty");
    check(std::setlocale(LC_CTYPE, ".UTF8") != nullptr, "UTF-8 locale available for invalid-sequence check");
    check(hud->toWideString(std::string(1, static_cast<char>(0xff))).empty(), "invalid UTF-8 input yields empty");
    std::setlocale(LC_CTYPE, "C");
    check(hud->replaceWideToken(L"#x# #x#", "#x#", L"wide") == L"wide #x#", "wide replacement changes first match only");
    check(hud->replaceWideToken(L"text", "missing", L"value") == L"text", "missing token leaves text");
    check(hud->replaceWideToken(L"text", "", L"value") == L"valuetext", "empty token inserts at beginning");
    check(hud->replaceWideTokenFromString(L"#x# #x#", "#x#", "narrow") == L"narrow #x#", "narrow replacement conversion");
    check(hud->replaceWideTokenFromInt(L"value=#n#", "#n#", -2147483647 - 1) == L"value=-2147483648", "signed integer formatting");
    check(hud->replaceStringTokenFromInt("#n#/#n#", "#n#", 42) == L"42/#n#", "narrow integer replacement forwards to narrow string arm");
    check(hud->replaceStringToken("#x# #x#", "#x#", "done") == L"done #x#", "all narrow values convert before replacing");

    io::ExcelLexicon lex;
    lex.m_strings.insert("KEY", L"translated");
    lex.m_strings.insert("EMPTY", L"");
    io::lexicon = &lex;
    hudManager = hud;
    check(hud->getLocalizedString("key") == L"translated", "real lexicon lookup remains case insensitive");
    check(hud->getLocalizedString("unknown") == L"unknown", "real lexicon miss returns key");
    check(hud->getLocalizedString("EMPTY") == L"EMPTY", "empty translation falls back to Core conversion");
    hudManager = nullptr;
    check(hud->getLocalizedString("key") == L"key", "absent HUD global selects fallback even after successful lookup");
    io::lexicon = nullptr;

    // Constructor-only teardown, as in hud_manager_runtime: real HUD destruction
    // still needs widget/filter/event lifetime recovery and is not exercised here.
    delete hud->m_field2dc;
    using WideString = std::wstring;
    using String = std::string;
    using WordList = std::list<int>;
    using StringList = std::list<std::string>;
    using WordVector = std::vector<std::uint32_t>;
    hud->m_field33c.~StringList();
    hud->m_field32c.~WordVector();
    hud->m_field304.~String();
    hud->m_field2e0.~StringList();
    hud->m_field0e8.~WordList();
    hud->m_field0c0.~String();
    hud->m_field08c.~String();
    hud->m_field060.~WideString();
    hud->m_field038.~WideString();
    hud->m_field010.~WideString();
    ::operator delete(storage);
    std::printf("HUD prefix: %u/%u checks passed; native x86 size %u\n", checks - failures, checks, unsigned(sizeof(BF2HudManager)));
    return failures ? 1 : 0;
}
