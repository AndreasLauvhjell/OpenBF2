// Base interface and shared state for strategic objects. Preserve virtual declaration order.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICOBJECT_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICOBJECT_H

#include <list>
#include <set>

// Vec2 and Vec3 cannot be forward-declared in this tree: both are typedefs to
// `dice::hfe::Math::Vec2/Vec3`, so a local `struct Vec3;` is a redefinition with a different basic
// type (C2371). The header has to come in.
#include "../../../../Math/Vec3.h"
#include "../AIStrategic/StatusMap.h"

namespace dice {
namespace hfe {
namespace ai {

class EnemyStatistics;
class ISAI;
class Strategy;

class AIStrategicObject
{
public:
    // Strategic object classifications used by console scripts. Unknown values have no printed name.
    enum StrategicObjectType
    {
        Flank          = 1,
        Centre         = 2,
        Base           = 3,
        Close          = 4,
        Remote         = 5,
        AirSpawner     = 6,
        LandSpawner    = 7,
        NavalSpawner   = 8,
        SoldierSpawner = 9,
        StrongPoint    = 10,
        ChokePoint     = 11,
        Bridge         = 12,
        AirField       = 13,
        SupplyPoint    = 14,
        Route          = 15,
        ControlPoint   = 16,
        North          = 17,
        West           = 18,
        South          = 19,
        East           = 20,
        // 21 has no name in the writer
        Front          = 22,
        Safe           = 23,
        Enemy          = 24,
        // 25 has no name in the writer
        NeutralType    = 26     // spelled "Neutral" by the writer; renamed here only because
                                // `StatusMap::Status::Neutral` would collide at this scope
    };

    AIStrategicObject(float radius, float f148);

    // the vtable, , thirty-three slots
    virtual ~AIStrategicObject();                                      // slots 0/1,

    virtual int getType() const;                                       // slot 2,
    virtual const Vec2& getPosition(int side) const;                   // slot 3,
    virtual void setPosition(int side, const Vec2& position);          // slot 4,

    // Slots 5..17, 21..25, 28, 31 and 32 are `__cxa_pure_virtual` in this class's table. Every name
    // and parameter list below is taken from the matching slot of `AIStrategicArea`'s vtable; the
    // return types are [INFERRED] from whether the override leaves its result in eax or in st0.
    virtual Vec3 getOrderPos(int a, int b) const = 0;                  // slot 5
    virtual void updateTemperature(const Strategy* strategy, int side) = 0;               // slot 6
    virtual void update(int side, ISAI* sai, bool& changed) = 0;                          // slot 7; paired Area body has no return
    virtual int  getChangedAttackDefence(int side) = 0;                                   // slot 8
    virtual float getControlPointTakeOverCounter() = 0;                                  // slot 9; ST0 float chain
    virtual bool isEnemyTakingOverBase(int side) = 0;                                     // slot 10
    virtual int  calculateDefenseNeed(int side) = 0;                                      // slot 11
    virtual int  calculateAttackValue(int side) = 0;                                      // slot 12
    virtual int  calculateAttackCost(int side) = 0;                                       // slot 13
    virtual bool isNeighbour(AIStrategicObject* object) const = 0;                        // slot 14
    virtual bool hasNeighbourWithStatus(StatusMap::Status status, int side) const = 0;    // slot 15
    virtual bool isNeighbouringObject(AIStrategicObject* object) const = 0;               // slot 16
    virtual bool hasNeighbouringObjectWithStatus(StatusMap::Status status,
                                                 int side) const = 0;                     // slot 17

    virtual int  getObjectTypeFlags() const;                           // slot 18,
    virtual bool hasObjectTypeFlag(StrategicObjectType type) const;    // slot 19,
    virtual bool hasObjectTypeFlags(int flags) const;                  // slot 20,

