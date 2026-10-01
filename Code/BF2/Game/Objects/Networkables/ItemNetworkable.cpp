// Item replication. Mask bit 0x04 reports item-type changes but neither wire direction transfers it.

#include "Networkables.h"
#include <cmath>
#include <cstring>
#include "../Item.h"
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../IO/BitStream.h"
#include "../../../Math/Mat4.h"

namespace dice {
namespace hfe {
namespace {
Vec3 itemStatsColour()
{
    Vec3 colour; colour.x = 140.0f; colour.y = 140.0f; colour.z = 190.0f;
    return colour;
}

// Inlined sign/magnitude writes in ELF 0x823B0C0. Width excludes the sign bit.
void writeItemSigned(io::BitStream& stream, int value, unsigned int width)
{
    unsigned int sign = value < 0 ? 1u : 0u;
    stream.writeBits(&sign, 1);
    unsigned int magnitude = value < 0 ? 0u - static_cast<unsigned int>(value)
                                       : static_cast<unsigned int>(value);
    stream.writeBits(&magnitude, width);
}
int readItemSigned(io::BitStream& stream, unsigned int width)
{
    unsigned int sign = 0, magnitude = 0;
    stream.readBits(&sign, 1);
    stream.readBits(&magnitude, width);
    return sign == 1 ? -static_cast<int>(magnitude) : static_cast<int>(magnitude);
}
} // namespace

// ELF static initializer 0x823BB00: the extra "Object" is the shipped stats label.
io::RegistredNetworkStats ItemNetworkable::m_stats("ItemObjectNetworkable", itemStatsColour());

IBase* ItemNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    ItemNetworkable* self = new ItemNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

// ELF factory's inlined stores; dedicated PE has a distinct ctor at 0x4D9AE0.
ItemNetworkable::ItemNetworkable()
{
    m_refCount = 1;
    m_records = 0;
}


ItemNetworkable::~ItemNetworkable()
{
    if (m_records)
    {
        delete[] m_records;
        m_records = 0;
    }
}

// /80/A0/F0; PE folds the quartet with sibling networkables.
void ItemNetworkable::addRef() { ++m_refCount; }
int ItemNetworkable::getRef() { return m_refCount; }
int ItemNetworkable::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        delete this;
    return remaining;
}
void* ItemNetworkable::queryInterface(unsigned int iid) const
{
    ItemNetworkable* self = const_cast<ItemNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// . Only slot zero gets gameplay fields during init.
// The other slots' owner/type/enabled/team bytes are deliberately not initialized.
bool ItemNetworkable::init(void* object)
{
    m_object = static_cast<world::IObject*>(object);
    m_networkableObject =
        static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));
    m_records = new Record[4];
    for (int i = 0; i != 4; ++i)
    {
        m_records[i].time = 0.0f;
        m_records[i].byte04 = false;
        m_records[i].position.x = m_records[i].position.y = m_records[i].position.z = 0.0f;
        m_records[i].rotation.x = m_records[i].rotation.y = m_records[i].rotation.z = 0.0f;
        m_records[i].rotation.w = 1.0f;
    }
    m_next = 0;
    const Mat4& transform = m_object->getParent() ? m_object->getRelativeTransformation()
                                                  : m_object->getAbsoluteTransformation();
    m_records[m_next].position.x = transform.m[12];
    m_records[m_next].position.y = transform.m[13];
    m_records[m_next].position.z = transform.m[14];
    m_records[m_next].ownerId = static_cast<world::Item*>(m_object)->getOwnerId();
    m_records[m_next].itemType = static_cast<world::Item*>(m_object)->getItemType();
    m_records[m_next].enabled = static_cast<world::Item*>(m_object)->isEnabled();
    m_records[m_next].team = static_cast<world::Item*>(m_object)->getTeam();
    return m_records != 0;
}

// . No flag gate; reload the object for every snapshot.
void ItemNetworkable::updateStateMask(unsigned int* stateMask)
{
    m_prev = m_head;
    m_head = (m_head + 1) & 3;
    m_next = m_head;
    const Mat4& transform = m_object->getParent() ? m_object->getRelativeTransformation()
                                                  : m_object->getAbsoluteTransformation();
    m_records[m_next].position.x = transform.m[12];
    m_records[m_next].position.y = transform.m[13];
    m_records[m_next].position.z = transform.m[14];
    const Vec3& next = m_records[m_next].position;
    const Vec3& prev = m_records[m_prev].position;
    const float eps = 1.1920929e-7f;
    if (!(std::fabs(next.x - prev.x) <= eps
       && std::fabs(next.y - prev.y) <= eps
       && std::fabs(next.z - prev.z) <= eps))
        *stateMask |= 0x01;
    m_records[m_next].ownerId = static_cast<world::Item*>(m_object)->getOwnerId();
    if (m_records[m_next].ownerId != m_records[m_prev].ownerId)
        *stateMask |= 0x02;
    m_records[m_next].itemType = static_cast<world::Item*>(m_object)->getItemType();
    if (m_records[m_next].itemType != m_records[m_prev].itemType)
        *stateMask |= 0x04;
    m_records[m_next].enabled = static_cast<world::Item*>(m_object)->isEnabled();
    if (m_records[m_next].enabled != m_records[m_prev].enabled)
        *stateMask |= 0x08;
    m_records[m_next].team = static_cast<world::Item*>(m_object)->getTeam();
    if (m_records[m_next].team != m_records[m_prev].team)
        *stateMask |= 0x10;
}

