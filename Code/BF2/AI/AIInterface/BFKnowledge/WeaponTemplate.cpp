// [INFERRED] Source placement.

#include "WeaponTemplate.h"

#include "BFGame.h"                                          // IAIGame::getIsPrecaching (+0x68)
#include "WeaponObject.h"

#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

namespace dice {
namespace hfe {
namespace ai {

// WeaponTemplate - , , every store read. The strengths array is checkPointer'd and
// zeroed by index; +0x4D..+0x64 are left as the allocator returned them (see the header).
WeaponTemplate::WeaponTemplate(int strengthCount)
    : m_indirect(false), m_initialized(false), m_isThrown(false), m_useAimerOnly(false),
      m_projectileMaterial(-1), m_maxAmmo(0), m_maxAmmoInMag(0), m_fireRate(-1), m_drag(0.0f),
      m_minRange(0.0f), m_maxRange(0.0f), m_explosionRadius(0.0f), m_soundSphereRadius(50.0f),
      m_exitVelocity(0.0f), m_gravityModifier(0.0f), m_deviation(14.0f),
      m_deviationCorrectionTime(15.0f), m_deviationCorrectionRate(0.06666667f),
      m_strengths(checkPointer(new int[strengthCount])), m_firingPose(SPStanding),
      m_allowedDeviation(0.5f), m_optimalRangePercentage(0.35f), m_firesThroughTransparent(true),
      m_weaponGuidingSystem(WGSNone)
{
    for (int i = 0; i < strengthCount; ++i)
        m_strengths[i] = 0;
}

// the strengths array, null-tested as delete[] always is.
WeaponTemplate::~WeaponTemplate()
{
    delete[] m_strengths;
}

// createWeapon - , / : `new WeaponObject(this)` (0x28 B in the ELF, 0x30
// in the PE - MSVC aligns its double to 8) through checkPointer. The firearm it is for is passed
// and never read, in both images. GenericFireArm attaches the result as its IID_IWeapon component.
IWeapon* WeaponTemplate::createWeapon(world::IObject* /*object*/)
{
    return checkPointer(new WeaponObject(this));
}

// the console's `isThrown` goes through these rather than the raw byte.
bool WeaponTemplate::getIsThrown() const
{
    return m_isThrown;
}

void WeaponTemplate::setIsThrown(bool thrown)
{
    m_isThrown = thrown;
}

// Initializes once outside precaching from the live weapon. Script-owned fields remain
// except drag and weaponFire, which are replaced. Exit velocity is seeded only at zero;
// NaN is left unchanged.
bool WeaponTemplate::init(float drag, float gravityModifier, float exitVelocity, int projectileMaterial,
                          int maxAmmoInMag, int maxAmmo, io::PlayerInputMap weaponFire,
                          io::PlayerInputMap weaponZoom, io::PlayerInputMap altFire, int itemIndex,
                          WeaponType weaponType, float replenishRadius,
                          WeaponGuidingSystem guidingSystem)
{
    if (m_initialized ||
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getIsPrecaching())    // +0x68
        return false;

    m_drag = drag;
    m_gravityModifier = gravityModifier;
    if (m_exitVelocity == 0.0f)
        m_exitVelocity = exitVelocity;
    m_initialized = true;
    m_projectileMaterial = projectileMaterial;
    m_maxAmmo = maxAmmo;
    m_maxAmmoInMag = maxAmmoInMag;
    m_weaponFire = weaponFire;
    m_weaponZoom = weaponZoom;
    m_altFire = altFire;
    initActivate(itemIndex);
    m_weaponType = weaponType;
    m_replenishRadius = replenishRadius;
    m_weaponGuidingSystem = guidingSystem;
    return true;
}

// initActivate - , : the weapon's item slot 1..9 becomes the matching weapon-select
// input; any other index - the -1 of a firearm that is no item included - becomes PINone.
void WeaponTemplate::initActivate(int itemIndex)
{
    switch (itemIndex)
    {
    case 1:  m_weaponActivate = io::PIWeaponSelect1; break;
    case 2:  m_weaponActivate = io::PIWeaponSelect2; break;
    case 3:  m_weaponActivate = io::PIWeaponSelect3; break;
    case 4:  m_weaponActivate = io::PIWeaponSelect4; break;
    case 5:  m_weaponActivate = io::PIWeaponSelect5; break;
    case 6:  m_weaponActivate = io::PIWeaponSelect6; break;
    case 7:  m_weaponActivate = io::PIWeaponSelect7; break;
    case 8:  m_weaponActivate = io::PIWeaponSelect8; break;
    case 9:  m_weaponActivate = io::PIWeaponSelect9; break;
    default: m_weaponActivate = io::PINone; break;
    }
}

} // namespace ai
} // namespace hfe
} // namespace dice
