// [INFERRED] split in the attested BF2HudManager.cpp family. Full client PE
// 7B1ED0/7B1F50 and console signatures read.
#include "BF2HudManager.h"
#if !DEDICATED
#include "Bf2MenuMapNode.h"
#include "../Game/Common/ClientVoteManager.h"
#include "../Game/GameLogic/SquadManager/ISquadManager.h"
#include "../Game/Objects/ICameraObject.h"
#include "../Game/Objects/IPlayer.h"
#include "../Scene/Object/PlayerManager.h"
namespace dice { namespace hfe {
extern IBase* squadManager;
namespace hud {
void BF2HudManager::commanderResign()
{
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (!player) return;
    if (hudMap) hudMap->setZoom(0, true);               // +D8 777980
    const int team = player->getTeam();
    static_cast<ISquadManager*>(squadManager)->resignCommander(team);
    player->disableVOHelpMessage(23);
    world::ICameraObject* camera = player->getCamera();
    if (camera) camera->setViewMode(camera->getViewMode(), nullptr, true);
}
void BF2HudManager::commanderMutiny()
{
    ClientVoteManager::getInstance()->sendMutinyIssue();
}
} } }
#endif
