// Dedicated-server receive-thread lifecycle. [INFERRED] Source placement.

#include "NetServerThread.h"

#include "NetServer.h"
#include "../SocketManager.h"                // io::socketManager, and its select() wait
#include "../NetworkLayer/Socket/UDPSocket.h"   // m_listenSocket is a UDPSocket*; the
                                              // vector holds ISocket*, so the base must
                                              // be visible for the conversion

#include <vector>

namespace dice {
namespace hfe {
namespace io {

// Constructs the thread wrapper and borrows server for its receive loop.
NetServerThread::NetServerThread(NetServer* server)
    : m_netServer(server)                               // +0x08
{
}

// Destroys the thread wrapper. The caller must stop and join it first; m_netServer is borrowed.
NetServerThread::~NetServerThread()
{
}

// Receives on the listen socket captured at thread start; replacing that socket requires a restart.
// Waits up to four seconds, updates connections and drains packets until result 8 or 14.
// m_stopMutex protects the stop request; m_mutex protects updates and the shutdown check.
// The owner must signal shutdown and join through Thread::stop before destroying the server.
// [UNRESOLVED] names of the two drain-result enumerators.
void NetServerThread::run()
{
    // One socket, gathered once. Not refreshed inside the loop - see above.
    std::vector<ISocket*> sockets;
    sockets.push_back(m_netServer->m_listenSocket);     // +0x468

    for (;;)
    {
        m_netServer->m_stopMutex.lock();                 // +0x4CC - guards only the stop flag
        if (m_netServer->m_stopRequested)                     // +0x4EC - the stop flag
            break;                                      // leaves the lock held - released below
        m_netServer->m_stopMutex.unlock();

        socketManager->sleep(sockets, 4.0f);            // the select() wait

        m_netServer->m_mutex.lock();                    // +0x4C8 - guards the connection update

        int status = 0;                                 // the out-parameter both calls share
        m_netServer->_updateConnections(&status);

        int result;
        do
        {
            result = m_netServer->_update(&status);
        }
        while (result != 8 && result != 14);            // [UNRESOLVED] sentinels - see above

        const bool leaving = m_netServer->m_shuttingDown;    // +0x47D, read under m_mutex
        m_netServer->m_mutex.unlock();

        if (leaving)
            return;                                     // the exit that does not unlock m_stopMutex
    }

    // The break above exits with m_stopMutex still held; this is its release. Two exits, only one
    // of which passes through here - which is why the unlock is outside the loop rather than at the
    // top of it.
    m_netServer->m_stopMutex.unlock();
}

} // namespace io
} // namespace hfe
} // namespace dice
