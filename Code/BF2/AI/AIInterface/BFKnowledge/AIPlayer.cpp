// Engine implementation of the AI player interface.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#include "AIPlayer.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../../Core/Ids.h"                        // CID_LadderContainerTemplate, CID_Parachute*
#include "../../../Game/GameLogic/Player.h"           // world::IPlayer - every forward's target
#include "../../../Game/Objects/IObject.h"            // getTemplate
#include "../../../Game/Objects/IObjectTemplate.h"    // getClassID
#include "../../../Math/Mat4.h"                       // the lazily built identity matrix
#include "../../../Scene/Object/PlayerManager.h"      // the `playerManager` global
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "AIPlayerFactory.h"                          // the id-to-player mapping pair

namespace dice {
namespace hfe {
namespace ai {

// Zero-initialised, filled on the first construction, never freed.
Mat4* AIPlayer::identityMatrix = 0;

// The interface destructor has no additional cleanup.
IAIPlayer::~IAIPlayer()
{
}

// Wraps the supplied player and initializes view distance from its current value.
// The player is borrowed and must outlive this wrapper.
AIPlayer::AIPlayer(world::IPlayer* player)
    : m_player(player)
    , m_i8(0)
    , m_viewDistance(0.0f)
{
    m_viewDistance = m_player->getViewDistance();

    if (!identityMatrix)
    {
        Mat4* const matrix = checkPointer(new Mat4);

        // Preserves the repeated identity-matrix stores, including diagonal indices 0, 5, 10, and 15.
        matrix->m[0]  = 1.0f;  matrix->m[1]  = 0.0f;  matrix->m[2]  = 0.0f;  matrix->m[3]  = 0.0f;
        matrix->m[4]  = 0.0f;  matrix->m[5]  = 1.0f;  matrix->m[6]  = 0.0f;  matrix->m[7]  = 0.0f;
        matrix->m[8]  = 0.0f;  matrix->m[9]  = 0.0f;  matrix->m[10] = 1.0f;  matrix->m[11] = 0.0f;
        matrix->m[12] = 0.0f;  matrix->m[13] = 0.0f;  matrix->m[14] = 0.0f;  matrix->m[15] = 1.0f;

        identityMatrix = matrix;
    }
}

// - eight bytes longer than the `(IPlayer*)` form and not read.
// The extra length is presumably the null player it starts from; nothing in this tree calls it.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
AIPlayer::AIPlayer()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::AIPlayer()");
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Destruction remains unimplemented; AIPlayerFactory::destroy reaches this boundary.
AIPlayer::~AIPlayer()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::~AIPlayer");
}

// Delegates to the wrapped engine player through the typed interface.

// - and it resets the one field that is genuinely this object's, not the
// player's. A recycled `AIPlayer` therefore keeps its view distance; only +8 is cleared.
void AIPlayer::reset()
{
    m_i8 = 0;
}

world::IPlayer* AIPlayer::getPlayer() const
{
    return m_player;
}

// it resolves the ID and then calls its own virtual slot +0x14 - a virtual
// self-call, not a direct one, so an override of the pointer form would catch this path too.
void AIPlayer::setPlayer(int playerId)
{
    setPlayer(world::playerManager->getPlayerFromId(playerId));
}

// Replaces the factory id mapping when the player changes. Seeds view distance only
// when its current value equals zero; NaN is left unchanged.
void AIPlayer::setPlayer(world::IPlayer* player)
{
    IAIPlayerFactory* const factory =
        static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory);

    if (m_player)
        factory->removeIdToPlayerMapping(this);

    m_player = player;

    if (m_player)
        factory->setIdToPlayerMapping(this);

