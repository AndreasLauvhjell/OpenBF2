// Packet-filter registration and buffering. [INFERRED] Source placement.

#include "PacketFilter.h"

#include <cstdlib>                           // malloc / free
#include <cstring>                           // memcpy / strncpy

namespace dice {
namespace hfe {
namespace io {

// Pure virtual base destructor definition for filtering agents.
FilteringAgent::~FilteringAgent()
{
}

// Creates one empty agent slot and the processing mutex. Registration does not grow the slot array.
PacketFilter::PacketFilter()
    : m_agents(1, static_cast<AgentData*>(0))       // allocate(4) + fill_n(begin, 1, 0)
{
}

// Deletes the first agent record before destroying the processing mutex and slot array.
PacketFilter::~PacketFilter()
{
    AgentData* const data = m_agents[0];
    if (data)
        delete data;
}

// Registers an agent under the processing lock. type must index the existing slot array.
// Replacing an occupied slot does not release its previous record.
void PacketFilter::registerAgent(FilteredPacketType type, FilteringAgent* agent)
{
    m_processMutex.lock();                              // +0x0C

    AgentData* data = new AgentData;                    // operator new(0xC), 0x8479B4A
    // AgentData owns the packet-list sentinel through its container constructor.
    data->m_agent = agent;                              // +0x00

    m_agents[type] = data;                              // indexed - see the note above

    m_processMutex.unlock();
}

// Frees queued payloads, deletes the classifier and destroys the packet list.
PacketFilter::AgentData::~AgentData()
{
    for (std::list<StoredPacket>::iterator i = m_packetList.begin();
         i != m_packetList.end(); ++i)
    {
        free(i->data);                                  // the malloc'd payload
    }
    if (m_agent)
        delete m_agent;
    m_packetList.clear();                               // __clear at 0x8479A39
}

// Copies a packet into owned malloc storage and truncates its name to 31 characters.
// Allocation failure silently drops the packet. getPackets transfers payload ownership to its caller.
void PacketFilter::AgentData::bufferPacket(const unsigned char* src, unsigned int size,
                                           const char* name)
{
    void* copy = malloc(size);
    if (!copy)
        return;                                         // silent drop - see above

    memcpy(copy, src, size);

    StoredPacket stored;
    stored.data = copy;                                 // +0x00
    stored.size = size;                                 // +0x04
    strncpy(stored.name, name, 32);                     // +0x08
    stored.name[31] = 0;                                // the hand-written terminator

    m_packetList.insert(m_packetList.end(), stored);
}

// Classifies incoming data against filter slot zero under m_processMutex.
// Returns true when the packet should pass to the game protocol, false when the agent claims it.
// A registered record must contain a live agent. PunkBuster claims only the seven-byte prefix
// FF FF FF FF PB_; AgentData owns that agent. Its source placement here is [INFERRED].
bool PunkBusterFilterAgent::match(const unsigned char* data, unsigned int size)
{
    // The function-local static the ELF keeps in .rodata at 0x88DDC69.
    static const unsigned char header[7] = { 0xFF, 0xFF, 0xFF, 0xFF, 'P', 'B', '_' };

    return size > 6 && std::memcmp(data, header, 7) == 0;
}

bool PacketFilter::processPacket(const unsigned char* data, unsigned int size)
{
    m_processMutex.lock();

    AgentData* const d = m_agents[0];                   // hard-coded 0 - see above
    const bool claimed = (d != 0) && d->m_agent->match(data, size);   // FilteringAgent vtable +0x08

    m_processMutex.unlock();
    return !claimed;                                    // true == not filtered, pass it on
}

// Buffers a packet under the processing lock when an agent is registered. type must be a valid slot index.
void PacketFilter::storePacket(const unsigned char* data, unsigned int size,
                               FilteredPacketType type, const char* name)
{
    m_processMutex.lock();

    AgentData* const d = m_agents[type];                // indexed, unchecked
    if (d)
        d->bufferPacket(data, size, name);

    m_processMutex.unlock();
}

// Replaces out with the agent's queued packets and transfers their payload ownership to the caller.
// The caller must free payloads from any previous contents before calling; clearing a list does not free them.
void PacketFilter::getPackets(FilteredPacketType type, std::list<StoredPacket>* out)
{
    if (!out->empty())                                  // before the lock - see above
        out->clear();

    m_processMutex.lock();

    AgentData* const d = m_agents[type];
    if (d)
    {
        for (std::list<StoredPacket>::const_iterator i = d->m_packetList.begin();
             i != d->m_packetList.end(); ++i)
        {
            out->insert(out->end(), *i);                // 0x8479C4x
        }
        // cleared without freeing the payloads - ownership has moved to `out`.
        d->m_packetList.clear();                        // __clear at 0x8479C6x
    }

    m_processMutex.unlock();
}

} // namespace io
} // namespace hfe
} // namespace dice
