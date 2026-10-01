// Engine object queries and information-grid updates used by the AI.
// [RETIRE-WHEN: aidll-01-adapters] Remove when the corresponding implementation is complete and validated.

#include "BFObjectEnvironment.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../SingletonServer.h"   // BFSingletonServer::updateStaticMethods
#include <cstring>
#include "../../../Scene/Object/PlayerManager.h"   // isValidPlayerId's one call
#include "../../../Core/Ids.h"                     // the four container CIDs, IID_IPlayerControlObject
#include "../../../Game/Objects/IObject.h"         // getTemplate, getAIComponent
#include "../../../Game/Objects/IObjectTemplate.h" // getClassID
#include "../../../Game/Objects/IPlayer.h"         // the per-player tests and getId
#include "../../../Game/Objects/Game/PlayerControlObject.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../../../Game/IGameLogic.h"                                  // gameLogic: getPcoMap, giveDamage
#include "../../../Game/Objects/GameLogicComp/IArmorGLComp.h"          // getHitPoints / getMaxHitPoints
#include "../../../Game/Objects/GameLogicComp/ArmorGLCompTemplate.h"   // getCanBeDestroyed
#include "BFTimer.h"                                                   // IBFTimer::getGameTime

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IAIObjectEnvironment::IAIObjectEnvironment()
{
    if (!CommonSingletonServer::m_iaiObjectEnvironment)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->registerSimpleSingleton(std::string("IAIObjectEnvironment"), this);
        BFSingletonServer::updateStaticMethods();
    }
}

IAIObjectEnvironment::~IAIObjectEnvironment()
{
    if (CommonSingletonServer::m_iaiObjectEnvironment == this)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->unregisterSimpleSingleton(std::string("IAIObjectEnvironment"));
        BFSingletonServer::updateStaticMethods();
    }
}

// Zeroes the opaque vector storage, flag, and severe-damage timestamp separately.
// Leaves padding at +0x29..+0x2B untouched.
BFObjectEnvironment::BFObjectEnvironment()
{
    std::memset(m_opaque, 0, sizeof(m_opaque));  // +0x04 .. +0x27, the three vectors
    m_byte28 = 0;                                // +0x28, a byte
    m_lastSevereDamageTime = 0.0;                // +0x2C, a double
}

BFObjectEnvironment::~BFObjectEnvironment()
{
}

void BFObjectEnvironment::getWorldFrustumObjects(std::vector<IAIObject*>& out, const Frustum& frustum, AIObjectPredicate* filter) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getWorldFrustumObjects");
}

void BFObjectEnvironment::getWorldSphereObjects(std::vector<IAIObject*>& out, const Vec3& centre, float radius, AIObjectPredicate* filter) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getWorldSphereObjects");
}

void BFObjectEnvironment::getAllWorldObjects(std::vector<IAIObject*>& out, AIObjectPredicate* filter) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAllWorldObjects");
}

void BFObjectEnvironment::getAllDestroyableObjects(std::vector<DestroyableObject>& out) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAllDestroyableObjects");
}

void BFObjectEnvironment::getWorldPlayerIds(std::vector<int>& out) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getWorldPlayerIds");
}

void BFObjectEnvironment::getWorldPlayerOccupiedObjects(std::vector<IAIObject*>& out, AIObjectPredicate* filter) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getWorldPlayerOccupiedObjects");
}

