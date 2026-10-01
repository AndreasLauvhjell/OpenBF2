// Static collision-to-damage handlers. Contact speed, normal, relative contact point
// and material ids determine damage to the hit object's root. Land/water uses the
// surface material; object collisions use self's material.
// [CHOSEN name] g_collisionLog is disabled by default; enabling it performs the
// material/name queries even though the formatted diagnostic is discarded.

#ifndef GAME_GAMELOGIC_GAMELOGICCOLLISION_H     // [INFERRED] guard style
#define GAME_GAMELOGIC_GAMELOGICCOLLISION_H

#include "../../Math/Mat4.h"
#include "../../Math/Vec3.h"
#include "../IGameLogic.h"                 // world::MaterialCollisionType, CollisionFaceInfo, IProjectileObject

namespace dice {
namespace hfe {
namespace world {

class IObject;
class IPlayer;
class IArmorGLComp;
class IObjectTemplate;

// . The mass ratio other/self, clamped to [0.3, 3.0]; 1.0 unless both objects have
// a mobile physics node and both templates answer IID_ISimpleObjectTemplate. A soldier on the
// `self` side weighs a flat 200. Reads the two root templates' +92 (SimpleObjectTemplate::m_mass)
// directly, which is why SimpleObjectTemplate befriends it.
float getMassMod(IObject* other, IObject* self);

class GameLogicCollision
{
public:
    // The report's first argument. 0 and 1 and 2 are what the three written handlers pass; 3 and
    // 4 are gated by the log's +0x0B / +0x0C enable bytes and belong to the projectile /
    // replenisher handlers (names [UNRESOLVED]).
    enum GLCollisionType
    {
        GLCT_ObjectVsSoldier = 0,      // [CHOSEN names, recovered values]
        GLCT_ObjectVsObject  = 1,
        GLCT_LandOrWater     = 2,
        GLCT_Type3           = 3,      // [UNRESOLVED]
        GLCT_Type4           = 4       // [UNRESOLVED]
    };

    // . The layout is the static initialiser's store sequence (0x80DFBE0), the roles are
    // report()'s reads. [CHOSEN field names]
    struct CollisionLog
    {
        bool  enabled;                 // +0x00  0 - THE gate; nothing below runs while it is clear
        bool  showObjectNames;         // +0x01  1 - template names instead of "" for the two objects
        bool  showMaterialNames;       // +0x02  1 - material names instead of "%d"
        float minDamage;               // +0x04  1.0 - below it only -1.0 (and 0.0 with +0x08) report
        bool  showZeroDamage;          // +0x08  1
        bool  enableType1;             // +0x09  1 } per-GLCollisionType enables for types 1..4;
        bool  enableType2;             // +0x0A  1 } type 0 (object-vs-soldier) is always reported
        bool  enableType3;             // +0x0B  1 }
        bool  enableType4;             // +0x0C  1 }
        bool  detailed;                // +0x0D  1 - the six-term breakdown line
        bool  byte0E;                  // +0x0E  1 [UNRESOLVED] - set, never read by report
        bool  byte0F;                  // +0x0F  1 [UNRESOLVED]
        bool  suppressRepeats;         // +0x10  0 - with it, the same (ownMaterial, otherMaterial)
                                       //        pair twice in a row reports once
        int   lastOwnMaterial;         // +0x14  0
        int   lastOtherMaterial;       // +0x18  0

        // . (type, self, other, ownMaterial, otherMaterial, total, constant,
        // angle, speed, material, mass) - the six floats are the breakdown line's six "%.2f".
        void report(GLCollisionType type, IObject* self, IObject* other, int ownMaterial,
                    int otherMaterial, float total, float constant, float angle, float speed,
                    float material, float mass);
    };

    // the handlers ServerGameLogic dispatches into
    // . Land and water: `other` is always 0. Damage = damageMod(surface, own)
    // x max(armor angleMod, |cos(speed, normal)|) x speed^2 (a soldier: (speed - limit) x speed),
    // reported as type 2, given to the root with no player, team -1, the surface material.
    static void serverHandleCollisionLandOrWater(IObject* other, IObject* self, const Vec3& speed,
                                                 const Vec3& normal, const Vec3& relative,
                                                 int surfaceMaterial, int ownMaterial);
    // . A soldier `other` does nothing; a soldier `self` goes to the soldier
    // arm; otherwise damage = damageMod(other, own) x notProjectile x cos^2 x (speedMod x excess^n) x
    // massMod where excess = max(0, |speed| - collision-impact-speed-limit) and n is 2 or 3.
    static void serverHandleCollisionObjectVsObject(IObject* other, IObject* self, const Vec3& speed,
                                                    const Vec3& normal, const Vec3& relative,
                                                    int otherMaterial, int ownMaterial);
    // . A soldier hit by an object (a vehicle running a soldier over, or a
    // soldier running into one). Material 666 or a barbwire material kills at any speed.
    static void serverHandleCollisionObjectVsSoldier(IObject* other, IObject* self, const Vec3& speed,
                                                     const Vec3& normal, const Vec3& relative,
                                                     int otherMaterial, int ownMaterial);

