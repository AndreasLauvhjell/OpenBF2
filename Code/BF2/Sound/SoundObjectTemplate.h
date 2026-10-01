// Object-template wrapper for engine sounds, registered as the console's Sound template.
// [INFERRED] header placement. Keep ObjectTemplate followed by ISoundObjectTemplate.
// The implementation comes from the active sound engine; dedicated servers use NullSoundTemplate.
// Setters preserve unscaled values locally and forward values multiplied by the fixed gains.
// [CHOSEN name] m_isAmbientAnimSound suppresses setIsUsed for reload/deploy animation sounds.

#ifndef SOUND_SOUNDOBJECTTEMPLATE_H     // [INFERRED] guard style
#define SOUND_SOUNDOBJECTTEMPLATE_H

#include <vector>
#include "NullSound.h"
#include "ISoundObjectTemplate.h"
#include "../Game/Objects/ObjectTemplate.h"

namespace dice {
namespace hfe {

// The free helpers setFilename routes through (SoundObjectTemplate.cpp; both symbols attested).
// setTemplateFilenames splits a comma-list into char* pieces (NUL-punching a static scratch copy
// in place) and hands the array to impl->setFilenames.
void setTemplateFilenames(ISoundTemplate* impl, const std::string& filenames);
// Parses filenames into pointers backed by scratch storage.
void parseTemplateFilenames(std::vector<const char*>* out, std::string* scratch,
                            const std::string& filenames);
void parseTemplateFilenames(std::vector<const char*>* out, std::string* scratch,
                            const std::string& filenames);

class SoundObjectTemplate : public world::ObjectTemplate, public ISoundObjectTemplate
{
public:
    SoundObjectTemplate();                                    // read in full

    // The per-class IBase quartet (the ELF emits it per class; refcount at +0x34).
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                                  // delete on 0
    virtual void* queryInterface(unsigned int iid) const;     // the 9986002 hop

    virtual unsigned int getClassID() const;                  // returns 9986001 (0x985FD1)
    virtual ~SoundObjectTemplate();                           // releases the impl
    virtual void templateInitialize();                        // quality-gated
                                                              // suffix logic; a fast no-op on the
                                                              // null engine (quality 0 != 2)
    virtual bool writeBinary(io::IStream*) const;             // returns false (empty stub)
    virtual bool readBinary(io::IStream*);                    // returns false (empty stub)
    virtual void* queryImplementation(unsigned int classId);  // 9986001/CID_ObjectTemplate
    virtual world::IObject* createObject() const;             // setIsUsed(); returns 0 (a
                                                              // Sound template makes no object)
    virtual void preCache() const;                            // tail-calls setIsUsed()
    virtual bool makeScript(io::IStream*);

    // Sound-object template virtual properties.
    virtual void  setIsUsed();                                // guarded impl fwd + flag 0x20
    virtual void  setFilename(const std::string&);            // setTemplateFilenames(impl,s)
    virtual std::string getFilename() const;
    virtual void  setVolumeEnvelope(const std::string&);
    virtual std::string getVolumeEnvelope() const;
    virtual void  setPitchEnvelope(const std::string&);
    virtual std::string getPitchEnvelope() const;
    virtual void  setLoopCount(int);                          // pure impl forward, no store
    virtual int   getLoopCount() const;                       // impl->getLoopCount()
    virtual void  setVolume(float);                           // store + impl fwd x multiplier
    virtual float getVolume() const;                          // m_volume
    virtual void  setPitch(float);
    virtual float getPitch() const;
    virtual void  setMinDistance(float);
    virtual float getMinDistance() const;
    virtual void  setHalfVolumeDistance(float);               // the ~0 default: 2*minDist
    virtual float getHalfVolumeDistance() const;
    virtual void  setReverbLevel(float);
    virtual float getReverbLevel() const;
    virtual void  setSoundRadius(float);
    virtual float getSoundRadius() const;
    virtual void  setLowSamples(int);                         // impl->setMaxSamples(2, n)
    virtual int   getLowSamples() const;                      // impl->getMaxSamples(2)
    virtual void  setMediumSamples(int);                      // impl->setMaxSamples(1, n)
    virtual int   getMediumSamples() const;                   // impl->getMaxSamples(1)
    virtual void  setRelativeTransformation(const Mat4*);     // position/direction columns
    virtual ISoundTemplate* impl() const;                     // returns m_impl

    // Stores multipliers and reapplies each base value through its virtual setter.
    void tweak(float volume, float pitch, float minDistance, float halfVolumeDistance,
               float reverbLevel, float soundRadius);

    int             m_refCount;                               // +0x34 ctor 1
    ISoundTemplate* m_impl;                                   // +0x38
    float           m_volume;                                 // +0x3C (names = their getters)
    float           m_pitch;                                  // +0x40
    float           m_minDistance;                            // +0x44
    float           m_halfVolumeDistance;                     // +0x48
    float           m_reverbLevel;                            // +0x4C
    float           m_soundRadius;                            // +0x50
    float           m_volumeMultiplier;                       // +0x54 ctor 1.0f x6 - engine-side
    float           m_pitchMultiplier;                        // +0x58 scale hooks, written only by
    float           m_minDistanceMultiplier;                  // +0x5C tweak() [CHOSEN names]
    float           m_halfVolumeDistanceMultiplier;           // +0x60
    float           m_reverbLevelMultiplier;                  // +0x64
    float           m_soundRadiusMultiplier;                  // +0x68
    unsigned char   m_isAmbientAnimSound;                     // +0x6C ctor 0 [CHOSEN name]
};

} // namespace hfe
} // namespace dice

#endif
