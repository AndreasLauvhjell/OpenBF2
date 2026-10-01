// Interface for script-defined sound object properties.

#ifndef SOUND_ISOUNDOBJECTTEMPLATE_H     // [INFERRED] guard style
#define SOUND_ISOUNDOBJECTTEMPLATE_H

#include <string>
#include "../Core/IBase.h"

namespace dice {
namespace hfe {

struct Mat4;                           // dice::hfe::Mat4 (Math/Mat4.h) - hfe scope, unlike Vec3
class ISoundTemplate;                  // NullSound.h - the engine-side template this one wraps

class ISoundObjectTemplate : public IBase
{
public:
    // The IBase quartet occupies +0x00..+0x0C (inherited pure). The sound surface:
    virtual void  setIsUsed() = 0;                                           // +0x10
    virtual void  setFilename(const std::string&) = 0;                       // +0x14
    virtual std::string getFilename() const = 0;                             // +0x18 by-value
    virtual void  setVolumeEnvelope(const std::string&) = 0;                 // +0x1C
    virtual std::string getVolumeEnvelope() const = 0;                       // +0x20 by-value
    virtual void  setPitchEnvelope(const std::string&) = 0;                  // +0x24
    virtual std::string getPitchEnvelope() const = 0;                        // +0x28 by-value
    virtual void  setLoopCount(int) = 0;                                     // +0x2C
    virtual int   getLoopCount() const = 0;                                  // +0x30
    virtual void  setVolume(float) = 0;                                      // +0x34
    virtual float getVolume() const = 0;                                     // +0x38
    virtual void  setPitch(float) = 0;                                       // +0x3C
    virtual float getPitch() const = 0;                                      // +0x40
    virtual void  setMinDistance(float) = 0;                                 // +0x44
    virtual float getMinDistance() const = 0;                                // +0x48
    virtual void  setHalfVolumeDistance(float) = 0;                          // +0x4C
    virtual float getHalfVolumeDistance() const = 0;                         // +0x50
    virtual void  setReverbLevel(float) = 0;                                 // +0x54
    virtual float getReverbLevel() const = 0;                                // +0x58
    virtual void  setSoundRadius(float) = 0;                                 // +0x5C
    virtual float getSoundRadius() const = 0;                                // +0x60
    virtual void  setLowSamples(int) = 0;                                    // +0x64
    virtual int   getLowSamples() const = 0;                                 // +0x68
    virtual void  setMediumSamples(int) = 0;                                 // +0x6C
    virtual int   getMediumSamples() const = 0;                              // +0x70
    virtual void  setRelativeTransformation(const Mat4*) = 0;                // +0x74
    virtual ISoundTemplate* impl() const = 0;                                // +0x78
};

} // namespace hfe
} // namespace dice

#endif
