#include "StickyProjectileEvent.h"

#include "../IGame.h"                            // queryInterface(IID_IGameServer)
#include "../Objects/IObject.h"                  // getNetworkable, getChild, getAbsoluteTransformation
#include "../Objects/IPlayer.h"                  // getId, getVehicle
#include "../Objects/Projectiles/GenericProjectile.h" // producer owner lookup through getPlayer
#include "../Objects/IWeaponObject.h"            // queried secondary face, addProjectileReference
#include "../Objects/ObjectLayoutAccess.h"       // LayoutAccess::sibling (+0x2C)
#include "../../Core/Ids.h"
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../IO/Network/INetworkable.h"       // m_networkableId in getNetIdCount
#include "../../IO/Network/NetworkManager.h"     // getObject
#include "../../Scene/Object/ObjectUtil.h"        // the two typed producer-side tree helpers
#include "../../Scene/Object/PlayerManager.h"    // getLocalHumanPlayer
#include "../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"   // getActiveWeapon

namespace dice {
namespace hfe {
namespace io { extern NetworkManager* networkManager; }      // ds:0x8AC3D1C

namespace {
// .rodata dword GEStickyProjectile (0x888A7A4) = 32.
const int GEStickyProjectile = 32;

// Use typed projectile calls so each compiler adjusts the interface base correctly.
// The weapon argument already points to IWeaponObject and must retain that interface.
void  genericProjectileStick(world::IObject* projectile, world::IObject* target,
                             const Mat4& transform)
{
    static_cast<world::GenericProjectile*>(projectile)->stick(target, transform);
}
void genericProjectileStickAbsolute(world::IObject* projectile, const Mat4& transform)
{
    static_cast<world::GenericProjectile*>(projectile)->stickAbsolute(transform);
}
void genericFireArmAddProjectileReference(void* weaponObject, world::IObject* projectile)
{
    static_cast<world::IWeaponObject*>(weaponObject)->addProjectileReference(projectile);
}

// Finds the Nth networkable object in pre-order: node, entire child subtree, then
// next sibling. The counter is shared across recursive calls.
world::IObject* internalGetObjectFromNetIdCount(world::IObject* node, int wanted, int& counter)
{
    if (!node)
        return 0;

    if (node->getNetworkable())                  // slot 54, +0xD8
    {
        if (wanted == counter)
            return node;
        ++counter;
    }

    world::IObject* found = internalGetObjectFromNetIdCount(node->getChild(), wanted, counter);
    if (!found)
    {
        found = internalGetObjectFromNetIdCount(
            world::object_detail::LayoutAccess::sibling(node), wanted, counter);   // +0x2C
    }
    return found;
}

world::IObject* getObjectFromNetIdCount(unsigned short netId, int wanted)
{
    world::IObject* root = io::networkManager->getObject(netId);
    if (!root)
        return 0;

    int counter = 0;
    return internalGetObjectFromNetIdCount(root, wanted, counter);
}

// The producer-side inverse of the pair above. findFirstRealNetworkable (ELF 0x807C090) climbs
// from the selected anchor until it finds a networkable whose id is non-zero; getNetIdCount
// (0x807C0E0) publishes that id and asks internalGetNetIdCount (0x807C000) for the selected
// anchor's pre-order networkable index inside the real networkable's subtree.
world::IObject* findFirstRealNetworkable(world::IObject* object)
{
    while (object)
    {
        // Both dispatches are shipped: the successful arm does not reuse the test result.
        if (object->getNetworkable() && object->getNetworkable()->m_networkableId != 0)
            return object;
        object = object->getParent();
    }
    return 0;
}

bool internalGetNetIdCount(world::IObject* node, world::IObject* wanted, int& count)
{
    if (!node)
        return false;
    if (node == wanted)
        return true;

    if (node->getNetworkable())
        ++count;

    if (internalGetNetIdCount(node->getChild(), wanted, count))
        return true;
    return internalGetNetIdCount(
        world::object_detail::LayoutAccess::sibling(node), wanted, count);
}

void getNetIdCount(world::IObject* object, unsigned short& netId, int& count)
{
    netId = 0;
    count = 0;

    world::IObject* const realNetworkable = findFirstRealNetworkable(object);
    if (realNetworkable)
    {
        netId = realNetworkable->getNetworkable()->m_networkableId;
        internalGetNetIdCount(realNetworkable, object, count);
    }
}
} // namespace

// Initializes an identity matrix; IDs and other tail fields remain unset until deserialization.
StickyProjectileEvent::StickyProjectileEvent()
    : GameEvent()
{
    for (int i = 0; i < 16; ++i)
        m_transform.m[i] = 0.0f;
    m_transform.m[0]  = 1.0f;                    // +0x0C
    m_transform.m[5]  = 1.0f;                    // +0x20
    m_transform.m[10] = 1.0f;                    // +0x34
    m_transform.m[15] = 1.0f;                    // +0x48
}

// Captures a sticky projectile's target-relative transform. Without a networkable
// target ancestor, uses the projectile's absolute transform instead. Unowned
// projectiles retain player ID -1.
StickyProjectileEvent::StickyProjectileEvent(world::GenericProjectile* projectile,
                                             world::IObject* target, const Mat4& transform)
    : GameEvent()
{
    for (int i = 0; i < 16; ++i)
        m_transform.m[i] = 0.0f;
    m_transform.m[0]  = 1.0f;
    m_transform.m[5]  = 1.0f;
    m_transform.m[10] = 1.0f;
    m_transform.m[15] = 1.0f;

    m_projectileNetId  = 0;                      // +0x4C
    m_targetNetId      = 0;                      // +0x4E
    m_targetNetIdCount = 0;                      // +0x50
    m_playerId         = static_cast<unsigned char>(-1);   // +0x51 - -1 in a byte
    m_childIndex       = 0;                      // +0x52

    world::IObject* anchor = world::findClosestParentHavingNetworkable(target);

    if (projectile)
    {
        m_projectileNetId = projectile->getNetworkable()->m_networkableId;

        world::IPlayer* owner = projectile->getPlayer();
        if (owner)
            m_playerId = static_cast<unsigned char>(owner->getId());          // +0x54
    }

    if (anchor)
    {
        m_transform = transform;                 // the caller's matrix, verbatim

        int count = 0;
        getNetIdCount(anchor, m_targetNetId, count);
        m_targetNetIdCount = static_cast<unsigned char>(count);

        int childIndex = 0;
        world::getChildIndexOfType(
            anchor, target, world::CID_RotationalBundleTemplate, childIndex);
        m_childIndex = static_cast<unsigned char>(childIndex);
    }
    else
    {
        // THE argument IS thrown away - see the block above.
        m_transform = reinterpret_cast<world::IObject*>(projectile)->getAbsoluteTransformation();
    }
}

int StickyProjectileEvent::getType() const
{
    return GEStickyProjectile;
}

// its own, empty. This event does not log.
void StickyProjectileEvent::logInfo(bool, int)
{
}

// Attaches the projectile to its target or absolute fallback and updates the local
// owner's weapon. Zero projectile ids are ignored. The local weapon tail requires
// IWeaponObject even though only its first operation is guarded.
void StickyProjectileEvent::executeClient()
{
    if (!m_projectileNetId)
        return;

    world::IObject* projectile = io::networkManager->getObject(m_projectileNetId);
    if (!projectile)
        return;

    static_cast<world::GenericProjectile*>(projectile)->activate();

    world::IObject* stickTo = 0;
    if (m_targetNetId != 0)
    {
        world::IObject* anchor = getObjectFromNetIdCount(m_targetNetId, m_targetNetIdCount);
        if (anchor)
        {
            int out = 0;
            stickTo = world::findChildTypeIndex(anchor, m_childIndex,
                                         world::CID_RotationalBundleTemplate, out);   // [FRONTIER]
        }
    }

    if (stickTo)
        genericProjectileStick(projectile, stickTo, m_transform);
    else
        genericProjectileStickAbsolute(projectile, m_transform);

    world::IPlayer* local = world::playerManager->getLocalHumanPlayer();      // +0x30
    if (!local || local->getId() != m_playerId)                               // +0x54
        return;

    world::IObject* vehicle = local->getVehicle();                            // +0x48
    if (!vehicle)
        return;

    world::IPlayerControlObject* pco = 0;
    for (world::IObject* o = vehicle; o != 0; o = o->getParent())
    {
        pco = static_cast<world::IPlayerControlObject*>(
            o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            break;
    }
    if (!pco)
        return;

    world::IObject* weapon = pco->getActiveWeapon(0);                         // +0x70
    if (!weapon)
        return;

    void* asWeaponObject = weapon->queryInterface(world::IID_IWeaponObject);
    if (asWeaponObject)
        genericFireArmAddProjectileReference(asWeaponObject, projectile);

    // outside the guard above - see the header note.
    //  / PE 0x53DF1A dereference the queried weapon face here, even if null.
    void* weaponAsObject = static_cast<world::IWeaponObject*>(asWeaponObject)
        ->queryInterface(world::IID_IObject);
    static_cast<world::GenericProjectile*>(projectile)->setLauncherWeapon(
        static_cast<world::IObject*>(weaponAsObject));
}

// the "answer whether I am a server and do nothing" shape.
bool StickyProjectileEvent::executeServer(int /*connectionId*/)
{
    return game->queryInterface(IID_IGameServer) != 0;
}

// Reads IDs, optional count/child index, player ID, then matrix rows 2, 0, 1, 3.
// Resets the matrix's homogeneous row before decoding.
bool StickyProjectileEvent::deSerialize(io::BitStream* stream)
{
    int projectileNetId = 0;
    stream->readBits(&projectileNetId, 16);
    m_projectileNetId = static_cast<unsigned short>(projectileNetId);

    int targetNetId = 0;
    stream->readBits(&targetNetId, 16);
    m_targetNetId = static_cast<unsigned short>(targetNetId);

    if (m_targetNetId != 0)
    {
        int count = 0;
        stream->readBits(&count, 8);
        m_targetNetIdCount = static_cast<unsigned char>(count);

        int childIndex = 0;
        stream->readBits(&childIndex, 8);
        m_childIndex = static_cast<unsigned char>(childIndex);
    }

    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = static_cast<unsigned char>(playerId);

    m_transform.m[15] = 1.0f;
    m_transform.m[12] = 0.0f;
    m_transform.m[13] = 0.0f;
    m_transform.m[14] = 0.0f;

    static const int rows[4] = { 8, 0, 4, 12 };  // the wire's row order - see the header
    for (int r = 0; r < 4; ++r)
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        stream->readBits(&x, 32);
        stream->readBits(&y, 32);
        stream->readBits(&z, 32);
        m_transform.m[rows[r] + 0] = x;
        m_transform.m[rows[r] + 1] = y;
        m_transform.m[rows[r] + 2] = z;
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x807CA60, 576 B. The exact mirror, and the twelve float writes go straight out of
// the matrix with no temporaries.
bool StickyProjectileEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int projectileNetId = m_projectileNetId;
    stream->writeBits(&projectileNetId, 16);

    int targetNetId = m_targetNetId;
    stream->writeBits(&targetNetId, 16);

    if (m_targetNetId != 0)
    {
        int count = m_targetNetIdCount;
        stream->writeBits(&count, 8);

        int childIndex = m_childIndex;
        stream->writeBits(&childIndex, 8);
    }

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    static const int rows[4] = { 8, 0, 4, 12 };
    for (int r = 0; r < 4; ++r)
    {
        stream->writeBits(&m_transform.m[rows[r] + 0], 32);
        stream->writeBits(&m_transform.m[rows[r] + 1], 32);
        stream->writeBits(&m_transform.m[rows[r] + 2], 32);
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
