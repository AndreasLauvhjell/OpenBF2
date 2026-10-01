// HUD kit-selection controls. [INFERRED] Names and source placement.
#include "HudKitSelection.h"
#if !DEDICATED
#include "BF2HudManager.h"
#include "HudInformationAdapter.h"
#include "HudQuestionManager.h"
#include "Bf2MenuMapNode.h"
#include "../Game/HudInformationLayer.h"
#include "../Game/IGameLogic.h"
#include "../Game/Main/BF2ClientSeams.h"
#include "../Game/Objects/IPlayer.h"
#include "../Scene/Object/PlayerManager.h"
#include "../Core/EventManager.h"
namespace dice { namespace hfe { namespace hud {
HudKitSelection::HudKitSelection()
    : m_pendingTeam(-1), m_teamChangePending(false), m_field005(false),
      m_selectedTeam(2), m_selectedKit{2, 2, 2}, m_selectedSpawnGroup{0, 0, 0}
{}
HudKitSelection::~HudKitSelection() {} // Real empty destruction; no virtual table.
void HudKitSelection::reset() // 7A1E10; selected team/kit and pending id are retained.
{
    m_teamChangePending = false;
    m_selectedSpawnGroup[0] = -1;
    m_selectedSpawnGroup[1] = -1;
    m_selectedSpawnGroup[2] = -1;
}
void HudKitSelection::suicide()
{
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (!player || !player->getIsAlive()) return;
    static_cast<HudInformationLayer*>(g_hudInformationLayer)->setDeathMessage(
        hudManager->getLocalizedString("HUD_TEXT_MENU_COMMITTED_SUICIDE"));
    gameLogic->suicide();
    eventManager->postEvent(ECHud, 2, nullptr, 0.0f);
}
void HudKitSelection::selectKit(int kit)
{
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (!player) return;
    if (player->getKit() != kit)
        gameLogic->selectKitAndUnlockLevel(player, kit,
            player->getChosenUnlockLevel(kit, m_selectedTeam));
    if (hudManager->getMode() == 1)
        hudManager->getInformationAdapter()->setSelectedKit(kit);
    m_selectedKit[0] = kit;
    m_selectedKit[1] = kit;
    m_selectedKit[2] = kit;
}
void HudKitSelection::showSpawnMenu(bool show) // 7A1F90, CMP byte [arg],0
{
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (!player) return;
    if (!show) hudManager->setMode(1, true);
    else if (player->getIsSquadLeader()) hudManager->setMode(17, true);
    else hudManager->setMode(18, true);
}
void HudKitSelection::cycleUnlock(int kit)
{
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    selectKit(kit); // The second local-player query is intentional, even if the first was null.
    if (!player) return;
    const int previous = player->getChosenUnlockLevel(kit, player->getTeam());
    int next = previous + 1;
    if (!player->isUnlocked(kit, player->getTeam(), next)) next = 0;
    player->setChosenUnlockLevel(kit, player->getTeam(), next);
    static_cast<HudInformationLayer*>(g_hudInformationLayer)->updateKits(m_selectedTeam);
    if (next != previous) {
        gameLogic->selectKitAndUnlockLevel(player, kit, next);
        hudManager->disableVOHelpMessage(61);
    }
}
void HudKitSelection::selectTeam(int team)
{
    if (m_teamChangePending) return;
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (!player) return;
    if (player->getTeam() != team && team != -1) {
        m_teamChangePending = true;
        m_pendingTeam = team;
        // test EAX/EAX followed by JBE is a nonzero test, including negative ids.
        if (player->getSquadId() != 0) hudManager->leaveSquad();
        hudManager->setMode(1, true);
        if (hudManager->getQuestionManager())
            hudManager->getQuestionManager()->removeQuestions(3, -1, true, 0.0f);
        gameLogic->selectTeam(player, team);
        return;
    }
    if (team == -1) {
        if (player->getTeam() == -1) team = 2;
        else team = player->getTeam(); // The repeated virtual read is shipped.
    }
    setDisplayedTeam(team);
}
void HudKitSelection::setSpawnGroup(int group)
{
    if (!world::playerManager->getLocalHumanPlayer()) return;
    m_selectedSpawnGroup[m_selectedTeam] = group;
    hudManager->getMap()->m_spawnGroup = group;
}
void HudKitSelection::setDisplayedTeam(int team)
{
    m_selectedTeam = team;
    const int group = m_selectedSpawnGroup[team];
    setSpawnGroup(group);
    if (hudManager->getInformationAdapter()->getBool("SpawnShow")) {
        hudManager->getInformationAdapter()->setSelectedTeam(team);
        hudManager->getInformationAdapter()->setTeam(team);
        static_cast<HudInformationLayer*>(g_hudInformationLayer)->updateKits(team);
    }
}
} } }
#endif
