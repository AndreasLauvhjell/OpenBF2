// Combines global wind with local affectors. Source placement [INFERRED].
// getWind samples both; getWindSpeed calls it virtually. getWindDirection ignores position and
// returns only the global direction. getWindDirectionAndSpeed scales by magnitude rather than
// normalizing; preserve the differing sampling contracts.

#include "WindManager.h"

#include "../../Core/Ids.h"     // IID_IBase, world::IID_IWindManager

#include <cmath>
#include <limits>               // std::numeric_limits<float>::epsilon

namespace dice {
namespace hfe {
namespace world {

// construction and destruction

// Starts with one reference and wind direction +Z; setting speed alone therefore produces +Z wind.
WindManager::WindManager()
    : m_refCount(1)
    , m_windAffectors()                 // +0x08, allocate(20) at 0x831FF99
    , m_unknown14()                     // +0x14, allocate(20) at 0x831FFD5
    , m_affectorsInRange()              // +0x20, allocate(20) at 0x8320011
    , m_globalWindSpeed(0.0f)           // +0x2C
{
    m_globalWindDirection.x = 0.0f;
    m_globalWindDirection.z = 1.0f;     // 0x832004F  stored before .y - scheduling
    m_globalWindDirection.y = 0.0f;

    m_globalWind.x = 0.0f;
    m_globalWind.y = 0.0f;
    m_globalWind.z = 0.0f;
}

// Destroys set storage without releasing affectors; callers retain ownership of registered affectors.
WindManager::~WindManager()
{
}

// the IBase quartet

// Vtable +0x00. Materialises the new count into eax; the declared return
// type stays `void` to match Core/IBase.h, which is what every other class in the tree follows.
void WindManager::addRef()
{
    ++m_refCount;
}

// Vtable +0x04.
int WindManager::getRef()
{
    return m_refCount;
}

// Vtable +0x08. Returns the NEW count (`mov eax, ebx` on both paths).

// `test edx, edx / jz` at 0x8320D37 is the null-`this` test the compiler inserts for `delete this`,
// not a statement. The call it guards is `[eax+4Ch]` - the deleting destructor, vtable +0x4C.
int WindManager::release()
{
    const int remaining = --m_refCount; // 0x8320D2F  (`dec ebx`, stored back at 0x8320D32)

    if (remaining == 0)
        delete this;                    // 0x8320D40, call [eax+4Ch]

    return remaining;                   // 0x8320D50 (`mov eax, ebx`, including after deletion)
}

// Returns IBase or IWindManager for the matching global id, otherwise null.
// Both interface pointers have the same address.
void* WindManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<WindManager*>(this);

    if (iid == IID_IWindManager)                                // 0x8320D75, cmovz
        return const_cast<WindManager*>(this);