// Collects AI objects matching the supplied predicate within the requested region.
void BFObjectEnvironment::getWorldPlayerOccupiedObjectsWithIds(std::vector<std::pair<int, IAIObject*> >& out, AIObjectPredicate* filter) const
{
    const std::list<world::IPlayer*>& players = world::playerManager->getPlayers();

    for (std::list<world::IPlayer*>::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        world::IPlayer* player = *it;

        if (!player || !player->getIsAlive() || player->getIsManDown() || !player->getVehicle())
            continue;

        IAIObject* aiObject = static_cast<IAIObject*>(player->getVehicle()->getAIComponent());

        if (!aiObject)
        {
            if ((*it)->getVehicle()->getTemplate()->getClassID() == world::CID_LadderContainerTemplate)
            {
                world::IObject* occupying =
                    static_cast<world::PlayerControlObject*>(player->getVehicle())->getOccupingObject();
                if (occupying)
                    aiObject = static_cast<IAIObject*>(occupying->getAIComponent());
            }
            else if ((*it)->getVehicle()->getTemplate()->getClassID() == world::CID_ParachuteTemplate)
            {
                world::IPlayerControlObject* control = static_cast<world::IPlayerControlObject*>(
                    player->getVehicle()->queryInterface(world::IID_IPlayerControlObject));
                aiObject = static_cast<IAIObject*>(control->getOccupingObject()->getAIComponent());
            }
            else if ((*it)->getVehicle()->getTemplate()->getClassID() == world::CID_ZiplineContainerTemplate ||
                     (*it)->getVehicle()->getTemplate()->getClassID() == world::CID_GrapplingHookRopeContainerTemplate)
            {
                world::IObject* occupying =
                    static_cast<world::PlayerControlObject*>(player->getVehicle())->getOccupingObject();
                if (occupying)
                    aiObject = static_cast<IAIObject*>(occupying->getAIComponent());
            }

            if (!aiObject)
                continue;
        }

        if (filter)
        {
            if (filter->predicate(aiObject))
                out.push_back(std::make_pair((*it)->getId(), aiObject));
        }
        else
        {
            out.push_back(std::make_pair((*it)->getId(), aiObject));
        }
    }
}

IAIObject* BFObjectEnvironment::getLocalPlayerObject() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getLocalPlayerObject");
}

bool BFObjectEnvironment::getLocalPlayerPosition(Vec3& position) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getLocalPlayerPosition");
}

bool BFObjectEnvironment::getLocalPlayerCameraTransformation(Mat4& transform) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getLocalPlayerCameraTransformation");
}

int BFObjectEnvironment::getLocalPlayerId() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getLocalPlayerId");
}

const std::vector<int>& BFObjectEnvironment::getActivePlayerIds() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getActivePlayerIds");
}

// Returns whether PlayerManager resolves the id to a player.
bool BFObjectEnvironment::isValidPlayerId(int id) const
{
    return world::playerManager->getPlayerFromId(id) != 0;
}

int BFObjectEnvironment::getNextPlayerId(int id) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getNextPlayerId");
}

bool BFObjectEnvironment::collideLineWithWorld(const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore, bool a, float b, bool c, bool& hit)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::collideLineWithWorld");
}

bool BFObjectEnvironment::collideLineWithWorld(const Vec3& from, const Vec3& to, AIObjectPredicate* filter, Vec3* hit)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::collideLineWithWorld");
}

bool BFObjectEnvironment::getMaxHeight(const Vec3& from, const Vec3& to, AIObjectPredicate* filter, float& height)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getMaxHeight");
}

bool BFObjectEnvironment::hasIndirectLineOfFire(IWeapon* weapon, const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::hasIndirectLineOfFire");
}

void BFObjectEnvironment::pickRandomSensePosition(Vec3& position, Vec3& normal, Vec3 hint, IAIObject* object)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::pickRandomSensePosition");
}

bool BFObjectEnvironment::pickSoldierRandomSensePosition(Vec3& position, Vec3& normal, IAIObject* object, unsigned int& part)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::pickSoldierRandomSensePosition");
}

bool BFObjectEnvironment::getSoldierSensePosition(Vec3& position, Vec3& normal, IAIObject* object, unsigned int part)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getSoldierSensePosition");
}

int BFObjectEnvironment::getHeightAndNormalForObject(IAIObject* object, const Vec3& position, float& height, Vec3& normal, float& slope)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getHeightAndNormalForObject");
}

void BFObjectEnvironment::getVehicleSpawnerPositions(std::vector<Vec3>& out, unsigned char team) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getVehicleSpawnerPositions");
}

IAIObject* BFObjectEnvironment::getObjectFromName(const std::string& name)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getObjectFromName");
}

