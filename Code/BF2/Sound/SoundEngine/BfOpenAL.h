
// Loads OpenAL dynamically and resolves the required entry points by name.


#ifndef SOUND_SOUNDENGINE_BFOPENAL_H
#define SOUND_SOUNDENGINE_BFOPENAL_H

namespace dice {
namespace hfe {

// OpenAL 1.x types and the enumerators this image uses
typedef char           ALboolean;
typedef char           ALchar;
typedef int            ALint;
typedef unsigned int   ALuint;
typedef int            ALsizei;
typedef int            ALenum;
typedef float          ALfloat;
typedef void           ALvoid;
typedef struct ALCdevice_struct  ALCdevice;
typedef struct ALCcontext_struct ALCcontext;

enum
{
    AL_NONE                     = 0,
    AL_CONE_INNER_ANGLE         = 0x1001,   // 4097  OpenALSample::applyCone
    AL_CONE_OUTER_ANGLE         = 0x1002,   // 4098
    AL_PITCH                    = 0x1003,   // 4099
    AL_POSITION                 = 0x1004,   // 4100
    AL_DIRECTION                = 0x1005,   // 4101
    AL_VELOCITY                 = 0x1006,   // 4102
    AL_LOOPING                  = 0x1007,   // 4103
    AL_BUFFER                   = 0x1009,   // 4105
    AL_GAIN                     = 0x100A,   // 4106
    AL_ORIENTATION              = 0x100F,   // 4111
    AL_SOURCE_STATE             = 0x1010,   // 4112
    AL_INITIAL                  = 0x1011,   // 4113
    AL_PLAYING                  = 0x1012,   // 4114
    AL_PAUSED                   = 0x1013,   // 4115
    AL_STOPPED                  = 0x1014,   // 4116
    AL_BUFFERS_QUEUED           = 0x1015,   // 4117
    AL_BUFFERS_PROCESSED        = 0x1016,   // 4118
    AL_REFERENCE_DISTANCE       = 0x1020,   // 4128
    AL_ROLLOFF_FACTOR           = 0x1021,   // 4129
    AL_CONE_OUTER_GAIN          = 0x1022,   // 4130
    AL_FORMAT_MONO16            = 0x1101,   // 4353
    AL_FORMAT_STEREO16          = 0x1103,   // 4355
    AL_INVERSE_DISTANCE_CLAMPED = 0xD002,   // 53250
    ALC_FREQUENCY               = 0x1007,   // 4103 - alcCreateContext attribute
    ALC_DEVICE_SPECIFIER        = 0x1005    // 4101 - alcGetString
};

// EAX 3.0 listener/buffer property GUIDs and parameter layouts.
struct EAXGUID { unsigned int Data1; unsigned short Data2; unsigned short Data3; unsigned char Data4[8]; };
extern const EAXGUID g_eaxListenerGuid;
extern const EAXGUID g_eaxBufferGuid;
enum { EAX_ALLPARAMETERS = 1 };

struct EAXVECTOR { float x, y, z; };

struct EAXLISTENERPROPERTIES                // 112 bytes - EAXSet(listener, 1, 0, &p, 112)
{
    unsigned long ulEnvironment;
    float         flEnvironmentSize;
    float         flEnvironmentDiffusion;
    long          lRoom;
    long          lRoomHF;
    long          lRoomLF;
    float         flDecayTime;
    float         flDecayHFRatio;
    float         flDecayLFRatio;
    long          lReflections;
    float         flReflectionsDelay;
    EAXVECTOR     vReflectionsPan;
    long          lReverb;
    float         flReverbDelay;
    EAXVECTOR     vReverbPan;
    float         flEchoTime;
    float         flEchoDepth;
    float         flModulationTime;
    float         flModulationDepth;
    float         flAirAbsorptionHF;
    float         flHFReference;
    float         flLFReference;
    float         flRoomRolloffFactor;
    unsigned long ulFlags;
};

struct EAXBUFFERPROPERTIES                  // 72 bytes - EAXSet(buffer, 1, source, &p, 72)
{
    long          lDirect;
    long          lDirectHF;
    long          lRoom;
    long          lRoomHF;
    long          lObstruction;
    float         flObstructionLFRatio;
    long          lOcclusion;
    float         flOcclusionLFRatio;
    float         flOcclusionRoomRatio;
    float         flOcclusionDirectRatio;
    long          lExclusion;
    float         flExclusionLFRatio;
    long          lOutsideVolumeHF;
    float         flDopplerFactor;
    float         flRolloffFactor;
    float         flRoomRolloffFactor;
    float         flAirAbsorptionFactor;
    unsigned long ulFlags;
};

// The resolved entry points, one global each, in loadOpenAL's order (ds:10046378..)
typedef ALint     (__cdecl* PFN_alGetInteger)(ALenum);
typedef const ALchar* (__cdecl* PFN_alGetString)(ALenum);
typedef ALenum    (__cdecl* PFN_alGetError)();
typedef ALboolean (__cdecl* PFN_alIsExtensionPresent)(const ALchar*);
typedef void*     (__cdecl* PFN_alGetProcAddress)(const ALchar*);
typedef ALenum    (__cdecl* PFN_alGetEnumValue)(const ALchar*);
typedef void      (__cdecl* PFN_alListenerf)(ALenum, ALfloat);
typedef void      (__cdecl* PFN_alListenerfv)(ALenum, const ALfloat*);
typedef void      (__cdecl* PFN_alListener3f)(ALenum, ALfloat, ALfloat, ALfloat);
typedef void      (__cdecl* PFN_alGetListenerfv)(ALenum, ALfloat*);
typedef void      (__cdecl* PFN_alGenSources)(ALsizei, ALuint*);
typedef void      (__cdecl* PFN_alDeleteSources)(ALsizei, const ALuint*);
typedef void      (__cdecl* PFN_alSourcei)(ALuint, ALenum, ALint);
typedef void      (__cdecl* PFN_alSourcef)(ALuint, ALenum, ALfloat);
typedef void      (__cdecl* PFN_alSourcefv)(ALuint, ALenum, const ALfloat*);
typedef void      (__cdecl* PFN_alGetSourcei)(ALuint, ALenum, ALint*);
typedef void      (__cdecl* PFN_alGetSourcef)(ALuint, ALenum, ALfloat*);
typedef void      (__cdecl* PFN_alGetSourcefv)(ALuint, ALenum, ALfloat*);
typedef void      (__cdecl* PFN_alSourcePlay)(ALuint);
typedef void      (__cdecl* PFN_alSourceStop)(ALuint);
typedef void      (__cdecl* PFN_alSourceRewind)(ALuint);
typedef void      (__cdecl* PFN_alGenBuffers)(ALsizei, ALuint*);
typedef void      (__cdecl* PFN_alDeleteBuffers)(ALsizei, const ALuint*);
typedef void      (__cdecl* PFN_alBufferData)(ALuint, ALenum, const ALvoid*, ALsizei, ALsizei);
typedef void      (__cdecl* PFN_alGetBufferi)(ALuint, ALenum, ALint*);
typedef void      (__cdecl* PFN_alSourceUnqueueBuffers)(ALuint, ALsizei, ALuint*);
typedef void      (__cdecl* PFN_alSourceQueueBuffers)(ALuint, ALsizei, const ALuint*);
typedef void      (__cdecl* PFN_alDopplerFactor)(ALfloat);
typedef void      (__cdecl* PFN_alDistanceModel)(ALenum);
typedef ALCcontext* (__cdecl* PFN_alcCreateContext)(ALCdevice*, const ALint*);
typedef ALboolean (__cdecl* PFN_alcMakeContextCurrent)(ALCcontext*);
typedef void      (__cdecl* PFN_alcDestroyContext)(ALCcontext*);
typedef ALenum    (__cdecl* PFN_alcGetError)(ALCdevice*);
typedef ALCdevice* (__cdecl* PFN_alcOpenDevice)(const ALchar*);
typedef ALboolean (__cdecl* PFN_alcCloseDevice)(ALCdevice*);
typedef ALboolean (__cdecl* PFN_alcIsExtensionPresent)(ALCdevice*, const ALchar*);
typedef const ALchar* (__cdecl* PFN_alcGetString)(ALCdevice*, ALenum);
typedef void      (__cdecl* PFN_alcGetIntegerv)(ALCdevice*, ALenum, ALsizei, ALint*);
// EAX-RAM's alGenBuffers replacement takes (n, buffers, storageMode).
typedef ALenum    (__cdecl* PFN_EAXSetBufferMode)(ALsizei, ALuint*, ALint);
// EAXSet/EAXGet(propertySetId, property, source, value, size).
typedef ALenum    (__cdecl* PFN_EAXSet)(const EAXGUID*, ALuint, ALuint, ALvoid*, ALuint);
typedef ALenum    (__cdecl* PFN_EAXGet)(const EAXGUID*, ALuint, ALuint, ALvoid*, ALuint);

extern PFN_alGetInteger           al_GetInteger;              // ds:10046378
extern PFN_alGetString            al_GetString;               // ds:1004637C
extern PFN_alGetError             al_GetError;                // ds:10046380
extern PFN_alIsExtensionPresent   al_IsExtensionPresent;      // ds:10046384
extern PFN_alGetProcAddress       al_GetProcAddress;          // ds:10046388
extern PFN_alGetEnumValue         al_GetEnumValue;            // ds:1004638C
extern PFN_alListenerf            al_Listenerf;               // ds:10046390
extern PFN_alListenerfv           al_Listenerfv;              // ds:10046394
extern PFN_alListener3f           al_Listener3f;              // ds:10046398
extern PFN_alGetListenerfv        al_GetListenerfv;           // ds:1004639C
extern PFN_alGenSources           al_GenSources;              // ds:100463A0
extern PFN_alDeleteSources        al_DeleteSources;           // ds:100463A4
extern PFN_alSourcei              al_Sourcei;                 // ds:100463A8
extern PFN_alSourcef              al_Sourcef;                 // ds:100463AC
extern PFN_alSourcefv             al_Sourcefv;                // ds:100463B0
extern PFN_alGetSourcei           al_GetSourcei;              // ds:100463B4
extern PFN_alGetSourcef           al_GetSourcef;              // ds:100463B8
extern PFN_alGetSourcefv          al_GetSourcefv;             // ds:100463BC
extern PFN_alSourcePlay           al_SourcePlay;              // ds:100463C0
extern PFN_alSourceStop           al_SourceStop;              // ds:100463C4
extern PFN_alSourceRewind         al_SourceRewind;            // ds:100463C8
extern PFN_alGenBuffers           al_GenBuffers;              // ds:100463CC
extern PFN_alDeleteBuffers        al_DeleteBuffers;           // ds:100463D0
extern PFN_alBufferData           al_BufferData;              // ds:100463D4
extern PFN_alGetBufferi           al_GetBufferi;              // ds:100463D8
extern PFN_alSourceUnqueueBuffers al_SourceUnqueueBuffers;    // ds:100463DC
extern PFN_alSourceQueueBuffers   al_SourceQueueBuffers;      // ds:100463E0
extern PFN_alDopplerFactor        al_DopplerFactor;           // ds:100463E4
extern PFN_alDistanceModel        al_DistanceModel;           // ds:100463E8
extern PFN_alcCreateContext       alc_CreateContext;          // ds:100463EC
extern PFN_alcMakeContextCurrent  alc_MakeContextCurrent;     // ds:100463F0
extern PFN_alcDestroyContext      alc_DestroyContext;         // ds:100463F4
extern PFN_alcGetError            alc_GetError;               // ds:100463F8
extern PFN_alcOpenDevice          alc_OpenDevice;             // ds:100463FC
extern PFN_alcCloseDevice         alc_CloseDevice;            // ds:10046400
extern PFN_alcIsExtensionPresent  alc_IsExtensionPresent;     // ds:10046404
extern PFN_alcGetString           alc_GetString;              // ds:10046408
extern PFN_alcGetIntegerv         alc_GetIntegerv;            // ds:1004640C
extern PFN_EAXSetBufferMode       eax_SetBufferMode;
// Uses the fallback until EAX-RAM is available.
extern int                        eax_StorageAutomatic;       // ds:AL_STORAGE_AUTOMATIC
extern int                        eax_StorageAccessible;      // ds:AL_STORAGE_ACCESSIBLE
extern int                        g_openALFlags;              // ds:bit 0 = system OpenAL
extern PFN_EAXSet                 eax_Set;                    // ds:OpenALProvider::detectEax
extern PFN_EAXGet                 eax_Get;                    // ds:1004642C
extern bool                       g_eaxAvailable;             // ds:EAX3.0 (or Emulated)

// [CHOSEN, not recovered] Not in retail. AL_SOFT_direct_channels (OpenAL Soft) lets 2D sources
// with multi-channel buffers - the music stream and stereo 2D samples - play straight to the
// output channels instead of being virtualised as speakers, which matters under HRTF. The flag is
// set by OpenALProvider::open only when the loaded OpenAL reports the extension, so the retail
// BF2OpenAL.dll path never receives the property. Issue: pending.
const ALenum AL_DIRECT_CHANNELS_SOFT = 0x1033;
extern bool                       g_directChannelsAvailable;
// Sets AL_DIRECT_CHANNELS_SOFT on `source` when the extension is available; no-op otherwise.
void setDirectChannels(ALuint source, bool enable);

// BfOpenAL.cpp's own functions
bool loadOpenAL(const char* fileName);      // LoadLibrary + the 39 GetProcAddress calls
int  unloadOpenAL();                        // FreeLibrary and null every pointer
// alGetError(); nothing pending -> true. Otherwise report "OpenAL Error: <what>: <alGetString>"
// through the trace error slot (as BfOpenAL.cpp line 256) and return false. `what` may be null.
bool checkOpenALError(const char* what);
bool detectEax5();                          // also switches on EAX-RAM buffer storage
int  getOpenALFlags();                      // returns g_openALFlags
int  setOpenALFlags(int flags);
// alGenBuffers(n, out) then eax_SetBufferMode(n, out, mode). The first
// asks for AL_STORAGE_AUTOMATIC (static samples), the second for AL_STORAGE_ACCESSIBLE (streams).
void genBuffersAutomatic(int n, ALuint* out);
void genBuffersAccessible(int n, ALuint* out);

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_BFOPENAL_H
