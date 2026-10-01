// Test scaffolding. Validate the actual child listener, not only the requested port.
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include "../../BF2/Game/GameServer/GameServer.h"
#include "../../BF2/Core/SettingsRepostitory.h"
using namespace dice::hfe;
bool tier2Listener()
{
    const int port = std::atoi(std::getenv("BF2_PORT"));
    auto server = static_cast<GameServer*>(game);
    bool found = false;
    ULONG bytes = 0;
    GetExtendedUdpTable(0, &bytes, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    std::vector<unsigned char> data(bytes);
    if (GetExtendedUdpTable(data.data(), &bytes, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) != NO_ERROR)
        return false;
    auto table = reinterpret_cast<MIB_UDPTABLE_OWNER_PID*>(data.data());
    for (DWORD i = 0; i < table->dwNumEntries; ++i)
    {
        const auto& row = table->table[i];
        if (row.dwOwningPid != GetCurrentProcessId()) continue;
        const int bound = ntohs(static_cast<unsigned short>(row.dwLocalPort));
        std::printf("[tier2-listener] pid=%lu address=%08lx port=%d\n",
                    row.dwOwningPid, ntohl(row.dwLocalAddr), bound);
        if ((ntohl(row.dwLocalAddr) >> 24) != 127 || bound == 16567) return false;
        if (bound == port && ntohl(row.dwLocalAddr) == INADDR_LOOPBACK) found = true;
    }
    if (!server) return false;
    const char* const expectedLevel = std::getenv("BF2_TIER2_LEVEL");
    const auto map = server->getMapInfo();
    std::printf("[tier2-map] name=%s mode=%s layer=%d\n", map.getMapName().c_str(),
                map.getGameMode().c_str(), map.getMaxNumberOfPlayers());
    const bool pass = port != 16567 && server->m_port == port && found
        && expectedLevel && map.getMapName() == expectedLevel
        && map.getGameMode() == "gpm_cq" && map.getMaxNumberOfPlayers() == 64
        && !settings->boolGet("UseAI", false)
        && settings->stringGet("AIDLL", std::string()).empty();
    std::printf("[tier2-listener] PASS=%d requested=%d server=%d\n", pass, port, server ? server->m_port : -1);
    std::fflush(stdout);
    return pass;
}
