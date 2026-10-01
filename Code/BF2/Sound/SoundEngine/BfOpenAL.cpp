
// BF2Audio.dll only. Without BF2_IMAGE_BF2AUDIO this file compiles to nothing: the recursive
// sweeps that build BF2.exe and bf2_w32ded.exe (CMake's bf2_client_objects glob, build.cmd's
// all-TU census) must not carry the DLL's engine: its DllMain, its two exports and its own null
// Sound/SoundTemplate pair would collide with the host's, which loads this image at run time.
// Same convention as
// Code/BF2/RendDX9/BF2Render.cpp's BF2_IMAGE_RENDDX9 guard. Build with Code\Build\bf2audio.cmd.
#if defined(BF2_IMAGE_BF2AUDIO)
#include "BfOpenAL.h"
#include "SoundTrace.h"
#include "Win32System.h"

namespace dice {
namespace hfe {

PFN_alGetInteger           al_GetInteger = 0;
PFN_alGetString            al_GetString = 0;
PFN_alGetError             al_GetError = 0;
PFN_alIsExtensionPresent   al_IsExtensionPresent = 0;
PFN_alGetProcAddress       al_GetProcAddress = 0;
PFN_alGetEnumValue         al_GetEnumValue = 0;
PFN_alListenerf            al_Listenerf = 0;
PFN_alListenerfv           al_Listenerfv = 0;
PFN_alListener3f           al_Listener3f = 0;
PFN_alGetListenerfv        al_GetListenerfv = 0;
PFN_alGenSources           al_GenSources = 0;
PFN_alDeleteSources        al_DeleteSources = 0;
PFN_alSourcei              al_Sourcei = 0;
PFN_alSourcef              al_Sourcef = 0;
PFN_alSourcefv             al_Sourcefv = 0;
PFN_alGetSourcei           al_GetSourcei = 0;
PFN_alGetSourcef           al_GetSourcef = 0;
PFN_alGetSourcefv          al_GetSourcefv = 0;
PFN_alSourcePlay           al_SourcePlay = 0;
PFN_alSourceStop           al_SourceStop = 0;
PFN_alSourceRewind         al_SourceRewind = 0;
PFN_alGenBuffers           al_GenBuffers = 0;
PFN_alDeleteBuffers        al_DeleteBuffers = 0;
PFN_alBufferData           al_BufferData = 0;
PFN_alGetBufferi           al_GetBufferi = 0;
PFN_alSourceUnqueueBuffers al_SourceUnqueueBuffers = 0;
PFN_alSourceQueueBuffers   al_SourceQueueBuffers = 0;
PFN_alDopplerFactor        al_DopplerFactor = 0;
PFN_alDistanceModel        al_DistanceModel = 0;
PFN_alcCreateContext       alc_CreateContext = 0;
PFN_alcMakeContextCurrent  alc_MakeContextCurrent = 0;
PFN_alcDestroyContext      alc_DestroyContext = 0;
PFN_alcGetError            alc_GetError = 0;
PFN_alcOpenDevice          alc_OpenDevice = 0;
PFN_alcCloseDevice         alc_CloseDevice = 0;
PFN_alcIsExtensionPresent  alc_IsExtensionPresent = 0;
PFN_alcGetString           alc_GetString = 0;
PFN_alcGetIntegerv         alc_GetIntegerv = 0;
PFN_EAXSetBufferMode       eax_SetBufferMode = 0;
int                        eax_StorageAutomatic = 0;
int                        eax_StorageAccessible = 0;
int                        g_openALFlags = 0;
PFN_EAXSet                 eax_Set = 0;
PFN_EAXGet                 eax_Get = 0;
bool                       g_eaxAvailable = false;

const EAXGUID g_eaxListenerGuid = { 0xA8FA6882, 0xB476, 0x11D3, { 0xBD, 0xB9, 0x00, 0xC0, 0xF0, 0x2D, 0xDF, 0x87 } };
const EAXGUID g_eaxBufferGuid   = { 0xA8FA6881, 0xB476, 0x11D3, { 0xBD, 0xB9, 0x00, 0xC0, 0xF0, 0x2D, 0xDF, 0x87 } };

namespace {

void*    s_openALModule = 0;        // ds:1004641C (hModule)
bool     s_allResolved  = false;    // ds:10046420

// the EAX-RAM stand-in until detectEax5 finds the real EAXSetBufferMode - accepts
// the three arguments and does nothing. The DLL compares eax_SetBufferMode against this function
// to know whether EAX-RAM has already been hooked up.
ALenum __cdecl setBufferModeNoop(ALsizei, ALuint*, ALint) { return 0; }

} // namespace

void genBuffersAutomatic(int n, ALuint* out)
{
    al_GenBuffers(n, out);
    eax_SetBufferMode(n, out, eax_StorageAutomatic);
}

void genBuffersAccessible(int n, ALuint* out)
{
    al_GenBuffers(n, out);
    eax_SetBufferMode(n, out, eax_StorageAccessible);
}

int getOpenALFlags()          { return g_openALFlags; }
int setOpenALFlags(int flags) { g_openALFlags = flags; return flags; }

// Returns true when OpenAL reports no error; a null context is treated as an empty string.
bool checkOpenALError(const char* what)
{
    const ALenum err = al_GetError();
    if (!err)
        return true;
    if (!what)
        what = "";
    const char* desc = al_GetString(err);
    g_soundTrace->warning(sndFormat("OpenAL Error: %s: %s", what, desc),
                        "C:\\dice\\Projects\\BF2Branches\\Patch_1_50\\Code\\BF2\\Sound\\SoundEngine\\BfOpenAL.cpp", 256);
    return false;
}

// True when the loaded OpenAL exposes EAX 5.0. As a side effect - and only the
// first time, while eax_SetBufferMode is still the no-op - it also picks up EAX-RAM's buffer-storage
// control, so that sample buffers created afterwards can be placed in X-RAM.
bool detectEax5()
{
    if (!al_IsExtensionPresent || !al_IsExtensionPresent("EAX5.0"))
        return false;
    g_soundTrace->info(sndFormat("Detected OpenAL extension EAX5.0"),
                       "C:\\dice\\Projects\\BF2Branches\\Patch_1_50\\Code\\BF2\\Sound\\SoundEngine\\BfOpenAL.cpp", 284);
    if (eax_SetBufferMode == &setBufferModeNoop && al_IsExtensionPresent("EAX-RAM") == 1)
    {
        g_soundTrace->info(sndFormat("Detected OpenAL extension EAX-RAM"),
                           "C:\\dice\\Projects\\BF2Branches\\Patch_1_50\\Code\\BF2\\Sound\\SoundEngine\\BfOpenAL.cpp", 292);
        eax_SetBufferMode     = (PFN_EAXSetBufferMode) al_GetProcAddress("EAXSetBufferMode");
        eax_StorageAutomatic  = al_GetEnumValue("AL_STORAGE_AUTOMATIC");
        eax_StorageAccessible = al_GetEnumValue("AL_STORAGE_ACCESSIBLE");
    }
    return true;
}

// FreeLibrary and forget every pointer. Returns 0 (its only caller uses it as
// the failure value of loadOpenAL).
int unloadOpenAL()
{
    win32::freeLibrary(s_openALModule);
    s_openALModule = 0;
    al_GetInteger = 0;             al_GetString = 0;              al_GetError = 0;
    al_IsExtensionPresent = 0;     al_GetProcAddress = 0;         al_GetEnumValue = 0;
    al_Listenerf = 0;              al_Listenerfv = 0;             al_Listener3f = 0;
    al_GetListenerfv = 0;          al_GenSources = 0;             al_DeleteSources = 0;
    al_Sourcei = 0;                al_Sourcef = 0;                al_Sourcefv = 0;
    al_GetSourcei = 0;             al_GetSourcef = 0;             al_GetSourcefv = 0;
    al_SourcePlay = 0;             al_SourceStop = 0;             al_SourceRewind = 0;
    al_GenBuffers = 0;             al_DeleteBuffers = 0;          al_BufferData = 0;
    al_GetBufferi = 0;             al_SourceUnqueueBuffers = 0;   al_SourceQueueBuffers = 0;
    al_DopplerFactor = 0;          al_DistanceModel = 0;          alc_CreateContext = 0;
    alc_MakeContextCurrent = 0;    alc_DestroyContext = 0;        alc_GetError = 0;
    alc_OpenDevice = 0;            alc_CloseDevice = 0;           alc_IsExtensionPresent = 0;
    alc_GetString = 0;             alc_GetIntegerv = 0;           eax_SetBufferMode = 0;
    return 0;
}

// One LoadLibraryA and thirty-nine GetProcAddress calls, each clearing the
// "all resolved" flag on a miss; a single miss unloads the library again. EAX-RAM starts out as the
// no-op so genBuffersAutomatic/Accessible can be called unconditionally.
bool loadOpenAL(const char* fileName)
{
    s_openALModule = win32::loadLibrary(fileName);
    if (!s_openALModule)
        return false;
    s_allResolved = true;
#define BF2_AL_RESOLVE(var, type, name) \
    var = (type) win32::getProcAddress(s_openALModule, name); \
    if (!var) s_allResolved = false;
    BF2_AL_RESOLVE(al_GetInteger,           PFN_alGetInteger,           "alGetInteger")
    BF2_AL_RESOLVE(al_GetString,            PFN_alGetString,            "alGetString")
    BF2_AL_RESOLVE(al_GetError,             PFN_alGetError,             "alGetError")
    BF2_AL_RESOLVE(al_IsExtensionPresent,   PFN_alIsExtensionPresent,   "alIsExtensionPresent")
    BF2_AL_RESOLVE(al_GetProcAddress,       PFN_alGetProcAddress,       "alGetProcAddress")
    BF2_AL_RESOLVE(al_GetEnumValue,         PFN_alGetEnumValue,         "alGetEnumValue")
    BF2_AL_RESOLVE(al_Listenerf,            PFN_alListenerf,            "alListenerf")
    BF2_AL_RESOLVE(al_Listenerfv,           PFN_alListenerfv,           "alListenerfv")
    BF2_AL_RESOLVE(al_Listener3f,           PFN_alListener3f,           "alListener3f")
    BF2_AL_RESOLVE(al_GetListenerfv,        PFN_alGetListenerfv,        "alGetListenerfv")
    BF2_AL_RESOLVE(al_GenSources,           PFN_alGenSources,           "alGenSources")
    BF2_AL_RESOLVE(al_DeleteSources,        PFN_alDeleteSources,        "alDeleteSources")
    BF2_AL_RESOLVE(al_Sourcei,              PFN_alSourcei,              "alSourcei")
    BF2_AL_RESOLVE(al_Sourcef,              PFN_alSourcef,              "alSourcef")
    BF2_AL_RESOLVE(al_Sourcefv,             PFN_alSourcefv,             "alSourcefv")
    BF2_AL_RESOLVE(al_GetSourcei,           PFN_alGetSourcei,           "alGetSourcei")
    BF2_AL_RESOLVE(al_GetSourcef,           PFN_alGetSourcef,           "alGetSourcef")
    BF2_AL_RESOLVE(al_GetSourcefv,          PFN_alGetSourcefv,          "alGetSourcefv")
    BF2_AL_RESOLVE(al_SourcePlay,           PFN_alSourcePlay,           "alSourcePlay")
    BF2_AL_RESOLVE(al_SourceStop,           PFN_alSourceStop,           "alSourceStop")
    BF2_AL_RESOLVE(al_SourceRewind,         PFN_alSourceRewind,         "alSourceRewind")
    BF2_AL_RESOLVE(al_GenBuffers,           PFN_alGenBuffers,           "alGenBuffers")
    BF2_AL_RESOLVE(al_DeleteBuffers,        PFN_alDeleteBuffers,        "alDeleteBuffers")
    BF2_AL_RESOLVE(al_BufferData,           PFN_alBufferData,           "alBufferData")
    BF2_AL_RESOLVE(al_GetBufferi,           PFN_alGetBufferi,           "alGetBufferi")
    BF2_AL_RESOLVE(al_SourceUnqueueBuffers, PFN_alSourceUnqueueBuffers, "alSourceUnqueueBuffers")
    BF2_AL_RESOLVE(al_SourceQueueBuffers,   PFN_alSourceQueueBuffers,   "alSourceQueueBuffers")
    BF2_AL_RESOLVE(al_DopplerFactor,        PFN_alDopplerFactor,        "alDopplerFactor")
    BF2_AL_RESOLVE(al_DistanceModel,        PFN_alDistanceModel,        "alDistanceModel")
    BF2_AL_RESOLVE(alc_CreateContext,       PFN_alcCreateContext,       "alcCreateContext")
    BF2_AL_RESOLVE(alc_MakeContextCurrent,  PFN_alcMakeContextCurrent,  "alcMakeContextCurrent")
    BF2_AL_RESOLVE(alc_DestroyContext,      PFN_alcDestroyContext,      "alcDestroyContext")
    BF2_AL_RESOLVE(alc_GetError,            PFN_alcGetError,            "alcGetError")
    BF2_AL_RESOLVE(alc_OpenDevice,          PFN_alcOpenDevice,          "alcOpenDevice")
    BF2_AL_RESOLVE(alc_CloseDevice,         PFN_alcCloseDevice,         "alcCloseDevice")
    BF2_AL_RESOLVE(alc_IsExtensionPresent,  PFN_alcIsExtensionPresent,  "alcIsExtensionPresent")
    BF2_AL_RESOLVE(alc_GetString,           PFN_alcGetString,           "alcGetString")
    BF2_AL_RESOLVE(alc_GetIntegerv,         PFN_alcGetIntegerv,         "alcGetIntegerv")
#undef BF2_AL_RESOLVE
    eax_SetBufferMode = &setBufferModeNoop;
    if (!s_allResolved)
        unloadOpenAL();
    return s_allResolved;
}

} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_BF2AUDIO
