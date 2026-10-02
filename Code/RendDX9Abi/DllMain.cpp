// [CHOSEN, not recovered] Minimal DLL entry point.
// Static initialization constructs the four modules using the shared native CRT.
// Keep loader-lock-sensitive initialization out of DllMain.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        // DisableThreadLibraryCalls is safe here and removes two callbacks per thread from a game
        // that creates several. Nothing in this image has per-thread state.
        break;

    case DLL_PROCESS_DETACH:
        // deinitDll performs teardown before FreeLibrary; repeating it here would double-free
        // resources under loader lock.
        break;

    default:
        break;
    }
    return TRUE;
}
