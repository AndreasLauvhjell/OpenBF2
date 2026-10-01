// [TEMP] [CHOSEN, not recovered] Source-extracted bf2WinMain control-flow fixture.
// All desktop APIs and engine collaborators below are test doubles; no server or UI changes.
#include <windows.h>
#include <cstdio>
#include <cstring>
static STICKYKEYS s_startupStickyKeys;
static TOGGLEKEYS s_startupToggleKeys;
static FILTERKEYS s_startupFilterKeys;
static UINT s_startupMouseTrails;
static BOOL s_screenSaverActive;
static int runs, dumpNames, dumps, restores, fpu, desktopCalls, debuggerQueries, checks, failures;
static bool debugger, fail;
struct SystemFixture { void setUseFpuExceptions(bool value) { if (value) ++fpu; } } systemFixture;
static SystemFixture* g_system = &systemFixture;
struct InfoFixture { bool getIsDebuggerInstalled() { ++debuggerQueries; return debugger; } } infoFixture;
static InfoFixture* systemInfo() { return &infoFixture; }
static BOOL WINAPI fixtureSystemParametersInfo(UINT, UINT, PVOID, UINT) { ++desktopCalls; return TRUE; }
static void allowAccessibilityShortcutKeys(bool) {}
static void restoreDesktopSettings() { ++restores; }
static const wchar_t* getCrashDumpDirectory() { return L"fixture"; }
static const wchar_t* crashDumpFileName(const wchar_t*) { return L"fixture.dmp"; }
static const wchar_t* crashDumpTargetName(const wchar_t* name) { ++dumpNames; return name; }
static void writeCrashDump(EXCEPTION_POINTERS* info, const wchar_t*)
{
    if (info && info->ExceptionRecord->ExceptionCode == 0xE0424242) ++dumps;
}
static int bf2Main(HINSTANCE, HINSTANCE, LPSTR)
{
    ++runs;
    if (fail) RaiseException(0xE0424242, 0, 0, 0);
    return 17;
}
#define SystemParametersInfoA fixtureSystemParametersInfo
#include "startup_body.inl"
#undef SystemParametersInfoA
static void check(bool ok, const char* name)
{
    ++checks;
    if (!ok) { ++failures; std::printf("FAIL %s\n", name); }
}
static void reset()
{
    runs = dumpNames = dumps = restores = fpu = desktopCalls = debuggerQueries = 0;
    debugger = fail = false;
}
int main()
{
    char normal[] = "+dedicated 1";
    char noSeh[] = "+seh 0";
    char useFpu[] = "+useFpuExceptions";
    reset();
    check(bf2WinMain(0, 0, normal) == 17, "SEH normal return");
    check(runs == 1 && restores == 1 && !dumps && !dumpNames && desktopCalls == 5,
          "default startup calls engine once and restores desktop");
    reset(); fail = true;
    check(bf2WinMain(0, 0, normal) == 42, "SEH maps exception to exit42");
    check(runs == 1 && dumpNames == 1 && dumps == 1 && restores == 1,
          "first engine exception is inside filter and still restores desktop");
    reset();
    check(bf2WinMain(0, 0, noSeh) == 17 && runs == 1 && !dumps && restores == 1,
          "explicit SEH bypass remains one call");
    check(debuggerQueries == 1, "explicit SEH bypass still queries debugger as retail does");
    reset(); debugger = true;
    check(bf2WinMain(0, 0, normal) == 17 && runs == 1 && !dumps && restores == 1,
          "debugger bypass remains one call");
    reset();
    check(bf2WinMain(0, 0, useFpu) == 17 && fpu == 1 && runs == 1,
          "FPU option remains before one engine call");
    std::printf("Startup SEH runtime: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
