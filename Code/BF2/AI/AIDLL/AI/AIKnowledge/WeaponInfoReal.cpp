
#include "WeaponInfoReal.h"

namespace dice { namespace hfe { namespace ai {

WeaponInfoReal::WeaponInfoReal(IWeapon* weapon)
    : WeaponInfo(weapon)
{
}

WeaponInfoReal::~WeaponInfoReal()
{
}

bool WeaponInfoReal::update() { return true; }
int WeaponInfoReal::getShotsInMag(int magazine) { return m_weapon->getAmmoInMag(magazine); }
int WeaponInfoReal::getAmmo() const { return m_weapon->getAmmo(); }
int WeaponInfoReal::getCurrentAmmo() const { return m_weapon->getCurrentAmmo(); }
int WeaponInfoReal::getNumberOfMags() const { return m_weapon->getNumberOfMags(); }
bool WeaponInfoReal::isReadyToFire() const { return m_weapon->isReadyToFire(); }
double WeaponInfoReal::getTimeOfFiring() const { return m_weapon->getTimeOfFiring(); }
float WeaponInfoReal::getHeat() const { return m_weapon->getHeat(); }
bool WeaponInfoReal::usesHeat() const { return m_weapon->usesHeat(); }
float WeaponInfoReal::getSecurity() const { return 1.0f; }
float WeaponInfoReal::getSecurity(double) const { return 1.0f; }

} } }