    // Returns whether damage was applied. Vector arguments are velocity, normal,
    // and root-relative collision offset; the offset builds the world contact point.
    static bool serverHandleCollisionForProjectile(IObject* projectile, IObject* hitObject,
                                                   const Vec3& velocity, const Vec3& normal,
                                                   const Vec3& collisionOffset, int materialA,
                                                   int materialB, CollisionFaceInfo* faceA,
                                                   CollisionFaceInfo* faceB);

    // The two support-projectile handlers. Their mangled names settle these signatures; the
    // bodies are recovered from ELF 0x80DD6C0/0x80DDA90 and PE 0x56AA00/0x56AB70.
    static bool serverHandleCollisionForReplenisher(bool, IObject*, IObject*, const Vec3&,
                                                    const Vec3&, const Vec3&, int, int);
    static bool serverHandleCollisionForResurrecter(IObject*, IObject*, float, float, const Vec3&,
                                                    const Vec3&, const Vec3&, int, int);
    static void doProjectileImpact(int, int, MaterialCollisionType, const Vec3&, const Vec3&,
                                   const Vec3&, IProjectileObject*, IObject*, IObject*,
                                   IArmorGLComp*, bool, CollisionFaceInfo*);

    // Client impact sound, effect, and decal; inactive on dedicated images. Material
    // ids select the collision response, objectB receives the decal, and face identifies
    // its surface. effectRotation is negated ZXY Euler degrees; ignoreUnderWater
    // bypasses the projectile-depth test. [CHOSEN] Parameter names.
    static void doImpact(int, int, MaterialCollisionType, const Vec3&, const Vec3&, const Vec3&,
                         IObject*, IObject*, IProjectileObject*, CollisionFaceInfo*, Vec3, bool);

    // Spawns a collision effect with the supplied frame and returns the object, or null.
    // Dedicated images return null without effects. [INFERRED] Pointer return type.
    static IObject* playCollisionEffect(Vec3 position, const IObjectTemplate* effectTemplate,
                                        Vec3 up, Vec3 normal);
    // BF2.exe 0x5CA910, 164 B: the spawn itself - createObject, the EffectBundle collision plane,
    // relative/absolute transformation, init, template message 0; returns the new object.
    static IObject* playCollisionEffect(const IObjectTemplate* effectTemplate, const Mat4& transform);

    // . The projectile arm's "penetrate or bounce" test. hardness = the two
    // materials' projectileCollisionHardness multiplied (material 0 stands in for an unknown id);
    // <= 0: out = 0.1, false. Else out = sqrt(1 - d^2) x hardness where d = -(speed . normal) and
    // the answer is cos((1 - out) x pi/2) >= d.
    static bool calculateCollisionType(const Vec3& speed, const Vec3& normal, int materialA,
                                       int materialB, float& out);

    // . Who to credit: the occupying player of `other`'s PCO and that player's
    // team. If `self` is a PCO on the same team and both objects have physics nodes, the credit is
    // withdrawn (player 0, team -1) when self's speed projected on the contact speed is the larger
    // i.e. a team-mate you drove into is at fault only if you were the one moving.
    static void getAttackPlayer(const IObject* other, const IObject* self, const Vec3& speed,
                                IPlayer*& player, int& team);

    static Mat4 m_effectTransformation;      // identity; used by the effect helpers
                                             // (BF2.exe 0x973F98, .data, statically identity)
    //  / PE 0x569DB0 / BF2.exe 0x5CA5F0 (469 B, fastcall: position ECX, up EDX,
    // normal on the stack). All three return the static matrix's address, immediately copied
    // by DestroyableWindow; const-reference return is [INFERRED] from that source-level use.
    static const Mat4& getCollisionEffectTransformation(const Vec3& position, const Vec3& up,
                                                       const Vec3& normal);
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_GAMELOGICCOLLISION_H
