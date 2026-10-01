// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: bf2audio-smoke]
// OpenAL logging shim for differential audio tests. Forwards to wrap_oal_real.dll,
// logs through BF2_ALSHIM_LOG, and normalizes source/buffer/device handles to ordinals.
// The resolved engine and EAX functions use typed logging; other exports use generated trampolines.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <map>

#include "alshim.h"

typedef char          ALboolean;
typedef char          ALchar;
typedef int           ALint;
typedef unsigned int  ALuint;
typedef int           ALsizei;
typedef int           ALenum;
typedef float         ALfloat;
typedef void          ALvoid;
struct ALCdevice;
struct ALCcontext;
struct EAXGUID { unsigned int Data1; unsigned short Data2; unsigned short Data3; unsigned char Data4[8]; };

namespace {

HMODULE g_real = 0;
FILE*   g_log  = 0;
bool    g_loaded = false;
std::map<const void*, int> g_devices;
std::map<const void*, int> g_contexts;
std::map<ALuint, int> g_sources;
std::map<ALuint, int> g_buffers;
int g_nextSource = 0;
int g_nextBuffer = 0;

// The music thread and the engine's main thread both call OpenAL, so the log is written under a lock.
struct LogLock
{
    CRITICAL_SECTION cs;
    LogLock()  { InitializeCriticalSection(&cs); }
    ~LogLock() { DeleteCriticalSection(&cs); }
} g_logLock;

void logLine(const char* fmt, ...)
{
    if (!g_log)
        return;
    EnterCriticalSection(&g_logLock.cs);
    va_list va;
    va_start(va, fmt);
    std::vfprintf(g_log, fmt, va);
    va_end(va);
    std::fputc('\n', g_log);
    std::fflush(g_log);
    LeaveCriticalSection(&g_logLock.cs);
}

// Devices and contexts get a fresh ordinal at every open/create (wrap_oal may hand the same block
// back after a close, which is its allocator's business, not the engine's) and lose it on close.
int g_nextDevice = 0;
int g_nextContext = 0;
int handleId(std::map<const void*, int>& m, const void* p)
{
    if (!p)
        return 0;
    std::map<const void*, int>::iterator it = m.find(p);
    if (it != m.end())
        return it->second;
    const int id = (&m == &g_devices) ? ++g_nextDevice : ++g_nextContext;
    m[p] = id;
    return id;
}

// Source / buffer names as ordinals. An unknown name (never generated through this shim) prints raw.
const char* nameOf(char* buf, std::map<ALuint, int>& m, char prefix, ALuint id)
{
    std::map<ALuint, int>::iterator it = m.find(id);
    if (id == 0)
        std::sprintf(buf, "%c0", prefix);
    else if (it != m.end())
        std::sprintf(buf, "%c%d", prefix, it->second);
    else
        std::sprintf(buf, "%c?%u", prefix, id);
    return buf;
}
#define SID(id) nameOf(sbuf, g_sources, 's', (id))
#define BID(id) nameOf(bbuf, g_buffers, 'b', (id))

unsigned int fnv1a(const void* data, int bytes)
{
    unsigned int h = 2166136261u;
    const unsigned char* p = (const unsigned char*) data;
    for (int i = 0; i < bytes; ++i)
    {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

const char* str(const char* s) { return s ? s : "(null)"; }

// the real entry points
#define REAL(name, ret, args) typedef ret (__cdecl* PFN_##name) args; PFN_##name r_##name = 0;
REAL(alGetInteger,           ALint,        (ALenum))
REAL(alGetString,            const ALchar*,(ALenum))
REAL(alGetError,             ALenum,       ())
REAL(alIsExtensionPresent,   ALboolean,    (const ALchar*))
REAL(alGetProcAddress,       void*,        (const ALchar*))
REAL(alGetEnumValue,         ALenum,       (const ALchar*))
REAL(alListenerf,            void,         (ALenum, ALfloat))
REAL(alListenerfv,           void,         (ALenum, const ALfloat*))
REAL(alListener3f,           void,         (ALenum, ALfloat, ALfloat, ALfloat))
REAL(alGetListenerfv,        void,         (ALenum, ALfloat*))
REAL(alGenSources,           void,         (ALsizei, ALuint*))
REAL(alDeleteSources,        void,         (ALsizei, const ALuint*))
REAL(alSourcei,              void,         (ALuint, ALenum, ALint))
REAL(alSourcef,              void,         (ALuint, ALenum, ALfloat))
REAL(alSourcefv,             void,         (ALuint, ALenum, const ALfloat*))
REAL(alGetSourcei,           void,         (ALuint, ALenum, ALint*))
REAL(alGetSourcef,           void,         (ALuint, ALenum, ALfloat*))
REAL(alGetSourcefv,          void,         (ALuint, ALenum, ALfloat*))
REAL(alSourcePlay,           void,         (ALuint))
REAL(alSourceStop,           void,         (ALuint))
REAL(alSourceRewind,         void,         (ALuint))
REAL(alGenBuffers,           void,         (ALsizei, ALuint*))
REAL(alDeleteBuffers,        void,         (ALsizei, const ALuint*))
REAL(alBufferData,           void,         (ALuint, ALenum, const ALvoid*, ALsizei, ALsizei))
REAL(alGetBufferi,           void,         (ALuint, ALenum, ALint*))
REAL(alSourceUnqueueBuffers, void,         (ALuint, ALsizei, ALuint*))
REAL(alSourceQueueBuffers,   void,         (ALuint, ALsizei, const ALuint*))
REAL(alDopplerFactor,        void,         (ALfloat))
REAL(alDistanceModel,        void,         (ALenum))
REAL(alcCreateContext,       ALCcontext*,  (ALCdevice*, const ALint*))
REAL(alcMakeContextCurrent,  ALboolean,    (ALCcontext*))
REAL(alcDestroyContext,      void,         (ALCcontext*))
REAL(alcGetError,            ALenum,       (ALCdevice*))
REAL(alcOpenDevice,          ALCdevice*,   (const ALchar*))
REAL(alcCloseDevice,         ALboolean,    (ALCdevice*))
REAL(alcIsExtensionPresent,  ALboolean,    (ALCdevice*, const ALchar*))
REAL(alcGetString,           const ALchar*,(ALCdevice*, ALenum))
REAL(alcGetIntegerv,         void,         (ALCdevice*, ALenum, ALsizei, ALint*))
REAL(EAXSet,                 ALenum,       (const EAXGUID*, ALuint, ALuint, ALvoid*, ALuint))
REAL(EAXGet,                 ALenum,       (const EAXGUID*, ALuint, ALuint, ALvoid*, ALuint))
REAL(EAXSetBufferMode,       ALenum,       (ALsizei, ALuint*, ALint))
#undef REAL

void* resolveReal(const char* name)
{
    return (void*) GetProcAddress(g_real, name);
}

// How many floats a vector-valued property carries (the engine only uses these).
int floatsFor(ALenum e)
{
    switch (e)
    {
    case 0x1004: case 0x1005: case 0x1006: return 3;   // position, direction, velocity
    case 0x100F:                           return 6;   // orientation
    default:                               return 1;
    }
}

void logFloats(char* out, size_t cap, const ALfloat* v, int n)
{
    size_t used = 0;
    out[0] = 0;
    for (int i = 0; i < n && used + 24 < cap; ++i)
        used += (size_t) std::sprintf(out + used, "%s%.7g", i ? ", " : "", v[i]);
}

bool isListenerGuid(const EAXGUID* g) { return g && g->Data1 == 0xA8FA6882u; }
bool isBufferGuid(const EAXGUID* g)   { return g && g->Data1 == 0xA8FA6881u; }

void logEaxBlock(const char* who, const EAXGUID* g, ALuint prop, ALuint source, const void* data, ALuint size)
{
    char sbuf[32];
    const unsigned int* d = (const unsigned int*) data;
    const float* f = (const float*) data;
    if (isListenerGuid(g) && size == 112 && prop == 1)
    {
        logLine("%s(listener, ALL, %s) env=%u size=%.7g diff=%.7g room=%d roomHF=%d roomLF=%d decay=%.7g decayHF=%.7g decayLF=%.7g refl=%d reflDelay=%.7g reflPan=(%.7g,%.7g,%.7g) reverb=%d revDelay=%.7g revPan=(%.7g,%.7g,%.7g) echo=(%.7g,%.7g) mod=(%.7g,%.7g) airHF=%.7g hfRef=%.7g lfRef=%.7g roomRolloff=%.7g flags=0x%X",
                who, SID(source), d[0], f[1], f[2], (int) d[3], (int) d[4], (int) d[5], f[6], f[7], f[8], (int) d[9], f[10],
                f[11], f[12], f[13], (int) d[14], f[15], f[16], f[17], f[18], f[19], f[20], f[21], f[22], f[23], f[24], f[25], f[26], d[27]);
        return;
    }
    if (isBufferGuid(g) && size == 72 && prop == 1)
    {
        logLine("%s(buffer, ALL, %s) direct=%d directHF=%d room=%d roomHF=%d obstr=%d obstrLF=%.7g occl=%d occlLF=%.7g occlRoom=%.7g occlDirect=%.7g excl=%d exclLF=%.7g outsideHF=%d doppler=%.7g rolloff=%.7g roomRolloff=%.7g air=%.7g flags=0x%X",
                who, SID(source), (int) d[0], (int) d[1], (int) d[2], (int) d[3], (int) d[4], f[5], (int) d[6], f[7], f[8], f[9],
                (int) d[10], f[11], (int) d[12], f[13], f[14], f[15], f[16], d[17]);
        return;
    }
    char hex[1024] = "";
    size_t used = 0;
    for (ALuint i = 0; i < size / 4 && used + 12 < sizeof(hex); ++i)
        used += (size_t) std::sprintf(hex + used, "%s%08X", i ? " " : "", d[i]);
    logLine("%s(guid=%08X, prop=%u, %s, size=%u) %s", who, g ? g->Data1 : 0, prop, SID(source), size, hex);
}

} // namespace

extern "C" void __cdecl ensureLoaded()
{
    if (g_loaded)
        return;
    g_loaded = true;
    const char* logPath = std::getenv("BF2_ALSHIM_LOG");
    if (logPath && *logPath)
        g_log = std::fopen(logPath, "w");
    char realPath[MAX_PATH] = "";
    const char* real = std::getenv("BF2_ALSHIM_REAL");
    if (!real || !*real)
    {
        HMODULE self = 0;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR) &ensureLoaded, &self);
        GetModuleFileNameA(self, realPath, MAX_PATH);
        char* slash = std::strrchr(realPath, '\\');
        std::strcpy(slash ? slash + 1 : realPath, "wrap_oal_real.dll");
        real = realPath;
    }
    g_real = LoadLibraryA(real);
    logLine("# alshim: real=%s -> %s", real, g_real ? "loaded" : "FAILED");
    if (!g_real)
        return;
#define RES(name) r_##name = (PFN_##name) resolveReal(#name);
    RES(alGetInteger) RES(alGetString) RES(alGetError) RES(alIsExtensionPresent) RES(alGetProcAddress) RES(alGetEnumValue)
    RES(alListenerf) RES(alListenerfv) RES(alListener3f) RES(alGetListenerfv) RES(alGenSources) RES(alDeleteSources)
    RES(alSourcei) RES(alSourcef) RES(alSourcefv) RES(alGetSourcei) RES(alGetSourcef) RES(alGetSourcefv)
    RES(alSourcePlay) RES(alSourceStop) RES(alSourceRewind) RES(alGenBuffers) RES(alDeleteBuffers) RES(alBufferData)
    RES(alGetBufferi) RES(alSourceUnqueueBuffers) RES(alSourceQueueBuffers) RES(alDopplerFactor) RES(alDistanceModel)
    RES(alcCreateContext) RES(alcMakeContextCurrent) RES(alcDestroyContext) RES(alcGetError) RES(alcOpenDevice)
    RES(alcCloseDevice) RES(alcIsExtensionPresent) RES(alcGetString) RES(alcGetIntegerv) RES(EAXSet) RES(EAXGet)
#undef RES
    r_EAXSetBufferMode = (PFN_EAXSetBufferMode) resolveReal("EAXSetBufferMode");
    resolveTrampolines(resolveReal);
}

