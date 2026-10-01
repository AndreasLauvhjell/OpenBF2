// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SquadMember operations.

#include "SquadMember.h"

#include "../Main/AIDllFrontier.h"
#include "../Main/IBotManager.h"
#include "BotMain.h"
#include "SquadCoordinator.h"
#include "SquadFormation.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"

namespace dice {
namespace hfe {
namespace ai {

// [INFERRED] local helper names. The update pair compares the x87 sum directly;
// spilling previous+9 to double first would change the boundary at large times.
static bool memberPathUpdateDue(double previous, double now)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const double interval = 9.0;
    bool due = false;
    __asm {
        fld previous
        fadd interval
        fcomp now
        fnstsw ax
        test ah, 5
        jp notDue
        mov due, 1
    notDue:
    }
    return due;
#else
    return static_cast<long double>(now) > static_cast<long double>(previous) + 9.0L;
#endif
}

// [PLATFORM] PE spills only dx, then sums (dz*dz+dx*dx)+dy*dy. ELF retains
// all three deltas wide and sums (dx*dx+dy*dy)+dz*dz. Compare before narrowing.
// MSVC x86 is the validated target; the portable long-double arm is not an ELF
// runtime or x64-equivalence claim.
static bool memberPathDisplaced(const Vec3& previous, const Vec3& current)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const oldPosition = &previous;
    const Vec3* const newPosition = &current;
    const float threshold = 0.01f;
    float deltaX;
    bool displaced = false;
    __asm {
        mov eax, oldPosition
        mov edx, newPosition
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fmul st(0), st(0)
        fld deltaX
        fmul st(0), st(0)
        faddp st(1), st(0)
        fxch st(1)
        fmul st(0), st(0)
        faddp st(1), st(0)
        fcomp threshold
        fnstsw ax
        test ah, 41h
        jnz notDisplaced
        mov displaced, 1
    notDisplaced:
    }
    return displaced;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(previous.x) - current.x);
#else
    const long double dx = static_cast<long double>(previous.x) - current.x;
#endif
    const long double dy = static_cast<long double>(previous.y) - current.y;
    const long double dz = static_cast<long double>(previous.z) - current.z;
#if defined(_WIN32)
    return (dz * dz + dx * dx) + dy * dy > static_cast<long double>(0.01f);
#else
    return (dx * dx + dy * dy) + dz * dz > static_cast<long double>(0.01f);
#endif
#endif
}

// Preserve the stores around the two virtual calls: publishing
// this member to its bot happens before request/position flags are initialized. The Vec3 members
// are cleared before the body and again later. There is no null player/manager guard.
SquadMember::SquadMember(SquadCoordinator* squad, IAIPlayer* player,
                         int squadIndex, int side, int memberIndex)
    : m_position{0.0f, 0.0f, 0.0f}
    , m_vector54{0.0f, 0.0f, 0.0f}
    , m_vector60{0.0f, 0.0f, 0.0f}
{
    m_player = player;
    m_playerId = m_player->getId();
    m_squadIndex = squadIndex;
    m_isSquadLeader = false;
    m_memberIndex = memberIndex;
    m_squad = squad;
    m_maxSpeed = 0.0f;
    m_side = side;
    m_estimatedArrival = 1.0f;

    Bot* const bot = IBotManager::instance->getBotFromPlayerId(m_playerId);
    if (bot)
    {
        m_bot = bot;
        m_bot->setSquadMember(this);
    }
    else
    {
        m_bot = 0;
    }

    m_flag30 = false;
    m_havePath = false;
    m_request = new RequestState();
    m_request->m_action = 0;
    m_int38 = 0;
    m_position = {0.0f, 0.0f, 0.0f};
    m_positionValid = false;
    m_flag49 = false;
    m_flag4A = false;
    m_flag4B = false;
    m_formationDistanceSq = 900.0f;
    m_moved = false;
    m_time24 = 0.0;
    m_vector54 = {0.0f, 0.0f, 0.0f};
    m_vector60 = {0.0f, 0.0f, 0.0f};
}

// only these two stores. Retail does not free m_request or
// clear the bot's back-pointer here. Preserve both omissions rather than inventing ownership.
SquadMember::~SquadMember()
{
    m_player = 0;
    m_bot = 0;
}

