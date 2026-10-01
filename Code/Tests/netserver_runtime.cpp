// [TEMP] SCAFFOLDING: exercises real NetServer/SWConnection/UDPSocket methods.
// Synthetic connection-map and statistics-ring contents model only the tested conditions.
// All datagrams stay on 127.0.0.1; no retail client or join state machine is involved.
#include <winsock2.h>
#include "../BF2/IO/Network/ProtocolLayer/NetServer.h"
#include "../BF2/IO/Network/ProtocolLayer/SWConnection.h"
#include "../BF2/IO/Network/NetworkLayer/Socket/UDPSocket.h"
#include "../BF2/IO/BitStream.h"
#include "../BF2/Core/Ids.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

using namespace dice::hfe;
using namespace dice::hfe::io;

static int checks = 0, failures = 0;
static void check(bool ok, const char* message)
{
    ++checks;
    if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
}
static bool nearEqual(float a, float b) { return std::fabs(a - b) < 0.0001f; }

// [TEMP] Only observes deletion; the production NetServer destructor still executes.
struct TrackedServer : NetServer
{
    bool* destroyed;
    explicit TrackedServer(bool* flag) : destroyed(flag) {}
    ~TrackedServer() { *destroyed = true; }
};

static void testSettings()
{
    bool destroyed = false;
    INetServer* api = new TrackedServer(&destroyed);
    check(api->getRef() == 1 && api->addRef() == 2, "reference increment returns count");
    check(api->release() == 1 && !destroyed, "release retains nonzero references");
    check(api->queryInterface(IID_IBase) == api, "IBase identity");
    check(api->queryInterface(IID_INetServer) == api, "INetServer identity");
    check(api->queryInterface(0) == 0 && api->getRef() == 1, "query does not add a reference");
    check(api->release() == 0 && destroyed, "last release invokes virtual destruction");

    NetServer server;
    api = &server;
    check(api->getConnectionTimeOut() == 10000, "default connection timeout");
    api->setConnectionTimeOut(0xffffffffu);
    check(api->getConnectionTimeOut() == 0xffffffffu, "timeout preserves all unsigned bits");
    check(api->getMaxNumberOfConnections() == 64, "default connection limit");
    api->setMaxNumberOfConnections(256);
    api->setMaxNumberOfConnections(257);
    check(api->getMaxNumberOfConnections() == 256, "oversize limit is refused");
    check(api->getNumberOfConnections() == 0 && api->getMaxPacketSize() == 1472,
          "empty map and exact packet ceiling");
    check(api->setPassword(std::string(32, 'p')) && !api->setPassword(std::string(33, 'q')),
          "normal password boundary");
    check(api->getPassword() == std::string(32, 'p'), "rejected password preserves old value");
    check(api->setReservedPassword(std::string(32, 'r'), -2), "reserved password accepts 32 bytes");
    check(static_cast<int>(server.m_reservedPasswordSize) == -2 &&
          static_cast<int>(server.m_dword4BC) == -2, "reserved slot count is not clamped");
    check(!api->setReservedPassword(std::string(33, 'x'), 8) &&
          server.m_reservedPassword == std::string(32, 'r') &&
          static_cast<int>(server.m_dword4BC) == -2, "rejected reservation is atomic");
    check(api->setReconnectPassword("1234", 3) && server.m_reconnectTimeout == 180000,
          "reconnect password starts three-minute timeout");
    server.m_reconnectTimeout = 41;
    check(!api->setReconnectPassword("12345", 8) && server.m_reconnectPassword == "1234" &&
          server.m_reconnectPasswordSize == 3 && server.m_reconnectTimeout == 41,
          "rejected reconnect password preserves password, count and timer");
    check(api->setReconnectPassword("", -1) && server.m_reconnectTimeout == 180000,
          "empty reconnect password still restarts timeout");
    server.setName("alpha");
    std::string name = server.getName();
    name[0] = 'z';
    check(server.getName() == "alpha", "getName returns an independent value");
    api->setPacketDropRatio(1.0f);
    check(api->getPacketDropRatio() == 0.0f, "drop controls tolerate absent socket");
}