// The host drops markers into the AL log so frames can be aligned with its own output.
extern "C" __declspec(dllexport) void __cdecl alshim_mark(const char* text)
{
    ensureLoaded();
    logLine("== %s", str(text));
}

// the EAX functions the engine fetches through alGetProcAddress
extern "C" ALenum __cdecl EAXSet(const EAXGUID* g, ALuint prop, ALuint source, ALvoid* data, ALuint size)
{
    ensureLoaded();
    logEaxBlock("EAXSet", g, prop, source, data, size);
    const ALenum r = r_EAXSet ? r_EAXSet(g, prop, source, data, size) : 0;
    logLine("  -> 0x%X", r);
    return r;
}

extern "C" ALenum __cdecl EAXGet(const EAXGUID* g, ALuint prop, ALuint source, ALvoid* data, ALuint size)
{
    ensureLoaded();
    char sbuf[32];
    const ALenum r = r_EAXGet ? r_EAXGet(g, prop, source, data, size) : 0;
    logLine("EAXGet(guid=%08X, prop=%u, %s, size=%u) -> 0x%X", g ? g->Data1 : 0, prop, SID(source), size, r);
    return r;
}

static ALenum __cdecl EAXSetBufferMode_wrapped(ALsizei n, ALuint* buffers, ALint mode)
{
    ensureLoaded();
    char bbuf[32];
    const ALenum r = r_EAXSetBufferMode ? r_EAXSetBufferMode(n, buffers, mode) : 0;
    logLine("EAXSetBufferMode(n=%d, first=%s, mode=0x%X) -> 0x%X", n, (n > 0 && buffers) ? BID(buffers[0]) : "b0", mode, r);
    return r;
}

