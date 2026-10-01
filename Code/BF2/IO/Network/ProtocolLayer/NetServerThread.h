// Server receive thread. Its virtual destructor supports shutdown through the Thread interface.
// [INFERRED] Source placement.

#ifndef IO_NETWORK_PROTOCOLLAYER_NETSERVERTHREAD_H    // [INFERRED] guard style
#define IO_NETWORK_PROTOCOLLAYER_NETSERVERTHREAD_H

#include "../../../Core/Thread.h"

namespace dice {
namespace hfe {
namespace io {

class NetServer;

class NetServerThread : public Thread
{
public:
    explicit NetServerThread(NetServer* server);
    virtual ~NetServerThread();

    // Runs the socket receive loop until shutdown.
    virtual void run();

private:
    /* +0x08 */ NetServer* m_netServer;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_PROTOCOLLAYER_NETSERVERTHREAD_H
