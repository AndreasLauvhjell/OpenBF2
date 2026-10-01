// [TEMP]
// Test scaffolding for real UDP bind, address conversion, blocking mode, send/receive, and close.
// All traffic stays on loopback; no clients or game protocol are exercised.

#include "../BF2/IO/Network/NetworkLayer/Socket/UDPSocket.h"

#include <winsock2.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>

using dice::hfe::io::UDPSocket;

static int checks = 0, failures = 0;
static void check(bool ok, const char* what)
{
    ++checks;
    if (!ok) { ++failures; std::printf("  FAIL  %s\n", what); }
}

int main()
{
    // The WSAStartup that SocketManager::init performs. Called directly so this test does not drag
    // in SocketManager's own dependency set; the substitution being verified is the same one.
    WSADATA wsa;
    check(WSAStartup(MAKEWORD(2, 2), &wsa) == 0, "WSAStartup(2.2) succeeds");


    // 1. stringToNameAndPort - the split, including the documented quirks.

    {
        UDPSocket s;
        std::string name; unsigned short port = 0xBEEF;

        s.stringToNameAndPort(std::string("127.0.0.1:16994"), &name, &port);
        check(name == "127.0.0.1", "split: name is 127.0.0.1");
        check(port == 16994,       "split: port is 16994");

        // an empty name is the INADDR_ANY case the dedicated server uses when sv.serverIP is unset
        s.stringToNameAndPort(std::string(":16994"), &name, &port);
        check(name.empty(),  "split: empty name survives (INADDR_ANY)");
        check(port == 16994, "split: port parses with no host");

        // no colon at all: whole string is the name, port stays 0
        port = 0xBEEF;
        s.stringToNameAndPort(std::string("nocolon"), &name, &port);
        check(name == "nocolon", "split: no colon -> whole string is the name");
        check(port == 0,         "split: no colon -> port zeroed");

        // the scan is backward, so the last colon separates
        s.stringToNameAndPort(std::string("a:b:99"), &name, &port);
        check(name == "a:b", "split: last colon wins (backward scan)");
        check(port == 99,    "split: port after the last colon");
    }


    // 2. THE bind itself, on loopback.

    const unsigned short kPort = 16994;
    char addr[64];
    std::sprintf(addr, "127.0.0.1:%u", (unsigned)kPort);

    {
        UDPSocket s;
        check(s.m_socket == -1, "ctor seeds the handle to -1, not 0");

        const bool bound = s.listen(std::string(addr), false);
        check(bound, "listen(\"127.0.0.1:16994\", false) BINDS");
        check(s.m_socket != -1 && s.m_socket != 0, "a real descriptor is held after listen");

        // false was asked for, so the socket must be NON-blocking - the polled-receive contract
        check(s.getBlockingMode() == false, "blocking mode is false (non-blocking) as requested");

        // ask the OS independently: the port really is ours
        sockaddr_in got; int len = sizeof(got);
        check(getsockname((SOCKET)s.m_socket, (sockaddr*)&got, &len) == 0, "getsockname succeeds");
        check(ntohs(got.sin_port) == kPort, "the OS agrees the socket is on port 16994");
        check(got.sin_addr.s_addr == inet_addr("127.0.0.1"), "the OS agrees it is on 127.0.0.1");

        s.close();
        check(s.m_socket == -1, "close() resets the handle to -1");
    }


    // 3. immediate rebind - the property the listener tranche asks for, and the one the conditional
    //    SO_REUSEADDR actually decides. Port 16994 has a non-zero high byte, so the option IS set.

    {
        UDPSocket a;
        check(a.listen(std::string(addr), false), "rebind: first bind takes the port");
        a.close();

        UDPSocket b;
        check(b.listen(std::string(addr), false), "rebind: SAME PORT rebinds immediately");
        b.close();
    }


    // 4. blocking mode both ways.

    {
        UDPSocket s;
        check(s.listen(std::string("127.0.0.1:16995"), true), "listen with blocking=true binds");
        check(s.getBlockingMode() == true, "blocking mode is true when requested");
        check(s.setBlockingMode(false), "setBlockingMode(false) succeeds");
        check(s.getBlockingMode() == false, "the cached flag follows the change");
        s.close();
    }


    // 5. A bind that must fail - an address that is not on this machine. This is what proves the
    //    address is genuinely reaching bind() rather than being ignored.

    {
        UDPSocket s;
        check(s.listen(std::string("203.0.113.7:16994"), false) == false,
              "listen to a non-local address FAILS (the address really reaches bind)");
    }


    // 6. A real loopback round trip, now that send/receiveFrom are recovered bodies rather than
    //    aborting fixtures. Two sockets, one datagram each way, and the peer-address handshake that
    //    UDPSocket's +0x19 selector and receiveFrom's m_peerAddress write are supposed to make work.

    {
        UDPSocket rx, tx;
        check(rx.listen(std::string("127.0.0.1:16996"), true), "roundtrip: receiver binds 16996");
        check(tx.listen(std::string("127.0.0.1:16997"), true), "roundtrip: sender binds 16997");

        // Point the sender at the receiver. m_useOwnAddress stays false, so send() takes
        // m_peerAddress - the selector resolved from send()'s own instructions this session.
        sockaddr_in to;
        memset(&to, 0, sizeof(to));
        to.sin_family      = AF_INET;
        to.sin_port        = htons(16996);
        to.sin_addr.s_addr = inet_addr("127.0.0.1");
        memcpy(tx.m_peerAddress.sockaddr, &to, 16);

        const unsigned char payload[] = "BF2-loopback";
        const int           n         = (int)sizeof(payload);

        check(tx.m_packetsSent == 0, "roundtrip: send counter starts at 0");
        check(tx.send(payload, n) == 1, "roundtrip: send() reports success");
        check(tx.m_packetsSent == 1, "roundtrip: +0x30 incremented on a successful send");

        unsigned char buf[64];
        memset(buf, 0, sizeof(buf));
        check(rx.receiveFrom(buf, (int)sizeof(buf)) == n, "roundtrip: receiveFrom returns the count");
        check(memcmp(buf, payload, n) == 0, "roundtrip: the payload arrives intact");

        // receiveFrom stores the sender into the receiver's m_peerAddress - the other half of the
        // selector. The receiver can now reply without being told who to reply to.
        sockaddr_in from;
        memcpy(&from, rx.m_peerAddress.sockaddr, 16);
        check(ntohs(from.sin_port) == 16997, "roundtrip: receiveFrom recorded the sender's PORT");
        check(from.sin_addr.s_addr == inet_addr("127.0.0.1"),
              "roundtrip: receiveFrom recorded the sender's ADDRESS");

        const unsigned char reply[] = "ack";
        check(rx.send(reply, (int)sizeof(reply)) == 1,
              "roundtrip: receiver replies to the stored peer with no extra bookkeeping");

        unsigned char back[64];
        memset(back, 0, sizeof(back));
        check(tx.receiveFrom(back, (int)sizeof(back)) == (int)sizeof(reply),
              "roundtrip: the reply comes back to the original sender");
        check(memcmp(back, reply, sizeof(reply)) == 0, "roundtrip: reply payload intact");

        rx.close();
        tx.close();
    }

    std::printf("socket loopback: %d checks, %d failures\n", checks, failures);
    WSACleanup();
    return failures ? 1 : 0;
}