// the wrapped entry points
extern "C" ALint __cdecl alGetInteger(ALenum e)
{
    ensureLoaded();
    const ALint r = r_alGetInteger(e);
    logLine("alGetInteger(0x%X) -> %d", e, r);
    return r;
}

extern "C" const ALchar* __cdecl alGetString(ALenum e)
{
    ensureLoaded();
    const ALchar* r = r_alGetString(e);
    logLine("alGetString(0x%X) -> \"%s\"", e, str(r));
    return r;
}

extern "C" ALenum __cdecl alGetError()
{
    ensureLoaded();
    const ALenum r = r_alGetError();
    logLine("alGetError() -> 0x%X", r);
    return r;
}

extern "C" ALboolean __cdecl alIsExtensionPresent(const ALchar* name)
{
    ensureLoaded();
    const ALboolean r = r_alIsExtensionPresent(name);
    logLine("alIsExtensionPresent(\"%s\") -> %d", str(name), (int) r);
    return r;
}

extern "C" void* __cdecl alGetProcAddress(const ALchar* name)
{
    ensureLoaded();
    void* r = r_alGetProcAddress(name);
    const char* note = "";
    if (name && !std::strcmp(name, "EAXSet"))            { note = " (wrapped)"; if (r) r = (void*) &EAXSet; }
    if (name && !std::strcmp(name, "EAXGet"))            { note = " (wrapped)"; if (r) r = (void*) &EAXGet; }
    if (name && !std::strcmp(name, "EAXSetBufferMode"))  { note = " (wrapped)"; if (r) r = (void*) &EAXSetBufferMode_wrapped; }
    logLine("alGetProcAddress(\"%s\") -> %s%s", str(name), r ? "found" : "null", note);
    return r;
}