    virtual int  getTacticalStatus(int side) const = 0;                                   // slot 21
    virtual bool isAreaTakeable() = 0;                                                    // slot 22
    virtual int  collectSquadResources(int side, ISAI* sai) = 0;                          // slot 23
    virtual int  collectAdditionalSquadResources(int side, int amount, ISAI* sai) = 0;    // slot 24
    virtual int  releaseSurplus(int side) = 0;                                            // slot 25

    virtual float gravity(const Vec3& position, int side);             // slot 26,
    virtual float gravity(const Vec2& position, int side);             // slot 27,

    virtual bool retainBot(int side, int a, ISAI* sai) = 0;                               // slot 28

    virtual float getXExtent(int side) const;                          // slot 29,
    virtual float getZExtent(int side) const;                          // slot 30,

    virtual void setAsDirty(int side) = 0;                                                // slot 31
    virtual int  orderBot(int side, const std::list<AIStrategicObject*>& objects,
                          int a, ISAI* sai) = 0;                                          // slot 32

    int getNumberOfAssignedUnits(int side) const;
    bool isAttack(int side) const;
    // Borrowed mutable statistics at m_statistics[side - 1]; reference spelling is [INFERRED].
    EnemyStatistics& getBattleStatistics(int side) const;
    int releaseAssignedResources(int side);

    // [INFERRED] accessor names/forms. SAI::updateStates and the area's neighbour scans inline
    // these loads: status is at +0x70 + 4*side and index at +0xF0 in both shipped x86 ports.
    StatusMap::Status getStatus(int side) const { return m_status[side - 1]; }
    int getIndex() const { return m_index; }

    // [CHOSEN, not recovered] Public fields allow manager access; the original may have
    // used friendship instead. Offsets document ELF layout and do not constrain modern STL sizes.
    /*   +4 */ EnemyStatistics* m_statistics[4];   // new EnemyStatistics(6), (6), (2), (2) in order
    // Per-side strategic-object sets. [UNRESOLVED] Exact roles of these four pairs remain
    // unidentified; at least two pairs represent neighboring areas and objects.
    /*  +20 */ std::set<int> m_set20[2];
    /*  +44 */ std::set<int> m_set44[2];
    /*  +68 */ std::set<int> m_set68[2]; // assigned units, one-based side accessor
    /*  +92 */ std::set<int> m_set92[2];

    // All four set to 2. `StatusMap::Status(2)` is `Neutral`, which is the only reason this member
    // carries that name - [INFERRED], on the strength of one coincidence.
    /* +116 */ StatusMap::Status m_status[4];

    // one-based: `getPosition(side)` is `m_position[side - 1]`. See the header note.
    /* +132 */ Vec2 m_position[2];

    /* +148 */ float m_f148;        // the constructor's second argument; -666.0f from every caller
    /* +152 */ float m_size[3];     // [0] = the radius argument, [1] and [2] zeroed; read by side
    /* +164 */ float m_f164[4];     // all four set to the radius argument [UNRESOLVED]
    // Paired Area::update reads/writes these as floats, not the formerly guessed ints.
    // [INFERRED] names; one-based side pairs, all zeroed by the base constructor.
    /* +180 */ float m_needCurveInput[2];
    /* +188 */ float m_friendlyRatio[2];
    /* +196 */ float m_friendlyBattleStrength[2];
    /* +204 */ float m_enemyBattleStrength[2];
    /* +212 */ int m_unresolved212[2]; // zeroed; these two preceding counts remain unidentified
    /* +220 */ int m_resourceNeed[2];
    /* +228 */ int m_assignedSquadStrength[2];
    /* +236 */ bool  m_attack[2];   // both true; isAttack(side) reads [side - 1]
    /* +240 */ int   m_index;       // not initialised here - addStrategicObject assigns it
    /* +244 */ int   m_i244;        // 1 [UNRESOLVED]
    /* +248 */ int   m_i248;        // 0 [UNRESOLVED]
    /* +252 */ int   m_category;    // 1 until createFromControlPoint or create sets it
    /* +256 */ std::set<int> m_set256;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICOBJECT_H
