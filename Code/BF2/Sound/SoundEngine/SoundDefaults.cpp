// BF2Audio default services; excluded from host images to keep DLL-owned state separate.
#if defined(BF2_IMAGE_BF2AUDIO)
#include "SoundDefaults.h"
#include "Win32System.h"

#include <cstdio>
#include <cstdlib>

namespace dice {
namespace hfe {

void* DefaultSoundFileIO::open(const char* name)
{
    return std::fopen(name, "rb");
}

int DefaultSoundFileIO::close(void* handle)
{
    return std::fclose((FILE*) handle);
}

int DefaultSoundFileIO::seek(void* handle, int offset, int origin)
{
    if (std::fseek((FILE*) handle, offset, origin))
        return 0;
    return (int) std::ftell((FILE*) handle);
}

int DefaultSoundFileIO::read(void* handle, void* buffer, int bytes)
{
    return (int) std::fread(buffer, 1, (size_t) bytes, (FILE*) handle);
}

// Writes prefixed trace messages; assertions terminate the process with -1.
void DefaultSoundTrace::error(const char* msg, const char*, int)
{
    win32::outputDebugString("SND-ERR: ");
    win32::outputDebugString(msg);
    win32::outputDebugString("\n");
}

void DefaultSoundTrace::warning(const char* msg, const char*, int)
{
    win32::outputDebugString("SND-WRN: ");
    win32::outputDebugString(msg);
    win32::outputDebugString("\n");
}

void DefaultSoundTrace::info(const char* msg, const char*, int)
{
    win32::outputDebugString("SND-INF: ");
    win32::outputDebugString(msg);
    win32::outputDebugString("\n");
}

void DefaultSoundTrace::assertFailed(const char* msg, const char*, int)
{
    win32::outputDebugString("SND-ASSERTION FAILED: ");
    win32::outputDebugString(msg);
    win32::outputDebugString("\n");
    std::exit(-1);
}

void DefaultSoundTrace::debug(const char* msg, const char*, int)
{
    win32::outputDebugString("SND-DBG: ");
    win32::outputDebugString(msg);
    win32::outputDebugString("\n");
}

NullProfilerClient g_nullProfilerClient;
IProfilerClient*   g_profilerClient = (IProfilerClient*) &g_nullProfilerClient;   // ds:1003F0D4

} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_BF2AUDIO
