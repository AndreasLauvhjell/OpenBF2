#include "AmbientEffectAreaEvent.h"

#include "../Objects/Game/AmbientEffectArea.h"    // playEffect, the template's attach flag
#include "../Objects/Game/AmbientEffectAreaManager.h"
#include "../Objects/IObject.h"                   // getTemplate
#include "../Objects/ObjectLayoutAccess.h"       // direct base-template member read
#include "../../Core/Debug.h"                     // deSerialize's abort
#include "../../IO/BitStream.h"                   // readBits / writeBits
#include "../../IO/Network/NetworkManager.h"      // getObject

namespace dice {
namespace hfe {
namespace io    { extern NetworkManager* networkManager; }        // ds:0x8AC3D1C - Core/Globals.cpp

namespace {
// .rodata dword GEAmbientEffectArea (0x88882C8) = 34.
const int GEAmbientEffectArea = 34;

} // namespace


// THE position IS zeroed AND then  The ELF clears +0x0C..+0x14 before storing the
// argument into the same three words - dead stores GCC did not fold, reproduced only as the final
// assignment because the intermediate state is unobservable.
AmbientEffectAreaEvent::AmbientEffectAreaEvent(world::IObject* area, int team, Vec3 position,
                                               unsigned short objectNetId)
    : GameEvent()
    , m_position(position)                                            // +0x0C
    //  reads +14C; PE 53EC62 reads +16C. Native STL changes the
    // object layout again: read the recovered member through its concrete type.
    , m_areaId(static_cast<world::AmbientEffectArea*>(area)->m_id)
    , m_team(static_cast<unsigned char>(team))                        // +0x1C
    , m_area(static_cast<world::AmbientEffectArea*>(area))             // +0x20
{
    // The area template determines whether an attachment object is required.
    const world::AmbientEffectAreaTemplate* areaTemplate =
        static_cast<const world::AmbientEffectAreaTemplate*>(world::object_detail::LayoutAccess::objectTemplate(area));

    m_objectNetId = areaTemplate->m_attachToEventObject ? objectNetId : 0;        // +0x1E
}

int AmbientEffectAreaEvent::getType() const
{
    return GEAmbientEffectArea;
}

// Network ID zero means unparented and skips lookup. A missing nonzero ID also
// passes a null attachment to playEffect.
void AmbientEffectAreaEvent::executeClient()
{
    if (!m_area)                                                      // +0x20
        return;

    world::IObject* eventObject = 0;
    if (m_objectNetId != 0)
        eventObject = io::networkManager->getObject(m_objectNetId);

    m_area->playEffect(m_position, m_team, eventObject);              // [FRONTIER body in the tree]
}

// an explicit empty override. The server originates these; it never receives one.
bool AmbientEffectAreaEvent::executeServer(int /*connectionId*/)
{
    return true;
}

// Reads ids, resolves the effect area, then reads position. A missing area manager
// is fatal before the position is consumed.
bool AmbientEffectAreaEvent::deSerialize(io::BitStream* stream)
{
    int areaId = 0;
    stream->readBits(&areaId, 16);
    m_areaId = static_cast<unsigned int>(areaId);

    int team = 0;
    stream->readBits(&team, 8);
    m_team = static_cast<unsigned char>(team);

    int objectNetId = 0;
    stream->readBits(&objectNetId, 16);
    m_objectNetId = static_cast<unsigned short>(objectNetId);

    if (world::ambientEffectAreaManager)
    {
        m_area = world::ambientEffectAreaManager->getAreaFromId(static_cast<int>(m_areaId));
    }
    else
    {
        Debug(DEBUGTYPE_7, "Game/GameEvents/AmbientEffectAreaEvent.cpp", 68, "")
            << "Ambienteffectareamanager not found.";   // aborts - and does not return
    }

    stream->readBits(&m_position.x, 32);
    stream->readBits(&m_position.y, 32);
    stream->readBits(&m_position.z, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x8069350, 275 B. The same order, less the lookup.
bool AmbientEffectAreaEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int areaId = static_cast<int>(m_areaId);
    stream->writeBits(&areaId, 16);

    int team = m_team;
    stream->writeBits(&team, 8);

    int objectNetId = m_objectNetId;
    stream->writeBits(&objectNetId, 16);

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
