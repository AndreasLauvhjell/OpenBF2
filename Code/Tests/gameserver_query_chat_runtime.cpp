// [TEMP] [CHOSEN, not recovered] component fixture; never linked into production.
// Real GameServer/Player/ClientConnection constructors and all newly recovered method bodies.
// Collaborator overrides observe chat posting, name availability, network queries and socket sends.
// Objects live to process exit. No listener, production boot, Python dispatch or retail-client run.
#include <winsock2.h>
#include <Windows.h>
#include <DbgHelp.h>
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#define private public
#define protected public
#include "../BF2/Game/GameServer/GameServer.h"
#include "../BF2/Game/GameLogic/Player.h"
#include "../BF2/Game/GameEvents/ChangePlayerNameEvent.h"
#undef protected
#undef private
#include "../BF2/Game/Common/ClientConnection.h"
#include "../BF2/Game/Common/StringManager.h"
#include "../BF2/Game/Common/GameEvent.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/System.h"
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/IO/Network/ProtocolLayer/NetServer.h"
#include "../BF2/IO/Network/NetworkLayer/Socket/UDPSocket.h"

using namespace dice::hfe;
namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* name)
{
    ++checks;
    if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    std::fflush(stdout);
}
LONG WINAPI reportFault(EXCEPTION_POINTERS* fault)
{
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, 0, TRUE);
    std::fprintf(stderr, "Fixture exception %08lx at %p\n", fault->ExceptionRecord->ExceptionCode,
                 fault->ExceptionRecord->ExceptionAddress);
    CONTEXT context = *fault->ContextRecord;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = context.Eip; frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;
    for (int i = 0; i < 16; ++i)
    {
        char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO); symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
            std::fprintf(stderr, "  %08llx %s+%llu\n", frame.AddrPC.Offset, symbol->Name, displacement);
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, GetCurrentThread(), &frame, &context,
                        0, SymFunctionTableAccess64, SymGetModuleBase64, 0)) break;
    }
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
struct Events : EventManager
{
    std::vector<StringReceivedEvent*> posted;
    bool contract = true;
    unsigned int postEvent(EventCategory category, EventNode* node, float delay) override
    {
        contract = contract && category == ECNetwork && delay == 0 && node->getType() == 11;
        posted.push_back(static_cast<StringReceivedEvent*>(node));
        return 17;
    }
};
struct Players : world::PlayerManager
{
    world::IPlayer* player = 0;
    world::IPlayer* local = 0;
    int localReads = 0;
    std::set<std::string> taken;
    std::vector<std::string> attempts, renamed;
    world::IPlayer* getLocalHumanPlayer() override { ++localReads; return local; }
    world::IPlayer* getPlayerFromId(int id) override
    { return player && player->getId() == id ? player : 0; }
    world::IPlayer* getPlayerFromName(const std::string& name) override
    { attempts.push_back(name); return taken.count(name) ? player : 0; }
    void renamePlayer(world::IPlayer*, std::string name) override { renamed.push_back(name); }
};
struct Server : GameServer
{
    std::vector<GameEvent*> broadcasts;
    void sendGameEventToAll(GameEvent* ev) override { broadcasts.push_back(ev); }
};
struct WrapperObserver : GameServer
{
    using GameServer::say;
    unsigned int channel = 0, type = 0, target = 0, flags = 0;
    int flagCalls = 0;
    bool say(const std::string&, TargetChannel c, ChannelMessageType t,
             unsigned int targetId, unsigned int f) override
    { channel = c; type = t; target = targetId; flags = f; return false; }
    void setCurrentFlags(int f) override { ++flagCalls; Game::setCurrentFlags(f); }
};
struct Connection : ClientConnection
{
    int ghosts, reads = 0;
    explicit Connection(int n) : ClientConnection(static_cast<ConnectionDemoMode>(0)), ghosts(n) {}
    int debugGetNumActiveGhosts() override { ++reads; return ghosts; }
};
struct Net : io::NetServer
{
    int statusId = -1, ipId = -1;
    bool result = true;
    bool getConnectionStatus(int id, io::ConnectionStatus& out) override
    { statusId = id; std::memset(&out, 0x12, sizeof(out)); return result; }
    bool getConnectionIp(int id, std::string& out) override
    { ipId = id; if (result) out = "127.0.0.1:16567"; return result; }
    std::string getServerAddress() const override { return "127.0.0.1"; }
};
struct Socket : io::UDPSocket
{
    std::vector<int> calls;
    int count = 0;
    std::string bytes;
    void setPeerAddress(const Address& peer) override
    { calls.push_back(1); io::UDPSocket::setPeerAddress(peer); }
    int send(const unsigned char* data, int n) override
    { calls.push_back(2); count = n; if (n > 0) bytes.assign(reinterpret_cast<const char*>(data), n); return n; }
};
}

