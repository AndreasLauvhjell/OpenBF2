// Incoming packet classifier embedded by value in NetServer. Header path [INFERRED].
// Register agents before socket processing starts. The classifier owns a one-slot vector of agent
// records plus a mutex; it is nonpolymorphic. The adjacent virtual interface is FilteringAgent.

#ifndef IO_NETWORK_PACKETFILTER_H                    // [INFERRED] guard style
#define IO_NETWORK_PACKETFILTER_H

#include <list>
#include <vector>

#include "../../Core/Mutex.h"

namespace dice {
namespace hfe {
namespace io {

// [UNRESOLVED] Original packet-kind enumerators; only slot zero is modeled.
enum FilteredPacketType { FILTEREDPACKETTYPE_0 = 0 };

// Stored packet with an owned malloc payload and a NUL-terminated name of at most 31 characters.
struct StoredPacket
{
    void*        data;                               // +0x00  owned (malloc), freed by ~AgentData
    unsigned int size;                               // +0x04
    char         name[32];                           // +0x08
};

// Packet classifier interface. Keep the virtual destructor before match.
class FilteringAgent
{
public:
    virtual ~FilteringAgent() = 0;
    // bool: PacketFilter::processPacket tests AL at ELF 0x8479CDA; the PE implementation
    // materializes AL at 0x68481D/832. Only the destructor slot is collapsed by MSVC.
    virtual bool match(const unsigned char* data, unsigned int size) = 0;   // +0x08
};

// PunkBuster packet classifier, installed even when server-side PunkBuster handling is disabled.
class PunkBusterFilterAgent : public FilteringAgent
{
public:
    virtual bool match(const unsigned char* data, unsigned int size);
};

// Owns its current first agent record, classifier and queued payloads. Re-registration overwrites without deleting the old record.
class PacketFilter
{
public:
    PacketFilter();
    ~PacketFilter();

    // all four .
    // processPacket returns bool, and the sense is inverted from the name - true means "nothing
    // filtered this, pass it on". See the block over its body.
    void registerAgent(FilteredPacketType, FilteringAgent*);
    void getPackets(FilteredPacketType, std::list<StoredPacket>*);
    bool processPacket(const unsigned char*, unsigned int);
    void storePacket(const unsigned char*, unsigned int,
                     FilteredPacketType, const char*);

    // Owns one classifier and its buffered packets.
    // The native list uses the toolchain allocator; packet payloads separately use malloc/free.
    class AgentData
    {
    public:
        AgentData() : m_agent(0) {}

        // ELF 0x8479A10 (D1) / 0x8479990 (D2), 114 B - and there is no D0, so no virtual
        // destructor and no vptr. It frees every stored payload before emptying the list.
        ~AgentData();

        // bufferPacket - Copies the payload and appends a StoredPacket.
        void bufferPacket(const unsigned char* src, unsigned int size, const char* name);

        /* +0x00 */ FilteringAgent*        m_agent;        // owned, deleted after queued payloads
        /* +0x04 */ std::list<StoredPacket> m_packetList;   // retail: 8 bytes, +0x04..+0x0B
    };

    // Native list layout depends on the STL implementation; do not add padding to imitate its internal fields.

public:
    /* +0x00 */ std::vector<AgentData*> m_agents;      // one null element, sized by the ctor
    /* +0x0C */ Mutex                   m_processMutex;
};

// no static_assert on sizeof(AgentData). One was added and then removed the same day - see the
// two-pass correction note inside the class for why the premise behind it was wrong.

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_PACKETFILTER_H
