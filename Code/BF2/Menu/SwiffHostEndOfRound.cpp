// Builds the end-of-round data shown by the menu plugin.
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include "SwiffHost.h"
#include "../Core/EventManager.h"                           // eventManager (ds:0x9FF964), postEvent +0x10
#include "../Core/StringUtil.h"                             // compareIgnoreCase (0x431950), formatString (0x4310D0)
#include "../Game/IGameLogic.h"                             // gameLogic (ds:0x99EF80): getTeamName +0x48, getTicketHistory +0x278
#include "../Game/Objects/IPlayer.h"
#include "../Game/Persistence/SimpleTokenizer.h"
#include "../Game/GameLogic/ScoreSystem/ScoreManager.h"     // IScoreManager +0x10 / +0x18 / +0x24, TeamScore
#include "../Game/GameLogic/SquadManager/ISquadManager.h"   // getSquadInfo +0x28, getSquadLeader +0x3C
#include "../Game/GameLogic/SquadManager/ISquadInfo.h"      // getSquadName +0x00
#include "../Scene/Object/PlayerManager.h"                  // world::playerManager (ds:0xA08F60)
#include "../Scene/Object/PlayerScoreData.h"
#include <cstdlib>                                          // atoi (MSVCR71 import 0x87F5A0)
#include <list>
#include <map>
#include <string>

