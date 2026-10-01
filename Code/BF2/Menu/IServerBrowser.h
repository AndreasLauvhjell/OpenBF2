// Server-browser interface and records shared with the menu plugin.
// This interface has no virtual destructor; concrete implementations append their own.
// Preserve virtual order. STL-containing records require explicit conversion across compiler ABIs.
#ifndef MENU_ISERVERBROWSER_H
#define MENU_ISERVERBROWSER_H
#include "../Core/IBase.h"
#include <string>
#include <vector>
#if !DEDICATED

// The SDK's opaque server handle (sb_serverbrowsing.h: `typedef struct _SBServer *SBServer;`),
// redeclared identically so engine headers do not pull the SDK (and winsock) in.
struct _SBServer;
typedef struct _SBServer* SBServer;

namespace dice { namespace hfe { namespace ui {

// The 76-byte player row (0x4840A0 builds one per "player_N" key, 0x483A70 one per local
// player). [INFERRED] names from the keys read into each field.
struct ServerBrowserPlayer
{
    int         pid;        // +0   "pid_N"
    std::string name;       // +4   "player_N"
    std::string team;       // +32  "team_N"
    int         skill;      // +60  "skill_N"
    int         deaths;     // +64  "deaths_N"
    int         score;      // +68  "score_N"
    int         aiBot;      // +72  "AIBot_N" - kept as the int the loop compares with 1
    ServerBrowserPlayer() : pid(0), skill(0), deaths(0), score(0), aiBot(0) {}
};

// The 280-byte entry half at record +4: what the master list and a basic query answer
// (0x4840A0's first block). Offsets are the half's own. [INFERRED] names from the keys.
struct ServerBrowserEntry
{
    std::string address;            // +0    inet_ntoa(SBServerGetPublicInetAddress)
    int         queryPort;          // +28   SBServerGetPublicQueryPort
    std::string privateAddress;     // +32   inet_ntoa(SBServerGetPrivateInetAddress)
    int         privateQueryPort;   // +60   SBServerGetPrivateQueryPort
    std::string hostName;           // +64   "hostname"
    int         ping;               // +92   SBServerGetPing
    int         numPlayers;         // +96   "numplayers"
    int         maxPlayers;         // +100  "maxplayers"
    int         reservedSlots;      // +104  qr2 key 78 "bf2_reservedslots"
    std::string gameMode;           // +108  "gametype"
    bool        password;           // +136  "password" != 0
    int         field140;           // +140  always 2 (0x484264, and 0x483A70's local record) [UNRESOLVED] role
    std::string mapName;            // +144  "mapname"
    std::string gameVariant;        // +172  "gamevariant" (the mod)
    std::string gameVersion;        // +200  "gamever", default "0.0"
    bool        dedicated;          // +228  "bf2_dedicated" != 0
    bool        ranked;             // +229  "bf2_ranked" == 1
    bool        antiCheat;          // +230  "bf2_anticheat" != 0
    std::string os;                 // +232  "bf2_os"
    bool        autoRecord;         // +260  "bf2_autorec" == 1
    bool        voip;               // +261  "bf2_voip" == 1
    bool        bots;               // +262  "bf2_bots" == 1
    bool        pure;               // +263  "bf2_pure" == 1
    int         mapSize;            // +264  "bf2_mapsize"
    float       fps;                // +268  "bf2_fps"
    int         hostPort;           // +272  "hostport" - the game port SwiffHost::joinServer reads (record +276)
    bool        plasma;             // +276  qr2 key 77 "bf2_plasma" != 0
    ServerBrowserEntry()
        : queryPort(0), privateQueryPort(0), ping(0), numPlayers(0), maxPlayers(0), reservedSlots(0),
          password(false), field140(0), dedicated(false), ranked(false), antiCheat(false),
          autoRecord(false), voip(false), bots(false), pure(false), mapSize(0), fps(0.0f), hostPort(0),
          plasma(false) {}
};

// The 296-byte details half at record +284: what a full query answers (0x4840A0's `full`
// block). Offsets are the half's own. [INFERRED] names from the keys.
struct ServerBrowserDetails
{
    std::string team1;              // +0    "bf2_team1", default "team_1"
    std::string team2;              // +28   "bf2_team2", default "team_2"
    std::string demoIndexUrl;       // +56   "bf2_d_idx"
    std::string demoDownloadUrl;    // +84   "bf2_d_dl"
    bool        friendlyFire;       // +112  "bf2_friendlyfire" != 0
    std::string tkMode;             // +116  "bf2_tkmode"
    int         startDelay;         // +144  "bf2_startdelay"
    float       spawnTime;          // +148  "bf2_spawntime"
    int         timeLimit;          // +152  "timelimit"
    int         roundTime;          // +156  "roundtime"
    std::string sponsorText;        // +160  "bf2_sponsortext"
    std::string sponsorLogoUrl;     // +188  "bf2_sponsorlogo_url"
    std::string communityLogoUrl;   // +216  "bf2_communitylogo_url"
    bool        autoBalanced;       // +244  "bf2_autobalanced" == 1
    float       teamRatio;          // +248  "bf2_teamratio"
    int         ticketRatio;        // +252  "bf2_ticketratio"
    int         scoreLimit;         // +256  "bf2_scorelimit"
    bool        globalUnlocks;      // +260  "bf2_globalunlocks" == 1
    std::vector<ServerBrowserPlayer> players;   // +264  one row per answered "player_N"
    float       coopBotRatio;       // +280  "bf2_coopbotratio"
    int         coopBotCount;       // +284  "bf2_coopbotcount"
    float       coopBotDifficulty;  // +288  "bf2_coopbotdiff"
    bool        noVehicles;         // +292  "bf2_novehicles" != 0.0
    ServerBrowserDetails()
        : friendlyFire(false), startDelay(0), spawnTime(0.0f), timeLimit(0), roundTime(0),
          autoBalanced(false), teamRatio(0.0f), ticketRatio(0), scoreLimit(0), globalUnlocks(false),
          coopBotRatio(0.0f), coopBotCount(0), coopBotDifficulty(0.0f), noVehicles(false) {}
};

// The 580-byte record.
struct ServerBrowserRecord
{
    SBServer             server;    // +0    the SDK server the record was filled from
    ServerBrowserEntry   entry;     // +4
    ServerBrowserDetails details;   // +284
    ServerBrowserRecord() : server(0) {}
};

// The 24-byte record list (+0x08 the results, +0x20 the pending queue). The SwiffPlayer front
// reads the revision at +16 through getRecords() to notice a change.
struct ServerBrowserRecordList
{
    std::vector<ServerBrowserRecord> records;   // +0   (VC7.1: allocator pad, begin +4, end +8, capacity +12)
    int revision;                               // +16  bumped by every change to the records
    int maxPing;                                // +20  -1 = unlimited; 0x485800 drops a record whose ping exceeds it
    ServerBrowserRecordList() : revision(0), maxPing(-1) {}
};

struct IServerBrowser : IBase
{
    virtual bool init(bool internet) = 0;                                                       // +0x10  0x485C90  the availability check, then ServerBrowserNew
    virtual void shutdown() = 0;                                                                // +0x14  0x482420  ServerBrowserFree
    virtual void update() = 0;                                                                  // +0x18  0x485380  the per-frame drain of the pending queue and Think
    virtual bool isBrowsing() = 0;                                                              // +0x1C  0x482900  pending records, or the SDK in listxfer / querying
    virtual ServerBrowserRecordList* getRecords() = 0;                                          // +0x20  0x659EC0  this+8
    virtual void reset() = 0;                                                                   // +0x24  0x485480  both lists cleared, ServerBrowserClear
    virtual bool startUpdate(bool async) = 0;                                                   // +0x28  0x4854B0  ServerBrowserUpdate (internet) / ServerBrowserLANUpdate (lan)
    virtual bool requeueRecords() = 0;                                                          // +0x2C  0x485E10  the results become the pending queue
    virtual void stopUpdate() = 0;                                                              // +0x30  0x485340  ServerBrowserHalt, then the last sort again
    virtual bool isInternet() = 0;                                                              // +0x34
    virtual void sort(const std::string& key, bool ascending, int stringCompare) = 0;          // +0x38  0x4858B0  ServerBrowserSort, then the results rebuilt
    virtual ServerBrowserRecord* getRecord(const std::string& address, int port, bool full, bool useCached) = 0;  // +0x3C
    virtual bool fillRecord(SBServer server, ServerBrowserRecord* out, bool full, bool useCached) = 0;            // +0x40
    virtual bool fillLocalRecord(ServerBrowserRecord* out) = 0;                                 // +0x44  0x483A70  the record of the server this client hosts
    virtual bool queryAddress(const std::string& address, int port, bool viaMaster) = 0;       // +0x48  0x482490  ServerBrowserAuxUpdateIP
    virtual bool removeAddress(const std::string& address, int port) = 0;                      // +0x4C  0x482530  ServerBrowserRemoveIP
    virtual std::string& getFilter() = 0;                                                       // +0x50  0x64E840  this+0x64
    virtual void setFilter(const std::string& filter) = 0;                                      // +0x54
    virtual SBServer getServer(const std::string& address, int port) = 0;                       // +0x58  0x482440  ServerBrowserGetServerByIP
    virtual void setMaxPing(int maxPing) = 0;                                                   // +0x5C
    virtual void setKeepUnansweredServers(bool keep) = 0;                                       // +0x60  0x482410  the +0x88 flag: list servers without basic keys (favourites, history)
};

} // namespace ui
} }
#endif // !dedicated
#endif