IAIPlayer* SquadMember::getPlayer() { return m_player; }

// return the stored squad pointer.
// Non-const in the mangled signature; no lookup, ownership transfer or null guard.
SquadCoordinator* SquadMember::getSquad()
{
    return m_squad;
}

int SquadMember::getPlayerId() { return m_playerId; }
bool SquadMember::isBot() { return m_bot != 0; }
// stored pointer, no lookup or guard.
Bot* SquadMember::getBot()
{
    return m_bot;
}

int SquadMember::getMemberIndex() { return m_memberIndex; }
void SquadMember::setIsSquadLeader(bool leader) { m_isSquadLeader = leader; }
bool SquadMember::isSquadLeader() { return m_isSquadLeader; }

// the second flag receives the inverse, not a constant clear.
void SquadMember::setHavePath(bool havePath)
{
    m_havePath = havePath;
    m_flag4A = !havePath;
}

// [INFERRED] void return: coordinator discards the result and exits have no common
// return materialization. Each missing dependency stops at its real call site;
// paths which retail skips continue through mission selection and movement.
void SquadMember::update(double time)
{
    updateMemberPos();
    SquadFormation* const formation = m_squad->getSquadFormation();

    if (memberPathUpdateDue(m_time24, time))
    {
        Vec3 position = {};
        if (memberPathDisplaced(m_vector54, m_vector60))
        {
            setHavePath(false);
            m_vector54 = m_vector60;
        }
        else if (m_bot && getPosition(position))
        {
            WaypointList* const waypoints = m_bot->getWaypoints();
            (void)waypoints;
            // ELF +0x08 / PE +0x04 takes this position and the freshly read Bot.
            // Its complete owner interface is not known. The time store is later.
            // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
            BF2_AI_FRONTIER("dice::hfe::ai::SquadMember::update (WaypointList position query)");
        }
        m_time24 = time;
    }

    if (m_bot && m_player && m_player->getIsAlive())
    {
        Vec3 position = {};
        if (getPosition(position))
        {
            if (isMobile())
            {
                // The bool danger query and subsequent avoidArtillery body remain
                // unrecovered, not replaced by a fabricated no-danger decision.
                // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
                BF2_AI_FRONTIER("dice::hfe::ai::SquadMember::update (SquadManager::isInArtilleryDangerZone)");
            }
            else
            {
                m_flag4B = false;
            }
        }
    }

    if (m_bot && !m_bot->isFiring())
    {
        // [PLATFORM] PE reads the manager after getCurrentEquipment; ELF captures
        // it first. Both reload the bot after isFiring and preserve pool semantics.
#if defined(_WIN32)
        const unsigned int handle = m_bot->getCurrentEquipment();
        Information* const information = informationManager->getInformation(handle);
#else
        InformationManager* const manager = informationManager;
        const unsigned int handle = m_bot->getCurrentEquipment();
        Information* const information = manager->getInformation(handle);
#endif
        if (information && (information->getObjectTypeFlags() & 0x200000u) &&
            information->hasArmament())
        {
            // The next call is the armament's selected-index query, ELF +0x14
            // PE +0x10. Keep the full missing owner/WeaponInfo chain explicit.
            // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
            BF2_AI_FRONTIER("dice::hfe::ai::SquadMember::update (IPIArmament selected weapon)");
        }
    }

    if (m_squad->getMissionStatus() == 1)
    {
        SquadMember* member = m_squad->getSquadLeader();
        if (!member || !member->getPlayer()->getIsAlive() || member->getPlayer()->isManDown())
        {
            member = this;
            if (!m_player->getIsAlive() || m_player->isManDown())
                member = nullptr;
        }
        if (member)
        {
            // The selected member goes to acceptMission before the engine's
            // accept/decline courier. Neither response is fabricated here.
            // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
            BF2_AI_FRONTIER("dice::hfe::ai::SquadMember::update (SquadCoordinator::acceptMission)");
        }
    }

    // Formation remains the entry capture; member index is fresh after callbacks.
    const bool inFormation = formation->isInFormation(m_memberIndex);
    m_int38 = 0;
    if (inFormation)
        moveWithFormation();
    else
        moveWithoutFormation();
}

