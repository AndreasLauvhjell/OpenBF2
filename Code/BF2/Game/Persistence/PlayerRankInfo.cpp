// Path: [INFERRED]. ELF 0x82C7300..0x82C8190 / PE 0x5D8480..0x5D8C80.
#include "PlayerRankInfo.h"
#include "SimpleTokenizer.h"
#include "../../Core/Ids.h"
#include "../../Core/StringUtil.h"
#include <cstdlib>
#include <ctime>
namespace dice { namespace hfe {
PlayerRankInfo::PlayerRankInfo() : m_refCount(1) {}
PlayerRankInfo::~PlayerRankInfo() = default;
IBase* PlayerRankInfo::create(unsigned int iid, IBase* outer)
{
    if (outer) return 0;
    PlayerRankInfo* object = new PlayerRankInfo();
    IBase* result = static_cast<IBase*>(object->queryInterface(iid));
    if (!result) object->release();
    return result;
}
void PlayerRankInfo::addRef() { ++m_refCount; }
int PlayerRankInfo::getRef() { return m_refCount; }
int PlayerRankInfo::release()
{
    const int remaining = --m_refCount;
    if (!remaining) delete this;
    return remaining;
}
void* PlayerRankInfo::queryInterface(unsigned int iid) const
{
    return iid == IID_IBase || iid == IID_IPlayerRankInfo ? const_cast<PlayerRankInfo*>(this) : 0;
}
bool PlayerRankInfo::getRankInfo(int profileId, RankInfo& info)
{
    if (!profileId) return false;
    if (isUpdating()) cancel();
    m_notification = false;
    m_parser.clear();
    std::string url = getGameSpyUrl() + "getrankinfo.aspx?pid=";
    url += formatString("%d", profileId);
    if (!get(url, false) || !getNumKeyMaps()) return false;
#ifdef _WIN32
    info.timestamp = static_cast<std::int32_t>(_time32(0));
#else
    info.timestamp = static_cast<std::int32_t>(std::time(0));
#endif
    info.profileId = profileId;
    info.rank = std::atoi(getKeyValue(0, "rank").c_str());
    info.change = std::atoi(getKeyValue(0, "chng").c_str());
    info.decrease = std::atoi(getKeyValue(0, "decr").c_str());
    return true;
}
bool PlayerRankInfo::clearRankNotification(int profileId)
{
    if (!profileId) return false;
    if (isUpdating()) cancel();
    m_notification = true;
    std::string url = getGameSpyUrl() + "ranknotification.aspx?pid=";
    url += formatString("%d", profileId);
    return get(url, false);
}
int PlayerRankInfo::getNumKeyMaps() { return m_parser.getNumKeyMaps(); }
int PlayerRankInfo::getNumKeys()
{
    if (!getNumKeyMaps()) return 0;
    return getKeyMap(0)->getNumKeys();
}
KeyMap* PlayerRankInfo::getKeyMap(unsigned int index) { return m_parser.getKeyMap(index); }
std::string PlayerRankInfo::getKeyValue(unsigned int index, const std::string& name)
{
    KeyMap* map = getKeyMap(index);
    if (map && map->findKey(name)) return map->findKey(name)->value;
    return "";
}
bool PlayerRankInfo::getKey(unsigned int index, int keyIndex, std::string& key, std::string& value)
{
    KeyMap* map = getKeyMap(index);
    if (!map) return false;
    KeyMap::Key* entry = map->getKey(static_cast<unsigned int>(keyIndex));
    if (!entry) return false;
    key = entry->key;
    value = entry->value;
    return true;
}
void PlayerRankInfo::onGetComplete(const std::string& response)
{
    HTTPRequest::onGetComplete(response);
    if (m_notification) return;
    // Retail's rank parser starts scanning immediately after O; the general
    // KeyParser modes skip metadata and cannot be substituted here.
    SimpleTokenizer tokens(response, "\t\n");
    std::string token;
    tokens.next(token);
    if (token != "O") return;
    bool hasData = false;
    for (;;)
    {
        if (!tokens.next(token)) return;
        if (token == "H") break;
        if (token == "$") return;
    }
    KeyMap map;
    m_parser.addKeyMap(map);
    while (tokens.next(token))
    {
        if (token == "D")
        {
            hasData = true;
            unsigned int index = 0;
            while (tokens.next(token))
            {
                if (token == "D")
                {
                    KeyMap next;
                    KeyMap* first = getKeyMap(0);
                    for (unsigned int i = 0; i < first->getNumKeys(); ++i)
                    {
                        KeyMap::Key key;
                        key.key = first->getKey(i)->key;
                        key.value = "";
                        next.addKey(key);
                    }
                    m_parser.addKeyMap(next);
                    index = 0;
                }
                else
                {
                    if (token == "$") return;
                    KeyMap::Key* key = m_parser.getKeyMap(m_parser.getNumKeyMaps() - 1)->getKey(index);
                    if (key) { ++index; key->value = token; }
                }
            }
        }
        else if (token == "$")
        {
            if (!hasData) m_parser.clear();
            return;
        }
        else
        {
            KeyMap::Key key;
            key.key = token.c_str();
            key.value = "";
            m_parser.getKeyMap(m_parser.getNumKeyMaps() - 1)->addKey(key);
        }
    }
}
void PlayerRankInfo::onGetFailure(const std::string& error) { HTTPRequest::onGetFailure(error); }
void PlayerRankInfo::onPostComplete(const std::string& response) { HTTPRequest::onPostComplete(response); }
void PlayerRankInfo::onPostFailure(const std::string& error) { HTTPRequest::onPostFailure(error); }
} }
