// Path: [INFERRED]. ELF 0x82C0670..0x82C0D00 / PE 0x5D7570..0x5D7830.
#include "LeaderBoard.h"
#include "../../Core/Ids.h"
#include "../../Core/StringUtil.h"
namespace dice { namespace hfe {
LeaderBoard::LeaderBoard() : m_refCount(1), m_updated(0), m_numPlayers(0) {}
LeaderBoard::~LeaderBoard() = default;
IBase* LeaderBoard::create(unsigned int iid, IBase* outer)
{
    if (outer) return 0;
    LeaderBoard* object = new LeaderBoard();
    IBase* result = static_cast<IBase*>(object->queryInterface(iid));
    if (!result) object->release();
    return result;
}
void LeaderBoard::addRef() { ++m_refCount; }
int LeaderBoard::getRef() { return m_refCount; }
int LeaderBoard::release()
{
    const int remaining = --m_refCount;
    if (!remaining) delete this;
    return remaining;
}
void* LeaderBoard::queryInterface(unsigned int iid) const
{
    return iid == IID_IBase || iid == IID_ILeaderBoard ? const_cast<LeaderBoard*>(this) : 0;
}
bool LeaderBoard::getLeaderBoard(const std::string& query, bool asynchronous)
{
    if (isUpdating()) cancel();
    m_parser.clear();
    m_updated = 0;
    m_numPlayers = 0;
    std::string url = getGameSpyUrl() + "getleaderboard.aspx?";
    url += query;
    return get(url, asynchronous);
}
bool LeaderBoard::getLeaderBoardPlayer(int profileId, const std::string& query, bool asynchronous)
{
    if (isUpdating()) cancel();
    m_parser.clear();
    m_updated = 0;
    m_numPlayers = 0;
    std::string url = getGameSpyUrl() + "getleaderboard.aspx?";
    url += formatString("pid=%d&", profileId);
    url += query;
    return get(url, asynchronous);
}
int LeaderBoard::getUpdated() { return m_updated; }
int LeaderBoard::getNumPlayers() { return m_numPlayers; }
int LeaderBoard::getNumKeyMaps() { return m_parser.getNumKeyMaps(); }
KeyMap* LeaderBoard::getKeyMap(unsigned int index) { return m_parser.getKeyMap(index); }
void LeaderBoard::onGetComplete(const std::string& response)
{
    HTTPRequest::onGetComplete(response);
    int metadata[2] = {};
    m_parser.parse(response, KeyParser::Type2, metadata);
    m_numPlayers = metadata[0];
    m_updated = metadata[1];
}
void LeaderBoard::onGetFailure(const std::string& error) { HTTPRequest::onGetFailure(error); }
void LeaderBoard::onPostComplete(const std::string& response) { HTTPRequest::onPostComplete(response); }
void LeaderBoard::onPostFailure(const std::string& error) { HTTPRequest::onPostFailure(error); }
} }