int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(reportFault);
    WSADATA winsock = {};
    check(WSAStartup(MAKEWORD(2,2), &winsock) == 0, "Winsock setup for address conversion only");
    g_system = new System();
    settings = new SettingsRepostitory();
    gameEventRegistry = new GameEventRegistry();
    Events* events = new Events(); eventManager = events;
    Players* players = new Players(); world::playerManager = players;
    Server* server = new Server();
    IGameServer* api = server;
    io::ConnectionStatus status, before;
    std::memset(&status, 0xa5, sizeof(status)); before = status;
    check(!api->getConnectionStatus(9, status) && std::memcmp(&status, &before, sizeof(status)) == 0,
          "missing listener returns false and preserves status output");
    check(api->getName().empty() && api->getServerAddress().empty(), "missing listener constructs empty strings");
    check(!api->getIsInternetServer(), "internet query reads constructor seed");
    server->m_isInternetServer = true;
    check(api->getIsInternetServer(), "internet query reads stored setting without a listener");
    api->sendUDPPacket(0, 0, 0, -7);
    check(!api->isClientReady(42) && api->getTotalActiveGhosts() == 0, "empty connection queries");
    std::string address("unchanged");
    check(!api->getPlayerConnectionIpAndPort(0, address) && address == "unchanged", "null player preserves address");
    check(!server->getConnectionIpAndPort(42, address), "missing player connection needs no listener");

    Net* net = new Net(); server->m_netServer = net;
    net->m_name = "fixture server";
    check(api->getName() == "fixture server" && api->getServerAddress() == "127.0.0.1", "listener getters return values");
    std::string copy = api->getName(); copy[0] = 'X';
    check(api->getName() == "fixture server", "returned name has independent string ownership");
    check(api->getConnectionStatus(301, status) && net->statusId == 301, "status forwards full signed id");
    net->result = false;
    check(!api->getConnectionStatus(-2, status) && net->statusId == -2, "status propagates listener false");
    net->result = true;
    Connection* first = new Connection(7);
    Connection* second = new Connection(11);
    server->m_connections[42] = first; server->m_connections[129] = second; server->m_connections[13] = 0;
    first->m_connectionId = 203;
    for (int state : {-1,0,3,4,5,2147483647})
    {
        first->m_connectionState = state;
        check(api->isClientReady(42) == (state > 3), "readiness preserves signed state threshold");
    }
    check(api->isClientReady(298), "readiness preserves existing byte-key alias");
    check(api->getTotalActiveGhosts() == 18 && first->reads == 1 && second->reads == 1,
          "ghost sum visits unready and recording-id entries, skips null");
    check(server->getConnectionIpAndPort(42, address) && net->ipId == 203 && address == "127.0.0.1:16567",
          "address helper resolves separate network id and keeps port");
    net->result = false; address = "kept";
    check(!server->getConnectionIpAndPort(42, address) && address == "kept", "address helper propagates false");
    net->result = true;
    Player* player = new Player(-1, 0); // retail sentinel avoids unrelated free-camera allocation
    player->m_playerId = 42; player->m_playerName = "Original"; players->player = player;
    check(api->getPlayerConnectionIpAndPort(player, address) && net->ipId == 203, "player wrapper uses player id");
    Socket* socket = new Socket(); net->m_listenSocket2 = socket;
    char payload[] = {'a',0,'b'};
    api->sendUDPPacket("127.0.0.1", 65535, payload, 3);
    const sockaddr_in* peer = reinterpret_cast<const sockaddr_in*>(socket->getPeerAddress().sockaddr);
    check(socket->calls == std::vector<int>({1,2}) && peer->sin_port == htons(65535) &&
          peer->sin_addr.s_addr == inet_addr("127.0.0.1"), "raw send sets peer before socket send");
    check(socket->bytes == std::string(payload,3), "raw send preserves binary payload");
    api->sendUDPPacket("127.0.0.1", 1, 0, -7);
    check(socket->count == -7, "raw send preserves retail absence of size guard");

    const std::string binaryText("hello\0world", 11);
    players->localReads = 0;
    check(api->say(binaryText, static_cast<TargetChannel>(14), static_cast<ChannelMessageType>(2),
                   0x12345678, 0x87654321), "chat reports success");
    StringReceivedEvent* ev = events->posted.back();
    check(events->contract && players->localReads == 1 && ev->m_senderId == -1,
          "server chat posts network event 11 with server sender");
    check(ev->m_targetId == 0x12345678 && static_cast<unsigned int>(ev->m_flags) == 0x87654321 &&
          ev->m_channel == 14 && ev->m_type == 2, "chat preserves full-width target and flags");
    check(ev->m_messageLength == 11 && std::memcmp(ev->m_message, binaryText.data(), 11) == 0 &&
          ev->m_message[11] == 0 && ev->m_message != reinterpret_cast<const unsigned char*>(binaryText.data()),
          "chat copies embedded NULs and adds final terminator");
    players->local = player; players->localReads = 0;
    check(api->sayAll(""), "all-chat accepts empty text");
    ev = events->posted.back();
    check(players->localReads == 2 && ev->m_senderId == 42 && ev->m_messageLength == 0 && ev->m_message[0] == 0,
          "local chat queries player twice and handles empty buffer");
    WrapperObserver* wrappers = new WrapperObserver();
    wrappers->m_currentChannel = static_cast<TargetChannel>(7);
    wrappers->m_currentType = static_cast<ChannelMessageType>(2); wrappers->m_currentFlags = 513;
    check(!static_cast<IGame*>(wrappers)->say("x") && wrappers->channel == 7 && wrappers->type == 2 &&
          wrappers->target == 0 && wrappers->flags == 513, "plain wrapper preserves virtual false and current settings");
    check(!wrappers->sayAll("x") && wrappers->channel == 12 && wrappers->type == 1 && wrappers->flags == 513,
          "all wrapper uses server channel and current flags");
    check(!wrappers->sayTeam(258,"x") && wrappers->flagCalls == 1 && wrappers->m_currentFlags == 258 &&
          wrappers->flags == 258 && wrappers->channel == 13 && wrappers->type == 1,
          "team wrapper invokes flag setter and leaves full team value stored");

    check(!api->changePlayerName(99,"new") && server->broadcasts.empty(), "missing player rename returns false");
    check(!api->changePlayerName(42,"Original") && players->attempts.empty(), "identical rename returns false before setName");
    const std::string longName(35,'L'), base(23,'L');
    check(api->changePlayerName(42,longName) && player->getName() == base, "rename truncates initial attempt to 23 bytes");
    ChangePlayerNameEvent* changed = static_cast<ChangePlayerNameEvent*>(server->broadcasts.back());
    check(changed->m_playerId == 42 && std::string(changed->m_name) == base, "rename broadcasts resulting name and player id");
    player->m_playerName = base; players->attempts.clear(); players->taken.clear();
    check(api->changePlayerName(42,longName) && player->getName() == base + "_0" &&
          players->attempts == std::vector<std::string>({base,base+"_0"}),
          "untruncated comparison and unchanged truncated name trigger suffix zero");
    player->m_playerName = "Original"; players->attempts.clear();
    players->taken = {"Taken","Taken_0"};
    check(api->changePlayerName(42,"Taken") && player->getName() == "Taken_1" &&
          players->attempts == std::vector<std::string>({"Taken","Taken_0","Taken_1"}),
          "rejected names retry sequential suffixes through real Player setName");
    check(server->broadcasts.size() == 3, "one event per successful rename only");
    for (StringReceivedEvent* posted : events->posted) delete posted;
    for (GameEvent* posted : server->broadcasts) delete posted;
    WSACleanup();
    std::printf("GameServer query/chat runtime: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
