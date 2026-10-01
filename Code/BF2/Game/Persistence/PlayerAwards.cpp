// Path: [INFERRED] Game/Persistence/PlayerAwards.cpp.

#include "PlayerAwards.h"
#include "../../Core/Ids.h"
#include "../../Core/StringUtil.h"
#include "../IGame.h"
#include "../IGameServer.h"
#include "../Objects/IPlayer.h"
#include "../../Scene/Object/PlayerManager.h"
#include "../Python/PythonHost.h"
#include "../Python/PyRef.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
namespace dice { namespace hfe {
// Names are attested by the ELF sort instantiations. Despite 'Ascending',
// both these comparators use greater-than (ELF 0x82C49D0/0x82C5230;
// /0x5D3E10). DescendingDateSort uses less-than.
struct AscendingPrioritySort {
    bool operator()(const IPlayerAwards::Award& a, const IPlayerAwards::Award& b) const
    { return a.priority > b.priority; }
};
struct AscendingDateSort {
    bool operator()(const IPlayerAwards::Award& a, const IPlayerAwards::Award& b) const
    { return a.when > b.when; }
};
struct DescendingDateSort {
    bool operator()(const IPlayerAwards::Award& a, const IPlayerAwards::Award& b) const
    { return a.when < b.when; }
};
PlayerAwards::PlayerAwards()
    : m_refCount(1), m_result(0), m_notifyPython(false), m_profileId(0) {}
PlayerAwards::~PlayerAwards() = default; // result is not freed here in either image
// : reject aggregation, query the requested face, release on failure.
IBase* PlayerAwards::create(unsigned int iid, IBase* outer)
{
    if (outer) return 0;
    PlayerAwards* object = new PlayerAwards();
    IBase* result = static_cast<IBase*>(object->queryInterface(iid));
    if (!result) object->release();
    return result;
}
void PlayerAwards::addRef() { ++m_refCount; }
int PlayerAwards::getRef() { return m_refCount; }
int PlayerAwards::release()
{
    const int remaining = --m_refCount;
    if (!remaining) delete this;
    return remaining;
}
void* PlayerAwards::queryInterface(unsigned int iid) const
{
    return iid == IID_IBase || iid == IID_IPlayerAwards
        ? const_cast<PlayerAwards*>(this) : 0;
}
//  / PE 0x5CF120. The parser clear occurs on both sides of cancel.
bool PlayerAwards::getAwards(int profileId, AwardList* output, bool asynchronous, bool notifyPython)
{
    if (!profileId) return false;
    m_notifyPython = notifyPython;
    m_profileId = profileId;
    if (output)
    {
        output->profileId = profileId;
        output->awards.clear();
        m_result = output;
    }
    else
        m_result = new AwardList;
    m_parser.clear();
    if (isUpdating()) cancel();
    m_parser.clear();
    std::string url = getGameSpyUrl() + "getawardsinfo.aspx?pid=";
    url += formatString("%d", profileId);
    return get(url, asynchronous);
}
void PlayerAwards::tick() { if (isUpdating()) update(); }
//  / PE 0x5CF510. HTTP success ignores parse's return;
// malformed rows have no missing-key guards in retail.
void PlayerAwards::onGetComplete(const std::string& response)
{
    HTTPRequest::onGetComplete(response);
    m_parser.parse(response, KeyParser::Stats, 0);
    for (unsigned int i = 0; i < m_parser.getNumKeyMaps(); ++i)
    {
        Award award;
        award.id = std::atoi(m_parser.getKeyMap(i)->findKey("award")->value.c_str());
        award.level = std::atoi(m_parser.getKeyMap(i)->findKey("level")->value.c_str());
        award.when = std::atol(m_parser.getKeyMap(i)->findKey("when")->value.c_str());
        award.first = std::atol(m_parser.getKeyMap(i)->findKey("first")->value.c_str());
        if (!m_notifyPython)
            getAwardInfoEx(award);
        m_result->awards.push_back(award);
    }
#ifdef _WIN32
    // The original PE uses 32-bit time_t; modern MSVC defaults to 64-bit.
    _time32(&m_result->timestamp);
#else
    m_result->timestamp = static_cast<std::int32_t>(std::time(0));
#endif
    if (m_notifyPython)
    {
        IGameServer* server = game
            ? static_cast<IGameServer*>(game->queryInterface(IID_IGameServer)) : 0;
        if (server)
            server->setCurrentPersistenceObject(static_cast<IPlayerAwards*>(this));
        std::vector<std::vector<int>> awards;
        for (std::vector<Award>::const_iterator it = m_result->awards.begin();
             it != m_result->awards.end(); ++it)
        {
            std::vector<int> entry;
            entry.push_back(it->id);
            entry.push_back(it->level);
            awards.push_back(entry);
        }
        world::IPlayer* player = world::playerManager->getPlayerFromProfileId(m_profileId);
        python::PyRef args = python::buildArgs(1, player, awards);
        python::pythonHost()->handleEvent(static_cast<ScriptEvent>(20), args.get());
        delete m_result;
        m_profileId = 0;
    }
    else
        m_profileId = 0;
}
//  / PE 0x5CECD0. No result deletion or profile reset on failure.
void PlayerAwards::onGetFailure(const std::string& error)
{
    HTTPRequest::onGetFailure(error);
    if (m_notifyPython)
    {
        world::IPlayer* player = world::playerManager->getPlayerFromProfileId(m_profileId);
        python::PyRef args = python::buildArgs(0, player, error);
        python::pythonHost()->handleEvent(static_cast<ScriptEvent>(20), args.get());
    }
}
void PlayerAwards::onPostComplete(const std::string& response) { HTTPRequest::onPostComplete(response); }
void PlayerAwards::onPostFailure(const std::string& error) { HTTPRequest::onPostFailure(error); }

//  (EDX:EAX return); inlined in PE 0x5CEDE0 and 0x5CEEA0.
std::uint64_t getId(const IPlayerAwards::Award& award)
{
    const std::uint64_t id = static_cast<std::uint32_t>(award.id);
    return award.type == 1
        ? id | (static_cast<std::uint64_t>(static_cast<std::uint32_t>(award.level)) << 32)
        : id;
}
void PlayerAwards::getAwardInfoEx(Award& award)
{
    award.type = award.id > 3000000 ? 2 : (award.id < 2000001 ? 1 : 0);
    const auto found = m_awards.find(getId(award));
    if (found != m_awards.end())
    {
        award.priority = found->second.priority;
        award.name = found->second.name;
        award.description = found->second.description;
    }
}
IPlayerAwards::AwardList PlayerAwards::sortList(const AwardList& source)
{
    AwardList result;
    result.timestamp = source.timestamp;
    result.profileId = source.profileId;
    result.awards = source.awards;
    std::sort(result.awards.begin(), result.awards.end(), AscendingPrioritySort());
    return result;
}
IPlayerAwards::AwardList PlayerAwards::sortListDate(const AwardList& source, bool ascending)
{
    AwardList result;
    result.timestamp = source.timestamp;
    result.profileId = source.profileId;
    result.awards = source.awards;
    if (ascending)
        std::sort(result.awards.begin(), result.awards.end(), AscendingDateSort());
    else
        std::sort(result.awards.begin(), result.awards.end(), DescendingDateSort());
    return result;
}
void PlayerAwards::getAllAwards(AwardList& result)
{
    result.awards.clear();
    for (auto it = m_awards.begin(); it != m_awards.end(); ++it)
        result.awards.push_back(it->second);
}
//  / PE 0x5CEEA0: the first integer is unused; type is derived from id.
// The when/first fields are not initialized by this retail catalogue operation.
void PlayerAwards::addAward(int, int id, int level, int priority, std::string mod)
{
    Award award;
    award.type = id > 3000000 ? 2 : (id < 2000001 ? 1 : 0);
    award.id = id;
    award.level = level;
    award.priority = priority;
    award.mod = mod;
    std::string name, description;
    if (award.type == 1)
    {
        name = formatString("AWARD_NAME_%d_%d", id, level);
        description = formatString("AWARD_DESCRIPTION_%d_%d", id, level);
    }
    else
    {
        name = formatString("AWARD_NAME_%d", id);
        description = formatString("AWARD_DESCRIPTION_%d", id);
    }
    award.name = name;
    award.description = description;
    m_awards[getId(award)] = award;
}
bool PlayerAwards::matchModAward(int id, std::string mod)
{
    for (auto it = m_awards.begin(); it != m_awards.end(); ++it)
        if (it->second.id == id && compareIgnoreCase(it->second.mod.c_str(), mod.c_str()) == 0)
            return true;
    return false;
}
} }
