// Squad member state and coordinator association.
// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SquadMember operations.

#ifndef AI_AIDLL_AI_AIBOT_SQUADMEMBER_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_SQUADMEMBER_H

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class IAIPlayer;
class Bot;
class SquadCoordinator;

class SquadMember
{
public:
    SquadMember(SquadCoordinator* squad, IAIPlayer* player,
                int squadIndex, int side, int memberIndex);
    ~SquadMember();

    IAIPlayer* getPlayer();
    SquadCoordinator* getSquad();
    int getPlayerId();
    bool isBot();
    Bot* getBot();                           // borrowed
    int getMemberIndex();
    void setIsSquadLeader(bool leader);
    bool isSquadLeader();
    bool isMoving();                         // returns m_moved
    void setHavePath(bool havePath);
    void update(double time);                // bounded dependent calls
    void updateFormationStatus();
    bool getPosition(Vec3& position);         // copies even if invalid
    bool isAllowedInFormation();             // nonnull vehicle [FRONTIER]
    void updateMemberPos();                  // nonnull vehicle [FRONTIER]
    bool isMobile();                         // owner query [FRONTIER]
    void moveWithFormation();                // world-position [FRONTIER]
    void moveWithoutFormation();             // alive owner query [FRONTIER]
    float getVehicleRadius();                // [FRONTIER]

    // [INFERRED] carrier name. The two fields are attested by setCurrentRequestAction
    // its first parameter is the RequestAction enum, its second unsigned int.
    struct RequestState
    {
        int m_action;
        unsigned int m_handle;
    };

    // Offsets below are ELF. PE matches through +0x20, then adds four bytes of double alignment.
    /* +0x00 */ int m_squadIndex;
    /* +0x04 */ int m_memberIndex;
    /* +0x08 */ bool m_isSquadLeader;
    /* +0x0C */ int m_playerId;
    /* +0x10 */ IAIPlayer* m_player;
    /* +0x14 */ Bot* m_bot;
    /* +0x18 */ SquadCoordinator* m_squad;
    /* +0x1C */ int m_side;                  // zero-based, unlike the engine player's side
    /* +0x20 */ float m_estimatedArrival;    // getEstimatedArrival
    /* +0x24 */ double m_time24;             // last path reconsideration time; initialized to zero
    /* +0x2C */ float m_maxSpeed;            // getMaxSpeed
    /* +0x30 */ bool m_flag30;               // [UNRESOLVED]
    /* +0x31 */ bool m_havePath;
    /* +0x34 */ RequestState* m_request;
    /* +0x38 */ int m_int38;                 // [UNRESOLVED]
    /* +0x3C */ Vec3 m_position;
    /* +0x48 */ bool m_positionValid;
    /* +0x49 */ bool m_flag49;               // [UNRESOLVED]
    /* +0x4A */ bool m_flag4A;               // setHavePath writes the inverse of its argument
    /* +0x4B */ bool m_flag4B;               // [UNRESOLVED]
    /* +0x4C */ float m_formationDistanceSq; // [INFERRED] name, starts at 900.0
    /* +0x50 */ bool m_moved;
    /* +0x54 */ Vec3 m_vector54;             // [UNRESOLVED]
    /* +0x60 */ Vec3 m_vector60;             // [UNRESOLVED]
};

#if defined(_MSC_VER)
static_assert(sizeof(void*) != 4 || sizeof(SquadMember) == 112, "SquadMember PE x86 layout");
#endif

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_SQUADMEMBER_H
