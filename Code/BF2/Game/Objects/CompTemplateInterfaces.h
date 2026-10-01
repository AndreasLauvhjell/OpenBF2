// Template-component interfaces; preserve declaration order and the absence of virtual destructors.
// [INFERRED] grouping. [UNRESOLVED] accessor ownership remains represented by marker interfaces.

#ifndef GAME_OBJECTS_COMPTEMPLATEINTERFACES_H     // [INFERRED] guard style
#define GAME_OBJECTS_COMPTEMPLATEINTERFACES_H

#include "IObjectTemplateComponent.h"

namespace dice {
namespace hfe {
namespace Math { struct Vec3; }     // IIkTemplateComponent's accessors
typedef Math::Vec3 Vec3;
struct Mat4;
namespace world {

// the two with multi-implementor evidence

// Weapon-side component contract. Adds ONE slot over IObjectTemplateComponent:
class IWeaponCompTemplate : public IObjectTemplateComponent
{
public:
    // NON-pure: one shared body at ELF 0x8259070 (5 B - empty), inherited by all eight roots.
    // The void return type is corroborated by the overriding LPTargetCompTemplate call sites.
    virtual void preCache() const {}                     // +0x2C
};

// Projectile-component marker interface; adds no virtual slots.
class IProjectileCompTemplate : public IObjectTemplateComponent
{
};

// the twelve single-implementor markers
// Empty by evidence policy (see header block), NOT because they are known to be empty.

// Left and right inverse-kinematics targets. Position, matrix and pole getters return references.
// Euler getters return negated ZXY angles by value.
class IIkTemplateComponent         : public IObjectTemplateComponent
{
public:
    virtual void        setPositionLeft(const Vec3&)        = 0;   // +0x2C
    virtual const Vec3& getPositionLeft() const             = 0;   // +0x30
    virtual void        setRotationLeft(const Vec3&)        = 0;   // +0x34
    virtual Vec3        getRotationLeft() const             = 0;   // +0x38
    virtual const Mat4& getRotationLeftMat() const          = 0;   // +0x3C
    virtual void        setPoleVectorLeft(const Vec3&)      = 0;   // +0x40
    virtual const Vec3& getPoleVectorLeft() const           = 0;   // +0x44
    virtual void        setPositionRight(const Vec3&)       = 0;   // +0x48
    virtual const Vec3& getPositionRight() const            = 0;   // +0x4C
    virtual void        setRotationRight(const Vec3&)       = 0;   // +0x50
    virtual Vec3        getRotationRight() const            = 0;   // +0x54
    virtual const Mat4& getRotationRightMat() const         = 0;   // +0x58
    virtual void        setPoleVectorRight(const Vec3&)     = 0;   // +0x5C
    virtual const Vec3& getPoleVectorRight() const          = 0;   // +0x60
};
class IAbilityGLCompTemplate       : public IObjectTemplateComponent {};
class IAbilityHudCompTemplate      : public IObjectTemplateComponent {};
class IArmorGLCompTemplate         : public IObjectTemplateComponent {};
class IHelpHudCompTemplate         : public IObjectTemplateComponent {};
class IPostProcessCompTemplate     : public IObjectTemplateComponent {};
class IRadioGLCompTemplate         : public IObjectTemplateComponent {};
class IStrategicObjectCompTemplate : public IObjectTemplateComponent {};
class IVehicleHudCompTemplate      : public IObjectTemplateComponent {};
class IWarningHudCompTemplate      : public IObjectTemplateComponent {};
class IWeaponHudCompTemplate       : public IObjectTemplateComponent {};

// Not a pure marker - implements getInterfaceID (body at the interface, inherited by the
// concrete class; see the exception note above).
class IWindAffectorTemplate : public IObjectTemplateComponent
{
public:
    virtual unsigned int getInterfaceID();               // ELF body: returns its IID
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_COMPTEMPLATEINTERFACES_H
