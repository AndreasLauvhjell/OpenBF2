// Player statistics requests and response handling.
// Path attested by __FILE__; ELF 0x82C8200..0x82C89C0 and PE tables
//  / 0x7E0470 identify the primary and HTTPRequest interfaces.

#include "PlayerStats.h"
#include "../../Core/Ids.h"
#include "../../Core/StringUtil.h"
#include "../IGame.h"
#include "../IGameServer.h"
#include "../Objects/IPlayer.h"
#include "../../Scene/Object/PlayerManager.h"
#include "../Python/PythonHost.h"
#include "../Python/PyRef.h"
#include <cassert>

namespace dice { namespace hfe {
PlayerStats::PlayerStats() : m_refCount(1), m_profileId(0) {}
PlayerStats::~PlayerStats() = default;
// : reject aggregation, query the requested face, release on failure.
IBase* PlayerStats::create(unsigned int iid, IBase* outer)
{
    if (outer) return 0;
    PlayerStats* object = new PlayerStats();
    IBase* result = static_cast<IBase*>(object->queryInterface(iid));
    if (!result) object->release();
    return result;
}
void PlayerStats::addRef() { ++m_refCount; }
int PlayerStats::getRef() { return m_refCount; }
int PlayerStats::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        delete this;
    return remaining;
}
void* PlayerStats::queryInterface(unsigned int iid) const
{
    return iid == IID_IBase || iid == IID_IPlayerStats
        ? const_cast<PlayerStats*>(this) : 0;
}

bool PlayerStats::getStats(int profileId, const std::string& query,
                           bool asynchronous, bool notifyPython)
{
    if (!profileId)
        return false;
    m_profileId = profileId;
    m_notifyPython = notifyPython;
    if (isUpdating())
        cancel();
    m_parser.clear();
    std::string url = getGameSpyUrl() + "getplayerinfo.aspx?pid=";
    url += formatString("%d", profileId);
    url += query;
    if (!get(url, asynchronous))
    {
#ifndef _WIN32
        // [PLATFORM] ELF asserts false (PlayerStats.cpp:99); PE returns false.
        assert(false);
#endif
        return false;
    }
    return true;
}
//  / PE 0x5D65A0. No boolean result materialized after update.
void PlayerStats::tick() { if (isUpdating()) update(); }
int PlayerStats::getNumKeyMaps() { return m_parser.getNumKeyMaps(); }
int PlayerStats::getNumKeys()
{
    if (!getNumKeyMaps())
        return 0;
    return getKeyMap(0)->getNumKeys();
}
KeyMap* PlayerStats::getKeyMap(unsigned int index) { return m_parser.getKeyMap(index); }
bool PlayerStats::getKey(unsigned int mapIndex, int keyIndex,
                         std::string& key, std::string& value)
{
    KeyMap* map = getKeyMap(mapIndex);
    if (!map)
        return false;
    KeyMap::Key* entry = map->getKey(static_cast<unsigned int>(keyIndex));
    if (!entry)
        return false;
    key = entry->key;
    value = entry->value;
    return true;
}
std::string PlayerStats::getKeyValue(unsigned int mapIndex, const std::string& name)
{
    KeyMap* map = getKeyMap(mapIndex);
    if (map && map->findKey(name))
        return map->findKey(name)->value; // repeated lookup is shipped
    return "";
}
//  / PE 0x5D66B0 (the PE callback receives HTTPRequest*).
void PlayerStats::onGetComplete(const std::string& response)
{
    HTTPRequest::onGetComplete(response);
    m_parser.parse(response, KeyParser::Stats, 0);
    if (m_notifyPython)
    {
        IGameServer* server = game
            ? static_cast<IGameServer*>(game->queryInterface(IID_IGameServer)) : 0;
        if (server)
            server->setCurrentPersistenceObject(static_cast<IPlayerStats*>(this));
        world::IPlayer* player = world::playerManager->getPlayerFromProfileId(m_profileId);
        python::PyRef args = python::buildArgs(1, player, response);
        python::pythonHost()->handleEvent(static_cast<ScriptEvent>(19), args.get());
    }
    m_profileId = 0;
}
//  / PE 0x5D6770; failure leaves the profile id intact.
void PlayerStats::onGetFailure(const std::string& error)
{
    HTTPRequest::onGetFailure(error);
    if (m_notifyPython)
    {
        world::IPlayer* player = world::playerManager->getPlayerFromProfileId(m_profileId);
        python::PyRef args = python::buildArgs(0, player, error);
        python::pythonHost()->handleEvent(static_cast<ScriptEvent>(19), args.get());
    }
}
void PlayerStats::onPostComplete(const std::string& response) { HTTPRequest::onPostComplete(response); }
void PlayerStats::onPostFailure(const std::string& error) { HTTPRequest::onPostFailure(error); }
} }
