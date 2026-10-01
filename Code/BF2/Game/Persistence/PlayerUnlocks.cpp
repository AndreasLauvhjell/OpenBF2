// Path: [INFERRED] Game/Persistence/PlayerUnlocks.cpp.

#include "PlayerUnlocks.h"
#include "../../Core/Ids.h"
#include "../../Core/StringUtil.h"
#include "../Objects/IPlayer.h"
#include "../../Scene/Object/PlayerManager.h"
#include "../GameLogic/ServerGameLogic.h"
#include "../IGameLogic.h"
#include "../Python/PythonHost.h"
#include "../Python/PyRef.h"
#include <cstdlib>
#include <map>
#include <set>
namespace dice { namespace hfe {
namespace {
//  / PE 0x5D5840; every other token has status 2.
int getStatus(std::string status)
{
    if (status == "n") return 0;
    if (status == "s") return 1;
    return 2;
}
}
// /0x898A560; PE 0x831F2C/0x831F9C (14 entries).
int unlockItems[14] = {11,22,33,44,55,66,77,88,99,111,222,333,444,555};
int unlockKits[14] = {0,1,2,3,4,5,6,1,2,3,4,5,0,6};

PlayerUnlocks::PlayerUnlocks()
    : m_refCount(1), m_unlocks(0), m_cache(0),
      m_asynchronous(false), m_profileId(0), m_cacheMode(false) {}
PlayerUnlocks::~PlayerUnlocks() = default;
// : reject aggregation, query the requested face, release on failure.
IBase* PlayerUnlocks::create(unsigned int iid, IBase* outer)
{
    if (outer) return 0;
    PlayerUnlocks* object = new PlayerUnlocks();
    IBase* result = static_cast<IBase*>(object->queryInterface(iid));
    if (!result) object->release();
    return result;
}
void PlayerUnlocks::addRef() { ++m_refCount; }
int PlayerUnlocks::getRef() { return m_refCount; }
int PlayerUnlocks::release()
{
    const int remaining = --m_refCount;
    if (!remaining) delete this;
    return remaining;
}
void* PlayerUnlocks::queryInterface(unsigned int iid) const
{
    return iid == IID_IBase || iid == IID_IPlayerUnlocks
        ? const_cast<PlayerUnlocks*>(this) : 0;
}

bool PlayerUnlocks::getUnlocks(int profileId, bool asynchronous, std::vector<int>& unlocks)
{
    if (!profileId)
    {
#ifndef _WIN32
        // [PLATFORM] The ELF self-deletes here; the PE only returns false.
        if (asynchronous) delete this;
#endif
        return false;
    }
    m_cacheMode = false;
    m_asynchronous = asynchronous;
    m_profileId = profileId;
    m_unlocks = asynchronous ? new std::vector<int> : &unlocks;
    m_unlocks->clear();
    if (isUpdating()) cancel();
    m_parser.clear();
    std::string url = getGameSpyUrl() + "getunlocksinfo.aspx?pid=";
    url += formatString("%d", profileId);
    return get(url, asynchronous);
}
//  / PE 0x5D5C50. This overload is blocking.
bool PlayerUnlocks::getUnlocks(int profileId, UnlockCache& cache)
{
    m_asynchronous = false;
    m_cache = &cache;
    cache.items.clear();
    if (isUpdating()) cancel();
    m_cacheMode = true;
    m_parser.clear();
    std::string url = getGameSpyUrl() + "getunlocksinfo.aspx?pid=";
    url += formatString("%d", profileId);
    return get(url, false);
}

bool PlayerUnlocks::setUnlocks(int profileId, int unlock)
{
    if (isUpdating()) cancel();
    m_parser.clear();
    std::string url = getGameSpyUrl() + "selectunlock.aspx?pid=";
    url += formatString("%d", profileId);
    url += formatString("&id=%d", unlock);
    return post(url, "", false);
}
//  / PE 0x5D5EC0. No missing-column guards in either image.
void PlayerUnlocks::onGetComplete(const std::string& response)
{
    HTTPRequest::onGetComplete(response);
    int metadata[2];
    m_parser.parse(response, KeyParser::Unlocks, metadata);
    if (m_parser.getNumKeyMaps())
    {
        if (m_cacheMode)
        {
            for (unsigned int i = 0; i < m_parser.getNumKeyMaps(); ++i)
            {
                KeyMap* map = m_parser.getKeyMap(i);
                Item item;
                item.id = std::atoi(map->getKey(0)->value.c_str());
                item.status = getStatus(map->getKey(1)->value);
                m_cache->items.push_back(item);
            }
            m_cache->available = metadata[0];
            m_cache->selected = metadata[1];
        }
        else
        {
            for (unsigned int i = 0; i < m_parser.getNumKeyMaps(); ++i)
            {
                KeyMap* map = m_parser.getKeyMap(i);
                const int id = std::atoi(map->getKey(0)->value.c_str());
                const int status = getStatus(map->getKey(1)->value);
                if (status == 1)
                    m_unlocks->push_back(id);
            }
        }
    }
    if (m_asynchronous && !m_cacheMode)
    {
        world::IPlayer* player = world::playerManager->getPlayerFromProfileId(m_profileId);
        python::PyRef args = python::buildArgs(1, player, *m_unlocks);
        python::pythonHost()->handleEvent(static_cast<ScriptEvent>(18), args.get());
        // The callback may change player state; retail resolves the profile again.
        player = world::playerManager->getPlayerFromProfileId(m_profileId);
        setPlayerUnlocks(player, m_unlocks);
        m_profileId = 0;
        if (m_asynchronous)
            delete m_unlocks;
    }
}
//  / PE 0x5D5A50. Failure notifies even in the blocking cache mode.
void PlayerUnlocks::onGetFailure(const std::string& error)
{
    HTTPRequest::onGetFailure(error);
    world::IPlayer* player = world::playerManager->getPlayerFromProfileId(m_profileId);
    python::PyRef args = python::buildArgs(0, player, error);
    python::pythonHost()->handleEvent(static_cast<ScriptEvent>(18), args.get());
    if (m_asynchronous)
        delete m_unlocks;
}
void PlayerUnlocks::onPostComplete(const std::string& response) { HTTPRequest::onPostComplete(response); }
void PlayerUnlocks::onPostFailure(const std::string& error) { HTTPRequest::onPostFailure(error); }
//  / PE 0x5D5D50; two maps receive the same per-kit counts.
void PlayerUnlocks::setPlayerUnlocks(world::IPlayer* player, std::vector<int>* unlocks)
{
    if (!player) return;
    std::map<int, int> kits[2];
    std::set<int> selected;
    for (std::vector<int>::const_iterator it = unlocks->begin(); it != unlocks->end(); ++it)
        selected.insert(*it);
    for (int i = 0; i < 14; ++i)
    {
        kits[0][unlockKits[i]] = 0;
        kits[1][unlockKits[i]] = 0;
    }
    for (int i = 0; i < 14; ++i)
    {
        if (selected.find(unlockItems[i]) != selected.end())
        {
            ++kits[0][unlockKits[i]];
            ++kits[1][unlockKits[i]];
        }
    }
    static_cast<ServerGameLogic*>(gameLogic)->setUnlocks(player, kits);
}
} }
