// Path:     [INFERRED] - no __FILE__ string attests the interface's source home. It is kept beside
// IObjectComponent.h and the WindAffector template/runtime pair whose primary face it is.
// Contents: recovered from the symbolised BF2 1.50 ELF and cross-checked against bf2_w32ded.exe.
// The seven WindAffector-specific operations occupy +0x38..+0x50 after IObjectComponent. The
// destructor follows them, then getClassID. MSVC emits one scalar-deleting-destructor slot where
// the Itanium ABI emits D1/D0; keeping every call typed lets the compiler absorb that platform
// difference.

#ifndef GAME_OBJECTS_IWINDAFFECTOR_H
#define GAME_OBJECTS_IWINDAFFECTOR_H

#include "IObjectComponent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

class IWindAffector : public IObjectComponent
{
public:
    virtual void        update(float deltaTime) = 0;                         // +0x38
    virtual bool        getIsActive() const = 0;                             // +0x3C
    virtual void        setIsActive(bool active) = 0;                        // +0x40
    virtual float       getBoundingRadius() const = 0;                       // +0x44
    virtual const Vec3& getAbsolutePosition() const = 0;                     // +0x48
    virtual void        getAbsoluteWindDirectionAndSpeed(
                            const Vec3& position, Vec3& direction,
                            float& speed) const = 0;                          // +0x4C
    virtual Vec3        getAbsoluteWind(const Vec3& position) const = 0;      // +0x50

    virtual ~IWindAffector() {}                                               // +0x54/+0x58 ELF
    virtual unsigned int getClassID() const = 0;                             // +0x5C +0x58 PE
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IWINDAFFECTOR_H
