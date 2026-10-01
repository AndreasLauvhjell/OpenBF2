// Static climbable ladder with lazily computed bounds and a shared id registry for replication.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_LADDER_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_LADDER_H

#include <map>

#include "../Base/SimpleObject.h"

namespace dice {
namespace hfe {
namespace world {

class LadderTemplate;
class LadderContainer;
class PlayerControlObject;
class ISoldier;

// Type name is attested by LadderContainer::_getExitPosition's mangling. The numeric values are
// read from every switch/caller; the descriptive enumerator spellings are [CHOSEN].
enum LadderExitType
{
    LadderExitNone   = 0,
    LadderExitTop    = 1,
    LadderExitBottom = 2,
    LadderExitSide   = 3
};

// Ladder intentionally attaches no networkable and does not expose IConsoleSaveable through queryInterface.
// [PLATFORM] Field offsets differ between images; use native members.
class Ladder : public SimpleObject
{
public:
    explicit Ladder(const LadderTemplate* objectTemplate);   // read in full
    virtual ~Ladder();                               // (D1) / 201 B (D0) - the map erase

    // the quartet, refcount at +0x138
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;// the two-case table

    virtual bool  handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                  int, int, CollisionFaceInfo*, CollisionFaceInfo*);
                                                     // the soldier-catch
    virtual void  initNetworkableComp();             // empty

    // the bounds - every getter recalculates first if the dirty flag is set
    float getLadderHeight();                         // +0x148
    float getAbsoluteTop();                          // +0x150
    float getAbsoluteBottom();                       // +0x154
    float getAbsoluteBottomReal();                   // +0x158
    void  recalcBounds();                            // geometry/terrain/water-derived

    // the soldier side, sizes are the ELF's
    void  attachSoldier(ISoldier* soldier);
    void  detachSoldier(ISoldier* soldier);
    float getCurrentState(ISoldier* soldier);        // (root position y - bottom) / height, clamped 0..1
    void  setCurrentState(ISoldier* soldier, float state);   // moves the root to bottom + height * state
    LadderExitType checkLimits(const Mat4& transform, bool climbingUp, bool climbingDown);
    Vec3  snapSoldierPosToLadder(const Vec3& position);      // clamps y into [bottom + step, top]
    int   checkCollision(const Mat4& transform, LadderContainer* container);

    int   getLadderId() const                        { return m_ladderId; }

    // The process-wide id source: .data, starts at 1 (0x8987B64), post-incremented by every ctor,
    // reset TO 1 by the dtor that empties the map. attested name (_ZN4dice3hfe5world6Ladder14globalLadderIdE).
    static int globalLadderId;

protected:
    friend class PlayerControlObject;                 // init's class-id-gated animation arm
    // layout +0x138..+0x15B - every ctor value below is from the 170-byte constructor, read in full.
    /* 0x138 */ int   m_refCount;                    // ctor 1
    /* 0x13C */ const LadderTemplate* m_template;    // ctor = the argument
    /* 0x140 */ int   m_ladderId;                    // ctor = globalLadderId++, then (id, this) is
                                                     // inserted into ladderMap
    /* 0x144 */ bool  m_boundsDirty;                 // [CHOSEN] ctor true - every bounds getter
                                                     // runs recalcBounds while set
    /* 0x145 */ char  _pad145[3];
    /* 0x148 */ float m_ladderHeight;                // ctor 2.0f
    /* 0x14C */ int   _dword14C;                     // [UNRESOLVED] NOT written by the ctor
                                                     // (recalcBounds' territory)
    /* 0x150 */ float m_absoluteTop;                 // ctor 0.0f
    /* 0x154 */ float m_absoluteBottom;              // ctor 0.0f
    /* 0x158 */ float m_absoluteBottomReal;          // ctor 0.0f
};

// The process-wide ladder registry the ctor/dtor maintain and SoldierNetworkable, GrapplingHookRope
// ::checkForEntry and ZiplineTemplate::checkForEntry read: a namespace-scope std::map<int, Ladder*>
// at ds:0x8AAD220 (its node count at 0x8AAD224), constructed by Ladder.cpp's static init. Unnamed in
// the ELF - `ladderMap` is [CHOSEN]; external linkage because those three TUs reach it.
extern std::map<int, Ladder*> ladderMap;

// [CHOSEN source split] The retail Ladder/LadderContainer bodies share one TU-local Vars sheet.
// This reconstruction keeps the two attested classes in separate inferred files, so the pointers
// are named in a detail namespace rather than duplicated (which would register every console var
// twice).
namespace ladder_detail {
extern float* ladderOffsetTop;
extern float* ladderSoldierStartGetoff;
extern float* ladderTopExitOffset;
extern float* ladderExitOffset;
extern float* ladderBottomMarginal;
extern float* ladderStepSize;
extern float* ladderSkidSpeedMod;
extern float* ladderClimbSpeedMod;
extern float* ladderEndCameraOffset;
extern float* ladderWaterLimit;
extern float* ladderGetoffSpeedMod;
extern float* ladderSoldierDist;
extern float* ladderDownAngleLimit;
extern float* ladderDownRayMag;
extern float* ladderDownRayMagSkid;
extern float* ladderUpRayMagSkid;
extern float* ladderGetoffRayMin;
extern int*   ladderRayDebug;
extern int*   ladderRayDebug2;
}

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_LADDER_H
