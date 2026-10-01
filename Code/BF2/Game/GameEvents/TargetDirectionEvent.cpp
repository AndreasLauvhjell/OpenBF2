#include "TargetDirectionEvent.h"

#include <vector>

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../Objects/IObject.h"                  // getNetworkable, the parent walk
#include "../Objects/IWeaponObject.h"            // setTargetDirection
#include "../../Core/Debug.h"                    // executeClient's abort
#include "../../Core/Ids.h"                      // the three IIDs
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../IO/Network/INetworkable.h"       // m_networkableId
#include "../../IO/Network/NetworkManager.h"     // getObject
#include "../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"   // getWeapons / getActiveWeapon

namespace dice {
namespace hfe {
namespace io { extern NetworkManager* networkManager; }   // ds:0x8AC3D1C - Core/Globals.cpp

namespace world {

//  / PE 0x57FCC0, 46 B.  getWeapons is dispatched exactly once, then the
// returned vector is searched from begin to end with a separately incremented signed index.  A
// miss returns -1 without logging.  Neither port checks pco or the returned vector for null.
int getWeaponIndex(IPlayerControlObject* pco, IObject* weapon)
{
    const std::vector<IObject*>* const weapons =
        static_cast<const std::vector<IObject*>*>(pco->getWeapons());
    std::vector<IObject*>::const_iterator current = weapons->begin();
    const std::vector<IObject*>::const_iterator end = weapons->end();
    int index = 0;
    while (current != end)
    {
        if (*current == weapon)
            return index;
        ++current;
        ++index;
    }
    return -1;
}

} // namespace world

namespace {
// .rodata dword GETargetDirection (0x888AA48) = 47.
const int GETargetDirection = 47;

// Both shipped constructors read the id directly from INetworkable +0x04, without a null guard:
//  and PE 0x53E8F5.  Keep the tiny helper solely as a typed home for that raw read.
unsigned short networkableGetNetId(io::INetworkable* networkable)
{
    return networkable->m_networkableId;
}
} // namespace

// Requires a player-control object in the object's parent chain. An exhausted
// chain retains the original null dereference; callers must satisfy that precondition.
TargetDirectionEvent::TargetDirectionEvent(world::IObject* object, Vec3& field0C, Vec3& field18)
    : GameEvent()
    , m_field0C(field0C)                                     // +0x0C
    , m_field18(field18)                                     // +0x18
{
    world::IPlayerControlObject* pco = 0;
    for (world::IObject* o = object; o != 0; o = o->getParent())
    {
        pco = static_cast<world::IPlayerControlObject*>(
                  o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            break;
    }

    // pco may be null here - see above. The ELF dereferences it anyway.
    world::IObject* asObject =
        static_cast<world::IObject*>(pco->queryInterface(world::IID_IObject));
    m_netId = networkableGetNetId(asObject->getNetworkable());        // +0xD8, then raw +0x04

    m_weaponIndex = static_cast<unsigned char>(world::getWeaponIndex(pco, object));
}

int TargetDirectionEvent::getType() const
{
    return GETargetDirection;
}

// executeClient - 0x807EF90, 242 B. A hard stop - Debug type 6 aborts. The 242 bytes are that one
// site and nothing else; the return below is unreachable in the shipped build.
void TargetDirectionEvent::executeClient()
{
    Debug(DEBUGTYPE_6, "Game/GameEvents/TargetDirectionEvent.cpp", 103, "")
        << "This should not be called on clients.";
}

// Applies target vectors only when the indexed weapon matches one of the control
// object's three active-weapon slots. Returns true even when the event is dropped.
// The server-interface query is performed but its result is not used as a guard.
bool TargetDirectionEvent::executeServer(int /*connectionId*/)
{
    game->queryInterface(IID_IGameServer);       // result discarded - see above

    world::IObject* object = io::networkManager->getObject(m_netId);
    if (!object)
        return true;

    world::IPlayerControlObject* pco = static_cast<world::IPlayerControlObject*>(
        object->queryInterface(world::IID_IPlayerControlObject));

    // getWeapons' return is a { begin, end } pair of IObject* - see the note above.
    world::IObject** const* weapons =
        static_cast<world::IObject** const*>(pco->getWeapons());                  // +0x6C
    world::IObject* const* begin = weapons[0];
    world::IObject* const* end   = weapons[1];

    if (static_cast<unsigned int>(end - begin) <= m_weaponIndex)
        return true;

    world::IObject* weapon = begin[m_weaponIndex];

    if (pco->getActiveWeapon(0) != weapon &&                                      // +0x70
        pco->getActiveWeapon(1) != weapon &&
        pco->getActiveWeapon(2) != weapon)
        return true;

    world::IWeaponObject* weaponObject = static_cast<world::IWeaponObject*>(
        weapon->queryInterface(world::IID_IWeaponObject));

    // copied TO locals first, and in the order the ELF copies them: the +0x18 vector into the
    // higher slot, the +0x0C vector into the lower, then the lower is argument 1.
    const Vec3 a = m_field0C;
    const Vec3 b = m_field18;
    weaponObject->setTargetDirection(a, b);                                        // +0x12C

    return true;
}

// deSerialize - 0x807ED40, 266 B. The two ids lead, then the six floats - not field order.
bool TargetDirectionEvent::deSerialize(io::BitStream* stream)
{
    int netId = 0;
    stream->readBits(&netId, 16);
    m_netId = static_cast<unsigned short>(netId);

    int weaponIndex = 0;
    stream->readBits(&weaponIndex, 8);
    m_weaponIndex = static_cast<unsigned char>(weaponIndex);

    stream->readBits(&m_field0C.x, 32);
    stream->readBits(&m_field0C.y, 32);
    stream->readBits(&m_field0C.z, 32);
    stream->readBits(&m_field18.x, 32);
    stream->readBits(&m_field18.y, 32);
    stream->readBits(&m_field18.z, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x807EE50, 317 B. The same order; the six floats go straight out of the object.
bool TargetDirectionEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int netId = m_netId;
    stream->writeBits(&netId, 16);

    int weaponIndex = m_weaponIndex;
    stream->writeBits(&weaponIndex, 8);

    stream->writeBits(&m_field0C.x, 32);
    stream->writeBits(&m_field0C.y, 32);
    stream->writeBits(&m_field0C.z, 32);
    stream->writeBits(&m_field18.x, 32);
    stream->writeBits(&m_field18.y, 32);
    stream->writeBits(&m_field18.z, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
