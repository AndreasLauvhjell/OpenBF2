// [INFERRED] Source placement.
#include "KitTemplate.h"
#include "IAIObjectPlugIn.h"                // IAIObject::getPlugIn, IAIObjectUnit (chunk 07)

namespace dice { namespace hfe { namespace ai {

// only the compiler-owned vptr store.
IKitTemplate::IKitTemplate() {}

// only the base vptr store.
IKitTemplate::~IKitTemplate() {}

// Allocation and initialization order
// matter: strategic allocation, battle allocation, six battle zeros, two strategic.
KitTemplate::KitTemplate(KitType type)
    : m_type(type), m_initialized(false)
{
    m_strategicStrengths = new int[2];
    m_battleStrengths = new int[6];
    for (int i = 0; i < 6; ++i)
        m_battleStrengths[i] = 0;
    m_strategicStrengths[0] = 0;
    m_strategicStrengths[1] = 0;
}

KitTemplate::~KitTemplate()
{
    delete[] m_strategicStrengths;
    delete[] m_battleStrengths;
}

// Void confirmed by the console setter consumers.
void KitTemplate::setStrategicStrength(int index, int strength)
{
    m_strategicStrengths[index] = strength;
}

// (folded leaf): borrowed array, no copy.
const int* KitTemplate::getStrategicStrengths() const
{
    return m_strategicStrengths;
}

// no bounds guard.
int KitTemplate::getStrategicStrength(int index) const
{
    return m_strategicStrengths[index];
}

// Void confirmed by the console setter consumers.
void KitTemplate::setBattleStrength(int index, int strength)
{
    m_battleStrengths[index] = strength;
}

// (folded leaf): borrowed array, no copy.
const int* KitTemplate::getBattleStrengths() const
{
    return m_battleStrengths;
}

// no bounds guard.
int KitTemplate::getBattleStrength(int index) const
{
    return m_battleStrengths[index];
}

// (folded leaf).
KitType KitTemplate::getType() const
{
    return m_type;
}

// typed contracts (IAIObject.h, IAIObjectPlugIn.h): the object's unit plug-in
// (getPlugIn(5), ELF +0x78 / PE +0x74), its battle strengths 0..5 (getBattleStrength
// ELF +0x28 / PE +0x24) each stored through the battle array pointer re-read per
// store, then the initialized byte. No null guards, as shipped. has
// no caller in the image.
void KitTemplate::initFromObject(IAIObject* object)
{
    IAIObjectUnit* const unit = static_cast<IAIObjectUnit*>(object->getPlugIn(AIOFUnit));
    for (int i = 0; i <= 5; ++i)
        m_battleStrengths[i] = unit->getBattleStrength(i);
    m_initialized = true;
}

} } }
