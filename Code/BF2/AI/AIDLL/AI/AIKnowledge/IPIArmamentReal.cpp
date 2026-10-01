
#include "IPIArmamentReal.h"

namespace dice {
namespace hfe {
namespace ai {

IPIArmamentReal::IPIArmamentReal(Information* information)
    : IPIArmament(information)
{
    updateWeapons();
}

IPIArmamentReal::~IPIArmamentReal()
{
}

unsigned int IPIArmamentReal::getWeapon(int index)
{
    return m_weapons[index];
}

std::vector<unsigned int>& IPIArmamentReal::getWeapons()
{
    return m_weapons;
}

void IPIArmamentReal::updateWeapons()
{
    updateWeaponsInternal(true);
}

} // namespace ai
} // namespace hfe
} // namespace dice