extern "C" ALenum __cdecl alGetEnumValue(const ALchar* name)
{
    ensureLoaded();
    const ALenum r = r_alGetEnumValue(name);
    logLine("alGetEnumValue(\"%s\") -> 0x%X", str(name), r);
    return r;
}

extern "C" void __cdecl alListenerf(ALenum e, ALfloat v)
{
    ensureLoaded();
    logLine("alListenerf(0x%X, %.7g)", e, v);
    r_alListenerf(e, v);
}

extern "C" void __cdecl alListenerfv(ALenum e, const ALfloat* v)
{
    ensureLoaded();
    char buf[256];
    logFloats(buf, sizeof(buf), v, floatsFor(e));
    logLine("alListenerfv(0x%X, [%s])", e, buf);
    r_alListenerfv(e, v);
}

extern "C" void __cdecl alListener3f(ALenum e, ALfloat x, ALfloat y, ALfloat z)
{
    ensureLoaded();
    logLine("alListener3f(0x%X, %.7g, %.7g, %.7g)", e, x, y, z);
    r_alListener3f(e, x, y, z);
}

extern "C" void __cdecl alGetListenerfv(ALenum e, ALfloat* v)
{
    ensureLoaded();
    r_alGetListenerfv(e, v);
    char buf[256];
    logFloats(buf, sizeof(buf), v, floatsFor(e));
    logLine("alGetListenerfv(0x%X) -> [%s]", e, buf);
}

