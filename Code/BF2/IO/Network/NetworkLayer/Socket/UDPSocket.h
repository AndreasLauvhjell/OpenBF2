// UDP socket with cached peer addresses, blocking mode and simulated packet retention.
// [PLATFORM] POSIX and Winsock operations are selected in the implementation.

#ifndef IO_NETWORK_NETWORKLAYER_SOCKET_UDPSOCKET_H   // [INFERRED] guard style
#define IO_NETWORK_NETWORKLAYER_SOCKET_UDPSOCKET_H

#include <string>
#include <list>
#include <vector>
#include <cstdint>

#include "ISocket.h"

// Declare the platform socket type in global scope so include order cannot create a namespaced type.
struct sockaddr_in;

namespace dice {
namespace hfe {
namespace io {

class UDPSocket : public ISocket
{
public:
    // [CHOSEN, not recovered] native-x64 port: Winsock socket is UINT_PTR. Keep the
    // original int carrier on x86/POSIX and widen only the Windows native handle,
    // never Address, packet lengths, counters or network identifiers.
#if defined(_WIN64)
    typedef std::uintptr_t SocketHandle;
#else
    typedef int SocketHandle;
#endif

    UDPSocket();
    virtual ~UDPSocket();

    // ISocket, in slot order. Implemented entries are marked written below
    virtual void close();                                               // +0x08 0x847D990  written
    // all three return 0 on error, not -1 - indistinguishable from a zero-length datagram.
    virtual int  send(const unsigned char*, int);                       // +0x0C 0x847D9E0  written
    virtual int  receiveFrom(unsigned char*, int);                      // +0x10 0x847DB60  written
    virtual int  receive(unsigned char*, int);                          // +0x14 0x847DB10  written
    virtual bool setBroadcast(unsigned short);                          // +0x18
    virtual void setPeerAddress(const Address&);                        // +0x1C 0x847E430,  69 B  written
    virtual const Address& getPeerAddress() const;                      // +0x20 0x847E490,  11 B  written
    virtual unsigned int   getChallengeIp(const Address&) const;        // +0x24 0x847E480,  11 B  written
    // bool, not void - . The `movzx eax, al` at 0x847E78A is the tell; the
    // full three-witness argument is on ISocket::addressToString, which this overrides.
    virtual bool addressToString(const Address&, std::string*);         // +0x28 0x847E770,  31 B  written
    virtual bool getPacketInfo(PacketInfo*);                            // +0x2C
    virtual bool getBlockingMode() const;                               // +0x30 0x847DBC0  written
    virtual bool setBlockingMode(bool);                                 // +0x34 0x847DBD0  written

    // Creates and binds a socket to addressAndPort with the requested blocking mode. Returns whether binding succeeded.
    bool listen(std::string addressAndPort, bool blocking);
    bool create(bool blocking);
    bool connect(const Address&, bool);
    void showError();

    // Splits an address string into host name and port.
    static bool stringToNameAndPort(const std::string&, std::string*, unsigned short*);
    static bool stringToAddress(const std::string&, struct sockaddr_in*);
    // Resolves a host or numeric address. Returns 0 on success, 1 when no host is found,
    // or 2 when numeric address text exceeds fifteen characters.
    static int nameToAddress(const std::string&, Address*, std::list<Address>*);
    static bool equalAddressAndPort(const Address*, const Address*);
    static bool equalAddress(const Address*, const Address*);
    // Formats a UDP address into the supplied string and returns true.
    static bool UDPaddressToString(const Address&, std::string*);
    float getPacketDropRatio() const;
    void  setPacketDropRatio(float);
    void  getNonLocalInterfaceList(std::vector<Address>&);

public:
    /* +0x04 */ SocketHandle m_socket;        // = -1, the invalid handle; offset is x86 only
    /* +0x08 */ Address      m_address;       // 17 bytes; length seeded to 0x10
    /* +0x19 */ bool         m_useOwnAddress; // send() uses m_address when true, otherwise m_peerAddress.
    /* +0x1A */ Address      m_peerAddress;   // 17 bytes; length seeded to 0x10  [INFERRED name -
                                              //   setPeerAddress/getPeerAddress are the pair]
    /* +0x2B */ bool         m_blocking;      // the cached blocking mode - set/getBlockingMode
                                              //   are the pair. cached, not queried: Winsock's
                                              //   fionbio is set-only, so nothing can ask the OS.
    /* +0x2C */ float        m_float2C;       // Retained percentage, initially 100.
    /* +0x30 */ unsigned int m_packetsSent;    // ++ on every successful sendto, never else.
                                              //   a delivery count, not an attempt count.
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_NETWORKLAYER_SOCKET_UDPSOCKET_H
