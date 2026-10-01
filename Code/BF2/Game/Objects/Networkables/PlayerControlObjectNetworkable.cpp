// Replicates vehicle-seat control state through a four-entry baseline ring.

#include "Networkables.h"

#include <cstdlib>                                   // std::abort - the 11-bit length overflow
#include <string>
#include <vector>

#include "../IObject.h"
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Core/StringUtil.h"                // formatString - the Debug payloads, as shipped
#include "../../../IO/BitStream.h"
#include "../../../IO/Network/NetworkManager.h"      // io::StatePrecisionFull - the apply gate
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {

// The TU's static - RegistredNetworkStats("PlayerControlObjectNetworkable", Vec3(100, 100, 100)),
// built by _..._252 (registered at 0x823EC10); all three colour components are 0x42C80000 = 100.0f.
namespace {
Vec3 pcoStatsColour()
{
    Vec3 c; c.x = 100.0f; c.y = 100.0f; c.z = 100.0f;
    return c;
}
} // namespace
io::RegistredNetworkStats PlayerControlObjectNetworkable::m_stats("PlayerControlObjectNetworkable",
                                                                  pcoStatsColour());

// Returns weapon's index in weapons, or logs an error and returns -1 when it is absent.
int getWeaponIndex(const std::vector<world::IObject*>& weapons, world::IObject* weapon)
{
    for (unsigned int i = 0; i < weapons.size(); ++i)
    {
        if (weapons[i] == weapon)
            return static_cast<int>(i);
    }

    Debug(DEBUGTYPE_6, "Game/Objects/Networkables/PlayerControlObjectNetworkable.cpp", 63, "")
        << "No matching weapon found";
    return -1;
}

// Marks weapon-slot and ducking changes for seats with cameras.
// Seats without cameras delegate to an active child soldier; requires a player-control ancestor.
void PlayerControlObjectNetworkable::updateStateMask(unsigned int* stateMask)
{
    if (m_object->getFlags() & 1)
        return;                                          // before the ring moves

    // the parent-chain walk - see above.
    world::IPlayerControlObject* pco = 0;
    for (world::IObject* o = m_object; o; o = o->getParent())
    {
        pco = static_cast<world::IPlayerControlObject*>(
                  o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            break;
    }

    // NOT NULL-checked - see above. The ELF dereferences pco here regardless.
    if (!pco->getOwnCameras()->empty())
    {
        m_prev = m_head;
        m_head = (m_head + 1) & 3;
        m_next = m_head;

        Record& next = m_records[m_next];
        Record& prev = m_records[m_prev];

        for (int slot = 0; slot < 3; ++slot)
        {
            if (pco->getActiveWeapon(slot))              // called twice per slot
            {
                // getWeapons returns a vector through an untyped interface slot.
                // [RETIRE-WHEN: pco-getweapons-vector]
                const std::vector<world::IObject*>& weapons =
                    *static_cast<const std::vector<world::IObject*>*>(pco->getWeapons(slot));
                next.weaponIndex[slot] = getWeaponIndex(weapons, pco->getActiveWeapon(slot));
            }
            else
            {
                next.weaponIndex[slot] = -1;             // same sentinel as a failed lookup
            }

            if (next.weaponIndex[slot] != prev.weaponIndex[slot])
                *stateMask |= (1u << slot);
        }

        next.isDucking = pco->getIsDucking() ? 1 : 0;
        if (prev.isDucking != next.isDucking)
            *stateMask |= 0x08;
    }
    else if (pco->isChildSoldierActive())
    {
        // THE delegation - see above.
        world::IObject* const occupant = pco->getOccupingObject();
        io::INetworkable* const networkable = occupant->getNetworkable();
        networkable->updateStateMask(stateMask);
    }
}

// create - 0x8779E80, 145 B: outer -> 0; new(0x58); NetworkableBase::NetworkableBase; the
// NetworkableObjectBase stores; the two vptrs; +0x50 = 1; +0x54 = 0; queryInterface(iid), release()
// on a null answer. The same factory shape as every sibling.
IBase* PlayerControlObjectNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    PlayerControlObjectNetworkable* self = new PlayerControlObjectNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

// [INFERRED] from create's inlined stores.
PlayerControlObjectNetworkable::PlayerControlObjectNetworkable()
{
    m_refCount = 1;                                  // +0x50
    m_records  = 0;                                  // +0x54
}

// D2/D1 0x823D670/0x823D6B0, 64 B each: revector both vptrs, delete[] the ring when there is one,
// then NetworkableBase's chain. unlike SpringNetworkable's dtor it does NOT null the pointer
// after the delete - reproduced.
PlayerControlObjectNetworkable::~PlayerControlObjectNetworkable()
{
    if (m_records)
        delete[] m_records;
}

// The quartet - refcount +0x50.
void  PlayerControlObjectNetworkable::addRef() { ++m_refCount; }
int   PlayerControlObjectNetworkable::getRef() { return m_refCount; }
int   PlayerControlObjectNetworkable::release()
{
    const bool last = (m_refCount == 1);
    const int  r    = --m_refCount;
    if (last)
        delete this;
    return r;
}

// IBase / INetworkable -> the primary; INetworkableObject -> the +0x3C base.
void* PlayerControlObjectNetworkable::queryInterface(unsigned int iid) const
{
    PlayerControlObjectNetworkable* self = const_cast<PlayerControlObjectNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// init - 0x823D730, 110 B. Bind (+0x44 / the cached +0x18 interface), ring of 4 x 36 with only the
// two header fields zeroed per record (time and byte04 - the weapon indices and the ducking flag
// start as heap residue), then m_next = 0 explicitly - the one sibling init that reseeds the ring
// cursor - and true. No reset(), no priming updateStateMask (compare SpringNetworkable::init).
bool PlayerControlObjectNetworkable::init(void* object)
{
    m_object = static_cast<world::IObject*>(object);                             // +0x44
    m_networkableObject =
        static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));   // +0x18

    Record* ring = new Record[4];                                                // 0x90 bytes
    for (int i = 0; i < 4; ++i)
    {
        ring[i].time   = 0.0f;
        ring[i].byte04 = false;
        // weaponIndex[0..2], _gap14, isDucking and _pad21 deliberately NOT zeroed - the
        // ELF's loop is two stores per record and nothing else.
    }
    m_records = ring;                                                            // +0x54
    if (!ring)
        return false;
    m_next = 0;                                                                  // +0x38
    return true;
}

// The in-TU copy of IO/BitStream.inl's ranged write - the same pair RankEvent.cpp and
// ConnectionTypeEvent.cpp carry, because the ELF inlines the .inl at every use site. lower is
// always 0 here; kept as a parameter so the Debug line prints the same three numbers.
namespace {
unsigned int bitsToHold(unsigned int n)              // how many bits cover 0..n-1
{
    unsigned int bits = 0;
    do
        ++bits;
    while ((1u << bits) - 1u < n);
    return bits;
}

void writeRangedBits(io::BitStream& stream, unsigned int value,
                     unsigned int lower, unsigned int upper)
{
    if (value > upper)
    {
        stream.setError(4);                          // reachedEndOfStream = 1, errorCode = 4
        Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
            << "(fatal error) Value out of bounds val: " << value
            << " lower " << lower
            << " upper: " << upper;
        return;                                      // the value is NOT written
    }
    int scratch = static_cast<int>(value);
    stream.writeBits(&scratch, bitsToHold(upper + 1));
}
} // namespace

// Writes seat state using a 4-bit mask, or an 11-bit length-prefixed occupant update without cameras.
// Returns false on write failure; delegated payloads above 2047 bits abort.
// The caller must roll back the stream after a failed delegated write.
bool PlayerControlObjectNetworkable::getNetUpdate(io::BitStream& stream, io::BaseLineData* baseline,
                                                  unsigned int stateMask, int precision)
{
    // the parent-chain walk - identical to updateStateMask's, missing null check included.
    world::IPlayerControlObject* pco = 0;
    for (world::IObject* o = m_object; o; o = o->getParent())
    {
        pco = static_cast<world::IPlayerControlObject*>(
                  o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            break;
    }

    if (!pco->getOwnCameras()->empty())
    {
        m_updateStartBitPos = stream.getBitPosition();                           // +0x30
        m_next = m_head;                                                         // the re-sync

        unsigned int mask = stateMask;
        stream.writeBits(&mask, 4);

        const Record& rec = m_records[m_next];
        if (stateMask & 0x01)
            writeRangedBits(stream, static_cast<unsigned int>(rec.weaponIndex[0] + 1), 0, 8);
        if (stateMask & 0x02)
            writeRangedBits(stream, static_cast<unsigned int>(rec.weaponIndex[1] + 1), 0, 8);
        if (stateMask & 0x04)
            writeRangedBits(stream, static_cast<unsigned int>(rec.weaponIndex[2] + 1), 0, 8);
        if (stateMask & 0x08)
        {
            if (rec.isDucking)
            {
                unsigned int one = 1;
                stream.writeBits(&one, 1);
            }
            else
            {
                unsigned int zero = 0;
                stream.writeBits(&zero, 1);
            }
        }

        if (stream.hasError())
        {
            stream.clearError();
            return false;
        }
        m_stats.m_writeBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
        return true;
    }

    // the delegation wrapper
    io::BSPosition mark;
    if (stream.m_readWriteSwitch == 1)
    {
        mark.m_readWriteSwitch = 1;
        mark.m_position        = stream.m_readBitPos;
    }
    else
    {
        mark.m_readWriteSwitch = stream.m_readWriteSwitch;
        mark.m_position        = stream.m_bitPos;
    }
    stream.skipBits(11);                                                         // the length hole
    const unsigned int payloadStart = stream.getBitPosition();

    if (pco->isChildSoldierActive())
    {
        world::IObject* const occupant = pco->getOccupingObject();
        io::INetworkable* const networkable = occupant->getNetworkable();
        if (!networkable->getNetUpdate(stream, baseline, stateMask, precision))
            return false;                            // hole left unpatched - the caller rewinds
    }

    const unsigned int payloadBits = stream.getBitPosition() - payloadStart;
    if (payloadBits > 0x7FF)
        std::abort();                                // the field cannot carry it - as shipped

    stream.setWritePosition(mark);
    unsigned int length = payloadBits;
    stream.writeBits(&length, 11);
    if (stream.m_canResetWritePosition)
    {
        stream.m_canResetWritePosition = false;
        stream.setPosition(*reinterpret_cast<io::BSPosition*>(&stream.m_dword24));
    }
    else
    {
        Debug(DEBUGTYPE_6, "IO/BitStream.inl", 341)
            << formatString("Call to resetWritePosition without prior call to setPosition");
    }
    return true;
}

// Decodes seat state or delegates a length-prefixed occupant update.
// Applies weapon/ducking fields only at full precision or while occupied. Unknown occupants' payloads are skipped.
void PlayerControlObjectNetworkable::setNetUpdate(io::BitStream& stream, float updateTime,
                                                  bool ghostActive, io::BaseLineData* baseline,
                                                  int precision)
{
    // the same walk, third copy - as inlined three times in the ELF.
    world::IPlayerControlObject* pco = 0;
    for (world::IObject* o = m_object; o; o = o->getParent())
    {
        pco = static_cast<world::IPlayerControlObject*>(
                  o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            break;
    }

    if (!pco->getOwnCameras()->empty())
    {
        unsigned int mask = 0;
        makeRoomForNextUpdate(ghostActive);                                      // result unused
        if (m_prev != m_next)
            m_records[m_next] = m_records[m_prev];                               // the nine-dword copy
        m_records[m_next].time = updateTime;
        m_updateStartBitPos = stream.getBitPosition();                           // +0x30

        stream.readBits(&mask, 4);
        if (stream.hasError())
        {
            // attested AND fatal - type 6 aborts; the clear is the ELF's dead tail.
            Debug(DEBUGTYPE_6, "Game/Objects/Networkables/PlayerControlObjectNetworkable.cpp", 219,
                  __FUNCTION__)
                << formatString("error reading data");
            stream.clearError();
            return;
        }

        Record& rec = m_records[m_next];
        for (int slot = 0; slot < 3; ++slot)
        {
            if (mask & (1u << slot))
            {
                unsigned int value = 0;
                stream.readBits(&value, bitsToHold(9));                          // 4 bits
                rec.weaponIndex[slot] = static_cast<int>(value) - 1;
            }
        }
        if (mask & 0x08)
        {
            unsigned int ducking = 0;
            stream.readBits(&ducking, 1);
            rec.isDucking = (ducking == 1);
        }

        m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);

        if (stream.hasError())
        {
            // the second attested site - line 229. Same fatality, same dead tail.
            Debug(DEBUGTYPE_6, "Game/Objects/Networkables/PlayerControlObjectNetworkable.cpp", 229,
                  __FUNCTION__)
                << formatString("error reading data");
            stream.clearError();
        }
        else if (io::StatePrecisionFull == precision || pco->getOccupingPlayer())
        {
            for (int slot = 0; slot < 3; ++slot)
            {
                if ((mask & (1u << slot)) && rec.weaponIndex[slot] != -1)
                {
                    const std::vector<world::IObject*>& weapons =
                        *static_cast<const std::vector<world::IObject*>*>(pco->getWeapons(slot));
                    pco->setActiveWeapon(weapons[rec.weaponIndex[slot]], slot);
                }
            }
            if (mask & 0x08)
                pco->setIsDucking(rec.isDucking);
        }
        return;
    }

    // the wrapper mirror
    unsigned int length = 0;
    stream.readBits(&length, 11);
    if (length)
    {
        if (pco->isChildSoldierActive())
        {
            world::IObject* const occupant = pco->getOccupingObject();
            io::INetworkable* const networkable = occupant->getNetworkable();
            networkable->setNetUpdate(stream, updateTime, ghostActive, baseline, precision);
        }
        else
        {
            stream.skipBits(length);                 // an unknown occupant's state, stepped over
        }
    }
}

// 8 - eight bits of descriptor headroom.
unsigned int PlayerControlObjectNetworkable::getMinimumBitSize() const
{
    return 8;
}

// -1 - a control object claims every ghost-state bit.
unsigned int PlayerControlObjectNetworkable::getGhostStateMask() const
{
    return 0xFFFFFFFFu;
}

// empty.
void PlayerControlObjectNetworkable::predict(float)
{
}

} // namespace hfe
} // namespace dice
