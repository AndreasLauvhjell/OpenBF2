// Device/context provider owned by BfSoundEngine. [INFERRED] header and interface names.
// Routes sample creation, listener properties, gain and reverb through the 15-slot interface.
// Open returns 0 on success, 1 for an unknown provider, 2 for a missing library,
// 3 when system OpenAL refuses EAX 5, and 4 for other device/context failures.
// Slot +0x20 is [UNRESOLVED] and returns zero; setAirAbsorption is an [INFERRED] name.

#ifndef SOUND_SOUNDENGINE_OPENALPROVIDER_H
#define SOUND_SOUNDENGINE_OPENALPROVIDER_H

#include "BfOpenAL.h"
#include "../../Math/Vec3.h"

#include <string>
#include <vector>

namespace dice {
namespace hfe {

class Sample;
class SoundTemplate;

class ISoundProvider
{
public:
    virtual ~ISoundProvider() {}                                                             // +0x00
    virtual Sample** createSample(Sample** out, SoundTemplate* t, int index, const Vec3* pos) = 0; // +0x04
    virtual int   getProviderNames(std::vector<const char*>* out) = 0;                       // +0x08
    virtual int   open(const std::string& name) = 0;                                         // +0x0C
    virtual void  close() = 0;                                                               // +0x10
    virtual void  setListenerVelocity(const Vec3* v) = 0;                                    // +0x14
    virtual void  setMasterVolume(float gain) = 0;                                           // +0x18
    virtual void  updateReverb(const EAXLISTENERPROPERTIES* p) = 0;                          // +0x1C
    virtual int   unresolvedSlot8() = 0;                                                     // +0x20 [UNRESOLVED]
    virtual void  setDopplerFactor(float f) = 0;                                             // +0x24
    virtual float getDopplerFactor() = 0;                                                    // +0x28
    virtual void  setAirAbsorption(float dbPerMeter) = 0;                                    // +0x2C [INFERRED name]
    virtual int   getEaxVersion() = 0;                                                       // +0x30
    virtual int   getMaxVoices() = 0;                                                        // +0x34
    virtual const char* getInfo() = 0;                                                       // +0x38
};

// Built-in provider record. deviceName is passed to OpenAL; displayName is used by settings and open().
struct ProviderInfo
{
    std::string deviceName;     // +0x00 "DirectSound" / "DirectSound3D" / "Default Device" / "MMSYSTEM"
    std::string displayName;    // +0x1C "Software Audio + EAX" ...
    bool        systemOpenAL;   // +0x38 load OpenAL32.dll instead of BF2OpenAL.dll
    bool        wantEax;        // +0x39 probe EAX 3.0 after the context is up
    bool        useSourcePool;  // +0x3A > property "use-source-pool"
    bool        use2dOcclusion; // +0x3B > property "use-2d-occlusion"
};

// OpenAL-backed sound provider.
class OpenALProvider : public ISoundProvider
{
public:
    OpenALProvider();
    virtual ~OpenALProvider();

    virtual Sample** createSample(Sample** out, SoundTemplate* t, int index, const Vec3* pos);
    virtual int   getProviderNames(std::vector<const char*>* out);
    virtual int   open(const std::string& name);
    virtual void  close();
    virtual void  setListenerVelocity(const Vec3* v);
    virtual void  setMasterVolume(float gain);
    virtual void  updateReverb(const EAXLISTENERPROPERTIES* p);
    virtual int   unresolvedSlot8() { return 0; }
    virtual void  setDopplerFactor(float f);
    virtual float getDopplerFactor() { return m_dopplerFactor; }
    virtual void  setAirAbsorption(float) {}                         // `retn 4`
    virtual int   getEaxVersion() { return m_eaxVersion; }
    virtual int   getMaxVoices() { return m_maxVoices; }
    virtual const char* getInfo() { return m_info.c_str(); }

    // `new(0x80) OpenALProvider` with the constructor inlined; null when the allocation
    // fails. The engine's constructor stores the result at +0x30.
    static OpenALProvider* create();

private:
    void  addProviders();                   // the seven rows
    bool  countVoices();                    // alGenSources until it fails, max 128
    void  detectEax();                      // EAX3.0 / EAX3.0Emulated, EAXSet/EAXGet

    /* +0x04 */ // the vector's allocator word
    /* +0x08 */ std::vector<ProviderInfo> m_providers;
    /* +0x14 */ ALCdevice*  m_device;
    /* +0x18 */ std::string m_deviceName;   // the displayName currently open; open() is idempotent on it
    /* +0x34 */ ALCcontext* m_context;
    /* +0x38 */ std::string m_info;         // "<display> (<alcGetString(DEVICE_SPECIFIER)>)" + source line
    /* +0x54 */ std::string m_unused;       // constructed, destroyed, never assigned [UNRESOLVED]
    /* +0x70 */ float       m_dopplerFactor;
    /* +0x74 */ bool        m_reverbEnabled;
    /* +0x78 */ int         m_maxVoices;
    /* +0x7C */ int         m_eaxVersion;
};

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_OPENALPROVIDER_H
