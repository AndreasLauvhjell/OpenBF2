// Missile initialization events using typed projectile interfaces. Source path is attested.

#include "MissileInitEvent.h"

#include "../Objects/IObject.h"                  // queryInterface, updateFlags, isCreatedByLocalPlayer
#include "../Objects/IPlayer.h"                  // getVehicle / getDefaultVehicle / getTeam
#include "../Objects/IWeaponObject.h"            // getNetworkSafeItemIndex
#include "../Objects/Weapons/GenericFireArm.h"   // projectile bookkeeping pair
#include "../Objects/Projectiles/GenericProjectile.h"    // the six typed IProjectileObject slots
                                                 // this file dispatches, plus IProjectileObject
                                                 // itself (its base) - see the header block
#include "../../Core/Debug.h"                    // both abort sites
#include "../../Core/Ids.h"                      // the four IIDs
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../IO/Network/NetworkManager.h"     // getObject
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId
#include "../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"   // getOccupingObject,
                                                 // isChildSoldierActive, getWeapons

namespace dice {
namespace hfe {
namespace io { extern NetworkManager* networkManager; }      // ds:0x8AC3D1C

namespace {
// .rodata dword GEMissileInit (0x8889E00) = 43.
const int GEMissileInit = 43;

// The flag pair updateFlags is called with: set 0, clear 2. Bare immediates in the ELF; what bit 1
// of an object's flag word means is [UNRESOLVED].
const unsigned int PROJECTILE_FLAGS_SET   = 0u;
const unsigned int PROJECTILE_FLAGS_CLEAR = 2u;

// Finds the first player-control interface on this object or its ancestors.
world::IPlayerControlObject* findPlayerControlObject(world::IObject* start)
{
    for (world::IObject* o = start; o != 0; o = o->getParent())
    {
        world::IPlayerControlObject* pco = static_cast<world::IPlayerControlObject*>(
            o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            return pco;
    }
    return 0;
}
} // namespace

//  (C2) / 0x8076B80 (C1). Four stores; no zeroing, no loopback.
MissileInitEvent::MissileInitEvent(unsigned short projectileNetId, int playerId, int weaponSlot,
                                   bool useDefaultVehicle)
    : GameEvent()
    , m_projectileNetId(projectileNetId)                     // +0x0C
    , m_playerId(static_cast<unsigned char>(playerId))       // +0x0E - truncates
    , m_weaponSlot(static_cast<unsigned char>(weaponSlot))   // +0x0F - truncates
    , m_useDefaultVehicle(useDefaultVehicle)                 // +0x10
{
}

int MissileInitEvent::getType() const
{
    return GEMissileInit;
}

// its own, empty. See the header block: this event does not log.
void MissileInitEvent::logInfo(bool, int)
{
}

// Initializes a projectile's launcher, owner, team, and prediction link. The weapon
// search requires the requested slot to fit the list length, then matches each
// weapon's own slot number. The projectile receives the original control object,
// even when its active child soldier supplies the weapon list.
void MissileInitEvent::executeClient()
{
    world::IObject* projectile = io::networkManager->getObject(m_projectileNetId);
    if (!projectile)
        return;                                              // silent - see the header block

    world::IProjectileObject* const proj = static_cast<world::IProjectileObject*>(
        projectile->queryInterface(world::IID_IProjectileObject));                // IObject +0x0C
    if (!proj)
    {
        Debug(DEBUGTYPE_6, "Game/GameEvents/MissileInitEvent.cpp", 107, "")
            << "Destination projectile not existing on client!" << "\n";          // aborts
        return;
    }

    // See the anonymous-namespace note: the sole implementor, so this is retail's own dispatch.
    world::GenericProjectile* const target = static_cast<world::GenericProjectile*>(proj);

    target->activate();                                                           // +0x20
    projectile->updateFlags(PROJECTILE_FLAGS_SET, PROJECTILE_FLAGS_CLEAR);        // slot 24, +0x60

    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);   // +0x20
    if (!player)
        return;

    world::IObject* start = m_useDefaultVehicle ? player->getDefaultVehicle()     // +0xA4
                                                : player->getVehicle();          // +0x48
    if (!start)
        return;

    world::IPlayerControlObject* pco = findPlayerControlObject(start);
    if (!pco)
        return;

    // Where the weapon list comes from - see the note above.
    world::IPlayerControlObject* weaponSource = pco;
    if (pco->isChildSoldierActive())                                             // +0x140
    {
        world::IObject* occupied = pco->getOccupingObject();                     // +0x14
        weaponSource = occupied ? findPlayerControlObject(occupied) : 0;
    }