namespace dice { namespace hfe {

namespace world { extern IBase* scoreManager; }             // ds:0x9C5DA0 - Core/Globals.cpp
extern IBase* squadManager;                                 // ds:0x99EF84 - Core/Globals.cpp

namespace {

// Returns the named player's rank, or zero for an empty or unknown name or an unset rank.
// Requires PlayerManager. [INFERRED] Helper name.
int rankOfPlayerNamed(const std::string& name)
{
    if (name == "")                                                            // operator==(string, "")
        return 0;
    world::IPlayer* const player = world::playerManager->getPlayerFromName(name);   // +0x1C
    if (!player)
        return 0;
    const int rank = player->getScoreData()->m_rank;                           // PE +0x170, then +0x20
    return rank != -1 ? rank : 0;
}

} // namespace

// Parses backslash-delimited key/value data and queues the round screen after delay seconds.
// Keys are case-insensitive; unknown keys still consume a value. Requires initialized score, game and player services.
// Only kit and vehicle ids 0 through 6 are accepted. Returns true after publishing the record.
bool SwiffHost::setEndOfRoundInfo(const std::string& data, float delay)
{
    SwiffEndOfRoundInfo info;                                                  // 0x497980 on [ebp-0xF50]
    // Requires ScoreManager; winner and victory type use only the low byte of each result.
    info.winner = static_cast<unsigned char>(
        static_cast<world::IScoreManager*>(world::scoreManager)->getWinner());          // +0x18
    info.victoryType = static_cast<unsigned char>(
        static_cast<world::IScoreManager*>(world::scoreManager)->getVictoryType());     // +0x24
    SimpleTokenizer tokens;
    tokens.init(data, std::string("\\"));                                      // 0x59E120, temporary destroyed at once
    std::string key;                                                           // [ebp-0x3C]
    std::string value;                                                         // [ebp-0x1C]
    // Nine players by seven types each, although only players 0..2 are read back (the zeroing is
    // `rep stosd` of 0x3F dwords, vehicles first).
    int vehicleTimes[9][7];                                                    // [ebp-0x1148]
    int kitTimes[9][7];                                                        // [ebp-0x104C]
    for (int p = 0; p < 9; ++p)
        for (int t = 0; t < 7; ++t)
            vehicleTimes[p][t] = 0;
    for (int p = 0; p < 9; ++p)
        for (int t = 0; t < 7; ++t)
            kitTimes[p][t] = 0;

    while (tokens.next(key))
    {
        bool found = false;                                                    // [ebp+0xB], reset on every key
        // the three top players: np<i> tsp<i> ssp<i> csp<i> bfp<i> bhp<i> (tsp first, as shipped)
        if      (compareIgnoreCase(key, std::string("tsp0")) == 0) { tokens.next(value); info.topPlayers[0].teamScore = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("tsp1")) == 0) { tokens.next(value); info.topPlayers[1].teamScore = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("tsp2")) == 0) { tokens.next(value); info.topPlayers[2].teamScore = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("np0")) == 0)
        {
            tokens.next(value);
            info.topPlayers[0].name = value;
            info.topPlayers[0].rank = rankOfPlayerNamed(value);                       // inlined at 0x499717
            info.topPlayers[0].isLocalPlayer = isLocalPlayerName(info.topPlayers[0].name);   // own +0x2E8
        }
        else if (compareIgnoreCase(key, std::string("np1")) == 0)
        {
            tokens.next(value);
            info.topPlayers[1].name = value;
            info.topPlayers[1].rank = rankOfPlayerNamed(value);                       // inlined at 0x4997D6
            info.topPlayers[1].isLocalPlayer = isLocalPlayerName(info.topPlayers[1].name);
        }
        else if (compareIgnoreCase(key, std::string("np2")) == 0)
        {
            tokens.next(value);
            info.topPlayers[2].name = value;
            info.topPlayers[2].rank = rankOfPlayerNamed(value);                       // inlined at 0x499895
            info.topPlayers[2].isLocalPlayer = isLocalPlayerName(info.topPlayers[2].name);
        }
        else if (compareIgnoreCase(key, std::string("ssp0")) == 0) { tokens.next(value); info.topPlayers[0].skillScore = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("ssp1")) == 0) { tokens.next(value); info.topPlayers[1].skillScore = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("ssp2")) == 0) { tokens.next(value); info.topPlayers[2].skillScore = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("csp0")) == 0) { tokens.next(value); info.topPlayers[0].cmdScore = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("csp1")) == 0) { tokens.next(value); info.topPlayers[1].cmdScore = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("csp2")) == 0) { tokens.next(value); info.topPlayers[2].cmdScore = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("bfp0")) == 0) { tokens.next(value); info.topPlayers[0].bulletsFired = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("bfp1")) == 0) { tokens.next(value); info.topPlayers[1].bulletsFired = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("bfp2")) == 0) { tokens.next(value); info.topPlayers[2].bulletsFired = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("bhp0")) == 0) { tokens.next(value); info.topPlayers[0].bulletsHit = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("bhp1")) == 0) { tokens.next(value); info.topPlayers[1].bulletsHit = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("bhp2")) == 0) { tokens.next(value); info.topPlayers[2].bulletsHit = atoi(value.c_str()); }
        // the eleven category awards, endofround.py's findTop pairs, in its order
        else if (compareIgnoreCase(key, std::string("ts")) == 0)   { tokens.next(value); info.awards[0].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("tsn")) == 0)  { tokens.next(value); info.awards[0].name = value; info.awards[0].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("ss")) == 0)   { tokens.next(value); info.awards[1].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("ssn")) == 0)  { tokens.next(value); info.awards[1].name = value; info.awards[1].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("cpc")) == 0)  { tokens.next(value); info.awards[2].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("cpcn")) == 0) { tokens.next(value); info.awards[2].name = value; info.awards[2].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("cpa")) == 0)  { tokens.next(value); info.awards[3].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("cpan")) == 0) { tokens.next(value); info.awards[3].name = value; info.awards[3].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("cpd")) == 0)  { tokens.next(value); info.awards[4].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("cpdn")) == 0) { tokens.next(value); info.awards[4].name = value; info.awards[4].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("ka")) == 0)   { tokens.next(value); info.awards[5].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("kan")) == 0)  { tokens.next(value); info.awards[5].name = value; info.awards[5].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("he")) == 0)   { tokens.next(value); info.awards[6].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("hen")) == 0)  { tokens.next(value); info.awards[6].name = value; info.awards[6].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("rev")) == 0)  { tokens.next(value); info.awards[7].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("revn")) == 0) { tokens.next(value); info.awards[7].name = value; info.awards[7].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("rsp")) == 0)  { tokens.next(value); info.awards[8].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("rspn")) == 0) { tokens.next(value); info.awards[8].name = value; info.awards[8].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("rep")) == 0)  { tokens.next(value); info.awards[9].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("repn")) == 0) { tokens.next(value); info.awards[9].name = value; info.awards[9].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("drs")) == 0)  { tokens.next(value); info.awards[10].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("drsn")) == 0) { tokens.next(value); info.awards[10].name = value; info.awards[10].rank = rankOfPlayerNamed(value); }
        // Kit and vehicle leader names use C-string assignment, so embedded NUL characters truncate them.
        else if (compareIgnoreCase(key, std::string("sk0")) == 0)  { tokens.next(value); info.kitLeaders[0].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("skn0")) == 0) { tokens.next(value); info.kitLeaders[0].name = value.c_str(); info.kitLeaders[0].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sk1")) == 0)  { tokens.next(value); info.kitLeaders[1].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("skn1")) == 0) { tokens.next(value); info.kitLeaders[1].name = value.c_str(); info.kitLeaders[1].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sk2")) == 0)  { tokens.next(value); info.kitLeaders[2].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("skn2")) == 0) { tokens.next(value); info.kitLeaders[2].name = value.c_str(); info.kitLeaders[2].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sk3")) == 0)  { tokens.next(value); info.kitLeaders[3].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("skn3")) == 0) { tokens.next(value); info.kitLeaders[3].name = value.c_str(); info.kitLeaders[3].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sk4")) == 0)  { tokens.next(value); info.kitLeaders[4].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("skn4")) == 0) { tokens.next(value); info.kitLeaders[4].name = value.c_str(); info.kitLeaders[4].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sk5")) == 0)  { tokens.next(value); info.kitLeaders[5].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("skn5")) == 0) { tokens.next(value); info.kitLeaders[5].name = value.c_str(); info.kitLeaders[5].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sk6")) == 0)  { tokens.next(value); info.kitLeaders[6].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("skn6")) == 0) { tokens.next(value); info.kitLeaders[6].name = value.c_str(); info.kitLeaders[6].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sv0")) == 0)  { tokens.next(value); info.vehicleLeaders[0].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("svn0")) == 0) { tokens.next(value); info.vehicleLeaders[0].name = value.c_str(); info.vehicleLeaders[0].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sv1")) == 0)  { tokens.next(value); info.vehicleLeaders[1].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("svn1")) == 0) { tokens.next(value); info.vehicleLeaders[1].name = value.c_str(); info.vehicleLeaders[1].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sv2")) == 0)  { tokens.next(value); info.vehicleLeaders[2].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("svn2")) == 0) { tokens.next(value); info.vehicleLeaders[2].name = value.c_str(); info.vehicleLeaders[2].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sv3")) == 0)  { tokens.next(value); info.vehicleLeaders[3].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("svn3")) == 0) { tokens.next(value); info.vehicleLeaders[3].name = value.c_str(); info.vehicleLeaders[3].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sv4")) == 0)  { tokens.next(value); info.vehicleLeaders[4].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("svn4")) == 0) { tokens.next(value); info.vehicleLeaders[4].name = value.c_str(); info.vehicleLeaders[4].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sv5")) == 0)  { tokens.next(value); info.vehicleLeaders[5].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("svn5")) == 0) { tokens.next(value); info.vehicleLeaders[5].name = value.c_str(); info.vehicleLeaders[5].rank = rankOfPlayerNamed(value); }
        else if (compareIgnoreCase(key, std::string("sv6")) == 0)  { tokens.next(value); info.vehicleLeaders[6].score = atoi(value.c_str()); }
        else if (compareIgnoreCase(key, std::string("svn6")) == 0) { tokens.next(value); info.vehicleLeaders[6].name = value.c_str(); info.vehicleLeaders[6].rank = rankOfPlayerNamed(value); }
        else
        {
            // Reads kit and vehicle times for players 0..8 and types 0..6. Other indices are ignored.
            for (int p = 0; p < 9; ++p)
            {
                if (!found)
                {
                    for (int k = 0; k < 7; ++k)
                    {
                        const std::string candidate = formatString("tk%dp%d", k, p);
                        if (compareIgnoreCase(candidate, key) == 0)
                        {
                            std::string time;                                            // [ebp-0x160]
                            tokens.next(time);
                            kitTimes[p][k] = atoi(time.c_str());
                            found = true;
                            break;
                        }
                    }
                }
                if (!found)
                {
                    for (int v = 0; v < 7; ++v)
                    {
                        const std::string candidate = formatString("tv%dp%d", v, p);
                        if (compareIgnoreCase(candidate, key) == 0)
                        {
                            std::string time;                                            // [ebp-0x17C]
                            tokens.next(time);
                            vehicleTimes[p][v] = atoi(time.c_str());
                            found = true;
                            break;
                        }
                    }
                }
            }
            if (!found)
            {
                std::string dropped;                                             // [ebp-0xBC]
                tokens.next(dropped);                                            // the unknown key's value
            }
        }
    }

    // 0x49B399..0x49B4EF - the favourite kit / vehicle and the accuracy of each named top player.
    for (int p = 0; p < 3; ++p)
    {
        SwiffEndOfRoundTopPlayer& top = info.topPlayers[p];
        if (compareIgnoreCase(top.name, std::string("")) != 0)
        {
            int favouriteKit = -1;                                               // [ebp-0x44]
            int kitTime = 0;                                                     // [ebp-0x20]
            int favouriteVehicle = -1;                                           // [ebp-0x48]
            int vehicleTime = 0;                                                 // [ebp-0xA0]
            for (int t = 0; t < 7; ++t)
            {
                if (kitTimes[p][t])
                {
                    if (kitTimes[p][t] > kitTime)                                // signed, strict: first index wins a tie
                    {
                        favouriteKit = t;
                        kitTime = kitTimes[p][t];
                    }
                    top.kits.push_back(t);                                       // 0x4751A0 is the grow path
                }
                if (vehicleTimes[p][t] && vehicleTimes[p][t] > vehicleTime)
                {
                    favouriteVehicle = t;
                    vehicleTime = vehicleTimes[p][t];
                }
            }
            // Accuracy uses single-precision arithmetic; preserve its rounding before formatting.
            float inverse = 0.0f;
            if (top.bulletsFired)
                inverse = 1.0f / static_cast<float>(top.bulletsFired);
            top.favouriteKit = favouriteKit;
            top.favouriteVehicle = favouriteVehicle;
            top.accuracy = static_cast<float>(top.bulletsHit) * inverse * 100.0f;
        }
    }

    // 0x49B4F5..0x49B55F - the map the round was played on, through this object's own slots.
    info.mapName = getCurrentMapName();                                        // +0x58
    info.gameMode = getCurrentMapGameMode();                                   // +0x5C
    info.maxPlayers = getCurrentMapMaxPlayers();                               // +0x60
    info.mapSize = getCurrentMapSize();                                        // +0x64

    // 0x49B565..0x49BBCD - the two teams. `team` is the game's 1-based id; the PE compares
    // `getTeam() - 1` against the 0-based index (lea ecx, [ebx-1]).
    for (int i = 0; i < 2; ++i)
    {
        const int team = i + 1;
        std::map<int, int> squadScores;                                        // squad id -> summed score
        // ds:0x99EF80 (gameLogic) is used without a null test; the answer is a reference that is
        // kept and assigned again below for every top player of this team.
        const std::string& teamName = gameLogic->getTeamName(team);            // +0x48
        info.teams[i].name = teamName;
        // getTeamScore's answer is dereferenced without a test. TeamScore +0x88 is m_roundsWon
        // (friend: see Game/GameLogic/ScoreSystem/TeamScore.h).
        info.teams[i].roundsWon = static_cast<world::IScoreManager*>(world::scoreManager)
                                      ->getTeamScore(team)->m_roundsWon;       // +0x10, then +0x88
        std::list<world::IPlayer*> players = world::playerManager->getPlayersSortedByScore(team);   // PE +0x44, copied (0x483370)
        for (std::list<world::IPlayer*>::iterator it = players.begin(); it != players.end(); ++it)
        {
            world::IPlayer* const player = *it;
            if (player->getTeam() - 1 != i)                                    // PE +0xE4
                continue;
            SwiffEndOfRoundPlayer entry;                                       // [ebp-0xD8], all zero
            entry.name = player->getName();                                    // PE +0x20
            // getScoreData (PE +0x170) is asked six times, in this order; the rank is the raw
            // field here (a -1 stays -1), unlike the name lookup the other rows use.
            entry.rank = player->getScoreData()->m_rank;                       // +0x20
            entry.kills = player->getScoreData()->m_field14;                   // +0x14 kills (pmgr_getScore)
            entry.deaths = player->getScoreData()->m_field10;                  // +0x10 deaths
            entry.score = player->getScoreData()->m_score;                     // +0x00
            entry.teamScore = player->getScoreData()->m_field04;               // +0x04 rplScore
            entry.cmdScore = player->getScoreData()->m_field0C;                // +0x0C cmdScore
            for (int k = 0; k < 3; ++k)
                if (entry.name == info.topPlayers[k].name)                     // operator==(string, string)
                    info.topPlayers[k].team = teamName;
            if (player->getSquadId())                                          // PE +0x108
            {
                // std::map::find (0x6AF2E0) then operator[] (0x47AF50). The key is re-asked for
                // every use, as shipped.
                if (squadScores.find(player->getSquadId()) != squadScores.end())
                    squadScores[player->getSquadId()] += player->getScoreData()->m_score;
                else
                    squadScores[player->getSquadId()] = player->getScoreData()->m_score;
                // getLocalHumanPlayer (PlayerManager +0x30) is asked twice and never null-tested;
                // the local player's value is read before this player's in both compares.
                const int localSquad = world::playerManager->getLocalHumanPlayer()->getSquadId();
                bool squadMate = false;
                if (player->getSquadId() == localSquad)
                {
                    const int localTeam = world::playerManager->getLocalHumanPlayer()->getTeam();
                    squadMate = player->getTeam() == localTeam;
                }
                entry.isSquadMate = squadMate;
            }
            else
            {
                entry.isSquadMate = false;
            }
            entry.isCommander = player->getIsCommander() != 0;                 // PE +0x110, setne
            entry.isLocalPlayer = player == world::playerManager->getLocalHumanPlayer();
            info.teams[i].players.push_back(entry);
        }

        // 0x49B85F..0x49BB4E - the best squad: the highest summed score, walking the map in key
        // order from a floor of -10000 (0xFFFFD8F0), so the lowest squad id wins a tie.
        if (squadScores.size() > 0)
        {
            int bestSquad = 0;
            int bestScore = -10000;
            for (std::map<int, int>::iterator it = squadScores.begin(); it != squadScores.end(); ++it)
            {
                if (it->second > bestScore)
                {
                    bestSquad = it->first;
                    bestScore = it->second;
                }
            }
            if (bestSquad)
            {
                ISquadInfo* const squad =
                    static_cast<ISquadManager*>(squadManager)->getSquadInfo(team, bestSquad);          // +0x28
                world::IPlayer* const leader = world::playerManager->getPlayerFromId(
                    static_cast<ISquadManager*>(squadManager)->getSquadLeader(team, bestSquad));     // +0x3C, +0x20
                if (leader && squad)                                                 // leader tested first
                {
                    info.teams[i].bestSquadName = squad->getSquadName();             // ISquadInfo +0x00
                    info.teams[i].bestSquadScore = bestScore;
                    info.teams[i].squadLeader.name = leader->getName();
                    info.teams[i].squadLeader.rank = rankOfPlayerNamed(leader->getName());   // inlined; getName asked again
                    info.teams[i].squadLeader.score = leader->getScoreData()->m_score;
                    // The score-sorted list is fetched and copied a second time for the members.
                    std::list<world::IPlayer*> members = world::playerManager->getPlayersSortedByScore(team);
                    for (std::list<world::IPlayer*>::iterator it = members.begin(); it != members.end(); ++it)
                    {
                        world::IPlayer* const member = *it;
                        if (member->getTeam() - 1 != team - 1)
                            continue;
                        if (member->getSquadId() != bestSquad)
                            continue;
                        if (member == leader)
                            continue;
                        SwiffEndOfRoundPlayer row;                                   // [ebp-0x128], all zero
                        row.name = member->getName();
                        row.rank = rankOfPlayerNamed(member->getName());             // inlined; getName asked again
                        row.score = member->getScoreData()->m_score;
                        info.teams[i].squadMembers.push_back(row);
                    }
                }
            }
        }
    }

    m_endOfRoundInfo = info;                                                   // 0x498EA0 on this + 0x7BC
    // ECMenu (10) id 9 = ECMenuMenuEvent_EnableBetweenRounds (BF2Engine's registration), no payload,
    // the transmitted delay as the event delay. The only poster of (10, 9) in BF2.exe.
    eventManager->postEvent(static_cast<EventCategory>(10), 9, 0, delay);      // +0x10
    return true;                                                               // mov al, 1
}

