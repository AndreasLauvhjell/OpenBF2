// Owns squad coordinators and side-specific squad state.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOT_SQUADMANAGER_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_SQUADMANAGER_H

#include <list>
#include <map>
#include <vector>
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class SquadCoordinator;   // (`operator new(0xE0)`); held only by pointer here
class SquadMember;
class IAIPlayer;
// Attested enum name in both squad-strength ELF manglings. Enumerator names are not recovered.
enum StrategicStrengthType : int;

class SquadManager
{
public:
    SquadManager();

    // Direct AIMain scheduling call, double by value; neither image materializes a result.
    void update(double time);
    void updateArtilleryStatus(double time);
    void updateCommanderMessages();
    void updateHumanPlayers(double time);   // bounded
    void setPlayerOnValidPosition(IAIPlayer* player, bool valid, double time);

    // player id then one-based side. All two callers in
    // each image discard the result; neither body materializes one independently of its last
    // updateSquadLeader call. void is the recovered no-result contract, not decompiler EAX.
    // The relocation branch retains a named stop before removeSquadMember's first mutation.
    void addSquadMember(int playerId, int side);

    // Engine notifications use one-based side and squad ids. These bodies materialize no
    // result; the engine/courier callers discard it on both ports.
    void newSquadLeader(int side, int squadId);
    void playerJoinsSquad(int playerId, int squadId, int side);
    SquadMember* getSquadMember(int playerId, int side);
    SquadCoordinator* getSquadFromPlayer(int playerId, int side);
    int getSquadStrengths(int side, int areaIndex, StrategicStrengthType type);

    // Clears messages and position records by erasing each container node individually.
    void clearCommanderMessages();
    void clearPlayerValidPositions();

    static SquadManager* m_instance;

    // Queued commander message. Field and enumerator names are inferred.
    // Native dueTime offset/record size: ELF +0xC/20 bytes, PE +0x10/24 bytes.
    enum CommanderMessageType : int;
    class CommanderMessage
    {
    public:
        CommanderMessageType m_type;
        int m_squadIndex;
        int m_sideIndex;
        double m_dueTime;
    };

    // Three Vec3 values, not containers: incomingArtillery
    // and setValidArtilleryFleePos store their scalar components.
    // [INFERRED] field names. ELF offsets below; PE offsets are 0,8,16,32,40,41,44,56.
    // Only the three positions are zeroed here. The manager tail clears m_b0; both doubles
    // and the two validity flags remain unwritten by retail construction.
    class SideBlock
    {
    public:
        /*  +0 */ bool m_b0;
        /*  +4 */ double m_artilleryStartTime;
        /* +12 */ Vec3 m_artilleryPosition{};
        /* +24 */ double m_artilleryDelay;
        /* +32 */ bool m_validFlee0;
        /* +33 */ bool m_validFlee1;
        /* +36 */ Vec3 m_fleePos0{};
        /* +48 */ Vec3 m_fleePos1{};
    };

private:
    // no vtable. The constructor stores no vptr on any path, so this class has no virtuals at
    // all - checked before writing, because a missing vptr shifts every offset below by four.
    /*   0 */ std::vector<SquadCoordinator*> m_coordinators[2]; // ELF stride 12 / PE stride 16
    /*  24 */ int  m_squadCursor;         // = 0; [INFERRED] name, update uses unsigned comparison
    /*  28 */ int  m_sideCursor;          // = 1; [INFERRED] name, update alternates zero/nonzero
    // Per-side state blocks, each 60 bytes in the original layout. Construction clears
    // the leading flag and the same nine words in each block.
    /*  32 */ SideBlock m_sideBlock0;     // +32..+91
    /*  92 */ SideBlock m_sideBlock1;     // +92..+151
    /* 152 */ float  m_float152;          // = 40.0f (bit pattern )
    /* 156 */ double m_double156;         // = 25.0
    // with a bare `operator delete` one at a time.
    /* 164 */ std::list<CommanderMessage*> m_commanderMessages;
    // Maps player ids to their first invalid-position timestamp; a valid position removes the entry.
    /* 168 */ std::map<int, double> m_playerValidPositions;
    // The original ELF map includes its empty comparator at +176 and ends at +180.
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_SQUADMANAGER_H
