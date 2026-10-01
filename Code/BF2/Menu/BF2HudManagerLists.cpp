// HUD list updates. [INFERRED] Source split from Menu/BF2HudManager.cpp.
#include "BF2HudManager.h"
#if !DEDICATED
#include "HudVoip.h"
#include "HudScoreboard.h"
#include "HudSquad.h"
#include "HudCommander.h"
#include "HudChat.h"
#include "HudVoteMap.h"
namespace dice { namespace hfe { namespace hud {
meme::BfListBoxData* BF2HudManager::getListData(int selector)
{
    switch (selector) {
    case 1: return m_field120 ? m_field120->getScoreList(1) : nullptr;
    case 2: return m_field120 ? m_field120->getScoreList(2) : nullptr;
    case 3: return hudChat ? hudChat->getData() : nullptr;
    case 4: return m_field120 ? m_field120->m_finalList : nullptr;
    case 5: return m_field138 ? m_field138->m_memberList : nullptr;
    case 6: return m_field13c ? m_field13c->m_data : nullptr;
    case 7: return m_field140 ? m_field140->getData() : nullptr;
    case 8: return m_field138 ? m_field138->m_inviteList : nullptr;
    case 9: return m_field120 ? m_field120->getTotalList(1) : nullptr;
    case 10: return m_field120 ? m_field120->getTotalList(2) : nullptr;
    case 11:
        // The voice-list data belongs to the HUD-side owner when present.
        return m_field124 ? m_field124->m_data : nullptr;
    case 12: return m_field128 ? m_field128->m_data : nullptr;
    default: return nullptr;
    }
}
} } }
#endif
