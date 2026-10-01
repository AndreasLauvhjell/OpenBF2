#include "CommanderCamEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../Objects/IObject.h"                  // getTemplate
#include "../Objects/IObjectTemplate.h"          // getClassID
#include "../Objects/IPlayer.h"                  // getIsCommander / getTeam
#include "../Objects/Game/OverheadCamera.h"      // forceSetPosition
#include "../GameLogic/SquadManager/ISquadManager.h" // typed +0x70/+0x74 dispatch
#include "../../Core/IBase.h"
#include "../../Core/Ids.h"                      // IID_IGameServer, CID_OverheadCameraTemplate
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {

extern IBase* squadManager;                      // ds:0x8ACBFA4 - Core/Globals.cpp (IBase*, no layout)

namespace {
// .rodata dword GECommanderCam (0x8888884) = 36.
const int GECommanderCam = 36;

// The global retains Core/Globals.cpp's IBase* declaration; the vtable recovered in
// ISquadManager.h supplies the exact +0x70/+0x74 typed calls.
inline ISquadManager* squadManagerIface()
{
    return static_cast<ISquadManager*>(squadManager);
}
} // namespace

// PE626230, complete instructions. ELF81E8AE0 inlines the same three-float copy;
// there is no mangled ELF constructor with a contradictory parameter spelling.
CommanderCamEvent::CommanderCamEvent(Vec3 position)
    : GameEvent()
    , m_position(position)
{
}

int CommanderCamEvent::getType() const
{
    return GECommanderCam;
}

// an explicit empty override. The camera's new position comes back to the client
// as ordinary object state, not as this event replayed.
void CommanderCamEvent::executeClient()
{
}

// Moves the team's overhead commander camera when permitted. The result is not a success flag:
// a successful move returns false; only a commander with no squad manager returns true.
// The non-commander path requires squadManager and retains its discarded lookup calls.
bool CommanderCamEvent::executeServer(int connectionId)
{
    if (!game->queryInterface(IID_IGameServer))
        return false;

    world::IPlayer* player = world::playerManager->getPlayerFromId(connectionId);   // +0x20
    if (!player)
        return false;

    if (player->getIsCommander())                                                   // +0x114
    {
        if (!squadManager)
            return true;                        // the only true this function can return

        world::IObject* camera = static_cast<world::IObject*>(
            squadManagerIface()->getCommanderCam(player->getTeam()));               // +0x74

        if (camera && camera->getTemplate()->getClassID() == world::CID_OverheadCameraTemplate)
            static_cast<world::OverheadCamera*>(camera)->forceSetPosition(m_position);

        return false;
    }

    // Unguarded, and the result is discarded - see above.
    const int commanderId = squadManagerIface()->getCommander(player->getTeam());       // +0x70
    world::playerManager->getPlayerFromId(commanderId);                                 // +0x20

    return false;
}

// deSerialize - 0x806BC70, 156 B. Three raw 32-bit floats, read into temporaries and then stored -
// the ELF really does use three stack slots rather than reading straight into the Vec3, which is
// why this reads a little more verbosely than serialize.
bool CommanderCamEvent::deSerialize(io::BitStream* stream)
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    stream->readBits(&x, 32);
    stream->readBits(&y, 32);
    stream->readBits(&z, 32);

    m_position.x = x;
    m_position.y = y;
    m_position.z = z;

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x806BD10, 183 B. Written straight out of the Vec3, three contiguous 32-bit runs.
bool CommanderCamEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    stream->writeBits(&m_position.x, 32);
    stream->writeBits(&m_position.y, 32);
    stream->writeBits(&m_position.z, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
