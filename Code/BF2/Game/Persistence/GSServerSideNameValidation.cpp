// Path: [INFERRED] Game/Persistence/GSServerSideNameValidation.cpp.
// Contents: ELF 0x82BCD30..0x82BDEB0 / PE 0x55F880..0x55FEA0,
#include "GSServerSideNameValidation.h"
#include "../../Core/Ids.h"
#include "../../Core/System.h"
#include "../../Scene/Object/PlayerManager.h"
#include "../Objects/IPlayer.h"
#include "../Python/PythonHost.h"
#include "../Python/PyRef.h"
#include <cstdlib>
#include <sstream>
namespace dice { namespace hfe {
int GSServerSideNameValidation::outstandingRequests = 0;
GSServerSideNameValidation::GSServerSideNameValidation() : m_refCount(1) {}
GSServerSideNameValidation::~GSServerSideNameValidation() = default;
void GSServerSideNameValidation::addRef() { ++m_refCount; }
int GSServerSideNameValidation::getRef() { return m_refCount; }
int GSServerSideNameValidation::release()
{
    const int remaining = --m_refCount;
    if (!remaining) delete this;
    return remaining;
}
void* GSServerSideNameValidation::queryInterface(unsigned int iid) const
{
    return iid == IID_IBase || iid == IID_IGSServerSideNameValidation
        ? const_cast<GSServerSideNameValidation*>(this) : 0;
}
bool GSServerSideNameValidation::validateSoldierName(const std::string& nick,
    const std::string& clanTag, int profileId, const std::string& auth,
    int connectionId, GameServer* server)
{
    if (nick.empty() || !profileId || auth.empty() || connectionId < 0 || !server
        || outstandingRequests >= 200)
    {
        cancel();
        return false;
    }
    if (isUpdating()) cancel();
    m_parser.clear();
    m_connectionId = connectionId;
    m_playerId = profileId;
    m_gsAuth = auth;
    m_soldierNick = nick;
    m_clanTag = clanTag;
    m_scratch = "";
    std::stringstream url;
    url << getGameSpyUrl() << "VerifyPlayer.aspx" << "?auth=" << auth
        << "&SoldierNick=" << nick << "&pid=" << profileId;
    m_requestTime = g_system->getWallClockTime();
    if (!get(url.str(), true)) return false;
    ++outstandingRequests;
    return true;
}
int GSServerSideNameValidation::getNumKeyMaps() { return m_parser.getNumKeyMaps(); }
int GSServerSideNameValidation::getNumKeys()
{
    if (!getNumKeyMaps()) return 0;
    return getKeyMap(0)->getNumKeys();
}
KeyMap* GSServerSideNameValidation::getKeyMap(unsigned int index) { return m_parser.getKeyMap(index); }
bool GSServerSideNameValidation::getKey(unsigned int mapIndex, int keyIndex,
    std::string& key, std::string& value)
{
    KeyMap* map = getKeyMap(mapIndex);
    if (!map) return false;
    KeyMap::Key* entry = map->getKey(static_cast<unsigned int>(keyIndex));
    if (!entry) return false;
    key = entry->key;
    value = entry->value;
    return true;
}
std::string GSServerSideNameValidation::getKeyValue(unsigned int index, const std::string& name)
{
    KeyMap* map = getKeyMap(index);
    if (map && map->findKey(name)) return map->findKey(name)->value;
    return "";
}
// The request counter is not repaired on cancel/destruction. Completion decrements
// unconditionally; only failure clamps at zero. These differences are shipped.
void GSServerSideNameValidation::onGetComplete(const std::string& response)
{
    HTTPRequest::onGetComplete(response);
    --outstandingRequests;
    std::string metadata[4]; // profile, verified nick, result number, result string
    if (!m_parser.parse(response, KeyParser::Validation, metadata)) return;
    world::IPlayer* player = world::playerManager->getPlayerFromProfileId(m_playerId);
    if (!player) player = world::playerManager->getPlayerFromProfileId(std::atoi(metadata[0].c_str()));
    if (!player) return;
    if (metadata[3] != "Ok" && metadata[3] != "InvalidAuthProfileID")
    {
        if (metadata[3] == "InvalidReportedNick") player->setName(metadata[1]);
        else if (metadata[3] == "InvalidReportedProfileID")
        {
            player->setProfileId(std::atoi(metadata[0].c_str()));
            player->setName(metadata[1]);
        }
        else return;
    }
    python::PyRef args = python::buildArgs(metadata[1], m_soldierNick,
        std::atoi(metadata[0].c_str()), std::atoi(metadata[2].c_str()), player);
    python::pythonHost()->handleEvent(static_cast<ScriptEvent>(40), args.get());
}
void GSServerSideNameValidation::onGetFailure(const std::string& error)
{
    HTTPRequest::onGetFailure(error);
    --outstandingRequests;
    if (outstandingRequests < 0) outstandingRequests = 0;
}
} }