// Copies team names, victories, player rows, the winner and ticket history into out.
// Requires GameLogic and returns true.
bool SwiffHost::getEndOfRoundSummary(SwiffEndOfRoundSummary& out)
{
    for (int t = 0; t < 2; ++t)
    {
        const SwiffEndOfRoundTeam& team = m_endOfRoundInfo.teams[t];
        out.teams[t].name = team.name;
        out.teams[t].roundsWon = team.roundsWon;
        for (unsigned int i = 0; i < team.players.size(); ++i)                 // the size is re-read every pass
        {
            SwiffEndOfRoundSummaryPlayer row;                                  // [ebp-0x58], two empty strings
            row.name = team.players[i].name;
            row.score = team.players[i].score;
            row.teamScore = team.players[i].teamScore;
            row.text1C = "";                                                   // operator=(const char*) of 0x87FB54
            row.field48 = 0;
            row.deaths = team.players[i].deaths;
            row.kills = team.players[i].kills;
            out.teams[t].players.push_back(row);
        }
    }
    out.winner = m_endOfRoundInfo.winner;
    out.ticketHistory = gameLogic->getTicketHistory();                         // +0x278, then 0x498160
    return true;
}

// Returns whether playerName exactly matches the local human player, or false if none exists.
// Requires PlayerManager.
bool SwiffHost::isLocalPlayerName(const std::string& playerName)
{
    world::IPlayer* const local = world::playerManager->getLocalHumanPlayer();
    if (local && local->getName() == playerName)
        return true;
    return false;
}

} }
#endif
