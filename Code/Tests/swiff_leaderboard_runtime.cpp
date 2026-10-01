// [TEMP] [CHOSEN, not recovered]
// Tests SwiffHost leaderboard/awards caching through real HTTP and persistence code.
// Starts the local stats fixture on 127.0.0.1:18124 and stops it before returning.
// Injected cache objects avoid unrelated singleton/script paths; other force-linked dependencies stay uncalled.
#include "../BF2/Core/Ids.h"
#include "../BF2/Menu/SwiffHost.h"
#include "../BF2/Game/Persistence/LeaderBoard.h"
#include "../BF2/Game/Persistence/PlayerAwards.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace dice { namespace hfe {
struct SwiffHostFixture
{
    static void inject(SwiffHost& host, IBase* board, IBase* awards)
    {
        host.m_leaderBoard = board;
        host.m_playerAwards = awards;
    }
    static const SwiffLeaderboardCache& global(const SwiffHost& host) { return host.m_leaderboard; }
    static size_t players(const SwiffHost& host) { return host.m_leaderboardByPlayer.size(); }
    static size_t awardLists(const SwiffHost& host) { return host.m_awardCache.size(); }
};
} }

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* what)
{
    ++checks;
    if (!ok)
    {
        ++failures;
        std::printf("FAIL: %s\n", what);
    }
}

// The stats fixture, as a child process on its own port (never 16567, agents.md).
PROCESS_INFORMATION g_server = {};
const int kPort = 18124;
bool startServer(const char* fixtureDir)
{
    char command[1024];
    std::snprintf(command, sizeof command,
                  "python -u \"%s\\bf2_stats_fixture.py\" --port %d --lb-size 57 --lb-position 42 --lb-pid 1",
                  fixtureDir, kPort);
    STARTUPINFOA si = {};
    si.cb = sizeof si;
    if (!CreateProcessA(nullptr, command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &g_server))
        return false;
    // Wait for the listener (up to ten seconds).
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in a = {};
        a.sin_family = AF_INET;
        a.sin_port = htons(static_cast<u_short>(kPort));
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        const bool up = connect(s, reinterpret_cast<sockaddr*>(&a), sizeof a) == 0;
        closesocket(s);
        if (up)
            return true;
        Sleep(100);
    }
    return false;
}
void stopServer()
{
    if (g_server.hProcess)
    {
        TerminateProcess(g_server.hProcess, 0);
        WaitForSingleObject(g_server.hProcess, 5000);
        CloseHandle(g_server.hProcess);
        CloseHandle(g_server.hThread);
        g_server = PROCESS_INFORMATION();
    }
}
} // namespace