    // getWeapons() returns a { begin, end } pair of IObject* - IPlayerControlObject.h types the
    // return `void* [CHOSEN ret]`, so the arithmetic is spelled out rather than faked.
    world::IObject** const* weapons =
        static_cast<world::IObject** const*>(weaponSource->getWeapons());         // +0x6C
    world::IObject* const* begin = weapons[0];

    world::IObject* found = 0;
    if (m_weaponSlot < static_cast<unsigned int>(weapons[1] - begin))
    {
        for (world::IObject* const* p = begin; p != weapons[1]; ++p)   // end re-read - see above
        {
            if (!*p)
                continue;
            world::IWeaponObject* w = static_cast<world::IWeaponObject*>(
                (*p)->queryInterface(world::IID_IWeaponObject));
            if (w && m_weaponSlot == w->getNetworkSafeItemIndex())                // +0x178
            {
                found = *p;
                break;
            }
        }
    }

    // computed whether OR NOT A weapon WAS found, and from `pco` rather than `weaponSource`.
    world::IObject* controlObject = static_cast<world::IObject*>(
        pco->queryInterface(world::IID_IObject));
    if (!found)
        return;

    world::IPlayer* owner = world::playerManager->getPlayerFromId(m_playerId);    // the second lookup

    // this IS NOT THE order FireComp::setProjectileParameters uses. That one goes player, team,
    // weapon, vehicle; this one goes weapon, player, vehicle, team, and then postInit. Preserved as
    // the ELF emits it - setLauncherVehicle's shipped null arm clears the launcher weapon, so a
    // null controlObject here would undo the setLauncherWeapon two lines above it.
    target->setLauncherWeapon(found);                                            // +0x30
    target->setPlayer(owner);                                                    // +0x50  unguarded
    target->setLauncherVehicle(controlObject);                                   // +0x48
    target->setTeam(owner ? owner->getTeam() : 0);                               // +0x60
    target->postInit();                                                          // +0x28

    world::GenericFireArm* firearm = static_cast<world::GenericFireArm*>(found);
    firearm->setCurrentProjectile(projectile);

    if (projectile->isCreatedByLocalPlayer())                                    // slot 84, +0x150
    {
        // The handover: this authoritative round adopts the local player's predicted stand-in and
        // destroys whatever it was holding. GenericFireArm::getLocalPredictedProjectile consumes
        // it pops entries until one is still alive - so the firearm's queue shrinks here even
        // when it yields null, and a null result reaches the setter's clear-only arm.
        target->setLocalPredictedProjectile(firearm->getLocalPredictedProjectile());   // +0x114
    }
}

// executeServer - 0x80771C0, 400 B. A hard stop - Debug type 6 aborts, so the return is unreachable
// in the shipped build. The whole body is that one site.
bool MissileInitEvent::executeServer(int /*connectionId*/)
{
    Debug(DEBUGTYPE_6, "Game/GameEvents/MissileInitEvent.cpp", 200, "")
        << "This should only be sent FROM the server." << "\n";
    return true;
}

// deSerialize - 0x8076BE0, 208 B. Four narrow reads, all through zeroed temps, all stored back
// into fields narrower than the temp.
bool MissileInitEvent::deSerialize(io::BitStream* stream)
{
    int projectileNetId = 0;
    stream->readBits(&projectileNetId, 16);
    m_projectileNetId = static_cast<unsigned short>(projectileNetId);

    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = static_cast<unsigned char>(playerId);

    int weaponSlot = 0;
    stream->readBits(&weaponSlot, 4);            // four bits
    m_weaponSlot = static_cast<unsigned char>(weaponSlot);

    int useDefaultVehicle = 0;
    stream->readBits(&useDefaultVehicle, 1);
    m_useDefaultVehicle = (useDefaultVehicle == 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x8076CB0, 272 B. The exact mirror. The bool's two arms each have their OWN
// temporary in the ELF rather than sharing one; folded, because the two locals are never both live.
bool MissileInitEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int projectileNetId = m_projectileNetId;
    stream->writeBits(&projectileNetId, 16);

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int weaponSlot = m_weaponSlot;
    stream->writeBits(&weaponSlot, 4);

    int useDefaultVehicle = m_useDefaultVehicle ? 1 : 0;
    stream->writeBits(&useDefaultVehicle, 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
