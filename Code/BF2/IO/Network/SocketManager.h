// Stateless socket manager interface and implementation. [INFERRED] Source placement.

#ifndef IO_NETWORK_SOCKETMANAGER_H     // [INFERRED] guard style
#define IO_NETWORK_SOCKETMANAGER_H

#include <string>
#include <vector>

namespace dice {
namespace hfe {
namespace io {

class ISocket;

class SocketManager
{
public:
    SocketManager();                                        // C1 0x847D410, 14 B - installs the vptr

    // Preserve virtual order, with the destructor last. listen receives the port inside the address string.
    virtual bool init();                                    // +0x00
    virtual void close();                                   // +0x04  0x847D4A0,  5 B - empty
    virtual std::string getHostName();                      // +0x08
    // ELF 0x847D550 / PE 0x672360 return the allocated socket or null, not a flag.
    virtual ISocket* connect(const std::string& address, bool); // +0x0C  176 B
    // return type corrected bool -> ISocket*. NetServer::init assigns the result straight into
    // its UDPSocket* member (`this->listenSocket = <call>`), so it hands back the socket it opened,
    // not a success flag - and init's own failure test is `if (!listenSocket)`.
    virtual ISocket* listen(const std::string& addressAndPort, bool);   // +0x10
    virtual ISocket* createSocket();                        // +0x14
    virtual bool isInitialized();                           // +0x18
    virtual bool isLinkActive();                            // +0x1C
    virtual void sleep(std::vector<ISocket*>& sockets, float seconds);  // +0x20

    virtual ~SocketManager();                               // +0x24 D1 / +0x28 D0 - last, see above

    // no data members. sizeof is 4 - see the header block.
};

// ds:0x8AC8C9C. [INFERRED] name, from setSocketManager's single store.
extern SocketManager* socketManager;

// Allocates and publishes the socket-manager singleton, returning its pointer.
SocketManager* setSocketManager();
// Closes and deletes the socket manager, then clears the global pointer.
void shutdownSocketManager();

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_SOCKETMANAGER_H
