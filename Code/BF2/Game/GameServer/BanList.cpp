// Server bans, persistence, and administrator notifications.
#include "BanList.h"
#include "../IGameServer.h"                       // player IP lookup and deferred disconnect
#include "../GameEvents/KickBanEvent.h"           // alertOnKicked's broadcast
#include "../MatchMaking/Server/MatchMakingServer.h" // gamespy::g_gameID for CD-key bans
#include "../Python/PythonHost.h"                 // buildArgs / ScriptEvent 17
#include "../Python/PyRef.h"                      // owning result of buildArgs
#include "../../IO/Network/NetworkLayer/Socket/UDPSocket.h"   // nameToAddress / equalAddress
#include "../../IO/File/FileManager.h"            // fileManager - persistent ban-list writer
#include "../../Scene/Object/PlayerManager.h"      // alertOnKicked's player lookup
#include "../Objects/IPlayer.h"                    // ban eligibility / id lookups
#include <cctype>                                  // tolower - parseBanTime
#include <cstdio>                                  // sprintf - getSimplifiedBanList
#include <cstdlib>                                 // atoi - parseBanTime
#include <cstring>                                 // memset - address validation / lookup
#include <sstream>                                 // storeBanList's text buffer

#include <ctime>                                   // time(0) - the expiry clock

#include "../IGame.h"                              // game - queryInterface(IID_IGameServer)
#include "../../Core/Ids.h"                        // IID_IGameServer
#include "../../IO/Console/Console.h"              // mainConsole->run + Console::ignoredString_

