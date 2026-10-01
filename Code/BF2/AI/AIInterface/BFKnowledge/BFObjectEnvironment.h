// Queries engine objects for the AI.
// Preserve virtual declaration order. [INFERRED] Return types require validation where implementations remain incomplete.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_BFOBJECTENVIRONMENT_H
#define AI_AIINTERFACE_BFKNOWLEDGE_BFOBJECTENVIRONMENT_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "AIAdapterTypes.h"

namespace dice {
namespace hfe {

class IBase;

namespace ai {

class IAIObjectEnvironment
{
public:
    // [FRONTIER] These nested mesh types belong to IAIObjectEnvironment.
    // Their member layouts are not yet known.
    struct MeshAndBoundingBox;
    struct WaterMesh;
    struct DestroyableAddOnMesh;

    IAIObjectEnvironment();
    virtual ~IAIObjectEnvironment();

    virtual void getWorldFrustumObjects(std::vector<IAIObject*>& out, const Frustum& frustum, AIObjectPredicate* filter) const = 0;
    virtual void getWorldSphereObjects(std::vector<IAIObject*>& out, const Vec3& centre, float radius, AIObjectPredicate* filter) const = 0;
    virtual void getAllWorldObjects(std::vector<IAIObject*>& out, AIObjectPredicate* filter) const = 0;
    virtual void getAllDestroyableObjects(std::vector<DestroyableObject>& out) const = 0;
    virtual void getWorldPlayerIds(std::vector<int>& out) const = 0;
    virtual void getWorldPlayerOccupiedObjects(std::vector<IAIObject*>& out, AIObjectPredicate* filter) const = 0;
    virtual void getWorldPlayerOccupiedObjectsWithIds(std::vector<std::pair<int, IAIObject*> >& out, AIObjectPredicate* filter) const = 0;
    virtual IAIObject* getLocalPlayerObject() const = 0;
    virtual bool getLocalPlayerPosition(Vec3& position) const = 0;
    virtual bool getLocalPlayerCameraTransformation(Mat4& transform) const = 0;
    virtual int getLocalPlayerId() const = 0;
    virtual const std::vector<int>& getActivePlayerIds() const = 0;
    virtual bool isValidPlayerId(int id) const = 0;
    virtual int getNextPlayerId(int id) const = 0;
    virtual bool collideLineWithWorld(const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore, bool a, float b, bool c, bool& hit) = 0;
    virtual bool collideLineWithWorld(const Vec3& from, const Vec3& to, AIObjectPredicate* filter, Vec3* hit) = 0;
    virtual bool getMaxHeight(const Vec3& from, const Vec3& to, AIObjectPredicate* filter, float& height) = 0;
    virtual bool hasIndirectLineOfFire(IWeapon* weapon, const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore) = 0;
    virtual void pickRandomSensePosition(Vec3& position, Vec3& normal, Vec3 hint, IAIObject* object) = 0;
    virtual bool pickSoldierRandomSensePosition(Vec3& position, Vec3& normal, IAIObject* object, unsigned int& part) = 0;
    virtual bool getSoldierSensePosition(Vec3& position, Vec3& normal, IAIObject* object, unsigned int part) = 0;
    virtual int getHeightAndNormalForObject(IAIObject* object, const Vec3& position, float& height, Vec3& normal, float& slope) = 0;
    virtual void getVehicleSpawnerPositions(std::vector<Vec3>& out, unsigned char team) const = 0;
    virtual IAIObject* getObjectFromName(const std::string& name) = 0;
    virtual int getAllStaticCollisionMeshes(std::list<MeshAndBoundingBox>& meshes, std::list<WaterMesh>& water, std::map<const IAIObject*, DestroyableAddOnMesh>& addOns, std::list<MeshAndBoundingBox>& more, std::pair<Vec2, Vec2>& bounds, unsigned char team) const = 0;
    virtual int getAllStaticCollisionMeshesWithinBoundary(const Vec2& min, const Vec2& max, std::list<MeshAndBoundingBox>& meshes, std::list<WaterMesh>& water, std::map<const IAIObject*, DestroyableAddOnMesh>& addOns, std::list<MeshAndBoundingBox>& more, std::pair<Vec2, Vec2>& bounds, unsigned char team) const = 0;
    virtual bool isAsleep(unsigned int id) const = 0;
    virtual int getTargetObjects(std::vector<TargetData>& out, int team) const = 0;
    virtual void loadOvergrowthCollision(const std::string& path) const = 0;
    virtual bool initAiCombatAreas() const = 0;
    virtual bool isAiPositionInsideCombatArea(const Vec3& position) const = 0;
    virtual int getAiCombatAreaLayer() const = 0;
    virtual int getAICombatArea(std::list<Vec2>& out) const = 0;
    virtual void damageSeverelyDamagedVehicles() = 0;
    virtual int collideLineWithObjects(const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore, bool a, float b, bool c, bool& hit) = 0;
};

class BFObjectEnvironment : public IAIObjectEnvironment
{
public:
    BFObjectEnvironment();
    virtual ~BFObjectEnvironment();  // ELF / / ( is )

