// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: CONSOLE-RUN-QUOTES-FIXTURE]
// Tests OldConsole run/include quote stripping using recording overrides for those two file operations.
#include <cstdio>
#include <string>
#include <vector>
#include "../BF2/IO/Console/Console.h"
using namespace dice::hfe::io;
static unsigned checks, failures;
static void check(bool value, const char* label)
{ ++checks; if (!value) { ++failures; std::printf("FAIL %s\n", label); } }

// [TEMP] the real console, with the two script-opening virtuals turned into recorders
struct Recorder : OldConsole {
    std::vector<std::string> calls;
    bool run(const std::string& file, const std::string& a1, const std::string& a2,
             const std::string& a3, const std::string& a4, const std::string& a5,
             const std::string& a6, const std::string& a7, const std::string& a8,
             std::string&) override
    {
        calls.push_back("run:" + file + "|" + a1 + "|" + a2 + "|" + a3 + "|" + a4 + "|" + a5 +
                        "|" + a6 + "|" + a7 + "|" + a8);
        return true;
    }
    bool include(const std::string& file, const std::string& a1, const std::string& a2,
                 const std::string& a3, const std::string& a4, const std::string& a5,
                 const std::string& a6, const std::string& a7, const std::string& a8,
                 std::string&) override
    {
        calls.push_back("include:" + file + "|" + a1 + "|" + a2 + "|" + a3 + "|" + a4 + "|" +
                        a5 + "|" + a6 + "|" + a7 + "|" + a8);
        return true;
    }
};

int main()
{
    static_assert(sizeof(void*) == 4, "native x86 only");
    Recorder console;
    const dice::hfe::AccessType access = static_cast<dice::hfe::AccessType>(7); // executeLine's full-access value
    std::string result;

    // 1. the bare spelling, which always worked (HudSetupMain.con line 3)
    result.clear();
    check(console.handleCommand("run GeneralHudSettings.con", result, access) == 1,
          "bare run keyword returns the silent status");
    check(console.calls == std::vector<std::string>{"run:GeneralHudSettings.con||||||||"},
          "bare run hands the file name through unchanged");

    // 2. the quoted spelling (HudSetupMain.con lines 5..16) - the quotes must be gone
    console.calls.clear(); result.clear();
    check(console.handleCommand("run \"Weapons/HudSetupWeapons.con\"", result, access) == 1,
          "quoted run keyword returns the silent status");
    check(console.calls == std::vector<std::string>{"run:Weapons/HudSetupWeapons.con||||||||"},
          "quoted run reaches run() with the quote characters stripped");

    // 3. tabs, a quoted multi-word argument and a bare argument, as getArgs tokenises them
    console.calls.clear(); result.clear();
    console.handleCommand("run\t\"SpawnInterface/HudSetupSpawnInterface.con\"  BF2Editor \"two words\"\t", result, access);
    check(console.calls == std::vector<std::string>{"run:SpawnInterface/HudSetupSpawnInterface.con|BF2Editor|two words||||||"},
          "run arguments are quote-grouped and tab-delimited");

    // 4. an unterminated quote is getArgs' own hard error: the line is refused, run() never runs
    console.calls.clear(); result.clear();
    check(console.handleCommand("run \"Broken.con", result, access) == 0 && result == "no closing '\"'!",
          "unterminated quote refuses the run line with getArgs' message");
    check(console.calls.empty(), "refused run line never reaches run()");

    // 5. more than nine tokens: the ninth is the eighth argument, the rest are ignored
    console.calls.clear(); result.clear();
    console.handleCommand("run f.con 1 2 3 4 5 6 7 8 9 10", result, access);
    check(console.calls == std::vector<std::string>{"run:f.con|1|2|3|4|5|6|7|8"},
          "run keeps the file and its eight arguments only");

    // 6. include takes the identical path
    console.calls.clear(); result.clear();
    check(console.handleCommand("include \"Common/Foo.tweak\" arg", result, access) == 1,
          "quoted include keyword returns the silent status");
    check(console.calls == std::vector<std::string>{"include:Common/Foo.tweak|arg|||||||"},
          "quoted include reaches include() with the quote characters stripped");

    // 7. the keyword is matched without case, as every keyword in the chain is
    console.calls.clear(); result.clear();
    console.handleCommand("RUN \"Basic/HudSetupBasic.con\"", result, access);
    check(console.calls == std::vector<std::string>{"run:Basic/HudSetupBasic.con||||||||"},
          "RUN is the same keyword");

    std::printf("%u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
