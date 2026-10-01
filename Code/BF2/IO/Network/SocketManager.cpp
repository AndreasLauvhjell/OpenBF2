// Socket factories, platform initialization and readiness waiting. [INFERRED] Source placement.

#include "SocketManager.h"

// [PLATFORM] WSAStartup for init(), and fd_set/select/timeval for sleep(). The two platforms'
// fd_set are structurally different - see the block over sleep().
#if defined(_WIN32)
#  include <winsock2.h>
#else
#  include <sys/select.h>
#  include <sys/time.h>
#  include <unistd.h>
#endif

#include <cstring>
#include "NetworkLayer/Socket/UDPSocket.h"

namespace {
#if defined(_WIN32)
typedef SOCKET NativeSocketHandle;
#else
typedef int NativeSocketHandle;
#endif
} // anonymous namespace

namespace dice {
namespace hfe {
namespace io {

// ds:0x8AC8C9C. Not a statically-initialised pointer - setSocketManager() fills it at runtime from
// BF2EngineSetup::initNetwork, so `= 0` here is correct (unlike the nine in Core/Globals.cpp).
SocketManager* socketManager = 0;

// C1 0x847D410, 14 bytes: install the vptr, nothing else. There are no members to initialise.
SocketManager::SocketManager()
{
}

// D1 0x847D440, 27 B / D0 0x847D460, 40 B. The 27 bytes are the vptr restore and the frame; there
// is no member teardown because there are no members.
SocketManager::~SocketManager()
{
}

// Initializes the socket subsystem. POSIX returns true without work.
// [PLATFORM] Windows startup is a substitution that requests Winsock 2.2 and reports failure.
// Calls are not deduplicated; close() does not balance WSAStartup with WSACleanup.
bool SocketManager::init()                             // 0x847D490, read: `mov eax, 1`
{
#if defined(_WIN32)
    WSADATA wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)   // [PLATFORM] no ELF counterpart
        return false;
#endif
    return true;
}

// close() is empty in the binary (5 bytes: frame in, frame out) and is left empty. The tempting
// symmetry - a WSACleanup to pair the WSAStartup above - is not added: nothing in this tree calls
// SocketManager::close(), so the pairing would be untested, and an unbalanced WSACleanup tears
// Winsock down under live sockets. A process that exits without it loses nothing.
void SocketManager::close()         { }                // empty in the binary
bool SocketManager::isInitialized() { return true; }   // 0x847D7E0, read: `mov eax, 1`
bool SocketManager::isLinkActive()  { return true; }   // 0x847D7F0, read: `mov eax, 1`

// Creates a UDP socket and connects to address. Returns null for an empty name or failed resolution/connect.
// The socket is allocated before validation; blocking is forwarded to the socket implementation.
ISocket* SocketManager::connect(const std::string& address, bool blocking)
{
    UDPSocket* socket = new UDPSocket();
    ISocket::Address peer;
    std::memset(peer.sockaddr, 0, sizeof(peer.sockaddr));
    peer.length = 16;
    if (!address.empty() && UDPSocket::nameToAddress(address, &peer, 0) == 0
        && socket->connect(peer, blocking))
        return socket;

    delete socket;
    return 0;
}

// ELF 0x847D600 / PE 0x672400. An unconnected, nonblocking socket bound to an ephemeral port.
ISocket* SocketManager::createSocket()
{
    UDPSocket* socket = new UDPSocket();
    if (!socket->create(false))
    {
        delete socket;
        return 0;
    }
    return socket;
}

// ELF 0x847D650 / PE 0x672440. Both ignore gethostname's result and construct from the uninitialised
// 256-byte buffer directly: no failure guard, forced terminator, or fallback string is shipped.
std::string SocketManager::getHostName()
{
    char name[256];
    ::gethostname(name, sizeof(name));
    return std::string(name);
}

// setSocketManager - `new SocketManager()` (operator new(4u)), store into the global, return it.
// BF2EngineSetup::initNetwork tests the return value, not the global.
SocketManager* setSocketManager()
{
    SocketManager* manager = new SocketManager();
    socketManager = manager;                        // ds:0x8AC8C9C
    return manager;
}

// Closes and deletes the socket-manager singleton, then clears its pointer.
// The existing Windows close path does not balance WSAStartup.
void shutdownSocketManager()
{
    if (socketManager)                              // ds:0x8AC8C9C
    {
        socketManager->close();                     // vt +4
        delete socketManager;                       // operator delete in the ELF, no dtor - see the header
        socketManager = 0;
    }
}

// Allocates and binds a UDP socket to addressAndPort; returns ownership to the caller on success.
// On failure, deletes the socket and returns null. The address string is passed by value to listen.
ISocket* SocketManager::listen(const std::string& addressAndPort, bool b)
{
    UDPSocket* socket = new UDPSocket();                    // operator new(0x34), 0x847D4BE

    if (!socket->listen(addressAndPort, b))                 // by value - see above
    {
        delete socket;                                      // vtable-dispatched, 0x847D50x
        return 0;
    }
    return socket;
}

// Waits for socket readability. Timeout truncates to whole seconds, so subsecond values poll.
// Callers must fit FD_SETSIZE. [PLATFORM] Native descriptor-set representation is used;
// [CHOSEN, not recovered] Windows x64 passes zero for select's ignored first argument.
void SocketManager::sleep(std::vector<ISocket*>& sockets, float seconds)
{
    struct timeval timeout;
    timeout.tv_sec  = (long)seconds;
    // (seconds - seconds) - reproduced verbatim. See the block above; this is always 0.
    timeout.tv_usec = (long)((seconds - seconds) * 1000.0f);

    fd_set readfds;
    FD_ZERO(&readfds);                                  // [PLATFORM] the hand-rolled clear loop

#if !defined(_WIN64)
    int maxFd = 0;
#endif
    for (std::vector<ISocket*>::const_iterator i = sockets.begin(); i != sockets.end(); ++i)
    {
        // Read the concrete UDP socket handle; all entries must use this implementation.
        UDPSocket* const s = static_cast<UDPSocket*>(*i);
        FD_SET(static_cast<NativeSocketHandle>(s->m_socket), &readfds);
#if !defined(_WIN64)
        if (s->m_socket > maxFd)
            maxFd = s->m_socket;
#endif
    }

    // [UNRESOLVED] The original return type is ambiguous; existing callers ignore select's result.
#if defined(_WIN64)
    ::select(0, &readfds, 0, 0, &timeout);             // native handles remain in fd_array
#else
    ::select(maxFd + 1, &readfds, 0, 0, &timeout);      // nfds ignored on Winsock
#endif
}

} // namespace io
} // namespace hfe
} // namespace dice