    if (m_player && m_viewDistance == 0.0f)
        m_viewDistance = m_player->getViewDistance();
}

// - IPlayer +0x24.
const std::string& AIPlayer::getName() const
{
    return m_player->getName();
}

// - IPlayer +0x54.
int AIPlayer::getId() const
{
    return m_player->getId();
}

// - IPlayer +0x50. A different number from `getId`, and the two slots are
// adjacent on both classes; do not collapse them.
int AIPlayer::getInputId() const
{
    return m_player->getInputId();
}

// - IPlayer +0x6C, narrowed with `movzx`.
bool AIPlayer::getIsAlive() const
{
    return m_player->getIsAlive();
}

// - IPlayer +0x68.
void AIPlayer::setIsAlive(bool alive)
{
    m_player->setIsAlive(alive);
}

// - IPlayer +0x100.
bool AIPlayer::isManDown() const
{
    return m_player->getIsManDown();
}

// Checks the current vehicle template to identify ladder and parachute states.
bool AIPlayer::isClimbingLadder() const
{
    world::IObject* const vehicle = m_player->getVehicle();

    if (!vehicle)
        return false;

    return vehicle->getTemplate()->getClassID() == world::CID_LadderContainerTemplate;
}

bool AIPlayer::isParachuting() const
{
    world::IObject* const vehicle = m_player->getVehicle();

    if (!vehicle)
        return false;

    return vehicle->getTemplate()->getClassID() == world::CID_ParachuteTemplate;
}

// - IPlayer +0x154.
void AIPlayer::setSpawnGroup(int group)
{
    m_player->setSpawnGroup(group);
}

// forward the selected kit unchanged to
// IPlayer::setKit (ELF +0x16C / PE +0x168), with no player-null guard or validation.
void AIPlayer::setSpawnKit(int kit)
{
    m_player->setKit(kit);
}

// - IPlayer +0x90, `getFov`. this is what corrected the declared return type:
// the body materialises nothing in eax and leaves the player's float in st0.
float AIPlayer::getFieldOfView()
{
    return m_player->getFov();
}

// - IPlayer +0x9C, `getAIPlayerInput`. the player has A separate input block
// for AI, and this is the slot that hands it over; the name on the player side says so outright.
const void* AIPlayer::getPlayerInput() const
{
    return m_player->getAIPlayerInput();
}

// [FRONTIER] Unimplemented player queries and actions stop by name.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding AIPlayer operation.

int AIPlayer::getSpawnGroup() const
{
    // IPlayer virtual +0x158/+0x154, without a null guard or value conversion.
    return m_player->getSpawnGroup();
}

int AIPlayer::getSpawnKit() const
{
    // IPlayer virtual +0x170/+0x16C, with no null guard or value conversion.
    return m_player->getKit();
}

unsigned int AIPlayer::getPlayerControlObjectId()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::getPlayerControlObjectId");
}

IAIObject* AIPlayer::getSoldier()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::getSoldier");
}

IAIObject* AIPlayer::getDefaultVehicle()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::getDefaultVehicle");
}

IAIObject* AIPlayer::getCurrentVehicle()
{
    // One vehicle lookup; no template/occupant guards.
    // Ladder, parachute, zipline and rope wrappers expose their occupying object's AI component.
    world::IObject* vehicle = m_player->getVehicle();
    if (!vehicle)
        return 0;

    const unsigned int classId = vehicle->getTemplate()->getClassID();
    if (classId == world::CID_LadderContainerTemplate ||
        classId == world::CID_ParachuteTemplate ||
        classId == world::CID_ZiplineContainerTemplate ||
        classId == world::CID_GrapplingHookRopeContainerTemplate)
    {
        world::IPlayerControlObject* control = static_cast<world::IPlayerControlObject*>(
            vehicle->queryInterface(world::IID_IPlayerControlObject));
        return static_cast<IAIObject*>(control->getOccupingObject()->getAIComponent());
    }
    return static_cast<IAIObject*>(vehicle->getAIComponent());
}

float AIPlayer::getFieldOfViewModifier()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::getFieldOfViewModifier");
}

float AIPlayer::getStandardFieldOfView()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::getStandardFieldOfView"
                    " - eleven bytes, one float constant at ds:0x889AA58, not yet read");
}

float AIPlayer::getViewDistance()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::getViewDistance");
}

void AIPlayer::setViewDistance(float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::setViewDistance"
                    " - fourteen bytes; AISettings::setViewDistance reaches it once per live bot");
}

int AIPlayer::getSide()
{
    // unchecked IPlayer::getTeam forward.
    return m_player->getTeam();
}

// forward the side and false to
// IPlayer::setTeam (ELF +0xE4 / PE +0xE0), with no player-null guard.
void AIPlayer::setSide(int side)
{
    m_player->setTeam(side, false);
}

int AIPlayer::getNumShotsFired(IWeapon*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::getNumShotsFired"
                    " - seven bytes, so it returns a constant and ignores the weapon");
}

void AIPlayer::removePlayerFromGame()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::removePlayerFromGame"
                    " - read except for the singleton at ds:0x8AC37F8");
}

void AIPlayer::setName(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::setName");
}

int AIPlayer::getScore()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::getScore");
}

void AIPlayer::setIsAIPlayer(bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::setIsAIPlayer");
}

// forward the player's
// Boolean query through IPlayer (+0x64 / +0x60), with no null guard.
bool AIPlayer::getIsAIPlayer()
{
    return m_player->getIsAIPlayer();
}

const void* AIPlayer::getCameraTransformation() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayer::getCameraTransformation"
                    " - read in outline; the camera object's transform slot is not confirmed");
}

} // namespace ai
} // namespace hfe
} // namespace dice
