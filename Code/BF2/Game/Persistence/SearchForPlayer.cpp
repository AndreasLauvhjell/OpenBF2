// Path: attested ELF assertion, SearchForPlayer.cpp:74.
// Contents: ELF 0x82F8170..0x82F8DB0 / PE 0x5D6A10..0x5D6E90.
#include "SearchForPlayer.h"
#include "../../Core/Ids.h"
#include "../../Core/StringUtil.h"
#include <cassert>
#include <cstdlib>
namespace dice { namespace hfe {
SearchForPlayer::SearchForPlayer() : m_refCount(1) {}
SearchForPlayer::~SearchForPlayer() = default;
IBase* SearchForPlayer::create(unsigned int iid, IBase* outer)
{
    if (outer) return 0;
    SearchForPlayer* object = new SearchForPlayer();
    IBase* result = static_cast<IBase*>(object->queryInterface(iid));
    if (!result) object->release();
    return result;
}
void SearchForPlayer::addRef() { ++m_refCount; }
int SearchForPlayer::getRef() { return m_refCount; }
int SearchForPlayer::release()
{
    const int remaining = --m_refCount;
    if (!remaining) delete this;
    return remaining;
}
void* SearchForPlayer::queryInterface(unsigned int iid) const
{
    return iid == IID_IBase || iid == IID_ISearchForPlayer ? const_cast<SearchForPlayer*>(this) : 0;
}
//  / PE 0x5D6A70: this deliberately retains punctuation from ':' to '@',
// excludes '[' through '`', uses signed chars, and does not pad hexadecimal values.
std::string encodeString(const std::string& text)
{
    std::string result;
    for (unsigned int i = 0; i < text.size(); ++i)
    {
        const signed char c = text[i];
        if (c > 47 && (c <= 90 || c > 96) && c <= 122) result += static_cast<char>(c);
        else
        {
            result += "%";
            result += formatString("%X", static_cast<int>(c));
        }
    }
    return result;
}
bool SearchForPlayer::search(const std::string& nick, bool exact, bool asynchronous)
{
    if (isUpdating()) cancel();
    m_parser.clear();
    std::string url = getGameSpyUrl() + "searchforplayers.aspx?nick=";
    url += encodeString(nick);
    url += exact ? "&where=x" : "&where=a";
    url += "&sort=a";
    if (!get(url, asynchronous))
    {
#ifndef _WIN32
        assert(false); // [PLATFORM] ELF asserts; PE returns false.
#endif
        return false;
    }
    return true;
}
void SearchForPlayer::tick() { if (isUpdating()) update(); }
bool SearchForPlayer::getPlayers(std::vector<Player>& players)
{
    for (unsigned int i = 0; i < m_parser.getNumKeyMaps(); ++i)
    {
        KeyMap* map = m_parser.getKeyMap(i);
        Player player;
        player.profileId = std::atoi(map->findKey("pid")->value.c_str());
        player.score = std::atoi(map->findKey("score")->value.c_str());
        player.nick = map->findKey("nick")->value;
        players.push_back(player); // Retail appends without clearing and requires all three keys.
    }
    return true;
}
void SearchForPlayer::onGetComplete(const std::string& response)
{
    HTTPRequest::onGetComplete(response);
    m_parser.parse(response, KeyParser::Stats, 0); // Retail uses mode 0, not mode 3.
}
void SearchForPlayer::onGetFailure(const std::string& error) { HTTPRequest::onGetFailure(error); }
void SearchForPlayer::onPostComplete(const std::string& response) { HTTPRequest::onPostComplete(response); }
void SearchForPlayer::onPostFailure(const std::string& error) { HTTPRequest::onPostFailure(error); }
} }
