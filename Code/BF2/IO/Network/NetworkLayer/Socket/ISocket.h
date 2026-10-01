// Socket operations with the virtual destructor first. listen and create belong to the concrete class.
// [INFERRED] Source placement.

#ifndef IO_NETWORK_NETWORKLAYER_SOCKET_ISOCKET_H     // [INFERRED] guard style
#define IO_NETWORK_NETWORKLAYER_SOCKET_ISOCKET_H

#include <string>

namespace dice {
namespace hfe {
namespace io {

class ISocket
{
public:
    // Sixteen raw socket-address bytes followed by their length. Platform code interprets the address family.
    struct Address
    {
        unsigned char sockaddr[16];      // +0x00  zeroed by the constructor
        unsigned char length;            // +0x10  = 0x10
    };

    // Packet characteristics written by getPacketInfo. [UNRESOLVED] Field meanings and total original record size.
    struct PacketInfo
    {
        unsigned int value00; // +0x00 = 1
        unsigned int value04; // +0x04 = 1448
        unsigned int value08; // +0x08 = 1064
        unsigned int value0C; // +0x0C = 28 on PE; untouched by ELF
        unsigned int value10; // +0x10 = 1 on PE; untouched by ELF
    };

    virtual ~ISocket() {}                                                     // +0x00 / +0x04

    virtual void close()                                                 = 0; // +0x08
    virtual int  send(const unsigned char* data, int size)               = 0; // +0x0C
    virtual int  receiveFrom(unsigned char* buffer, int size)            = 0; // +0x10
    virtual int  receive(unsigned char* buffer, int size)                = 0; // +0x14
    virtual bool setBroadcast(unsigned short)                            = 0; // +0x18
    virtual void setPeerAddress(const Address&)                          = 0; // +0x1C
    virtual const Address& getPeerAddress() const                        = 0; // +0x20
    virtual unsigned int   getChallengeIp(const Address&) const          = 0; // +0x24
    // Formats the address into the output string and reports success.
    virtual bool addressToString(const Address&, std::string*)           = 0; // +0x28
    // PE 0x6677C8 explicitly writes AL=1; ELF 0x847E4BA explicitly writes EAX=1.
    virtual bool getPacketInfo(PacketInfo*)                              = 0; // +0x2C
    virtual bool getBlockingMode() const                                 = 0; // +0x30
    virtual bool setBlockingMode(bool)                                   = 0; // +0x34
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_NETWORKLAYER_SOCKET_ISOCKET_H
