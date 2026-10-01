// Path: [INFERRED]. ELF 0x82F5AF0..0x82F6B00 / PE 0x5D7890..0x5D8420.
#include "PlayerUnlockInfo.h"
#include "../../Core/Ids.h"
#include <cstdlib>
namespace dice { namespace hfe {
PlayerUnlockInfo::PlayerUnlockInfo() : m_refCount(1) {}
PlayerUnlockInfo::~PlayerUnlockInfo() = default;
IBase* PlayerUnlockInfo::create(unsigned int iid, IBase* outer)
{
    if (outer) return 0;
    PlayerUnlockInfo* object = new PlayerUnlockInfo();
    IBase* result = static_cast<IBase*>(object->queryInterface(iid));
    if (!result) object->release();
    return result;
}
void PlayerUnlockInfo::addRef() { ++m_refCount; }
int PlayerUnlockInfo::getRef() { return m_refCount; }
int PlayerUnlockInfo::release()
{
    const int remaining = --m_refCount;
    if (!remaining) delete this;
    return remaining;
}
void* PlayerUnlockInfo::queryInterface(unsigned int iid) const
{
    return iid == IID_IBase || iid == IID_IPlayerUnlockInfo ? const_cast<PlayerUnlockInfo*>(this) : 0;
}
bool PlayerUnlockInfo::init()
{
    if (isUpdating()) cancel();
#if defined(_WIN32)
    // [PLATFORM] PE destroys/deallocates the vector; ELF erases its elements.
    std::vector<Item>().swap(m_items);
#else
    m_items.erase(m_items.begin(), m_items.end());
#endif
    m_parser.clear();
    std::string url = getGameSpyUrl() + "getbackendinfo.aspx";
    return get(url, false);
}
bool PlayerUnlockInfo::getItemsByKit(unsigned int kit, std::vector<Item>& items)
{
    for (unsigned int i = 0; i < m_items.size(); ++i)
        if (m_items[i].kit == kit) items.push_back(m_items[i]);
    return true;
}
bool PlayerUnlockInfo::getItemById(unsigned int id, Item& item)
{
    for (unsigned int i = 0; i < m_items.size(); ++i)
        if (m_items[i].id == id) { item = m_items[i]; return true; }
    return false;
}
bool PlayerUnlockInfo::getItems(std::vector<Item>& items)
{
    for (unsigned int i = 0; i < m_items.size(); ++i) items.push_back(m_items[i]);
    return true;
}
float PlayerUnlockInfo::getVersion() { return m_version; }
void PlayerUnlockInfo::setVersion(float value) { m_version = value; }
bool PlayerUnlockInfo::getRanked() { return m_ranked; }
void PlayerUnlockInfo::setRanked(bool value) { m_ranked = value; }
void PlayerUnlockInfo::onGetComplete(const std::string& response)
{
    HTTPRequest::onGetComplete(response);
    const std::string text(response);
    int metadata[2] = {};
    m_parser.parse(text, KeyParser::Search, metadata);
    setVersion(static_cast<float>(metadata[0]));
    setRanked(metadata[1] != 0);
    if (m_parser.getNumKeyMaps())
    {
        m_parser.getKeyMap(0); // repeated read present in both ports
        for (unsigned int i = 0; i < m_parser.getNumKeyMaps(); ++i)
        {
            KeyMap* map = m_parser.getKeyMap(i);
            Item item;
            for (unsigned int j = 0; j < map->getNumKeys(); ++j)
            {
                switch (j)
                {
                case 0: item.id = std::atoi(map->getKey(0)->value.c_str()); break;
                case 1: item.kit = std::atoi(map->getKey(1)->value.c_str()); break;
                case 2: item.name = map->getKey(2)->value; break;
                case 3: item.description = map->getKey(3)->value; break;
                }
            }
            m_items.push_back(item);
        }
    }
}
void PlayerUnlockInfo::onGetFailure(const std::string& error) { HTTPRequest::onGetFailure(error); }
void PlayerUnlockInfo::onPostComplete(const std::string& response) { HTTPRequest::onPostComplete(response); }
void PlayerUnlockInfo::onPostFailure(const std::string& error) { HTTPRequest::onPostFailure(error); }
} }
