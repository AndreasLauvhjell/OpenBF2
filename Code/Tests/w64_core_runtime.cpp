// [TEMP] [CHOSEN, not recovered] Actual Console.cpp/System.cpp writer/name regressions.
// File and systemInfo collaborators are bounded test doubles, never linked into production.
#include "IO/Console/ConsoleCompiler.h"
#include "IO/File/File.h"
#include "Core/SystemInfo.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
int checks = 0, failures = 0, closes = 0, destroys = 0;
int modeSeen = -1, accessSeen = -1;
unsigned flagsSeen = 1;
bool shortWrite = false;
std::string openedName;
std::string stamp;
std::vector<unsigned char> bytes;
std::vector<unsigned> writes;
void check(bool ok, const char* name)
{
    ++checks;
    if (!ok) { ++failures; std::printf("FAIL %s\n", name); }
}
void record(char tag, const std::string& value, bool newline,
            std::vector<unsigned char>& out)
{
    out.push_back(static_cast<unsigned char>(tag));
    const unsigned length = static_cast<unsigned>(value.size()) + (newline ? 1u : 0u);
    for (unsigned i = 0; i != 4; ++i) out.push_back(static_cast<unsigned char>(length >> (8 * i)));
    out.insert(out.end(), value.begin(), value.end());
    if (newline) out.push_back('\n');
}
}

namespace dice { namespace hfe {
const wchar_t* crashDumpTargetName(const wchar_t*);
ISystemInfo::ISystemInfo() {}
ISystemInfo::~ISystemInfo() {}
class FixtureSystemInfo : public ISystemInfo {
public:
    const std::string& getComputerName() const override { static std::string unused; return unused; }
    std::string getDateTimeString() const override { return stamp; }
    unsigned getNumberOfPageFaults() const override { return 0; }
    unsigned getMemoryUsage() const override { return 0; }
    void getMemoryInfo(unsigned& a, unsigned& b) const override { a = b = 0; }
    bool getIsDebuggerInstalled() const override { return false; }
};
ISystemInfo* systemInfo() { static FixtureSystemInfo info; return &info; }

namespace io {
File::File(const std::string& path, IFileSystem::FileMode mode,
           IFileSystem::FileAccess access, unsigned flags) : m_file(0)
{
    openedName = path; modeSeen = mode; accessSeen = access; flagsSeen = flags;
}
File::~File() { ++destroys; close(); }
void File::close() { ++closes; }
unsigned File::write(const void* data, unsigned count)
{
    writes.push_back(count);
    const unsigned char* begin = static_cast<const unsigned char*>(data);
    bytes.insert(bytes.end(), begin, begin + count);
    return shortWrite ? 0u : count;
}
bool File::write(int value) { return write(&value, sizeof(value)) == sizeof(value); }
} } }

int main(int argc, char** argv)
{
    using namespace dice::hfe;
    if (argc > 1 && std::strcmp(argv[1], "no-extension") == 0)
    {
        stamp = "stamp";
        check(std::wstring(crashDumpTargetName(L"base")) == L"base_stamp.base",
              "no-extension wrap keeps retail npos+1 behavior");
    }
    else
    {
        using namespace dice::hfe::io;
        static_assert(sizeof(ConsoleCompiler) == sizeof(void*), "native owned File pointer");
        static_assert(sizeof(int) == 4, "transcript length is 32-bit");
        std::vector<unsigned char> expected;
        {
            ConsoleCompiler compiler("capture.con", true);
            check(openedName == "capture.con" && modeSeen == 0 && accessSeen == 2 && flagsSeen == 0,
                  "writer constructor preserves file mode/access/flags");
            compiler.setWorkingDir("mods/bf2/"); record('W', "mods/bf2/", false, expected);
            check(writes == std::vector<unsigned>({1, 4, 9}), "directory uses tag, int32, exact bytes");
            const char* excluded[] = {"include path", "run file", "rem comment"};
            const std::size_t before = bytes.size();
            for (const char* line : excluded) compiler.addLine(line);
            check(bytes.size() == before, "three exact lower-case prefixes excluded");
            const std::string included[] = {"", " include path", "Include path", "run", "rem", "xinclude ",
                                            std::string("a\0b", 3), std::string(300, 'x')};
            for (const std::string& line : included)
            {
                compiler.addLine(line); record('E', line, true, expected);
                check(bytes == expected, "command exact tagged golden bytes");
            }
            compiler.setWorkingDir(""); record('W', "", false, expected);
            check(bytes == expected, "empty directory records a zero length");
            const std::size_t writeCount = writes.size();
            shortWrite = true;
            compiler.addLine("last"); record('E', "last", true, expected);
            check(writes.size() == writeCount + 4 && bytes == expected,
                  "retail writer issues all four writes after short result");
            shortWrite = false;
        }
        check(destroys == 1 && closes == 2, "compiler closes before File destructor closes again");
        {
            ConsoleCompiler compiler("read.con", false);
            check(openedName == "read.con" && modeSeen == 2 && accessSeen == 0 && flagsSeen == 0,
                  "reader constructor preserves file mode/access/flags");
        }
        check(destroys == 2 && closes == 4, "reader ownership released");
        stamp = ":12 34:56";
        check(std::wstring(crashDumpTargetName(L"C:\\fixture\\bf2.dmp")) == L"C:\\fixture\\bf2_:12_34.56.dmp",
              "date punctuation normalized starting at index one");
        stamp = " 1:02";
        check(std::wstring(crashDumpTargetName(L"ignored.other")) == L"C:\\fixture\\bf2_:12_34.56_ 1.02.dmp",
              "static filename accumulates stamps and ignores later base argument");
    }
    std::printf("Core writer/name runtime: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