extern "C" void __cdecl alGenSources(ALsizei n, ALuint* out)
{
    ensureLoaded();
    char sbuf[32], sbuf2[32];
    r_alGenSources(n, out);
    for (int i = 0; i < n; ++i)
        if (out[i])
            g_sources[out[i]] = ++g_nextSource;
    logLine("alGenSources(%d) -> first=%s last=%s", n, n > 0 ? SID(out[0]) : "s0",
            n > 0 ? nameOf(sbuf2, g_sources, 's', out[n - 1]) : "s0");
}

extern "C" void __cdecl alDeleteSources(ALsizei n, const ALuint* ids)
{
    ensureLoaded();
    char sbuf[32], sbuf2[32];
    logLine("alDeleteSources(%d) first=%s last=%s", n, n > 0 ? SID(ids[0]) : "s0",
            n > 0 ? nameOf(sbuf2, g_sources, 's', ids[n - 1]) : "s0");
    r_alDeleteSources(n, ids);
    for (int i = 0; i < n; ++i)
        g_sources.erase(ids[i]);
}

extern "C" void __cdecl alSourcei(ALuint s, ALenum e, ALint v)
{
    ensureLoaded();
    char sbuf[32], bbuf[32];
    if (e == 0x1009)
        logLine("alSourcei(%s, AL_BUFFER, %s)", SID(s), BID((ALuint) v));
    else
        logLine("alSourcei(%s, 0x%X, %d)", SID(s), e, v);
    r_alSourcei(s, e, v);
}

extern "C" void __cdecl alSourcef(ALuint s, ALenum e, ALfloat v)
{
    ensureLoaded();
    char sbuf[32];
    logLine("alSourcef(%s, 0x%X, %.7g)", SID(s), e, v);
    r_alSourcef(s, e, v);
}

extern "C" void __cdecl alSourcefv(ALuint s, ALenum e, const ALfloat* v)
{
    ensureLoaded();
    char sbuf[32], buf[256];
    logFloats(buf, sizeof(buf), v, floatsFor(e));
    logLine("alSourcefv(%s, 0x%X, [%s])", SID(s), e, buf);
    r_alSourcefv(s, e, v);
}

extern "C" void __cdecl alGetSourcei(ALuint s, ALenum e, ALint* v)
{
    ensureLoaded();
    char sbuf[32];
    r_alGetSourcei(s, e, v);
    logLine("alGetSourcei(%s, 0x%X) -> %d", SID(s), e, *v);
}

