// Coordinates squad membership, leadership and its owned formation.
// [FRONTIER] Leader-change movement remains incomplete.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOT_SQUADCOORDINATOR_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_SQUADCOORDINATOR_H

#include <list>
#include <vector>

#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
enum KitType : int;
namespace ai {

class SquadFormation;
class SquadMember;
class IAIPlayer;
class AIStrategicArea;
enum StrategicStrengthType : int; // Attested type name; enum labels remain unrecovered.

class SquadCoordinator
{
public:
    // attested nested type name: `std::list<SquadCoordinator::SquadMemberFire*>` insert/erase/clear
    // are instantiated. on the ELF (`shotsFired` allocates 0x44), four Vec3s zeroed by
    // the allocation; the scalar members are written field by field. Roles come from shotsFired
    // (, ), removeSquadMemberFire and updateShotsFired; names are [INFERRED].
    // Nothing in this tree allocates one yet, so the list stays empty on every exercised path.
    struct SquadMemberFire
    {
        /* +0x00 */ double m_time;           // expiry: game time + 10 s, restamped on each shot
        /* +0x08 */ Vec3 m_v8;               // the shotsFired Vec3 argument [UNRESOLVED role]
        /* +0x14 */ Vec3 m_v20;              // seeded from it too; refreshed conditionally [UNRESOLVED]
        /* +0x20 */ int m_playerId;          // the shotsFired int; the search/removal key
        /* +0x24 */ SquadMember* m_member;   // getSquadMember(playerId, index), borrowed
        /* +0x28 */ Vec3 m_v40;              // shooter position at the latest shot [UNRESOLVED role]
        /* +0x34 */ Vec3 m_position;         // shooter position at the first shot; the 5 m anchor
        /* +0x40 */ int m_i64;               // 1, or the shooting bot's slot +0x80 answer [UNRESOLVED]
    };

    SquadCoordinator(int side, int squadIndex);

    void update(double time);                    // complete order
    void updateSquadPosition();                  // complete
    void updateShotsFired();                     // complete
    void updateRepairRequest(double time);       // PE inlined at ; complete
    void updateNeedOrdersMessage(double time);   // radio-enqueue seam
    void updateIncomingMessages();               // radio-enqueue seam
    bool isSquadTakingFlag();                    // complete
    void updateSquadManeuver();                  // selected maneuver seams
    int chooseSquadManeuver();                   // integer payload, enum spelling unresolved
    void updateSquadPaths();                     // complete invalidation loop
    int getSide();
    int getMissionStatus();                       // integer payload, enum spelling unresolved
    bool isSquadAssignedToArea(int areaIndex);
    int getStrategicStrength(StrategicStrengthType type); // nonnull vehicle seam
    void getSquadPosition(Vec3& position);       // inferred void
    SquadMember* getSquadLeader();
    int getNumberOfSquadMembers();
    int getSquadId();
    bool squadHasPosition();
    SquadFormation* getSquadFormation();         // borrowed
    const Vec3& getSquadGotoPos();               // borrowed, return spelling inferred
    void addSquadMember(IAIPlayer* player);
    void updateSquadLeader();                   // change seam
    void unAssignSquad();                       // PE inlined at
    SquadMember* getSquadMember(int index);
    SquadMember* getSquadMember(int playerId, int& index);
    bool hasMemberWithKit(KitType type);
    bool isSquadAssignedToAnyArea();
    AIStrategicArea* getSquadStrategicAreaAssignment(); // borrowed
    void resetNeedOrderMessageTimer(double now);

    // layout, on the ELF, read store by store out of the constructor
    // The [UNRESOLVED] gaps hold position and claim nothing; the constructor never writes them.
    /*   +0 */ std::vector<SquadMember*> m_squadMembers;   // `getSquadMember` indexes it, unchecked
    /*  +12 */ std::list<SquadMemberFire*> m_list12;       // owned fire records; see SquadMemberFire
    /*  +16 */ int m_squadIndex;               // = the constructor's second argument
    /*  +20 */ int m_side;                     // = the first argument; `getSide` returns it
    /*  +24 */ AIStrategicArea* m_strategicAreaAssignment; // [INFERRED] name; ELF +0x18 / PE +0x28, borrowed; initially null
    /*  +28 */ Vec3 m_v28;                     // zeroed [UNRESOLVED]
    /*  +40 */ int m_i40;                      // = 4 [UNRESOLVED]
    /*  +44 */ int m_i44;                      // = 0 [UNRESOLVED]
    /*  +48 */ double m_d48;                   // = -20.0 a "never happened" sentinel, not a time
    /*  +56 */ char m_gap56[4];                // never written by the constructor
    /*  +60 */ int m_i60;                      // = 0 [UNRESOLVED]
    /*  +64 */ Vec3 m_v64;                     // zeroed [UNRESOLVED]
    /*  +76 */ int m_i76;                      // = 4 [UNRESOLVED] - the twin of m_i40
    /*  +80 */ int m_i80;                      // = 0 [UNRESOLVED]
    /*  +84 */ char m_gap84[12];               // never written by the constructor
    /*  +96 */ int m_i96;                      // leader member index, initially -1
    /* +100 */ SquadFormation* m_formation;    // new SquadFormation(this)
    /* +104 */ std::list<int> m_list104;       // header only; 4-byte element [UNRESOLVED]
    /* +108 */ Vec3 m_v108;                    // zeroed twice - once before the formation, once after
    /* +120 */ int m_i120;                     // = 0 [UNRESOLVED]
    /* +124 */ double m_d124;                  // = 0.0 [UNRESOLVED]
    /* +132 */ double m_d132;                  // = 0.0 [UNRESOLVED]
    /* +140 */ double m_d140;                  // = 0.0 [UNRESOLVED]
    /* +148 */ int m_i148;                     // selected maneuver payload; initially 0
    /* +152 */ int m_i152;                     // = 0 [UNRESOLVED]
    /* +156 */ bool m_b156;                    // = false [UNRESOLVED]
    /* +157 */ bool m_b157;                    // = false [UNRESOLVED]
    /* +160 */ std::vector<Vec3> m_positions;  // seeded with two zero vectors - see the header
    // Repair request state: position, a 60-second game-time deadline, and confirmation flag.
    // Expiration clears both request and confirmation.
    /* +172 */ bool m_b172;                    // = false; isRepairRequested
    /* +176 */ Vec3 m_v176;                    // zeroed; getRepairRequestPos returns its address
    /* +188 */ double m_d188;                  // request deadline; the constructor never writes it
    /* +196 */ bool m_b196;                    // = false; haveConfirmedRepairRequest
    /* +200 */ Vec3 m_v200;                    // zeroed twice, like m_v108
    /* +212 */ double m_needOrderMessageTimer; // = IBFTimer::getGameTime at construction
    /* +220 */ bool m_b220;                    // = false [UNRESOLVED]
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_SQUADCOORDINATOR_H