// No result
// is materialized. Capture the formation and allowance once, but query membership twice
// and reload the member index after every callback. A join/leave clears only havePath:
// setHavePath(false) would incorrectly change m_flag4A as well.
void SquadMember::updateFormationStatus()
{
    SquadFormation* const formation = m_squad->getSquadFormation();
    const bool allowed = isAllowedInFormation();
    if (formation->isInFormation(m_memberIndex) && !allowed)
    {
        formation->leaveFormation(m_memberIndex);
        m_havePath = false;
    }
    if (!formation->isInFormation(m_memberIndex) && allowed)
    {
        formation->joinFormation(m_memberIndex);
        m_havePath = false;
    }
}

// Copy all coordinates before
// returning the stored validity flag; false does not preserve the caller's old output.
bool SquadMember::getPosition(Vec3& position)
{
    position = m_position;
    return m_positionValid;
}

// one byte load, ELF +0x50 / PE +0x54.
bool SquadMember::isMoving()
{
    return m_moved;
}

// These are the complete refusal paths before vehicle information.
// The remaining body queries the vehicle at ELF +0xB8 / PE +0xB4 with m_side+1, checks its
// information/type/physical records, then compares the squad destination distance if assigned.
// IAIObject is still forward-only, so its slot signature must be recovered before crossing here.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
bool SquadMember::isAllowedInFormation()
{
    if (!m_player->getIsAlive())
        return false;
    if (m_player->isManDown())
        return false;
    if (!m_player->getCurrentVehicle())
        return false;

    BF2_AI_FRONTIER("dice::hfe::ai::SquadMember::isAllowedInFormation (IAIObject information handle)");
}

// The getter occurs before either flag is cleared. A null current
// vehicle leaves the prior position untouched. The nonnull arm needs the same unknown IAIObject
// slot, Information::getPrimaryObject(side+1), and the primary object's position/physics record.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
void SquadMember::updateMemberPos()
{
    IAIObject* const vehicle = m_player->getCurrentVehicle();
    m_positionValid = false;
    m_moved = false;
    if (!vehicle)
        return;

    BF2_AI_FRONTIER("dice::hfe::ai::SquadMember::updateMemberPos (IAIObject information handle)");
}

// Retain
// the vehicle before testing the freshly read player's alive/man-down state.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
bool SquadMember::isMobile()
{
    IAIObject* const vehicle = m_player->getCurrentVehicle();
    if (!vehicle)
        return false;
    if (!m_player->getIsAlive())
        return false;
    if (m_player->isManDown())
        return false;

    BF2_AI_FRONTIER("dice::hfe::ai::SquadMember::isMobile (IAIObject information handle)");
}

// Position validity is deliberately ignored before the formation-world-position query.
// [INFERRED] void return: callers discard it and exits materialize no common value.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
void SquadMember::moveWithFormation()
{
    SquadFormation* const formation = m_squad->getSquadFormation();
    Vec3 position = {};
    getPosition(position);
    Vec3 slotPosition = {};
    (void)formation; (void)slotPosition;
    BF2_AI_FRONTIER("dice::hfe::ai::SquadMember::moveWithFormation (SquadFormation::getSlotWorldPos)");
}

// The no-bot and dead-player returns are shipped. The alive branch ignores position
// validity, copies the goal and queries the fresh player; no null vehicle guard follows.
// [INFERRED] void return, as above.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
void SquadMember::moveWithoutFormation()
{
    if (!m_bot)
        return;
    if (!m_player->getIsAlive())
        return;

    Vec3 position = {};
    getPosition(position);
    const Vec3 goal = m_squad->getSquadGotoPos();
    IAIObject* const vehicle = m_player->getCurrentVehicle();
    (void)goal; (void)vehicle;
    BF2_AI_FRONTIER("dice::hfe::ai::SquadMember::moveWithoutFormation (IAIObject information handle)");
}

// [FRONTIER] Returns the member's vehicle radius once the information query chain is implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding SquadMember operation.
float SquadMember::getVehicleRadius()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SquadMember::getVehicleRadius");
}

} // namespace ai
} // namespace hfe
} // namespace dice