static void testStats()
{
    NetServer server;
    SWConnection connection;
    INetServer* api = &server;
    ConnectionStatus status;
    std::memset(&status, 0x5a, sizeof(status));
    const ConnectionStatus original = status;
    check(!api->getConnectionStatus(7, status) &&
          std::memcmp(&status, &original, sizeof(status)) == 0, "unknown id preserves output");
    api->setStatsGatheringEnabled(7, true);
    check(!api->getStatsGatheringEnabled(7) && server.m_connections.empty(),
          "unknown stats id does not insert a connection");
    server.m_connections[7] = 0;
    check(!api->getConnectionStatus(7, status) && !api->getStatsGatheringEnabled(7),
          "null map entry is unavailable");
    server.m_connections[7] = &connection;
    check(api->getNumberOfConnections() == 1 && api->getStatsGatheringEnabled(7),
          "live map entry and default gathering flag");
    check(!api->getConnectionStatus(7, status) &&
          std::memcmp(&status, &original, sizeof(status)) == 0, "unallocated ring preserves output");

    // Two samples plus eight untouched slots. The retail denominator is ten, not two.
    CONNSTATSENTRY samples[10] = {};
    samples[0] = {10, 20, 30, 40, 50, 60, 700, 800, 103};
    samples[1] = {20, 40, 60, 80, 100, 120, 900, 1000, 108};
    connection.m_dword4 = new unsigned char[sizeof(samples)];
    std::memcpy(connection.m_dword4, samples, sizeof(samples));
    CONNSTATSENTRY average;
    check(connection.getConnectionStats(&average), "gathering reads allocated ring");
    check(nearEqual(average.bytesSentPerSecond, 3) && nearEqual(average.bytesReceivedPerSecond, 6) &&
          nearEqual(average.packetsSentPerSecond, 9) && nearEqual(average.packetsReceivedPerSecond, 12),
          "rates include the unused history slots");
    check(average.unknown18 == 0 && average.unknown1C == 0 && average.ping == 21,
          "reserved statistics stay zero and ping uses integer division");
    check(api->getConnectionStatus(7, status) && nearEqual(status.packetsLostSendPerSecond, 18) &&
          nearEqual(status.packetsLostReceivePerSecond, 15) && status.ping == 21,
          "status swaps the two loss-rate fields at the interface boundary");
    api->setStatsGatheringEnabled(7, false);
    const ConnectionStatus previous = status;
    check(!api->getStatsGatheringEnabled(7) && !api->getConnectionStatus(7, status) &&
          std::memcmp(&status, &previous, sizeof(status)) == 0, "disabled gathering preserves output");
    api->setStatsGatheringEnabled(7, true);
    check(api->getConnectionStatus(7, status), "re-enabling gathering retains history");
}