namespace dice {
namespace hfe {

// The TU's one static: `std::string banListFile("settings/banlist.con")` at ds:0x8AAB6AC (static
// init _90, 0x80A4050). Relative to the mounted mod root, like ServerSettings' settings file.
namespace {
std::string banListFile("settings/banlist.con");

// Run the ban file through the console - the shipped call in getInstance and endOfRound: eight
// empty argument strings and the shared ignored out-string.
void runBanListFile()
{
    io::mainConsole->run(banListFile, "", "", "", "", "", "", "", "", io::Console::ignoredString_);
}

// The GameSpy SDK owns the connected player's CD-key hash. Kept out of gcdkeys.h here so its
// common platform headers do not leak into the game-server TU, matching GameServer.cpp's seam.
extern "C" char* gcd_getkeyhash(int gameid, int localid);
} // namespace

BanList* BanList::_instance = 0;                   // ds:0x8AAB6A4

//  (the header).
BanList* BanList::getInstance()
{
    void* server = 0;
    if (game)
        server = game->queryInterface(IID_IGameServer);
    if (!server)
        return 0;
    if (!_instance)
    {
        _instance = new BanList();
        runBanListFile();
    }
    return _instance;
}

// : two empty maps (their header nodes), both cursors on their map's end(), the two
// trailing ints 0.
BanList::BanList()
    : m_field20(0)                                 // +0x20
    , m_field24(0)                                 // +0x24
{
    m_addressCursor = m_bannedAddresses.end();     // +0x1C = the +0x0C header
    m_keyCursor     = m_bannedKeys.end();          // +0x18 = the +0x00 header
}

// : the two maps.
BanList::~BanList()
{
}

// the per-tick expiry (the header). Addresses first, then keys - the shipped
// order; ten visits per map per tick.
bool BanList::update(float)
{
    const int now = (int)time(0);
    if (!m_bannedAddresses.empty())
    {
        if (m_addressCursor == m_bannedAddresses.end())
            m_addressCursor = m_bannedAddresses.begin();
        for (int visited = 0; ; )
        {
            if (m_addressCursor == m_bannedAddresses.end())
                m_addressCursor = m_bannedAddresses.begin();
            const BanInfo& ban = m_addressCursor->second;
            if (ban.banType == 0 && ban.expireTime != 0 && ban.expireTime <= now)
            {
                BanMap::iterator next = m_addressCursor;
                ++next;
                m_bannedAddresses.erase(m_addressCursor);
                m_addressCursor = next;
            }
            else
                ++m_addressCursor;
            if (++visited > 9 || m_bannedAddresses.empty())
                break;
        }
    }
    if (!m_bannedKeys.empty())
    {
        if (m_keyCursor == m_bannedKeys.end())
            m_keyCursor = m_bannedKeys.begin();
        for (int visited = 0; ; )
        {
            if (m_keyCursor == m_bannedKeys.end())
                m_keyCursor = m_bannedKeys.begin();
            const BanInfo& ban = m_keyCursor->second;
            if (ban.banType == 0 && ban.expireTime != 0 && ban.expireTime <= now)
            {
                BanMap::iterator next = m_keyCursor;
                ++next;
                m_bannedKeys.erase(m_keyCursor);
                m_keyCursor = next;
            }
            else
                ++m_keyCursor;
            if (++visited > 9 || m_bannedKeys.empty())
                break;
        }
    }
    return true;
}

// : drop everything, reload the file (round bans are not in the file).
void BanList::endOfRound()
{
    clearBanList();
    runBanListFile();
}

// Returns false on invalid expiry or address; successful insertion replaces the entry and resets expiry scanning.
// Address text remains the key, so hostnames are resolved again when checking connections.
bool BanList::addAddressToBanList(const std::string& address, const std::string& timeText)
{
    io::ISocket::Address resolved;
    std::memset(resolved.sockaddr, 0, sizeof(resolved.sockaddr));
    resolved.length = 16;

    BanInfo parsed;
    if (io::UDPSocket::nameToAddress(address, &resolved, 0) || !parseBanTime(timeText, parsed))
        return false;

    m_bannedAddresses[address] = parsed;
    m_addressCursor = m_bannedAddresses.begin();
    return true;
}

bool BanList::addKeyToBanList(const std::string& key, const std::string& timeText)
{
    BanInfo parsed;
    if (!parseBanTime(timeText, parsed))
        return false;

    m_bannedKeys[key] = parsed;
    m_keyCursor = m_bannedKeys.begin();
    return true;
}

// : both maps emptied, both cursors back on end().
void BanList::clearBanList()
{
    m_bannedAddresses.clear();
    m_bannedKeys.clear();
    m_addressCursor = m_bannedAddresses.end();
    m_keyCursor     = m_bannedKeys.end();
}

// : a linear walk of the key map comparing each key for equality (memcmp of the
// shorter length, then the length difference - i.e. ==); the first hit is erased and the cursor
// reset to begin(); true on a hit.
bool BanList::removeKeyFromBanList(const std::string& key)
{
    for (BanMap::iterator it = m_bannedKeys.begin(); it != m_bannedKeys.end(); ++it)
    {
        if (it->first == key)
        {
            m_bannedKeys.erase(it);
            m_keyCursor = m_bannedKeys.begin();
            return true;
        }
    }
    return false;
}

//  / PE 0x4C8120. Like removeKeyFromBanList, retail walks linearly instead of using
// map::find, erases only the first equal key, then resets the expiry cursor to begin().
bool BanList::removeAddressFromBanList(const std::string& address)
{
    for (BanMap::iterator it = m_bannedAddresses.begin(); it != m_bannedAddresses.end(); ++it)
    {
        if (it->first == address)
        {
            m_bannedAddresses.erase(it);
            m_addressCursor = m_bannedAddresses.begin();
            return true;
        }
    }
    return false;
}

// Parses empty/perm as permanent and round as a round ban. Other strings without
// '-' are timed bans: ':' prefixes an absolute epoch; otherwise atoi seconds are
// added to now. Non-numeric text is accepted as zero seconds.
bool BanList::parseBanTime(const std::string& text, BanInfo& out)
{
    const int now = static_cast<int>(std::time(0));
    std::string lowered(text);
    for (std::string::iterator it = lowered.begin(); it != lowered.end(); ++it)
        *it = static_cast<char>(std::tolower(*it));

    if (lowered.empty() || lowered == "perm")
    {
        out.expireTime = 0;
        out.banType = 1;
        return true;
    }

    if (lowered == "round")
    {
        out.expireTime = std::atoi(text.c_str());
        out.banType = 2;
        return true;
    }

    if (!isNumeric(text))
        return false;

    if (!text.empty() && text[0] == ':')
        out.expireTime = std::atoi(text.substr(1).c_str());
    else
        out.expireTime = now + std::atoi(text.c_str());
    out.banType = 0;
    return true;
}

bool BanList::isNumeric(const std::string& text)
{
    return text.find('-') == std::string::npos;
}

// getSimplifiedBanList - ELF 0x80A2460, 2,353 B / PE 0x4C81C0, 2,094 B. Addresses are emitted
// first, then keys. `verbose` selects the admin-list display dialect; false selects the exact
// persistence dialect storeBanList consumes. Round bans appear only in verbose output, which is
// why endOfRound can clear/reload the file without resurrecting a round-only ban.
bool BanList::getSimplifiedBanList(std::list<std::string>& out, bool verbose)
{
    out.clear();
    const int now = static_cast<int>(std::time(0));
    const std::string addressPrefix = verbose ? "IP: " : "";
    const std::string keyPrefix = verbose ? "Key: " : "key:";
    const std::string timeSeparator = verbose ? " Time Left: " : " :";
    char buffer[64];

    for (BanMap::const_iterator it = m_bannedAddresses.begin();
         it != m_bannedAddresses.end(); ++it)
    {
        const BanInfo& ban = it->second;
        if (ban.banType == 1)
            out.push_back(addressPrefix + it->first + " Perm");
        else if (ban.banType == 0)
        {
            std::sprintf(buffer, "%d", verbose ? ban.expireTime - now : ban.expireTime);
            out.push_back(addressPrefix + it->first + timeSeparator + buffer);
        }
        else if (ban.banType == 2 && verbose)
            out.push_back(addressPrefix + it->first + " Round");
    }

    for (BanMap::const_iterator it = m_bannedKeys.begin(); it != m_bannedKeys.end(); ++it)
    {
        const BanInfo& ban = it->second;
        if (ban.banType == 1)
            out.push_back(keyPrefix + it->first + " Perm");
        else if (ban.banType == 0)
        {
            std::sprintf(buffer, "%d", verbose ? ban.expireTime - now : ban.expireTime);
            out.push_back(keyPrefix + it->first + timeSeparator + buffer);
        }
        else if (ban.banType == 2 && verbose)
            out.push_back(keyPrefix + it->first + " Round");
    }

    return true;
}

bool BanList::isKeyBanned(const std::string& key) const
{
    return m_bannedKeys.find(key) != m_bannedKeys.end();
}

// Player ban surface - ELF 0x80A3490..0x80A39F0, independently matched in the PE. The id forms
// deliberately kick even when the supplied time fails to parse: add* rejects the ban, alertOnBanned
// returns after its own parse, then kickPlayer still queues reason 21. The address form excludes
// disconnected players and id 255; the key form tests the supplied id before looking the player up.
void BanList::banPlayer(int playerId)
{
    banPlayer(playerId, std::string());
}

void BanList::banPlayer(int playerId, const std::string& time)
{
    IGameServer* const server = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
    world::IPlayer* const player = world::playerManager->getPlayerFromId(playerId);
    if (player && player->getIsConnected() && player->getId() != 255)
    {
        std::string address;
        if (server->getPlayerConnectionIp(player, address))
            addAddressToBanList(address, time);

        alertOnBanned(playerId, time);
        server->kickPlayer(playerId, static_cast<io::PResult>(21), std::string());
    }
}

void BanList::banPlayerKey(int playerId)
{
    banPlayerKey(playerId, std::string());
}

void BanList::banPlayerKey(int playerId, const std::string& time)
{
    if (playerId == 255)
        return;

    IGameServer* server = 0;
    if (game)
        server = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));

