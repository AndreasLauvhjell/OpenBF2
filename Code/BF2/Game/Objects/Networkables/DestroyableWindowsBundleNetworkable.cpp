// Path: attested by __FILE__ in both ports. Complete surface
// ..0x8233B29, factory 0x8779D40 / PE 0x5E6D00..0x5E6FEE.
#include "Networkables.h"
#include "../Game/DestroyableWindowsBundle.h"
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Core/StringUtil.h"
#include "../../../IO/BitStream.h"

namespace dice { namespace hfe {
namespace {
Vec3 windowsStatsColour() { const Vec3 colour = {140, 40, 210}; return colour; }
}
io::RegistredNetworkStats DestroyableWindowsBundleNetworkable::m_stats(
    "DestroyableWindowsBundleNetworkable", windowsStatsColour());

// [INFERRED] constructor factoring: inlined in the retail factory. m_bundle is untouched until init.
DestroyableWindowsBundleNetworkable::DestroyableWindowsBundleNetworkable()
    : m_refCount(1), m_baselines(0) {}
IBase* DestroyableWindowsBundleNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer) return 0;
    DestroyableWindowsBundleNetworkable* self = new DestroyableWindowsBundleNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result) self->release();
    return result;
}
DestroyableWindowsBundleNetworkable::~DestroyableWindowsBundleNetworkable()
{
    if (m_baselines) { delete[] m_baselines; m_baselines = 0; }
}
void DestroyableWindowsBundleNetworkable::addRef() { ++m_refCount; }
int DestroyableWindowsBundleNetworkable::getRef() { return m_refCount; }
int DestroyableWindowsBundleNetworkable::release()
{
    const int result = --m_refCount;
    if (!result) delete this;
    return result;
}
void* DestroyableWindowsBundleNetworkable::queryInterface(unsigned int iid) const
{
    DestroyableWindowsBundleNetworkable* self = const_cast<DestroyableWindowsBundleNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable) return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject) return static_cast<io::INetworkableObject*>(self);
    return 0;
}
bool DestroyableWindowsBundleNetworkable::init(void* object)
{
    m_object = static_cast<world::IObject*>(object);
    m_bundle = static_cast<world::DestroyableWindowsBundle*>(m_object);
    m_networkableObject = static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));
    BaseLine* ring = new BaseLine[4];
    for (int i = 0; i < 4; ++i) { ring[i].time = 0.0f; ring[i].byte04 = false; }
    m_baselines = ring;
    m_next = 0;
    // Retail primes only slot zero. The other pane-state words remain untouched until sampled.
    ring[0].paneState = m_bundle->m_states;
    return m_baselines != 0;
}
void DestroyableWindowsBundleNetworkable::updateStateMask(unsigned int* stateMask)
{
    m_prev = m_head;
    m_head = (m_head + 1) & 3;
    m_next = m_head;
    m_baselines[m_next].paneState = m_bundle->m_states;
    if (m_baselines[m_next].paneState != m_baselines[m_prev].paneState) *stateMask |= 1u;
}
bool DestroyableWindowsBundleNetworkable::getNetUpdate(
    io::BitStream& stream, io::BaseLineData*, unsigned int, int)
{
    m_updateStartBitPos = stream.getBitPosition();
    m_next = m_head;
    const unsigned int states = m_baselines[m_head].paneState;
    stream.writeBits(&states, m_bundle->getNumStates());
    if (stream.hasError()) { stream.clearError(); return false; }
    m_stats.m_writeBits += stream.getBitPosition() - m_updateStartBitPos;
    return true;
}
void DestroyableWindowsBundleNetworkable::setNetUpdate(
    io::BitStream& stream, float updateTime, bool ghostActive, io::BaseLineData*, int)
{
    m_updateStartBitPos = stream.getBitPosition();
    makeRoomForNextUpdate(ghostActive);
    if (m_prev != m_next) m_baselines[m_next] = m_baselines[m_prev];
    m_baselines[m_next].time = updateTime;
    unsigned int states = 0;
    stream.readBits(&states, m_bundle->getNumStates());
    m_baselines[m_next].paneState = states;
    if (stream.hasError())
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/DestroyableWindowsBundleNetworkable.cpp", 122, "")
            << formatString("error reading data");
        stream.clearError();
        return;
    }
    m_stats.m_readBits += stream.getBitPosition() - m_updateStartBitPos;
    m_bundle->updateStates(m_baselines[m_next].paneState);
}
// Retail is empty on both ports: no interpolation for discrete pane bits.
void DestroyableWindowsBundleNetworkable::predict(float) {}
unsigned int DestroyableWindowsBundleNetworkable::getMinimumBitSize() const { return 32; }
} }