// . Position scale 0x3A03126F; signed widths 9/5
// include their sign. Overflow clears the error and returns false without rewinding.
bool ItemNetworkable::getNetUpdate(io::BitStream& stream, io::BaseLineData* /*baseline*/,
                                    unsigned int stateMask, int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();
    m_next = m_head;
    stream.writeBits(&stateMask, 5);
    if (stateMask & 0x01)
    {
        const Vec3& position = m_records[m_next].position;
        if (stream.hasCompressionVector())
            stream.writeCompressedVector(position, stream.getCompressionVector(), 0.0005f);
        else
        {
            unsigned int selector = 0;
            stream.writeBits(&selector, 2);
            stream.writeBits(&position.x, 32);
            stream.writeBits(&position.y, 32);
            stream.writeBits(&position.z, 32);
        }
    }
    if (stateMask & 0x02)
        writeItemSigned(stream, m_records[m_next].ownerId, 8);
    // Retail deliberately has no bit-0x04 payload.
    if (stateMask & 0x08)
    {
        unsigned int enabled = m_records[m_next].enabled ? 1u : 0u;
        stream.writeBits(&enabled, 1);
    }
    if (stateMask & 0x10)
        writeItemSigned(stream, m_records[m_next].team, 4);
    if (stream.hasError())
    {
        stream.clearError();
        return false;
    }
    m_stats.m_writeBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    return true;
}

// . Preserve the live orientation when applying position.
// Corrupt input takes the shipped Debug(6) site and returns without stats/application.
void ItemNetworkable::setNetUpdate(io::BitStream& stream, float updateTime, bool ghostActive,
                                    io::BaseLineData* /*baseline*/, int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();
    makeRoomForNextUpdate(ghostActive);
    if (m_prev != m_next)
        std::memcpy(&m_records[m_next], &m_records[m_prev], sizeof(Record));
    m_records[m_next].time = updateTime;
    unsigned int stateMask = 0;
    stream.readBits(&stateMask, 5);
    if (stateMask & 0x01)
        m_records[m_next].position =
            stream.readCompressedVector(stream.getCompressionVector(), 0.0005f);
    if (stateMask & 0x02)
        m_records[m_next].ownerId = readItemSigned(stream, 8);
    if (stateMask & 0x08)
    {
        unsigned int enabled = 0;
        stream.readBits(&enabled, 1);
        m_records[m_next].enabled = (enabled == 1);
    }
    if (stateMask & 0x10)
        m_records[m_next].team = readItemSigned(stream, 4);
    if (stream.hasError())
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/ItemNetworkable.cpp", 153, __FUNCTION__)
            << "error reading data";
        stream.clearError();
        return;
    }
    m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    if (stateMask)
    {
        if (stateMask & 0x01)
        {
            world::IObject* object = m_object;
            const Mat4& current = object->getParent() ? object->getRelativeTransformation()
                                                      : object->getAbsoluteTransformation();
            Quat rotation;
            rotation.x = rotation.y = rotation.z = 0.0f;
            rotation.fromMat(current);
            const Vec3 position = m_records[m_next].position;
            Mat4 transform = {};
            transform.m[0] = transform.m[5] = transform.m[10] = transform.m[15] = 1.0f;
            rotation.toMat(transform);
            transform.m[12] = position.x;
            transform.m[13] = position.y;
            transform.m[14] = position.z;
            if (object->getParent())
                object->setRelativeTransformation(transform);
            else
                object->setAbsoluteTransformation(transform);
        }
        if (stateMask & 0x02)
            static_cast<world::Item*>(m_object)->setOwnerId(m_records[m_next].ownerId);
        if (stateMask & 0x08)
        {
            if (m_records[m_next].enabled)
                static_cast<world::Item*>(m_object)->enableItem();
            else
                static_cast<world::Item*>(m_object)->disableItem();
        }
        if (stateMask & 0x10)
            static_cast<world::Item*>(m_object)->setTeam(m_records[m_next].team);
    }
}

// (empty); PE 0x5E5D90: ret 4.
void ItemNetworkable::predict(float) {}
// 36, despite the variable-width packet.
unsigned int ItemNetworkable::getMinimumBitSize() const { return 36; }

static_assert(sizeof(ItemNetworkable::Record) == 52, "Item baseline stride");
#if defined(_WIN64) || defined(__LP64__)
// [CHOSEN, not recovered] Native base-class vptrs/pointers and baseline pointer.
// The 52-byte Record above contains only fixed-width scalars and is copied with sizeof(Record);
// getNetUpdate/setNetUpdate continue to encode the same explicit field bit widths.
static_assert(sizeof(ItemNetworkable) == 128, "native x64 ItemNetworkable layout");
#else
static_assert(sizeof(ItemNetworkable) == 88, "ItemNetworkable x86 size");
#endif

} // namespace hfe
} // namespace dice