extern "C" void __cdecl alGetSourcef(ALuint s, ALenum e, ALfloat* v)
{
    ensureLoaded();
    char sbuf[32];
    r_alGetSourcef(s, e, v);
    logLine("alGetSourcef(%s, 0x%X) -> %.7g", SID(s), e, *v);
}

extern "C" void __cdecl alGetSourcefv(ALuint s, ALenum e, ALfloat* v)
{
    ensureLoaded();
    char sbuf[32], buf[256];
    r_alGetSourcefv(s, e, v);
    logFloats(buf, sizeof(buf), v, floatsFor(e));
    logLine("alGetSourcefv(%s, 0x%X) -> [%s]", SID(s), e, buf);
}

extern "C" void __cdecl alSourcePlay(ALuint s)   { ensureLoaded(); char sbuf[32]; logLine("alSourcePlay(%s)", SID(s));   r_alSourcePlay(s); }
extern "C" void __cdecl alSourceStop(ALuint s)   { ensureLoaded(); char sbuf[32]; logLine("alSourceStop(%s)", SID(s));   r_alSourceStop(s); }
extern "C" void __cdecl alSourceRewind(ALuint s) { ensureLoaded(); char sbuf[32]; logLine("alSourceRewind(%s)", SID(s)); r_alSourceRewind(s); }

extern "C" void __cdecl alGenBuffers(ALsizei n, ALuint* out)
{
    ensureLoaded();
    char bbuf[32], bbuf2[32];
    r_alGenBuffers(n, out);
    for (int i = 0; i < n; ++i)
        if (out[i])
            g_buffers[out[i]] = ++g_nextBuffer;
    logLine("alGenBuffers(%d) -> first=%s last=%s", n, n > 0 ? BID(out[0]) : "b0",
            n > 0 ? nameOf(bbuf2, g_buffers, 'b', out[n - 1]) : "b0");
}

extern "C" void __cdecl alDeleteBuffers(ALsizei n, const ALuint* ids)
{
    ensureLoaded();
    char bbuf[32], bbuf2[32];
    logLine("alDeleteBuffers(%d) first=%s last=%s", n, n > 0 ? BID(ids[0]) : "b0",
            n > 0 ? nameOf(bbuf2, g_buffers, 'b', ids[n - 1]) : "b0");
    r_alDeleteBuffers(n, ids);
    for (int i = 0; i < n; ++i)
        g_buffers.erase(ids[i]);
}

extern "C" void __cdecl alBufferData(ALuint b, ALenum fmt, const ALvoid* data, ALsizei size, ALsizei freq)
{
    ensureLoaded();
    char bbuf[32];
    logLine("alBufferData(%s, 0x%X, size=%d, freq=%d, fnv=%08X)", BID(b), fmt, size, freq, data ? fnv1a(data, size) : 0u);
    r_alBufferData(b, fmt, data, size, freq);
}

extern "C" void __cdecl alGetBufferi(ALuint b, ALenum e, ALint* v)
{
    ensureLoaded();
    char bbuf[32];
    r_alGetBufferi(b, e, v);
    logLine("alGetBufferi(%s, 0x%X) -> %d", BID(b), e, *v);
}

extern "C" void __cdecl alSourceUnqueueBuffers(ALuint s, ALsizei n, ALuint* out)
{
    ensureLoaded();
    char sbuf[32], bbuf[32];
    r_alSourceUnqueueBuffers(s, n, out);
    logLine("alSourceUnqueueBuffers(%s, %d) -> first=%s", SID(s), n, n > 0 ? BID(out[0]) : "b0");
}

extern "C" void __cdecl alSourceQueueBuffers(ALuint s, ALsizei n, const ALuint* ids)
{
    ensureLoaded();
    char sbuf[32], bbuf[32], bbuf2[32];
    logLine("alSourceQueueBuffers(%s, %d) first=%s last=%s", SID(s), n, n > 0 ? BID(ids[0]) : "b0",
            n > 0 ? nameOf(bbuf2, g_buffers, 'b', ids[n - 1]) : "b0");
    r_alSourceQueueBuffers(s, n, ids);
}