    world::IPlayer* const player = world::playerManager->getPlayerFromId(playerId);
    if (player && player->getIsConnected())
    {
        const std::string key(gcd_getkeyhash(gamespy::g_gameID, playerId));
        addKeyToBanList(key, time);
        alertOnBanned(playerId, time);
        server->kickPlayer(playerId, static_cast<io::PResult>(21), std::string());
    }
}

std::string BanList::consoleBanPlayer(const std::string& name)
{
    return consoleBanPlayer(name, std::string());
}

std::string BanList::consoleBanPlayer(const std::string& name, const std::string& time)
{
    world::IPlayer* const player = world::playerManager->getPlayerFromIdString(name);
    if (!player)
        return std::string("Player not found.");

    banPlayer(player->getId(), time);
    storeBanList();
    return std::string();
}

std::string BanList::consoleBanPlayerKey(const std::string& name)
{
    return consoleBanPlayerKey(name, std::string());
}

std::string BanList::consoleBanPlayerKey(const std::string& name, const std::string& time)
{
    world::IPlayer* const player = world::playerManager->getPlayerFromIdString(name);
    if (!player)
        return std::string("Player not found.");

    banPlayerKey(player->getId(), time);
    storeBanList();
    return std::string();
}

// the console forwarders: mutate the live map, then persist
void BanList::consoleAddAddressToBanList(const std::string& address)
{
    consoleAddAddressToBanList(address, std::string(""));
}

bool BanList::consoleAddAddressToBanList(const std::string& address, const std::string& time)
{
    addAddressToBanList(address, time);
    return storeBanList();
}

void BanList::consoleAddKeyToBanList(const std::string& key)
{
    consoleAddKeyToBanList(key, std::string(""));
}

bool BanList::consoleAddKeyToBanList(const std::string& key, const std::string& time)
{
    addKeyToBanList(key, time);
    return storeBanList();
}

