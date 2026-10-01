// HUD state accessors. [INFERRED] Source split.
#include "BF2HudManager.h"
#if !DEDICATED
namespace dice { namespace hfe { namespace hud {

HudAmmo* BF2HudManager::getAmmo() { return hudAmmo; }
HudVehicleInformation* BF2HudManager::getVehicleInformation() { return hudVehicleInformation; } // +24C, 751A80
HudMap* BF2HudManager::getMap() { return hudMap; }
HudSquad* BF2HudManager::getSquad() { return m_field138; }
HudCommander* BF2HudManager::getCommander() { return m_field13c; }
HudMessages* BF2HudManager::getMessages() { return m_field148; } // +25C, 504C60
bool BF2HudManager::getFlag229() { return m_field229; }
bool BF2HudManager::getFlag22A() { return m_field22a; }
void BF2HudManager::setFlagE7(bool value) { m_field0e7 = value; }
bool BF2HudManager::isSquadRenameActive() { return m_field0e5; } // +26C, 751AC0
void BF2HudManager::setSquadRenameActive(bool value) { m_field0e5 = value; }
bool BF2HudManager::isSquadCreateActive() { return m_field0e4; }
void BF2HudManager::setSquadCreateActive(bool value) { m_field0e4 = value; }
std::string BF2HudManager::getSquadText() // +27C, 4B1960: copy constructor into hidden result
{
    return std::string(m_field0c0);
}
void BF2HudManager::setFlagE6(bool value) { m_field0e6 = value; }

void BF2HudManager::setSatelliteEnabled(bool value) { m_field2c0 = value; }
bool BF2HudManager::getSatelliteEnabled() { return m_field2c0; }
void BF2HudManager::setUAVEnabled(bool value) { m_field2c2 = value; }
bool BF2HudManager::getUAVEnabled() { return m_field2c2; } // +30C, 7B1BF0
void BF2HudManager::setArtilleryEnabled(bool value) { m_field2c1 = value; }
bool BF2HudManager::getArtilleryEnabled() { return m_field2c1; }

// The setters are single-word stores and the getters FLD that same float. No
// clamping, enable-flag mutation or dependency calls are present in the retail.
void BF2HudManager::setSatelliteAvailability(float value) { m_field2c4 = value; }
float BF2HudManager::getSatelliteAvailability() { return m_field2c4; } // +31C, 4BE260
void BF2HudManager::setArtilleryAvailability(float value) { m_field2c8 = value; }
float BF2HudManager::getArtilleryAvailability() { return m_field2c8; }
void BF2HudManager::setUAVAvailability(float value) { m_field2cc = value; }
float BF2HudManager::getUAVAvailability() { return m_field2cc; } // +32C, 7B1C30
void BF2HudManager::setVehicleDropAvailability(float value) { m_field2d0 = value; }
float BF2HudManager::getVehicleDropAvailability() { return m_field2d0; }

} } }
#endif