extern "C" void __cdecl alDopplerFactor(ALfloat f) { ensureLoaded(); logLine("alDopplerFactor(%.7g)", f); r_alDopplerFactor(f); }
extern "C" void __cdecl alDistanceModel(ALenum e)  { ensureLoaded(); logLine("alDistanceModel(0x%X)", e); r_alDistanceModel(e); }

extern "C" ALCcontext* __cdecl alcCreateContext(ALCdevice* dev, const ALint* attrs)
{
    ensureLoaded();
    char buf[256] = "";
    size_t used = 0;
    for (int i = 0; attrs && attrs[i] && used + 32 < sizeof(buf); i += 2)
        used += (size_t) std::sprintf(buf + used, "%s0x%X=%d", i ? " " : "", attrs[i], attrs[i + 1]);
    ALCcontext* r = r_alcCreateContext(dev, attrs);
    logLine("alcCreateContext(dev#%d, [%s]) -> ctx#%d", handleId(g_devices, dev), buf, handleId(g_contexts, r));
    return r;
}

extern "C" ALboolean __cdecl alcMakeContextCurrent(ALCcontext* ctx)
{
    ensureLoaded();
    const ALboolean r = r_alcMakeContextCurrent(ctx);
    logLine("alcMakeContextCurrent(ctx#%d) -> %d", handleId(g_contexts, ctx), (int) r);
    return r;
}

extern "C" void __cdecl alcDestroyContext(ALCcontext* ctx)
{
    ensureLoaded();
    logLine("alcDestroyContext(ctx#%d)", handleId(g_contexts, ctx));
    r_alcDestroyContext(ctx);
    g_contexts.erase(ctx);
}

extern "C" ALenum __cdecl alcGetError(ALCdevice* dev)
{
    ensureLoaded();
    const ALenum r = r_alcGetError(dev);
    logLine("alcGetError(dev#%d) -> 0x%X", handleId(g_devices, dev), r);
    return r;
}

extern "C" ALCdevice* __cdecl alcOpenDevice(const ALchar* name)
{
    ensureLoaded();
    ALCdevice* r = r_alcOpenDevice(name);
    logLine("alcOpenDevice(\"%s\") -> dev#%d", str(name), handleId(g_devices, r));
    return r;
}

extern "C" ALboolean __cdecl alcCloseDevice(ALCdevice* dev)
{
    ensureLoaded();
    const int id = handleId(g_devices, dev);
    const ALboolean r = r_alcCloseDevice(dev);
    logLine("alcCloseDevice(dev#%d) -> %d", id, (int) r);
    g_devices.erase(dev);
    return r;
}

extern "C" ALboolean __cdecl alcIsExtensionPresent(ALCdevice* dev, const ALchar* name)
{
    ensureLoaded();
    const ALboolean r = r_alcIsExtensionPresent(dev, name);
    logLine("alcIsExtensionPresent(dev#%d, \"%s\") -> %d", handleId(g_devices, dev), str(name), (int) r);
    return r;
}

extern "C" const ALchar* __cdecl alcGetString(ALCdevice* dev, ALenum e)
{
    ensureLoaded();
    const ALchar* r = r_alcGetString(dev, e);
    logLine("alcGetString(dev#%d, 0x%X) -> \"%s\"", handleId(g_devices, dev), e, str(r));
    return r;
}

extern "C" void __cdecl alcGetIntegerv(ALCdevice* dev, ALenum e, ALsizei n, ALint* out)
{
    ensureLoaded();
    r_alcGetIntegerv(dev, e, n, out);
    logLine("alcGetIntegerv(dev#%d, 0x%X, %d) -> %d", handleId(g_devices, dev), e, n, n > 0 ? out[0] : 0);
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_DETACH && g_log)
    {
        logLine("# alshim: detach");
        std::fclose(g_log);
        g_log = 0;
    }
    return TRUE;
}
