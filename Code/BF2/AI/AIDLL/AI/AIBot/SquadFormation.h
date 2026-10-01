// Maintains squad member positions, movement flags and formation speed.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOT_SQUADFORMATION_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_SQUADFORMATION_H

#include <utility>
#include <vector>

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class SquadCoordinator;

class SquadFormation
{
public:
    // One position in the formation. , from the constructor's `operator new(0x24)`, and
    // the type name is attested by the mangled `std::vector<SquadFormation::Slot*>` instantiations.
    // The constructor writes every field; the names below are [INFERRED] from the two readers.
    class Slot
    {
    public:
        // [CHOSEN, not recovered] Source packaging for swapSlots' measured temporary:
        // only its Vec3 is initialized. Ordinary formation slots retain their constructor.
        struct SwapTemporary {};
        explicit Slot(SwapTemporary) : m_offset() {}

        Slot()
            : m_occupied(false)
            , m_memberIndex(-1)
            , m_i20(-1)
            , m_b24(false)
            , m_i28(0)
            , m_referenceSlot(0)
        {
            m_offset.x = 0.0f;
            m_offset.y = 0.0f;
            m_offset.z = 0.0f;
        }

        /*  +0 */ bool m_occupied;       // the flag every deep branch in this class is guarded by
        /*  +4 */ int  m_memberIndex;    // = -1; indexes the coordinator's squad-member vector
        /*  +8 */ Vec3 m_offset;         // the position the formation setters compute
        /* +20 */ int  m_i20;            // = -1 [UNRESOLVED]
        /* +24 */ bool m_b24;            // = false [UNRESOLVED]
        /* +28 */ float m_i28;           // legacy inferred name; float proved by leaveFormation's x87 loads/stores
        /* +32 */ int  m_referenceSlot;  // the slot this one is measured from [INFERRED]
    };

    // values attested, names [INFERRED]. `setFormation` switches on 0, 1, 2 and 3 and nothing
    // else; `changeFormation(Formations)` is the mangled name that says this is an enum at all.
    enum Formations
    {
        FormationLine     = 0,
        FormationWedge    = 1,
        FormationEchelonA = 2,
        FormationEchelonB = 3
    };

    explicit SquadFormation(SquadCoordinator* coordinator);

    void update(double time);                 // complete scheduler
    void updateFormation();                  // leader/anchor reassignment
    void updateWaitForSquadMembers(double time); // occupied timing seam
    void updateFormationForwardVector();     // occupied owner-query seam
    void updateTrailingPositions();          // complete
    void updateTrailingSlots();              // complete
    void selectFormation();                  // / PE inlined at
    int getSquadMemberIndexAtSlot(int slotIndex); // no occupied test
    bool shouldLeaveSquadLeader();           // returns m_b84
    void joinSquadLeader();                  // (inlines the movement update)
    float getFormationMaxSpeed();
    void updateFormationMaxSpeed();
    void setFormation();
    void setLineFormation();
    void setWedgeFormation();                // [FRONTIER]
    void setEchelonFormation();              // [FRONTIER]

    void joinFormation(int memberIndex);
    bool isInFormation(int memberIndex);      // ignores occupied flag
    void leaveFormation(int memberIndex);
    void resetSlot(int slotIndex);            // inlined
    void updateClosestSlots();
    int getClosestSlotIndex(int slotIndex);    // world-position seam
    void swapSlots(int first, int second);    // three payload fields only
    void updateMovementFlag(bool force);
    void updateSlotZeroPos();                 // inlined
    void resetTrailingPositions();

    // layout, on the ELF, read store by store out of the constructor
    /*   +0 */ std::vector<Slot*> m_slots;            // one per squad member, each `new`ed
    /*  +12 */ SquadCoordinator* m_coordinator;       // the constructor's argument
    /*  +16 */ int m_formation;                       // = 0 (line); `setFormation` switches on it
    /*  +20 */ Vec3 m_direction;                      // set by `setFormation` per formation
    /*  +32 */ unsigned int m_maxSquadMembers;        // = IAIGame::getMaxNumberOfSquadMembers
    /*  +36 */ bool m_b36;                            // = false; shouldSquadMoveOut returns it
    /*  +40 */ float m_formationMaxSpeed;             // FLT_MAX, then 5.0f - see the header note
    /*  +44 */ int m_side;                            // = the coordinator's side
    // ELF +0x30 / PE +0x34 is one `Vec2` - the formation's XZ forward vector, born (0, 1). The
    // Vec2 type is attested by the forward-vector update passing this address to
    // `vec3ToVec2XZ(const Vec3&, Vec2&)`; getSlotWorldPos is the only other reader. The split
    // below predates that evidence and stays until the occupied projection lands, so that the
    // retyping and its first writer arrive together.
    /*  +48 */ int m_i48;                             // = 0 Vec2 x (0.0f bits)
    /*  +52 */ float m_f52;                           // = 1.0f Vec2 y
    /*  +56 */ bool m_b56;                            // = false; hasFormationMoved returns it
    /*  +60 */ Vec3 m_v60;                            // zeroed; anchor position at the last movement stamp
    /*  +72 */ Vec3 m_v72;                            // zeroed; slot-zero world position (updateSlotZeroPos)
    /*  +84 */ bool m_b84;                            // = false; shouldLeaveSquadLeader returns it
    /*  +85 */ bool m_b85;                            // = false; set by joinSquadLeader, cleared by promotion
    /*  +88 */ int m_i88;                             // = 0; the scheduler's signed six-tick cycle counter
    /*  +92 */ double m_d92;                          // = 0.0; next movement-stamp time (now + 0.2)
    // [INFERRED] Per-slot breadcrumb targets. Anchor movement beyond six metres shifts the
    // trail and inserts its previous position; construction sizes it to the slot count.
    /* +100 */ std::vector<std::pair<bool, Vec3> > m_slotTargets;   // (valid, crumb) per index
    /* +112 */ Vec3 m_trailingPosition;               // [INFERRED name]; the newest crumb, seeded from m_v72
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_SQUADFORMATION_H
