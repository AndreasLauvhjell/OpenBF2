// Audio DLL platform services and process entry point.

// Audio DLL process entry point.


// BF2Audio.dll only. Without BF2_IMAGE_BF2AUDIO this file compiles to nothing: the recursive
// sweeps that build BF2.exe and bf2_w32ded.exe (CMake's bf2_client_objects glob, build.cmd's
// all-TU census) must not carry the DLL's engine: its DllMain, its two exports and its own null
// Sound/SoundTemplate pair would collide with the host's, which loads this image at run time.
// Same convention as
// Code/BF2/RendDX9/BF2Render.cpp's BF2_IMAGE_RENDDX9 guard. Build with Code\Build\bf2audio.cmd.
#if defined(BF2_IMAGE_BF2AUDIO)
#include "Win32System.h"
#include "SoundTrace.h"


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>     // waveformatex, which dsound.h needs and WIN32_LEAN_AND_MEAN leaves out
#include <dsound.h>

namespace dice {
namespace hfe {
namespace win32 {

void* g_dllInstance = 0;        // ds:10048E68

void* loadLibrary(const char* fileName)              { return (void*) ::LoadLibraryA(fileName); }
void  freeLibrary(void* module)                       { if (module) ::FreeLibrary((HMODULE) module); }
void* getProcAddress(void* module, const char* name) { return module ? (void*) ::GetProcAddress((HMODULE) module, name) : 0; }

void  outputDebugString(const char* text)             { ::OutputDebugStringA(text); }

void  sleep(unsigned int milliseconds)                { ::Sleep(milliseconds); }
long  exchange(volatile long* target, long value)     { return ::InterlockedExchange(target, value); }

namespace {

DWORD WINAPI threadStart(LPVOID parameter)
{
    ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    Thread* t = (Thread*) parameter;
    t->entry(t);
    return 0;
}

} // namespace

// The handle CreateThread returns is stored after the object is fully written,
// but the thread may already be running `entry` by then - the entry must not touch `handle`.
Thread* createThread(void (__cdecl* entry)(Thread*), void* argument)
{
    Thread* t = new Thread;
    t->entry = entry;
    t->argument = argument;
    t->stopRequested = 0;
    t->handle = (void*) ::CreateThread(0, 0, threadStart, t, 0, &t->id);
    return t;
}

// Raise the stop flag, wait forever for the thread, close, delete.
void joinAndDeleteThread(Thread* t)
{
    if (t)
    {
        ::InterlockedIncrement(&t->stopRequested);
        ::WaitForSingleObject((HANDLE) t->handle, INFINITE);
        ::CloseHandle((HANDLE) t->handle);
        delete t;
    }
}

bool  threadShouldStop(const Thread* t) { return t->stopRequested != 0; }
void* threadArgument(const Thread* t)   { return t->argument; }

namespace {

// Ignores the primary-device entry with null GUID; logs and marks named devices.
BOOL CALLBACK enumCallback(LPGUID guid, LPCSTR description, LPCSTR module, LPVOID context)
{
    if (!guid)
        return TRUE;
    if (!module)      module = "unknown";
    if (!description) description = "unknown";
    g_soundTrace->info(sndFormat("Found sound device %s (%s)", description, module),
                       "C:\\dice\\Projects\\BF2Branches\\Patch_1_50\\Code\\BF2\\Sound\\SoundEngine\\Win32System.cpp", 141);
    *(unsigned int*) context |= 1u;
    return FALSE;   // false stops the enumeration after the first named device. As shipped.
}

} // namespace

bool hasSoundDevice()
{
    unsigned int context = 0;
    const HRESULT hr = ::DirectSoundEnumerateA(enumCallback, &context);
    if (hr < 0)
    {
        g_soundTrace->warning(sndFormat("Enumerating sound devices failed (0x%08x). Sound will be disabled.", hr),
                              "C:\\dice\\Projects\\BF2Branches\\Patch_1_50\\Code\\BF2\\Sound\\SoundEngine\\Win32System.cpp", 152);
    }
    return context != 0;
}

} // namespace win32
} // namespace hfe
}
// Records the DLL instance.
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void) reason; (void) reserved;
    dice::hfe::win32::g_dllInstance = (void*) instance;
    return TRUE;
}

#endif // BF2_IMAGE_BF2AUDIO