    virtual void getWorldFrustumObjects(std::vector<IAIObject*>& out, const Frustum& frustum, AIObjectPredicate* filter) const;
    virtual void getWorldSphereObjects(std::vector<IAIObject*>& out, const Vec3& centre, float radius, AIObjectPredicate* filter) const;
    virtual void getAllWorldObjects(std::vector<IAIObject*>& out, AIObjectPredicate* filter) const;
    virtual void getAllDestroyableObjects(std::vector<DestroyableObject>& out) const;
    virtual void getWorldPlayerIds(std::vector<int>& out) const;
    virtual void getWorldPlayerOccupiedObjects(std::vector<IAIObject*>& out, AIObjectPredicate* filter) const;
    virtual void getWorldPlayerOccupiedObjectsWithIds(std::vector<std::pair<int, IAIObject*> >& out, AIObjectPredicate* filter) const;
    virtual IAIObject* getLocalPlayerObject() const;
    virtual bool getLocalPlayerPosition(Vec3& position) const;
    virtual bool getLocalPlayerCameraTransformation(Mat4& transform) const;
    virtual int getLocalPlayerId() const;
    virtual const std::vector<int>& getActivePlayerIds() const;
    virtual bool isValidPlayerId(int id) const;
    virtual int getNextPlayerId(int id) const;
    virtual bool collideLineWithWorld(const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore, bool a, float b, bool c, bool& hit);
    virtual bool collideLineWithWorld(const Vec3& from, const Vec3& to, AIObjectPredicate* filter, Vec3* hit);
    virtual bool getMaxHeight(const Vec3& from, const Vec3& to, AIObjectPredicate* filter, float& height);
    virtual bool hasIndirectLineOfFire(IWeapon* weapon, const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore);
    virtual void pickRandomSensePosition(Vec3& position, Vec3& normal, Vec3 hint, IAIObject* object);
    virtual bool pickSoldierRandomSensePosition(Vec3& position, Vec3& normal, IAIObject* object, unsigned int& part);
    virtual bool getSoldierSensePosition(Vec3& position, Vec3& normal, IAIObject* object, unsigned int part);
    virtual int getHeightAndNormalForObject(IAIObject* object, const Vec3& position, float& height, Vec3& normal, float& slope);
    virtual void getVehicleSpawnerPositions(std::vector<Vec3>& out, unsigned char team) const;
    virtual IAIObject* getObjectFromName(const std::string& name);
    virtual int getAllStaticCollisionMeshes(std::list<MeshAndBoundingBox>& meshes, std::list<WaterMesh>& water, std::map<const IAIObject*, DestroyableAddOnMesh>& addOns, std::list<MeshAndBoundingBox>& more, std::pair<Vec2, Vec2>& bounds, unsigned char team) const;
    virtual int getAllStaticCollisionMeshesWithinBoundary(const Vec2& min, const Vec2& max, std::list<MeshAndBoundingBox>& meshes, std::list<WaterMesh>& water, std::map<const IAIObject*, DestroyableAddOnMesh>& addOns, std::list<MeshAndBoundingBox>& more, std::pair<Vec2, Vec2>& bounds, unsigned char team) const;
    virtual bool isAsleep(unsigned int id) const;
    virtual int getTargetObjects(std::vector<TargetData>& out, int team) const;
    virtual void loadOvergrowthCollision(const std::string& path) const;
    virtual bool initAiCombatAreas() const;
    virtual bool isAiPositionInsideCombatArea(const Vec3& position) const;
    virtual int getAiCombatAreaLayer() const;
    virtual int getAICombatArea(std::list<Vec2>& out) const;
    virtual void damageSeverelyDamagedVehicles();
    virtual int collideLineWithObjects(const Vec3& from, const Vec3& to, const std::vector<IAIObject*>& ignore, bool a, float b, bool c, bool& hit);

private:
    /* +0x00 */ // vptr
    // [UNRESOLVED] Three pointer-vector fields remain opaque. The trailing clock value is a double.
    /* +0x04 */ unsigned char m_opaque[0x24];      // three std::vector members, zeroed as retail does
    /* +0x28 */ unsigned char m_byte28;            // [UNRESOLVED] ctor 0
    /* +0x2C */ double m_lastSevereDamageTime;     // [CHOSEN] name; ELF +0x2C / PE +0x38, ctor 0.0
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_BFOBJECTENVIRONMENT_H
