// Object creation events and their wire payloads.
#include "CreateObjectEvent.h"

#include "GameEvent.h"                       // gameEventTypeIdBitCount
#include "../IGame.h"                        // game->queryInterface
#include "../IGameServer.h"                  // confirmGhost
#include "../../Core/Debug.h"                // the two executeClient failure logs
#include "../../Core/Ids.h"                  // IID_IGameClient / IID_IPlayerControlObject
#include "../../IO/BitStream.h"              // readBits / writeBits

#include <cstdio>                            // the [FRONTIER] diagnostic
#include <cstdlib>                           // std::abort

#include "../IGameClient.h"                        // the interface both executeClients dispatch through
#include "../Objects/IObject.h"                   // the tail walk: getParent / queryInterface
#include "../../IO/Network/NetworkManager.h"      // the tail walk: getObject (PE 6A6C10)
#include "../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"  // setDefaultTeam (+0x90)

namespace dice {
namespace hfe {

namespace {
const int GECreateObject = 6;      // .rodata 0x8888D44
const int GECreateKit    = 31;     // .rodata 0x8888C40
} // namespace

CreateObjectEvent::CreateObjectEvent(unsigned int templateId, unsigned short networkableId, int defaultTeam,
                                     const Vec3& position, const Vec3& rotation)
{
    m_networkableId = networkableId;                 // +0x10
    m_templateId    = templateId;                    // +0x0C
    m_position      = position;                      // +0x14
    m_rotation      = rotation;                      // +0x20
    m_mode          = 1;                             // +0x30
    m_defaultTeam   = defaultTeam;                   // +0x2C
}

// : the transform-less form - both vectors zero.
CreateObjectEvent::CreateObjectEvent(unsigned int templateId, unsigned short networkableId, unsigned int mode,
                                     int defaultTeam)
{
    m_networkableId = networkableId;
    m_templateId    = templateId;
    m_position.x = m_position.y = m_position.z = 0.0f;
    m_rotation.x = m_rotation.y = m_rotation.z = 0.0f;
    m_defaultTeam   = defaultTeam;
    m_mode          = mode;
}

// The maker's forms - see the header. The ELF has no such constructors.
// m_mode starts AT 1, not 0: deSerialize's else-branch assumes it, and 0 would make executeClient
// take the multi-spawn arm with a zero-iteration loop.
CreateObjectEvent::CreateObjectEvent()
    : m_templateId(0)
    , m_networkableId(0)
    , m_defaultTeam(0)
    , m_mode(1)
{
    m_position.x = m_position.y = m_position.z = 0.0f;
    m_rotation.x = m_rotation.y = m_rotation.z = 0.0f;
}

CreateKitEvent::CreateKitEvent()
    : m_kitTemplateId(0)
    , m_networkableId(0)
    , m_creationIndex(0)
    , m_chosenUnlockLevel(0)
{
    m_position.x = m_position.y = m_position.z = 0.0f;
}

int CreateObjectEvent::getType() const { return GECreateObject; }

CreateKitEvent::CreateKitEvent(unsigned int kitTemplateId, unsigned short networkableId, unsigned int creationIndex,
                               int chosenUnlockLevel, const Vec3& position)
{
    m_networkableId     = networkableId;             // +0x10
    m_kitTemplateId     = kitTemplateId;             // +0x0C
    m_position          = position;                  // +0x1C
    m_creationIndex     = creationIndex;             // +0x14
    m_chosenUnlockLevel = chosenUnlockLevel;         // +0x18
}

int CreateKitEvent::getType() const { return GECreateKit; }


namespace {

// Tests all components against FLT_EPSILON to decide whether serialization can omit the vector.
inline bool isApproximatelyZero(const Vec3& v)
{
    const float eps = 1.1920929e-7f;
    return (v.x < 0.0f ? -v.x : v.x) <= eps
        && (v.y < 0.0f ? -v.y : v.y) <= eps
        && (v.z < 0.0f ? -v.z : v.z) <= eps;
}

// Sets the default team on the first controllable object at or above the networked object.
// Missing objects or parent chains without that interface are ignored.
void applyDefaultTeamToControlObject(unsigned short networkableId, int defaultTeam)
{
    for (world::IObject* object = io::networkManager->getObject(networkableId);
         object;
         object = object->getParent())                                             // +0x28, field
    {
        if (void* const pco = object->queryInterface(world::IID_IPlayerControlObject))   // +0x0C
        {
            static_cast<world::IPlayerControlObject*>(pco)->setDefaultTeam(defaultTeam); // +0x90
            break;
        }
    }
}

} // namespace

void CreateObjectEvent::logInfo(bool, int) {}        // empty
void CreateKitEvent::logInfo(bool, int)    {}        // empty

//  / 0x806D9D0, 50 B each. Both confirm the ghost that carried the event.
// The argument is a pointer TO the connection id, not the id - `confirmGhost(game, *a2, netId)`.
void CreateObjectEvent::eventReceivedByRemote(void* connectionId)
{
    game->confirmGhost(*static_cast<int*>(connectionId), m_networkableId);
}

void CreateKitEvent::eventReceivedByRemote(void* connectionId)
{
    game->confirmGhost(*static_cast<int*>(connectionId), m_networkableId);
}

// Object creation sends either an explicit mode or a transform, never both.
// Transform mode is 1; approximately zero vectors are omitted. Multi-create packets
// therefore create objects at the origin. Default team occupies two bits.
bool CreateObjectEvent::deSerialize(io::BitStream* stream)
{
    unsigned int templateId = 0;
    stream->readBits(&templateId, 32);
    m_templateId = templateId;

    int networkableId = 0;
    stream->readBits(&networkableId, 16);
    m_networkableId = static_cast<unsigned short>(networkableId);

    int defaultTeam = 0;
    stream->readBits(&defaultTeam, 2);
    m_defaultTeam = defaultTeam;

    int hasExplicitMode = 0;
    stream->readBits(&hasExplicitMode, 1);
    if (hasExplicitMode == 1)
    {
        int mode = 0;
        stream->readBits(&mode, 8);
        m_mode = mode;                       // no transform on this branch
    }
    else
    {
        m_mode = 1;

        int hasPosition = 0;
        stream->readBits(&hasPosition, 1);
        if (hasPosition == 1)
        {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            stream->readBits(&x, 32);
            stream->readBits(&y, 32);
            stream->readBits(&z, 32);
            m_position.x = x; m_position.y = y; m_position.z = z;
        }
        else
        {
            m_position.x = 0.0f; m_position.y = 0.0f; m_position.z = 0.0f;
        }

        int hasRotation = 0;
        stream->readBits(&hasRotation, 1);
        if (hasRotation == 1)
        {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            stream->readBits(&x, 32);
            stream->readBits(&y, 32);
            stream->readBits(&z, 32);
            m_rotation.x = x; m_rotation.y = y; m_rotation.z = z;
        }
        else
        {
            m_rotation.x = 0.0f; m_rotation.y = 0.0f; m_rotation.z = 0.0f;
        }
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool CreateObjectEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    unsigned int templateId = m_templateId;
    stream->writeBits(&templateId, 32);

    int networkableId = m_networkableId;
    stream->writeBits(&networkableId, 16);

    int defaultTeam = m_defaultTeam;
    stream->writeBits(&defaultTeam, 2);

    const bool explicitMode = (m_mode != 1);
    if (explicitMode) { int one = 1; stream->writeBits(&one,  1); }
    else              { int zero = 0; stream->writeBits(&zero, 1); }

    if (explicitMode)
    {
        int mode = static_cast<int>(m_mode);
        stream->writeBits(&mode, 8);
    }
    else
    {
        m_mode = 1;                          // a write from a serializer - see above

        const bool sendPosition = !isApproximatelyZero(m_position);
        if (sendPosition) { int one = 1; stream->writeBits(&one,  1); }
        else              { int zero = 0; stream->writeBits(&zero, 1); }
        if (sendPosition)
        {
            stream->writeBits(&m_position.x, 32);
            stream->writeBits(&m_position.y, 32);
            stream->writeBits(&m_position.z, 32);
        }

        const bool sendRotation = !isApproximatelyZero(m_rotation);
        if (sendRotation) { int one = 1; stream->writeBits(&one,  1); }
        else              { int zero = 0; stream->writeBits(&zero, 1); }
        if (sendRotation)
        {
            stream->writeBits(&m_rotation.x, 32);
            stream->writeBits(&m_rotation.y, 32);
            stream->writeBits(&m_rotation.z, 32);
        }
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// Creates one transformed object for modes 0/1, or consecutive IDs at zero transforms for larger counts.
// Creation failures log and continue. The default team is applied through the first object's parent chain.
// Use the queried IGameClient subobject directly; casting it to GameClient would apply the base adjustment twice.
void CreateObjectEvent::executeClient()
{
    void* const gameClient = game->queryInterface(IID_IGameClient);
    if (!gameClient)
        return;                              // the dedicated server stops here

    if (m_mode <= 1)
    {
        if (!static_cast<IGameClient*>(gameClient)->createObject(m_templateId, m_networkableId,
                                                                &m_position, &m_rotation))   // +0x24
        {
            Debug(DEBUGTYPE_6, "Game/GameEvents/CreateObjectEvent.cpp", 159, "")
                << "Failed to create requested object templ id" << static_cast<int>(m_templateId)
                << " obj id " << static_cast<int>(m_networkableId);
        }
    }
    else
    {
        for (unsigned int i = 0; i < m_mode; ++i)
        {
            Vec3 zeroPosition; zeroPosition.x = zeroPosition.y = zeroPosition.z = 0.0f;
            Vec3 zeroRotation; zeroRotation.x = zeroRotation.y = zeroRotation.z = 0.0f;
            if (static_cast<IGameClient*>(gameClient)->createObject(m_templateId,
                                        static_cast<unsigned short>(m_networkableId + i),
                                        &zeroPosition, &zeroRotation) != true)               // +0x24
            {
                Debug(DEBUGTYPE_6, "Game/GameEvents/CreateObjectEvent.cpp", 151, "")
                    << "Failed to create requested multispawnedobject templ id"
                    << static_cast<int>(m_templateId)
                    << " obj id" << static_cast<int>(m_networkableId)
                    << " index " << static_cast<int>(i);
            }
        }
    }

    applyDefaultTeamToControlObject(m_networkableId, m_defaultTeam);   // written
}

// Kit creation always transmits its full position. The four-bit unlock field stores
// level + 1, representing -1 through 14; zero means no unlock. Creation index is four bits.
bool CreateKitEvent::deSerialize(io::BitStream* stream)
{
    unsigned int kitTemplateId = 0;
    stream->readBits(&kitTemplateId, 32);
    m_kitTemplateId = kitTemplateId;

    int networkableId = 0;
    stream->readBits(&networkableId, 16);
    m_networkableId = static_cast<unsigned short>(networkableId);

    float x = 0.0f, y = 0.0f, z = 0.0f;
    stream->readBits(&x, 32);
    stream->readBits(&y, 32);
    stream->readBits(&z, 32);
    m_position.x = x; m_position.y = y; m_position.z = z;

    int creationIndex = 0;
    stream->readBits(&creationIndex, 4);
    m_creationIndex = creationIndex;

    int unlockLevel = 0;
    stream->readBits(&unlockLevel, 4);
    m_chosenUnlockLevel = unlockLevel - 1;          // the bias - see above

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool CreateKitEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    unsigned int kitTemplateId = m_kitTemplateId;
    stream->writeBits(&kitTemplateId, 32);

    int networkableId = m_networkableId;
    stream->writeBits(&networkableId, 16);

    stream->writeBits(&m_position.x, 32);
    stream->writeBits(&m_position.y, 32);
    stream->writeBits(&m_position.z, 32);

    int creationIndex = static_cast<int>(m_creationIndex);
    stream->writeBits(&creationIndex, 4);

    int unlockLevel = m_chosenUnlockLevel + 1;      // the bias
    stream->writeBits(&unlockLevel, 4);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// Creates the client kit with zero rotation and its received creation/unlock data.
// Failures use diagnostic type 7, unlike ordinary object creation.
void CreateKitEvent::executeClient()
{
    void* const gameClient = game->queryInterface(IID_IGameClient);
    if (!gameClient)
        return;                              // the dedicated server stops here

    const bool ok = static_cast<IGameClient*>(gameClient)->createKit(                  // +0x28
        m_kitTemplateId, m_networkableId, m_creationIndex, m_chosenUnlockLevel,
        &m_position, Vec3::getZero());
    if (!ok)
    {
        Debug(DEBUGTYPE_7, "Game/GameEvents/CreateKitEvent.cpp", 122, "")
            << "Failed to create requested kit templ id" << static_cast<int>(m_kitTemplateId)
            << " obj id " << static_cast<int>(m_networkableId)
            << " kid id " << static_cast<int>(m_creationIndex);
    }
}

} // namespace hfe
} // namespace dice
