// [TEMP] [CHOSEN, not recovered] component fixture. No socket, HTTP, map or client acceptance.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <array>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#define private public
#define protected public
#include "../BF2/Game/GameServer/GameServer.h"
#include "../BF2/Game/GameLogic/Player.h"
#include "../BF2/Game/Common/ClientConnection.h"
#include "../BF2/Physics/CollisionManager.h"
#include "../BF2/Physics/CollisionMeshTemplate.h"
#undef protected
#undef private
#include "../BF2/Game/Common/GameEvent.h"
#include "../BF2/Game/GameServer/ServerInfo.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/Core/System.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/IO/Console/Console.h"
#include "../BF2/IO/ChecksumContext.h"

using namespace dice::hfe;
namespace {
int checks = 0, failures = 0;
std::vector<int> calls;
void check(bool ok, const char* what)
{
    ++checks; if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what); std::fflush(stdout);
}
struct Events : EventManager
{
    bool registerEventHandler(EventCategory, IEventListener*, int) override { return true; }
    bool registerEvent(EventCategory, unsigned int, const std::string&) override { return true; }
};
struct PlayerProbe : dice::hfe::Player
{
    PlayerProbe() : dice::hfe::Player(-1, 0) { m_playerName = "Fixture Player"; }
    const std::string& getName() const override { calls.push_back(3); return dice::hfe::Player::getName(); }
};
struct Players : world::PlayerManager
{
    world::IPlayer* result = 0;
    int lastId = 0;
    world::IPlayer* getPlayerFromId(int id) override { lastId = id; calls.push_back(2); return result; }
};
struct Connection : ClientConnection
{
    int writes = 0;
    Connection() : ClientConnection(static_cast<ConnectionDemoMode>(0)) { m_connectionState = 2; }
    void setContentValid(bool value) override
    { ++writes; calls.push_back(5); ClientConnection::setContentValid(value); }
};
struct Server : GameServer
{
    mutable int reads = 0;
    bool scripted = false;
    IClientConnection* first = 0;
    IClientConnection* second = 0;
    IClientConnection* getConnection(int id) const override
    {
        calls.push_back(1); ++reads;
        if (scripted) return reads == 1 ? first : second;
        return GameServer::getConnection(id);
    }
};
struct Console : io::OldConsole
{
    Server* server = 0;
    std::vector<std::string> messages;
    bool pendingAtOutput = false;
    void output(const std::string& text) override
    { calls.push_back(4); pendingAtOutput = !server->m_disconnectTypes.empty(); messages.push_back(text); }
};
struct Part : world::CollisionMeshTemplate
{
    world::CollisionManager* manager;
    int id;
    Part* append = 0;
    Part(world::CollisionManager* owner, int number) : manager(owner), id(number) {}
    ~Part() override
    {
        calls.push_back(10 + id);
        auto found = manager->m_collisionMeshTemplates.find("mesh");
        check(found != manager->m_collisionMeshTemplates.end(), "mesh entry still exists during each virtual deletion");
        if (append) found->second.push_back(append);
    }
};
}

