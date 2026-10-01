// Shared fire-component settings, including fire rates and the fire-start delay distribution.
// [PLATFORM] Vector layout differs between the original ELF and PE; do not share size or offset assertions.
// [INFERRED] Header placement; implementation belongs to FireComponents/FireComp.cpp.

#ifndef GAME_OBJECTS_FIRECOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_FIRECOMPTEMPLATE_H

#include <vector>

#include "../../Core/Random.h"
#include "../../IO/Console/IConsoleSaveable.h"
#include "../../IO/Input/Input.h"
#include "../../Math/Vec3.h"
#include "CompTemplateInterfaces.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;

// The type name and signed 32-bit representation are attested by IWeaponObject's setFireRate
// signature and by std::vector<FireRate> symbols in the ELF.  Enumerator names remain unresolved.
enum FireRate : int;

// RTTI: IWeaponCompTemplate @ +0, io::IConsoleSaveable @ +4 in both shipped x86 images.
// Vtable order (ELF 0x88B75E0): the IBase quartet stays pure through IWeaponCompTemplate; the six
// component-template overrides follow; then preCache (inherited), the destructor pair, makeScript.
class FireCompTemplate : public IWeaponCompTemplate, public io::IConsoleSaveable
{
public:
    FireCompTemplate();

    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();
    virtual void  objectCreated(IObject*);
    virtual void  objectDestroyed(IObject*);
    virtual void  setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual ~FireCompTemplate();

    virtual bool  makeScript(io::IStream*);

    // The only two non-virtual FireCompTemplate methods emitted as named retail symbols.
    void setRoundsPerMinute(int roundsPerMinute);
    void setFireLaunchDelay(float delay);

    // [INFERRED] accessor names for direct console fields; these add no virtual slots.
    int getRoundsPerMinute() const { return m_roundsPerMinute; }
    int getTimePerRound() const { return m_timePerRound; }

    void setFireStartDelay(const RandomDistribution& delay) { m_fireStartDelay = delay; }
    const RandomDistribution& getFireStartDelay() const { return m_fireStartDelay; }

    float getFireLaunchDelay() const { return m_fireLaunchDelay; }
    int getFireLaunchDelayInTicks() const { return m_fireLaunchDelayTimes30; }

    void addFireRate(int rate) { m_fireRates.push_back(static_cast<FireRate>(rate)); }
    const std::vector<FireRate>& getFireRates() const { return m_fireRates; }

    void setFireInCameraDof(bool enabled) { m_fireInCameraDof = enabled; }
    bool getFireInCameraDof() const { return m_fireInCameraDof; }
    void setFireInCameraDofLocked(bool locked) { m_fireInCameraDofLocked = locked; }
    bool getFireInCameraDofLocked() const { return m_fireInCameraDofLocked; }

    void setBatchSize(int size) { m_batchSize = size; }
    int getBatchSize() const { return m_batchSize; }
    void setBurstSize(int size) { m_burstSize = size; }
    int getBurstSize() const { return m_burstSize; }

    void setFireInput(io::PlayerInputMap input) { m_fireInput = input; }
    io::PlayerInputMap getFireInput() const { return m_fireInput; }
    void setAltFireInput(io::PlayerInputMap input) { m_altFireInput = input; }
    io::PlayerInputMap getAltFireInput() const { return m_altFireInput; }
    void setIgnoreFireButton(io::PlayerInputMap input) { m_ignoreFireButton = input; }
    io::PlayerInputMap getIgnoreFireButton() const { return m_ignoreFireButton; }

    void setProjectileStartPosition(const Math::Vec3& position)
    {
        m_projectileStartPosition = position;
    }
    const Math::Vec3& getProjectileStartPosition() const { return m_projectileStartPosition; }

    void setProjectileStartRotation(const Math::Vec3& rotation)
    {
        m_projectileStartRotation = rotation;
    }
    const Math::Vec3& getProjectileStartRotation() const { return m_projectileStartRotation; }

    void setMaxProjectilesInWorld(int maximum) { m_maxProjectilesInWorld = maximum; }
    int getMaxProjectilesInWorld() const { return m_maxProjectilesInWorld; }

protected:
    RandomDistribution m_fireStartDelay;
    float               m_fireLaunchDelay;
    int                 m_fireLaunchDelayTimes30;

    // Neither constructor initializes these two dwords and makeScript does not serialize them.
    // Their presence and order before the vector are cross-checked in both images; roles unknown.
    int                 m_unknown24;                   // [UNRESOLVED], ELF +0x24
    int                 m_unknown28;                   // [UNRESOLVED], ELF +0x28

    std::vector<FireRate> m_fireRates;                 // source object begins at semantic +0x2C
    int                   m_roundsPerMinute;
    int                   m_timePerRound;              // cached 30 Hz ticks: 1800 / RPM

    bool                m_fireInCameraDof;
    bool                m_fireInCameraDofLocked;
                                                        // two natural padding bytes, untouched
    int                 m_batchSize;
    int                 m_burstSize;

    io::PlayerInputMap  m_fireInput;
    io::PlayerInputMap  m_altFireInput;
    io::PlayerInputMap  m_ignoreFireButton;

    Math::Vec3          m_projectileStartPosition;
    Math::Vec3          m_projectileStartRotation;

    int                 m_maxProjectilesInWorld;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_FIRECOMPTEMPLATE_H