    return 0;
}

// the affector registry

// WindManager::getAllWindAffectors - Vtable +0x14. const.

// Eleven bytes: load `this`, `add eax, 8`, return - a reference to the member at +0x08 and nothing
// else. Note it hands out the live registry, not a copy: a caller holding this reference sees later
// addWindAffector / removeWindAffector calls. Contrast getWindAffectors, which hands out a scratch
// buffer that the next call to it overwrites.
const std::set<IWindAffector*>& WindManager::getAllWindAffectors() const
{
    return m_windAffectors;
}

// WindManager::addWindAffector - Vtable +0x40.

// A single tail call to
//     std::_Rb_tree<IWindAffector*, IWindAffector*, std::_Identity<IWindAffector*>,
//                   std::less<IWindAffector*>, std::allocator<IWindAffector*> >::insert_unique
// on the tree at +0x08. INSERT_UNIQUE, not insert_equal: registering the same affector twice is a
// no-op, and no diagnostic is produced. The returned pair<iterator,bool> is discarded.

// no addRef on the affector, and no ownership taken - see the destructor.
void WindManager::addWindAffector(IWindAffector* affector)
{
    m_windAffectors.insert(affector);   // `add eax, 8` 0x83203E3, call 0x83203ED
}

// WindManager::removeWindAffector - Vtable +0x44.

// A single tail call to the same tree's `erase(const key_type&)` on +0x08. The size_type it returns
// is discarded, so removing something that was never added is silently fine. No release() either.
void WindManager::removeWindAffector(IWindAffector* affector)
{
    m_windAffectors.erase(affector);    // `add edx, 8` 0x8320410, call 0x8320416
}

// the global wind

// Vtable +0x1C. const. `fld dword ptr [eax+2Ch]`.
float WindManager::getGlobalWindSpeed() const
{
    return m_globalWindSpeed;
}

// WindManager::setGlobalWindSpeed - Vtable +0x20.

// Stores the speed, then recomputes the cached product m_globalWind = direction * speed. The three
// component stores land at +0x3C, +0x40, +0x44 in that order; the two multiplications for .y and .z
// are hoisted above the .x store by the scheduler, which is why the pseudocode looks shuffled.
void WindManager::setGlobalWindSpeed(float speed)
{
    m_globalWindSpeed = speed;                              // 0x8320DAF, `fst dword ptr [eax+2Ch]`

    m_globalWind.x = m_globalWindDirection.x * speed;
    m_globalWind.y = m_globalWindDirection.y * speed;
    m_globalWind.z = m_globalWindDirection.z * speed;
}

// Vtable +0x24. const. `add eax,30h` - a reference to +0x30.
const Vec3& WindManager::getGlobalWindDirection() const
{
    return m_globalWindDirection;
}

// WindManager::setGlobalWindDirection - Vtable +0x28.

// The mirror of setGlobalWindSpeed: store the direction, then recompute the cached product with the
// existing speed.

// it does not normalise. Whatever vector the caller passes is stored verbatim and multiplied by
// the speed, so an unnormalised direction silently scales the wind. Given that the constructor
// seeds a unit +Z, the invariant "direction is unit length" is the caller's to maintain. Do not add
// a normalise here - getWindDirectionAndSpeed's arithmetic depends on the raw value.
void WindManager::setGlobalWindDirection(const Vec3& direction)
{
    m_globalWindDirection.x = direction.x;
    m_globalWindDirection.y = direction.y;
    m_globalWindDirection.z = direction.z;

    m_globalWind.x = direction.x * m_globalWindSpeed;
    m_globalWind.y = direction.y * m_globalWindSpeed;
    m_globalWind.z = direction.z * m_globalWindSpeed;
}

// Vtable +0x2C. const. `add eax,3Ch` - a reference to +0x3C.
const Vec3& WindManager::getGlobalWind() const
{
    return m_globalWind;
}

// Returns the magnitude of virtual getWind(position), preserving derived overrides.
float WindManager::getWindSpeed(const Vec3& position) const
{
    Vec3 w = getWind(position);                             // 0x8320E5F, virtual [ebx+3Ch]

    return static_cast<float>(std::sqrt(w.x * w.x + w.y * w.y + w.z * w.z));
}

// Returns global wind direction, ignoring position and local wind affectors.
const Vec3& WindManager::getWindDirection(const Vec3& /*position*/) const
{
    return m_globalWindDirection;
}

// local-affector update and sampling - ELF 0x8320420..0x832087D.

void WindManager::update(float deltaTime)
{
    // Retail increments before dispatch. That ordering is load-bearing: a finite-duration
    // WindAffector erases itself from this set inside update().
    std::set<IWindAffector*>::iterator it = m_windAffectors.begin();
    while (it != m_windAffectors.end())
    {
        std::set<IWindAffector*>::iterator current = it++;
        (*current)->update(deltaTime);
    }
}

const std::set<IWindAffector*>& WindManager::getWindAffectors(
    const Vec3& position, float radius) const
{
    m_affectorsInRange.clear();
    for (std::set<IWindAffector*>::const_iterator it = m_windAffectors.begin();
         it != m_windAffectors.end(); ++it)
    {
        IWindAffector* const affector = *it;
        if (!affector->getIsActive())
            continue;

        const Vec3& centre = affector->getAbsolutePosition();
        const float x = centre.x - position.x;
        const float y = centre.y - position.y;
        const float z = centre.z - position.z;
        const float range = affector->getBoundingRadius() + radius;
        if (range * range > x * x + y * y + z * z)       // strict boundary, as shipped
            m_affectorsInRange.insert(affector);
    }
    return m_affectorsInRange;
}

Vec3 WindManager::getWind(const Vec3& position) const
{
    // Recompute rather than reading m_globalWind: that is the shipped body.
    Vec3 result = {
        m_globalWindDirection.x * m_globalWindSpeed,
        m_globalWindDirection.y * m_globalWindSpeed,
        m_globalWindDirection.z * m_globalWindSpeed
    };

    const std::set<IWindAffector*>& affectors = getWindAffectors(position, 0.0f);
    for (std::set<IWindAffector*>::const_iterator it = affectors.begin();
         it != affectors.end(); ++it)
    {
        const Vec3 local = (*it)->getAbsoluteWind(position);
        result.x += local.x;
        result.y += local.y;
        result.z += local.z;
    }
    return result;
}

void WindManager::getWindDirectionAndSpeed(const Vec3& position,
                                           Vec3& direction,
                                           float& speed) const
{
    const std::set<IWindAffector*>& affectors = getWindAffectors(position, 0.0f);
    if (affectors.empty())
    {
        direction = m_globalWindDirection;
        speed = m_globalWindSpeed;
        return;
    }

    direction.x = 0.0f;
    direction.y = 0.0f;
    direction.z = 0.0f;
    Vec3 localDirection = { 0.0f, 0.0f, 0.0f }; // initialised once, not once per affector
    float localSpeed;                            // deliberately uninitialised; see WindAffector
    for (std::set<IWindAffector*>::const_iterator it = affectors.begin();
         it != affectors.end(); ++it)
    {
        (*it)->getAbsoluteWindDirectionAndSpeed(position, localDirection, localSpeed);
        direction.x += localDirection.x * localSpeed;
        direction.y += localDirection.y * localSpeed;
        direction.z += localDirection.z * localSpeed;
    }

    const float localSquared = direction.x * direction.x
                             + direction.y * direction.y
                             + direction.z * direction.z;
    if (std::fabs(localSquared) < std::numeric_limits<float>::epsilon())
    {
        direction = m_globalWindDirection;
        speed = m_globalWindSpeed;
        return;
    }

    direction.x += m_globalWindDirection.x * m_globalWindSpeed;
    direction.y += m_globalWindDirection.y * m_globalWindSpeed;
    direction.z += m_globalWindDirection.z * m_globalWindSpeed;
    speed = std::sqrt(direction.x * direction.x
                    + direction.y * direction.y
                    + direction.z * direction.z);

    // shipped defect: the binary multiplies by the magnitude instead of normalising.
    direction.x *= speed;
    direction.y *= speed;
    direction.z *= speed;
}

// the class factory

// Creates a one-reference wind manager; rejects aggregation and releases on IID mismatch.
// No init or additional reference follows construction.
IBase* WindManager::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    WindManager* self = new WindManager();              // 0x87FA507, operator new(0x48u)
    void* itf = self->queryInterface(iid);              // 0x87FA522, virtual, vtable +0x0C
    if (itf == 0)
        self->release();                                // 0x87FA534, virtual, vtable +0x08
    return static_cast<IBase*>(itf);
}

} // namespace world
} // namespace hfe
} // namespace dice