static void testDatagrams()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa)) { check(false, "Winsock initialization"); return; }
    {
        UDPSocket sender, receiver;
        if (!sender.listen("127.0.0.1:0", false) || !receiver.listen("127.0.0.1:0", false))
        { check(false, "loopback sockets bind"); return; }
        sockaddr_in local = {};
        int length = sizeof(local);
        check(getsockname(static_cast<SOCKET>(receiver.m_socket),
                          reinterpret_cast<sockaddr*>(&local), &length) == 0,
              "receiver ephemeral port");
        NetServer server;
        server.m_listenSocket2 = &sender; // borrowed fixture; no init/shutDown ownership here
        server.setPacketDropRatio(0.25f);
        check(nearEqual(server.getPacketDropRatio(), 0.25f) && nearEqual(sender.m_float2C, 75),
              "drop ratio translates to retained percentage");
        server.setPacketDropRatio(-1.0f);
        check(nearEqual(server.getPacketDropRatio(), 0), "negative ratio clamps to zero");
        server.setPacketDropRatio(2.0f);
        check(nearEqual(server.getPacketDropRatio(), 1), "ratio above one clamps to one");
        server.setPacketDropRatio(std::numeric_limits<float>::quiet_NaN());
        check(std::isnan(server.getPacketDropRatio()), "unordered ratio remains unordered");
        server.setPacketDropRatio(0);

        const char payload[] = {'r', 'a', 'w', '\0', 'x'};
        server.sendRawPacket("127.0.0.1", ntohs(local.sin_port), payload, sizeof(payload));
        unsigned char buffer[1473];
        int received = -1;
        for (int i = 0; i < 100 && received < 0; ++i)
        { received = receiver.receiveFrom(buffer, sizeof(buffer)); if (received < 0) Sleep(1); }
        check(received == sizeof(payload) && std::memcmp(buffer, payload, sizeof(payload)) == 0,
              "raw send preserves embedded NUL and exact length");

        SWConnection connection;
        ISocket::Address peer = {};
        std::memcpy(peer.sockaddr, &local, sizeof(local));
        peer.length = sizeof(local);
        connection.setPeerAddress(peer);
        connection.m_hasReservedSlot = true;
        server.m_connections[7] = &connection;
        server.setReservedPassword("reserve", 2);
        server.m_dword4BC = 1;
        BitStream stream(0, 0);
        unsigned char outgoing[1473];
        server.m_bitStreamOut = &stream;
        server.m_packetOutBuffer = outgoing;
        check(server.closeConnection(99, 13, "bye") == 9, "unknown disconnect returns 9");
        const unsigned int sentBefore = sender.m_packetsSent;
        check(server.closeConnection(7, 13, "bye") == 0 &&
              sender.m_packetsSent == sentBefore + 3, "disconnect sends three datagrams");
        check(server.m_pendingRemovalCount == 1 && server.m_pendingRemovals[0] == 7 &&
              server.m_dword4BC == 2 && server.getNumberOfConnections() == 1,
              "disconnect queues removal and refunds the reserved slot");
        check(server.closeConnection(7, 13, "bye") == 9 &&
              sender.m_packetsSent == sentBefore + 3 && server.m_pendingRemovalCount == 1,
              "duplicate disconnect neither sends nor queues again");
        int datagrams = 0;
        for (int i = 0; i < 100 && datagrams != 3; ++i)
        {
            received = receiver.receiveFrom(buffer, sizeof(buffer));
            if (received < 0) { Sleep(1); continue; }
            ++datagrams;
            BitStream packet(buffer, received);
            packet.setBitSize(received * 8);
            unsigned char id = 255;
            unsigned int type = 0, reason = 0;
            check(readBasicHeader(&packet, id, type) && type == 5 &&
                  packet.readBits(&reason, 32) && reason == 13,
                  "disconnect packet header and reason survive loopback");
        }
        check(datagrams == 3, "receiver observes exactly three disconnect copies");
        // Signed jle in both binaries: an unsigned comparison would leave this at zero.
        server.m_connections[8] = &connection;
        connection.m_hasReservedSlot = false;
        server.setReservedPassword("", -1);
        server.m_dword4BC = 0;
        check(server.closeConnection(8, 13, "bye") == 0 && server.m_dword4BC == 0xffffffffu,
              "reserved-slot refund ceiling uses a signed comparison");
        receiver.close();
        UDPSocket rebound;
        const std::string address = "127.0.0.1:" + std::to_string(ntohs(local.sin_port));
        check(rebound.listen(address, false), "closed receiver port immediately rebinds");
    }
    WSACleanup();
}

int main()
{
    static_assert(sizeof(void*) == 4, "x86 runtime gate");
    testSettings();
    testStats();
    testDatagrams();
    std::printf("NetServer runtime: %d checks, %d failures (loopback only; no production boot)\n",
                checks, failures);
    return failures ? 1 : 0;
}