int BFObjectEnvironment::getAllStaticCollisionMeshes(std::list<MeshAndBoundingBox>& meshes, std::list<WaterMesh>& water, std::map<const IAIObject*, DestroyableAddOnMesh>& addOns, std::list<MeshAndBoundingBox>& more, std::pair<Vec2, Vec2>& bounds, unsigned char team) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAllStaticCollisionMeshes");
}

int BFObjectEnvironment::getAllStaticCollisionMeshesWithinBoundary(const Vec2& min, const Vec2& max, std::list<MeshAndBoundingBox>& meshes, std::list<WaterMesh>& water, std::map<const IAIObject*, DestroyableAddOnMesh>& addOns, std::list<MeshAndBoundingBox>& more, std::pair<Vec2, Vec2>& bounds, unsigned char team) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAllStaticCollisionMeshesWithinBoundary");
}

bool BFObjectEnvironment::isAsleep(unsigned int id) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::isAsleep");
}

int BFObjectEnvironment::getTargetObjects(std::vector<TargetData>& out, int team) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getTargetObjects");
}

// ELF, 10 bytes. Empty in the dedicated image.
void BFObjectEnvironment::loadOvergrowthCollision(const std::string& path) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::loadOvergrowthCollision");
}

bool BFObjectEnvironment::initAiCombatAreas() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::initAiCombatAreas");
}

bool BFObjectEnvironment::isAiPositionInsideCombatArea(const Vec3& position) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::isAiPositionInsideCombatArea");
}

int BFObjectEnvironment::getAiCombatAreaLayer() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAiCombatAreaLayer");
}

int BFObjectEnvironment::getAICombatArea(std::list<Vec2>& out) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::getAICombatArea");
}

// Updates the AI object environment during the simulation tick.
void BFObjectEnvironment::damageSeverelyDamagedVehicles()
{
    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();

    if (m_lastSevereDamageTime > now)
        m_lastSevereDamageTime = 0.0;

    if (!(now - m_lastSevereDamageTime > 4.0))
        return;

    m_lastSevereDamageTime = now;

    std::map<unsigned int, world::IObject*>* const pcos = gameLogic->getPcoMap();   // +0x198

    for (std::map<unsigned int, world::IObject*>::iterator it = pcos->begin(); it != pcos->end(); ++it)
    {
        world::IObject* const object = it->second;

        if (object->getTemplate()->getClassID() == world::CID_SoldierTemplate)
            continue;

        world::IPlayerControlObject* pco = 0;
        for (world::IObject* o = object; o; o = o->getParent())
        {
            pco = static_cast<world::IPlayerControlObject*>(
                o->queryInterface(world::IID_IPlayerControlObject));
            if (pco)
                break;
        }

        if (pco->isOccupied())                                                        // +0x18
            continue;

        world::IArmorGLComp* const armor =
            static_cast<world::IArmorGLComp*>(object->getArmorComponent());          // +0x3C
        world::ArmorGLCompTemplate* const armorTemplate = static_cast<world::ArmorGLCompTemplate*>(
            object->getTemplate()->getComponentFromClassID(world::CID_ArmorGLCompTemplate));

        if (!armor || !armorTemplate)
            continue;

        const float maxHitPoints = armor->getMaxHitPoints();                         // +0x84

        if (!armorTemplate->getCanBeDestroyed())                                      // +0x154
            continue;

        if (armor->getHitPoints() / maxHitPoints < 0.5f)                              // +0x3C
        {
            Vec3 position;  position.x = 0.0f;  position.y = 0.0f; position.z = 0.0f;
            Vec3 direction; direction.x = 0.0f; direction.y = 1.0f; direction.z = 0.0f;

            gameLogic->giveDamage(object, maxHitPoints * 0.1f, 0, 0, 0, -1, 0, 0,     // +0xE4
                                  position, direction, -1, false, true, false);
        }
    }
}

int BFObjectEnvironment::collideLineWithObjects(const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore, bool a, float b, bool c, bool& hit)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFObjectEnvironment::collideLineWithObjects");
}

} // namespace ai
} // namespace hfe
} // namespace dice