bool BanList::consoleRemoveAddressFromBanList(const std::string& address)
{
    removeAddressFromBanList(address);
    return storeBanList();
}

bool BanList::consoleRemoveKeyFromBanList(const std::string& key)
{
    removeKeyFromBanList(key);
    return storeBanList();
}

// Matches address bans by host, ignoring ports. Resolves each stored name on every
// call and skips failed resolutions; hostname entries can perform DNS work per request.
bool BanList::isAddressBanned(io::ISocket::Address* address)
{
    for (BanMap::const_iterator it = m_bannedAddresses.begin();
         it != m_bannedAddresses.end(); ++it)
    {
        io::ISocket::Address resolved;
        std::memset(resolved.sockaddr, 0, sizeof(resolved.sockaddr));
        resolved.length = 16;

        if (!io::UDPSocket::nameToAddress(it->first, &resolved, 0))      // 0 == resolved
        {
            if (io::UDPSocket::equalAddress(address, &resolved))
                return true;
        }
    }
    return false;
}

// storeBanList - ELF 0x80A2E80, 1,539 B / PE 0x4C8A60, 449 B. Opens through the engine VFS with
// (mode 0, access 2, flags 0), serialises only persistent entries, writes the exact byte count,
// flushes, releases the stream, and reports a short write. No OS-file shortcut and no null guard
// around fileManager are present in retail.
bool BanList::storeBanList()
{
    io::IStream* file = io::fileManager->open(
        banListFile, static_cast<io::FileMode>(0), io::FILEACCESS_2, 0);
    if (!file)
        return false;

    std::stringstream text;
    std::list<std::string> bans;
    getSimplifiedBanList(bans, false);
    for (std::list<std::string>::const_iterator it = bans.begin(); it != bans.end(); ++it)
    {
        if (it->find("key:") != 0)
            text << "admin.addAddressToBanList " << *it << std::endl;
        else
            text << "admin.addKeyToBanList " << it->substr(4) << std::endl;
    }

    const std::string contents = text.str();
    const unsigned int written = file->write(contents.c_str(), static_cast<unsigned int>(contents.size()));
    file->flushWrite();
    file->release();
    return written == contents.size();
}

bool BanList::consoleClearBanList()
{
    clearBanList();
    return storeBanList();
}

// The verbose formatter emits addresses first with "IP: " and keys with "Key: ". Retail filters
// that shared list rather than walking the maps directly and appends one newline per selected row.
std::string BanList::consoleListBannedAddresses()
{
    std::string result;
    std::list<std::string> bans;
    getSimplifiedBanList(bans, true);
    for (std::list<std::string>::const_iterator it = bans.begin(); it != bans.end(); ++it)
    {
        if (it->find("Key:") != 0)
            result += *it + "\n";
    }
    return result;
}

std::string BanList::consoleListBannedKeys()
{
    std::string result;
    std::list<std::string> bans;
    getSimplifiedBanList(bans, true);
    for (std::list<std::string>::const_iterator it = bans.begin(); it != bans.end(); ++it)
    {
        if (it->find("Key: ") == 0)
            result += *it + "\n";
    }
    return result;
}

//  / PE twin. A valid ban broadcasts the client notification first, then reports Python
// ScriptEvent 16 with (player, absolute expiry, ban type). Invalid time text produces no alert.
void BanList::alertOnBanned(int playerId, const std::string& time)
{
    world::IPlayer* const player = world::playerManager->getPlayerFromId(playerId);
    if (!player)
        return;

    BanInfo info;
    if (!parseBanTime(time, info))
        return;

    game->sendGameEventToAllConnected(new KickBanEvent(playerId, true, 0));
    python::PyRef args = python::buildArgs(player, info.expireTime, info.banType);
    python::pythonHost()->handleEvent(static_cast<ScriptEvent>(16), args.get());
}

// alertOnKicked - ELF 0x80A3F80, 203 B / PE 0x4C7DA0, 135 B. Both ports first look up the player;
// a missing id is a silent no-op. For a live player retail broadcasts KickBanEvent(id, false, 0),
// then reports ScriptEvent 17 to Python with that player as the sole argument. The game and Python
// singletons are intentionally unguarded, matching both shipped functions.
void BanList::alertOnKicked(int playerId)
{
    world::IPlayer* const player = world::playerManager->getPlayerFromId(playerId);
    if (!player)
        return;

    game->sendGameEventToAllConnected(new KickBanEvent(playerId, false, 0));
    python::PyRef args = python::buildArgs(player);
    python::pythonHost()->handleEvent(static_cast<ScriptEvent>(17), args.get());
}

} // namespace hfe
} // namespace dice
