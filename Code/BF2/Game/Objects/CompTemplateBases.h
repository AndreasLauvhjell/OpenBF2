// Abstract weapon-component template bases. Preserve each base's virtual order and destructor placement.
// TargetCompTemplate provides reference counting but leaves createComponent abstract.
// The projectile family has a different suffix after the shared interface. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_COMPTEMPLATEBASES_H     // [INFERRED] guard style
#define GAME_OBJECTS_COMPTEMPLATEBASES_H

#include <iosfwd>
#include <string>

#include "AmmoCompTemplate.h"
#include "Weapons/DeviationComponents/DeviationCompTemplate.h"
#include "FireCompTemplate.h"
#include "ZoomCompTemplate.h"
#include "Weapons/AnimationComponents/AnimationComp.h"

#include "CompTemplateInterfaces.h"
#include "IObjectTemplateComponent.h"
#include "../../IO/Console/IConsoleSaveable.h"
#include "../../IO/Input/Input.h"

namespace dice {
namespace hfe {
namespace io { class IStream; }
namespace world {

class IObject;
class Object;
class ObjectTemplate;
class IObjectTemplate;
class IObjectComponent;

// Common template-component overrides; reference counting remains abstract except in TargetCompTemplate.
// Each class declares makeScript separately to preserve its position relative to the destructor.
#define BF2_COMP_TEMPLATE_CONTRACT                                          \
    virtual unsigned int getClassID() const;                                \
    virtual unsigned int getInterfaceID();                                  \
    virtual void  objectCreated(IObject*);                                  \
    virtual void  objectDestroyed(IObject*);                                \
    virtual void  setTemplate(IObjectTemplate*);                            \
    virtual void* queryImplementation(unsigned int)

// Preserve virtual order: destructor pair precedes makeScript.
class RecoilCompTemplate : public IWeaponCompTemplate, public io::IConsoleSaveable
{ public:
          RecoilCompTemplate();
          BF2_COMP_TEMPLATE_CONTRACT;
          virtual IObjectComponent* createComponent() = 0;
          virtual ~RecoilCompTemplate();                  // +0x30 +0x34
          virtual bool makeScript(io::IStream*);          // +0x38
};

// Target-system selection.
enum TargetSystem : int
{
    TSNone        = 0,
    TSHeatSeeking = 1,
    TSLaserGuided = 2,
    TSWireGuided  = 3,
    TSTVGuided    = 4,
    TSParabellic  = 5
};

std::ostream& operator<<(std::ostream&, TargetSystem);
std::istream& operator>>(std::istream&, TargetSystem&);

class TargetCompTemplate : public IWeaponCompTemplate, public io::IConsoleSaveable
{ public:
    TargetCompTemplate();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent() = 0;
    virtual void  objectCreated(IObject*);
    virtual void  objectDestroyed(IObject*);
    virtual void  setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual bool  makeScript(io::IStream*);
    // No virtual destructor: the shipped table runs directly into makeScript.

    void setTargetInput(io::PlayerInputMap value) { m_targetInput = value; }
    io::PlayerInputMap getTargetInput() const { return m_targetInput; }

    void setOnLockChangeToWeapon(bool enabled, std::string weaponName);
    bool getOnLockChangeToWeapon() const { return m_onLockChangeToWeapon; }
    const std::string& getOnLockChangeToWeaponName() const { return m_onLockChangeToWeaponName; }
    void setSendVehicleLockWarning(bool value) { m_sendVehicleLockWarning = value; }
    bool getSendVehicleLockWarning() const { return m_sendVehicleLockWarning; }
    void setConnectionEffect(const std::string& value) { m_connectionEffect = value; }
    const std::string& getConnectionEffect() const { return m_connectionEffect; }
    void setConnectionEffectOnlyWhenZoomed(bool value) { m_connectionEffectOnlyWhenZoomed = value; }
    bool getConnectionEffectOnlyWhenZoomed() const { return m_connectionEffectOnlyWhenZoomed; }
    void setMaxDistance(float value) { m_maxDistance = value; }
    float getMaxDistance() const { return m_maxDistance; }
    void setSetTargetAtMaxDistance(bool value) { m_setTargetAtMaxDistance = value; }
    bool getSetTargetAtMaxDistance() const { return m_setTargetAtMaxDistance; }
    void setUseCameraDirectionToTarget(bool value) { m_useCameraDirectionToTarget = value; }
    bool getUseCameraDirectionToTarget() const { return m_useCameraDirectionToTarget; }
    void setTargetSystem(TargetSystem value) { m_targetSystem = value; }
    TargetSystem getTargetSystem() const { return m_targetSystem; }

protected:
    /* ELF +0x08 / PE +0x08 */ int m_refCount;
    /* ELF +0x0C / PE +0x0C */ io::PlayerInputMap m_targetInput;
    /* ELF +0x10 / PE +0x10 */ std::string m_unresolvedString;
    // [UNRESOLVED] Exact source identity is not exposed by either shipped image.  Both constructors
    // leave these twelve bytes untouched; both release paths destroy only the surrounding three
    // strings.  Keeping explicit storage is therefore the recovered construction/destruction
    // behaviour, not fabricated state.  ELF: 0x8278D50/0x8279AE0; PE: 0x5C3B40/0x5C3CC0.
    /* ELF +0x14 / PE +0x2C */ unsigned char m_unresolved14[12];
    /* ELF +0x20 / PE +0x38 */ std::string m_onLockChangeToWeaponName;
    /* ELF +0x24 / PE +0x54 */ bool m_onLockChangeToWeapon;
    /* ELF +0x25 / PE +0x55 */ bool m_sendVehicleLockWarning;
                 // ELF +0x26..+0x27 / PE +0x56..+0x57 deliberately untouched.
    /* ELF +0x28 / PE +0x58 */ std::string m_connectionEffect;
    /* ELF +0x2C / PE +0x74 */ bool m_connectionEffectOnlyWhenZoomed;
                 // ELF +0x2D..+0x2F / PE +0x75..+0x77 deliberately untouched.
    /* ELF +0x30 / PE +0x78 */ float m_maxDistance;
    /* ELF +0x34 / PE +0x7C */ bool m_setTargetAtMaxDistance;
    /* ELF +0x35 / PE +0x7D */ bool m_useCameraDirectionToTarget;
                 // ELF +0x36..+0x37 / PE +0x7E..+0x7F deliberately untouched.
    /* ELF +0x38 / PE +0x80 */ TargetSystem m_targetSystem;
};


} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_COMPTEMPLATEBASES_H
