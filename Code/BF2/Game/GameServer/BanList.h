// Lazy server-only ban registry for key hashes and addresses. Persistent entries
// load through settings/banlist.con; round bans are omitted when storing.
// Each update ignores dt and checks at most ten entries per map for wall-clock
// expiry. [UNRESOLVED] The two spare scalar fields have no identified role.

#ifndef GAME_GAMESERVER_BANLIST_H     // [INFERRED] guard style
#define GAME_GAMESERVER_BANLIST_H

#include <list>
#include "../../IO/Network/NetworkLayer/Socket/ISocket.h"   // ISocket::Address - isAddressBanned
#include <map>
#include <string>

namespace dice {
namespace hfe {

struct BanInfo
{
    // Field names [INFERRED]; their widths, offsets and values are recovered from both ports.
    int  expireTime;                               // +0x00  time(0) epoch; 0 = permanent/round
    int  banType;                                  // +0x04  0 timed / 1 permanent / 2 round
};

class BanList
{
public:
    static BanList* getInstance();

    BanList();                                                            //  (156 B twin)
    ~BanList();

    bool update(float dt);
    void endOfRound();
    bool addAddressToBanList(const std::string& address, const std::string& time);
    bool addKeyToBanList(const std::string& key, const std::string& time);
    bool removeKeyFromBanList(const std::string& key);
    bool removeAddressFromBanList(const std::string& address);
    void clearBanList();
    bool parseBanTime(const std::string& text, BanInfo& out);
    bool isNumeric(const std::string& text);                              // inlined in PE
    bool getSimplifiedBanList(std::list<std::string>& out, bool verbose);
    bool isKeyBanned(const std::string& key) const;
    // declared AND  - NetServer::handleConnectRequest calls it first, before parsing a
    // field. The non-const member and non-const Address* are frozen by ELF's mangled symbol and the
    // independently matched PE function.
    bool isAddressBanned(io::ISocket::Address* address);
    bool storeBanList();
    void banPlayer(int playerId);
    void banPlayer(int playerId, const std::string& time);
    void banPlayerKey(int playerId);
    void banPlayerKey(int playerId, const std::string& time);
    // the admin.* console surface
    std::string consoleBanPlayer(const std::string& name);
    std::string consoleBanPlayer(const std::string& name, const std::string& time);
    std::string consoleBanPlayerKey(const std::string& name);
    std::string consoleBanPlayerKey(const std::string& name, const std::string& time);
    void consoleAddAddressToBanList(const std::string& address);
    bool consoleAddAddressToBanList(const std::string& address, const std::string& time);
    void consoleAddKeyToBanList(const std::string& key);
    bool consoleAddKeyToBanList(const std::string& key, const std::string& time);
    bool consoleRemoveAddressFromBanList(const std::string& address);
    bool consoleRemoveKeyFromBanList(const std::string& key);
    bool consoleClearBanList();
    std::string consoleListBannedAddresses();
    std::string consoleListBannedKeys();
    void alertOnBanned(int playerId, const std::string& time);
    void alertOnKicked(int playerId);

    typedef std::map<std::string, BanInfo> BanMap;

    BanMap            m_bannedKeys;                // +0x00
    BanMap            m_bannedAddresses;           // +0x0C
    BanMap::iterator  m_keyCursor;                 // +0x18
    BanMap::iterator  m_addressCursor;             // +0x1C
    int               m_field20;                   // +0x20
    int               m_field24;                   // +0x24

    static BanList*   _instance;                   // ds:0x8AAB6A4
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMESERVER_BANLIST_H
