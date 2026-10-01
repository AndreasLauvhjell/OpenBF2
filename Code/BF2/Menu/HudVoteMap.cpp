// HUD map-vote controls. [INFERRED] Names and source placement.
#include "HudVoteMap.h"
#if !DEDICATED
#include "BfListBoxData.h"
#include "../Core/EventManager.h"
#include "../Game/HudInformationLayer.h"
#include "../Game/Main/BF2ClientSeams.h"
#include "../Scene/Object/PlayerManager.h"
#include "BF2HudManager.h"                      // getLocalizedString (+0x38)
#include "../External/BFMeme/System.h"           // loadTexture (+0x8C)
#include "../Core/StringUtil.h"                  // formatString (4310D0)
#include "../Game/MapList.h"                     // instance / getMapCount / getEntry
#include "../Game/Common/ClientVoteManager.h"    // the active vote's type and target
#include <new>
namespace dice { namespace hfe { namespace hud {
HudVoteMap::HudVoteMap(meme::System* system)
    : m_system(system), m_field00c(0), m_rowCount(0), m_visible(false),
      m_visibleRows(15), m_selected(false), m_rowOffset(2), m_field034("")
{
    eventManager->registerEventHandler(ECHud, this, 0);
    m_field028 = 12;
    m_columnWidth = 160;
}
HudVoteMap::~HudVoteMap()
{
    eventManager->unregisterEventHandler(ECHud, this);
    // 785220 only destroys the string; the list-data pointer is not released.
}
void HudVoteMap::setSystem(meme::System* system) { m_system = system; }
bool HudVoteMap::registerData()
{
    void* allocation = ::operator new(sizeof(meme::BfListBoxData));
    m_data = allocation ? new(allocation) meme::BfListBoxData("LevelsList") : nullptr;
    if (!m_data) return false;
    m_data->addColumn(m_columnWidth, true, false);
    m_data->addColumn(m_columnWidth, true, false);
    m_data->addHiddenColumn();
    m_data->m_visibleRows = m_visibleRows;
    m_data->m_field021 = true;
    m_data->m_field023 = false;
    m_data->m_field022 = false;
    static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerBool("VoteMapSelected", &m_selected);
    return true;
}
bool HudVoteMap::registerFunctions() { return true; } // Retail5A5E20, MOV AL,1; RET.
void HudVoteMap::handleEvent(EventCategory category, unsigned int, EventNode*, void*)
{
    if (category == ECHud) world::playerManager->getLocalHumanPlayer();
}
namespace {
// The same filler call refresh() makes seven times: the empty string at ds:87FB54, opaque white,
// value 0. It is deliberately not null-guarded - see refresh's failure path.
void appendEmpty(meme::BfListBoxData* list, int column)
{
    list->appendCell(column, std::string(), 1.0f, 1.0f, 1.0f, 1.0f, 0);
}
}

// Refreshes the map-vote list.
void HudVoteMap::refresh()
{
    if (!world::playerManager->getLocalHumanPlayer()) return;
    if (!m_data)
    {
        // Retail calls straight through the null pointer here. See the block comment.
        for (int column = 0; column <= 3; ++column) appendEmpty(m_data, column);
        return;
    }
    m_data->clearRows();
    const int count = MapList::instance()->getMapCount();
    for (int index = 0; index < count; ++index)
    {
        const MapList::Entry* const entry = MapList::instance()->getEntry(index);
        std::string ballot;      // column 1's texture path
        std::string tick;        // column 2's, empty unless this row is the vote's target
        if (ClientVoteManager::getInstance()->m_messagesEnabled)
        {
            if (ClientVoteManager::getInstance()->m_dword04)
            {
                if (ClientVoteManager::getInstance()->m_dword04 == 1)
                {
                    ballot = "Ingame/Scoreboard/Icons/kickbutton_down.tga";
                    if (index == ClientVoteManager::getInstance()->m_dword08)
                        tick = "Ingame/Scoreboard/Icons/vote_square.tga";
                    else
                        ballot = "Ingame/Scoreboard/Icons/kickvote_frame.tga";  // overwrites above
                }
                else
                {
                    ballot = "";
                }
            }
            else
            {
                ballot = "Ingame/Scoreboard/Icons/kickbutton_up.tga";
            }
        }

        if (!tick.empty())
            m_data->appendImageCell(2, m_system->loadTexture(tick.c_str(), true),
                19.0f, 19.0f, -2.0f, m_rowOffset - 2.0f, 0.4745000004768372f, 0.0f, 0.0f, 1.0f);
        else
            appendEmpty(m_data, 2);

        m_data->appendImageCell(1, m_system->loadTexture(ballot.c_str(), true),
            15.0f, 15.0f, 0.0f, m_rowOffset, 1.0f, 1.0f, 1.0f, 1.0f);

        m_data->appendPositionedWideCell0x7c8470(0,
            hudManager->getLocalizedString(formatString("HUD_LEVELNAME_%s", entry->name.c_str())),
            4.0f, m_rowOffset - 2.0f,
            0.4499999880790710f, 0.4269999861717224f, 0.4269999861717224f, 1.0f, 0);

        m_data->appendCell(3, entry->name, 1.0f, 1.0f, 1.0f, 1.0f, 0);
        ++m_rowCount;
    }
}

// Shows or hides the selected map vote panel. Without a selected map, visible state is unchanged.
void HudVoteMap::show(bool visible)
{
    if (!m_selected) return;
    if (visible && world::playerManager->getLocalHumanPlayer()) refresh();
    m_visible = visible;
}

std::string HudVoteMap::getSelectedMap()
{
    const std::string selected = m_data->getSelectedString(3);
    return std::string(selected.c_str());
}

// Shows or hides the map-vote menu.
void HudVoteMap::setVoteMapShow(int show)
{
    if (show) {
        if (ClientVoteManager::getInstance()->m_dword04 != 0
                && ClientVoteManager::getInstance()->m_dword04 != 1)
            return;
        if (!ClientVoteManager::getInstance()->participatesInVoting())
            return;
        if (!ClientVoteManager::getInstance()->m_messagesEnabled)               // +00
            return;
    }
    m_selected = show != 0;                                                     // +24, setne
}

// Handles a click on a map-vote row.
void HudVoteMap::singleClick(int)
{
    if (!m_selected)                                                            // +24
        return;
    if (!world::playerManager->getLocalHumanPlayer())                           // PE +0x30
        return;
    const std::string map = getSelectedMap();
    if (map == "")                                                              // ds:87FB54
        return;
    const int column = m_data->m_column;                                        // +50 -> +0x0C
    if (column <= 0 || column > 2)
        return;
    const int index = mapList->getIndexFromName(map);                           // ds:9A4B10, 448230
    if (index < 0)
        return;
    ClientVoteManager::getInstance()->sendMapIssue(index);
}
} } }
#endif