int main(int argc, char** argv)
{
    using namespace dice::hfe;
    if (argc != 2)
    {
        std::printf("usage: swiff_leaderboard_runtime <stats fixture directory>\n");
        return 2;
    }
    if (!startServer(argv[1]))
    {
        std::printf("FAIL: the stats fixture did not come up on 127.0.0.1:%d\n", kPort);
        stopServer();
        return 1;
    }
    char url[64];
    std::snprintf(url, sizeof url, "BF2_STATS_URL=http://127.0.0.1:%d/ASP/", kPort);
    _putenv(url);

    LeaderBoard* board = new LeaderBoard();
    PlayerAwards* awards = new PlayerAwards();
    SwiffHost* host = static_cast<SwiffHost*>(SwiffHost::create(IID_ISwiffHost, nullptr));
    check(host != nullptr, "the SwiffHost factory constructs the host");
    if (!host)
    {
        stopServer();
        return 1;
    }
    SwiffHostFixture::inject(*host, static_cast<IBase*>(static_cast<ILeaderBoard*>(board)),
                             static_cast<IBase*>(static_cast<IPlayerAwards*>(awards)));
    const SwiffLeaderboardCache& global = SwiffHostFixture::global(*host);

    // 1. A global kit board (request 7 = kit 2, group 7) from position 41: rows 41..57 of 57.
    check(host->fetchLeaderboard(0, 7, 41), "fetchLeaderboard(0, 7, 41) answers true");
    check(global.kits[2].numPlayers == 57, "the board's size is getNumPlayers (the answer's \"size\")");
    check(global.kits[2].updated.size() == 16 && global.kits[2].updated[4] == '-' && global.kits[2].updated[13] == ':',
          "the board's date is 0x487DC0's \"%d-%s-%.2d %.2d:%.2d\"");
    check(global.kits[2].rows.size() == 17 && global.position == 41, "seventeen rows from 41, and the position is stored");
    if (!global.kits[2].rows.empty())
    {
        const SwiffLeaderboardRow& r = global.kits[2].rows[0];
        check(r.n == 41 && r.pid == 40000041 && r.nick == "Player041", "row 0 is n 41, its pid and nick");
        check(r.values[0] == 59123 && r.values[1] == 59123 && r.values[2] == 309406,
              "a kit row reads killswith, deathsby and timeplayed");
        check(r.playerRank == 19 && r.countryCode == "GB", "and playerrank / countrycode");
    }

    // 2. getLeaderboard at the same position does not refetch; another position does.
    SwiffLeaderboardCache copy;
    check(host->getLeaderboard(&copy, 7, 41) && copy.kits[2].rows.size() == 17 && copy.position == 41,
          "getLeaderboard(7, 41) copies the cached board out");
    check(host->getLeaderboard(&copy, 7, 21) && copy.kits[2].rows.size() == 20 && copy.kits[2].rows[0].n == 21 && copy.position == 21,
          "another position refetches: twenty rows from 21");

    // 3. The rising star (request 1): the weekly score is kept as "%.2f" plus "%".
    check(host->fetchLeaderboard(0, 1, 1) && global.risingStar.rows.size() == 20, "the rising-star board: twenty rows");
    if (!global.risingStar.rows.empty())
        check(global.risingStar.rows[0].weeklyScore == "99.13%" && global.risingStar.rows[0].date.size() == 10,
              "weeklyscore 99.1300 is kept as \"99.13%\", the date as its wire text");

    // 4. A player board: an uncached player is refused by the fetcher, created by the reader.
    check(!host->fetchLeaderboard(77, 21, 0), "fetchLeaderboard refuses a player it has no cache for");
    SwiffLeaderboardCache mine;
    check(host->getLeaderboardPlayer(77, &mine, 21), "getLeaderboardPlayer(77, 21) answers true");
    check(SwiffHostFixture::players(*host) == 1, "the player's cache now exists");
    check(mine.weapons[2].rows.size() == 1 && mine.weapons[2].rows[0].pid == 77 && mine.weapons[2].rows[0].n == 42,
          "weapon 2 (request 21, group 14) holds the player's own row at 42");
    if (!mine.weapons[2].rows.empty())
    {
        const SwiffLeaderboardWeaponRow& w = mine.weapons[2].rows[0];
        check(w.values[0] == 58126 && w.values[1] == 0 && w.values[2] == 308172 && w.values[3] == 18,
              "a weapon row: killswith, \"detahsby\" [sic] reads 0 for the served deathsby, timeused, accuracy");
    }
    check(host->getLeaderboardPlayerPosition(77, 21) == 42, "getLeaderboardPlayerPosition answers row 0's n");
    check(host->getLeaderboardPlayerPosition(77, 5) == 0 && host->getLeaderboardPlayerPosition(78, 21) == 0 &&
          host->getLeaderboardPlayerPosition(0, 21) == 0,
          "an empty board, an uncached player and player 0 answer 0");

    // 5. goLeaderboardPlayer: weapon category 3, board 2 = request 21; the page starts nine above.
    host->setLeaderboardCategory(3);
    host->setLeaderboardId(2);
    host->goLeaderboardPlayer(77);
    check(host->getLeaderboardPosition() == 33, "goLeaderboardPlayer sets the position to 42 - 9");
    check(global.position == 33 && global.weapons[2].rows.size() == 20 && global.weapons[2].rows[0].n == 33,
          "and fetched the global weapon board from 33");
    host->setLeaderboardPosition(-4);
    check(host->getLeaderboardPosition() == 1, "setLeaderboardPosition clamps to 1");

    // 6. An out-of-range request fetches nothing but still stores the position.
    check(host->fetchLeaderboard(0, 31, 9) && global.position == 9, "request 31: true, position stored, no fetch");

    // 7. The awards cache: one HTTP fetch per profile, then the cached list.
    IPlayerAwards::AwardList list;
    check(host->getCachedAwards(77, &list), "getCachedAwards(77) answers true");
    check(list.profileId == 77 && list.awards.size() == 9, "nine awards for profile 77");
    if (list.awards.size() == 9)
    {
        check(list.awards[0].id == 2021403 && list.awards[0].type == 0, "a medal (2000001..3000000) is type 0");
        check(list.awards[3].id == 1031406 && list.awards[3].level == 3 && list.awards[3].type == 1, "a badge keeps its level, type 1");
        check(list.awards[6].id == 3191305 && list.awards[6].type == 2, "a ribbon (above 3000000) is type 2");
        check(list.awards[0].when > 1700000000 && list.awards[0].first > 1700000000, "when / first are the served times");
    }
    check(list.timestamp > 1700000000, "the list is stamped when its answer arrives");
    IPlayerAwards::AwardList again;
    check(host->getCachedAwards(77, &again) && again.awards.size() == 9 && SwiffHostFixture::awardLists(*host) == 1,
          "a second ask is served from the one cached list");

    // 8. resetStatsCache: the rows, the player map and the awards go; the position stays.
    host->resetStatsCache();
    check(global.kits[2].rows.empty() && global.weapons[2].rows.empty() && global.risingStar.rows.empty(),
          "resetStatsCache empties every global board's rows");
    check(global.kits[2].numPlayers == 57 && global.position == 9, "but keeps their counts and the position");
    check(SwiffHostFixture::players(*host) == 0 && SwiffHostFixture::awardLists(*host) == 0,
          "and drops the player caches and the awards cache");

    host->release();
    stopServer();
    std::printf("swiff_leaderboard_runtime: %d/%d checks passed; %d failures\n", checks - failures, checks, failures);
    return failures ? 1 : 0;
}