int main()
{
    static_assert(sizeof(void*) == 4, "x86 fixture only");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    g_system = new System; settings = new SettingsRepostitory;
    eventManager = new Events;
    auto* players = new Players; world::playerManager = players;
    gameEventRegistry = new GameEventRegistry;
    auto* server = new Server;
    auto* console = new Console; console->server = server; io::mainConsole = console;
    auto* player = new PlayerProbe;
    auto* connection = new Connection;
    server->m_connections[42] = connection;
    IGameServer* api = server;
    const std::array<unsigned char,16> misc = {{0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10}};
    std::array<unsigned char,16> zero = {}, full; full.fill(0xff);
    check(io::hash_to_string(misc.data()) == "0123456789abcdeffedcba9876543210", "hash formatter preserves byte/nibble order in lowercase");
    check(io::hash_to_string(zero.data()) == std::string(32,'0') &&
          io::hash_to_string(full.data()) == std::string(32,'f'), "hash formatter always returns 32 digits");
    const int ordinal = -123;
    auto reset = [&]() {
        calls.clear(); console->messages.clear(); console->pendingAtOutput = false;
        players->result = 0; server->reads = 0; server->scripted = false;
        server->m_disconnectTypes.clear();
        serverInfo->m_block.useContentChecking = true;
        server->m_useContentChecking = false; // distinct flag: callback reads ServerInfo.
        server->m_levelFingerprint = false;
        server->m_mapInfo.setChallengeOrdinal(ordinal);
        server->m_miscHashMd5 = io::hash_to_string(misc.data());
        server->m_stdArchiveMd5.clear(); server->m_stdArchiveModMd5.clear();
        server->m_bstArchiveMd5.clear(); server->m_bstArchiveModMd5.clear();
        server->m_levelFingerprintStrings.clear();
        connection->m_connectionState = 2;
        connection->ClientConnection::setContentValid(false); connection->writes = 0;
    };
    auto send = [&](int id = 42) { api->onContentCheckEvent(id, misc.data(), zero.data(), full.data()); };
    auto allArchives = [&](const std::string& value) {
        server->m_stdArchiveMd5[ordinal] = value; server->m_stdArchiveModMd5[ordinal] = value;
        server->m_bstArchiveMd5[ordinal] = value; server->m_bstArchiveModMd5[ordinal] = value;
    };
    for (int state : {-2147483647-1, -1, 0, 1})
    {
        reset(); connection->m_connectionState = state;
        ServerInfo* saved = serverInfo; serverInfo = 0;
        api->onContentCheckEvent(42,0,0,0); serverInfo = saved;
        check(calls == std::vector<int>({1}), "signed pre-join state returns before ServerInfo or hash reads");
    }
    reset(); serverInfo->m_block.useContentChecking = false;
    api->onContentCheckEvent(42,0,0,0);
    check(calls == std::vector<int>({1}), "disabled ServerInfo content checking skips payload reads");
    reset(); send();
    check(calls == std::vector<int>({1,2,4}) && connection->writes == 0,
          "no archive entry rejects even a matching miscellaneous hash");
    check(console->messages == std::vector<std::string>({"dep: kicking client 42 for using modified data"}) &&
          !console->pendingAtOutput && server->m_disconnectTypes.at(42).reason == 27,
          "failure outputs exact message before queueing disconnect reason 27");
    reset(); players->result = player; send(-2147483647-1);
    check(console->messages == std::vector<std::string>({"dep: kicking client -2147483648 (Fixture Player) for using modified data"}) &&
          calls == std::vector<int>({1,2,3,4}) && players->lastId == (-2147483647-1),
          "missing connection still processes failure with full signed id and optional player name");
    check(server->m_disconnectTypes.at(-2147483647-1).connectionId == (-2147483647-1) &&
          server->m_disconnectTypes.at(-2147483647-1).text.empty(), "disconnect entry preserves id and empty wire text");
    reset(); server->m_stdArchiveMd5[ordinal+1] = "irrelevant"; send();
    check(connection->writes == 0 && server->m_stdArchiveMd5.size() == 1, "archive lookup uses exact signed challenge without insertion");
    std::map<int,std::string>* tables[] = {&server->m_stdArchiveMd5, &server->m_stdArchiveModMd5,
                                         &server->m_bstArchiveMd5, &server->m_bstArchiveModMd5};
    for (auto* table : tables)
    {
        reset(); (*table)[ordinal] = "deliberately mismatched archive"; send();
        check(connection->getContentValid() && calls == std::vector<int>({1,2,1,5}) &&
              console->messages.empty(), "any archive table enables no-level-fingerprint acceptance without archive comparison");
    }
    reset(); allArchives("irrelevant"); server->m_miscHashMd5[10] = 'A'; send();
    check(connection->writes == 0 && !server->m_disconnectTypes.empty(), "miscellaneous hash comparison is case sensitive");
    reset(); allArchives("irrelevant"); server->m_levelFingerprint = true; send();
    check(connection->writes == 0, "loaded level fingerprint requires matching ordinal entry");
    for (auto* table : tables)
    {
        reset(); allArchives("different"); server->m_levelFingerprint = true;
        server->m_levelFingerprintStrings[ordinal] = io::hash_to_string(full.data());
        (*table)[ordinal] = io::hash_to_string(zero.data()); send();
        check(connection->getContentValid() && connection->writes == 1, "each of the four archive alternatives accepts with a matching level");
    }
    reset(); server->m_levelFingerprint = true;
    server->m_stdArchiveMd5[ordinal] = io::hash_to_string(zero.data());
    server->m_levelFingerprintStrings[ordinal] = io::hash_to_string(full.data()); send();
    check(connection->getContentValid(), "first matching archive short circuits absent later iterators");
    reset(); allArchives("different"); server->m_levelFingerprint = true;
    server->m_levelFingerprintStrings[ordinal] = io::hash_to_string(full.data()); send();
    check(!connection->getContentValid(), "four archive mismatches reject despite matching level");
    reset(); allArchives(io::hash_to_string(zero.data())); server->m_levelFingerprint = true;
    server->m_levelFingerprintStrings[ordinal] = io::hash_to_string(zero.data());
    connection->ClientConnection::setContentValid(true); send();
    check(connection->getContentValid() && connection->writes == 0 && !server->m_disconnectTypes.empty(),
          "later failed check queues disconnect without clearing previous content-valid flag");
    reset(); allArchives("different");
    server->m_disconnectTypes[42] = DisconnectType(777,99,"old");
    server->m_miscHashMd5.clear(); send();
    check(server->m_disconnectTypes.at(42).reason == 27 && server->m_disconnectTypes.at(42).text.empty(),
          "failure replaces an existing pending disconnect");
    reset(); allArchives("different");
    server->m_disconnectTypes[42] = DisconnectType(42,99,"old"); send();
    check(connection->getContentValid() && server->m_disconnectTypes.at(42).reason == 99,
          "success does not remove an existing pending disconnect");
    reset(); allArchives("different"); send(298);
    check(connection->getContentValid() && players->lastId == 298,
          "connection lookup retains byte-key alias while player lookup retains full id");
    reset(); allArchives("different"); server->scripted = true;
    server->first = 0; server->second = 0; send();
    check(calls == std::vector<int>({1,2,1}) && server->m_disconnectTypes.empty(),
          "valid missing connection performs second lookup without fabricating a connection");
    auto* replacement = new Connection;
    reset(); allArchives("different"); server->scripted = true;
    server->first = connection; server->second = replacement; send();
    check(!connection->getContentValid() && replacement->getContentValid(),
          "success marks result of fresh virtual lookup instead of cached initial connection");

    auto* meshes = new world::CollisionManager;
    world::ICollisionManager* meshApi = meshes;
    meshApi->deleteCollisionMesh("missing");
    check(meshes->m_collisionMeshTemplates.empty(), "missing mesh deletion is inert");
    meshes->m_collisionMeshTemplates["empty"] = {};
    meshApi->deleteCollisionMesh("EMPTY");
    check(meshes->m_collisionMeshTemplates.empty(), "case-insensitive empty mesh entry is erased");
    auto* first = new Part(meshes,1); auto* last = new Part(meshes,2);
    meshes->m_collisionMeshTemplates["mesh"] = {first,0,last};
    meshes->m_collisionMeshTemplates["other"] = {};
    calls.clear(); meshApi->deleteCollisionMesh("MeSh");
    check(calls == std::vector<int>({11,12}) && meshes->m_collisionMeshTemplates.size() == 1 &&
          meshes->m_collisionMeshTemplates.count("other"), "mesh deletion skips nulls, preserves index order and erases only the named entry");
    first = new Part(meshes,3); last = new Part(meshes,4); auto* appended = new Part(meshes,5);
    first->append = appended; meshes->m_collisionMeshTemplates["mesh"] = {first,last};
    calls.clear(); meshApi->deleteCollisionMesh("mesh");
    check(calls == std::vector<int>({13,14}), "mesh deletion snapshots count but rereads storage after destructor appends");
    meshes->m_collisionMeshTemplates["mesh"] = {}; delete appended;
    meshes->m_collisionMeshTemplates.clear();
    std::printf("%d checks, %d failures\n", checks, failures);
    // Loaded LOD payload teardown, server/manager destructors and production shutdown are untested.
    return failures ? 1 : 0;
}
