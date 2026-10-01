// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SquadCoordinator operations.

#include "SquadCoordinator.h"

#include "SquadFormation.h"
#include "SquadMember.h"
#include "BotMain.h"
#include "CommunicationAgent.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/AIMain.h"
#include "../AIStrategic/SAI.h"
#include "../AIStrategicEntity/AIStrategicArea.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"      // IBFTimer::getGameTime
#include "../../../AIInterface/BFKnowledge/BFSquad.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"

namespace dice {
namespace hfe {
namespace ai {

// [INFERRED] local helper name. PE's Vec3 subtraction helper spills all three
// deltas to float, then its length-squared helper keeps (dx*dx+dy*dy)+dz*dz wide
// through the ordered comparison. ELF keeps the subtractions wide and adds
// (dy*dy+dx*dx)+dz*dz. Only the MSVC x86 branch is runtime-validated here.
static bool maneuverMemberWithinRange(const Vec3& position, const Vec3& squadPosition)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const member = &position;
    const Vec3* const squad = &squadPosition;
    const float threshold = 8100.0f;
    float deltaX, deltaY, deltaZ;
    bool within = false;
    __asm {
        mov eax, squad
        mov edx, member
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fstp deltaY
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fstp deltaZ
        fld deltaX
        fmul st(0), st(0)
        fld deltaY
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld deltaZ
        fmul st(0), st(0)
        faddp st(1), st(0)
        fcomp threshold
        fnstsw ax
        test ah, 5
        jp notWithin
        mov within, 1
    notWithin:
    }
    return within;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(squadPosition.x) - position.x);
    const long double dy = static_cast<float>(static_cast<long double>(squadPosition.y) - position.y);
    const long double dz = static_cast<float>(static_cast<long double>(squadPosition.z) - position.z);
#else
    const long double dx = static_cast<long double>(squadPosition.x) - position.x;
    const long double dy = static_cast<long double>(squadPosition.y) - position.y;
    const long double dz = static_cast<long double>(squadPosition.z) - position.z;
#endif
#if defined(_WIN32)
    return (dx * dx + dy * dy) + dz * dz < 8100.0L;
#else
    return (dy * dy + dx * dx) + dz * dz < 8100.0L;
#endif
#endif
}

// [INFERRED] local helper name. A shooter's squared distance from where it first fired, ordered
// strictly above 25 (five metres). PE spills only dx to float and sums (dz*dz+dy*dy)+dx*dx;
// ELF keeps all three differences wide and sums (dy*dy+dx*dx)+dz*dz. Equality/NaN keep the record.
static bool shooterMovedAway(const Vec3& firePosition, const Vec3& position)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const fire = &firePosition;
    const Vec3* const member = &position;
    const float threshold = 25.0f;
    float deltaX;
    bool moved = false;
    __asm {
        mov eax, fire
        mov edx, member
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fmul st(0), st(0)
        fxch st(1)
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld deltaX
        fmul st(0), st(0)
        faddp st(1), st(0)
        fcomp threshold
        fnstsw ax
        test ah, 41h
        jnz notMoved
        mov moved, 1
    notMoved:
    }
    return moved;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(firePosition.x) - position.x);
#else
    const long double dx = static_cast<long double>(firePosition.x) - position.x;
#endif
    const long double dy = static_cast<long double>(firePosition.y) - position.y;
    const long double dz = static_cast<long double>(firePosition.z) - position.z;
#if defined(_WIN32)
    return (dz * dz + dy * dy) + dx * dx > 25.0L;
#else
    return (dy * dy + dx * dx) + dz * dz > 25.0L;
#endif
#endif
}

// [INFERRED] local helper name. The leader's squared distance to the goto position, narrowed to
// float. PE subtracts through its float-returning Vec3 operator (position - goto); ELF keeps
// goto - position wide. Both sum (dx*dx+dy*dy)+dz*dz.
static float leaderGotoDistanceSq(const Vec3& position, const Vec3& gotoPosition)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const p = &position;
    const Vec3* const g = &gotoPosition;
    float deltaX, deltaY, deltaZ, distance;
    __asm {
        mov eax, p
        mov edx, g
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fstp deltaY
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fstp deltaZ
        fld deltaX
        fmul st(0), st(0)
        fld deltaY
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld deltaZ
        fmul st(0), st(0)
        faddp st(1), st(0)
        fstp distance
    }
    return distance;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(position.x) - gotoPosition.x);
    const long double dy = static_cast<float>(static_cast<long double>(position.y) - gotoPosition.y);
    const long double dz = static_cast<float>(static_cast<long double>(position.z) - gotoPosition.z);
#else
    const long double dx = static_cast<long double>(gotoPosition.x) - position.x;
    const long double dy = static_cast<long double>(gotoPosition.y) - position.y;
    const long double dz = static_cast<long double>(gotoPosition.z) - position.z;
#endif
    return static_cast<float>((dx * dx + dy * dy) + dz * dz);
#endif
}

// [INFERRED] local helper name. Squared member distance ordered strictly below the (float)
// squared radius. PE: control point - position through its float Vec3 operator, then
// (dx*dx+dy*dy)+dz*dz; ELF: position - control point wide, then (dy*dy+dx*dx)+dz*dz.
static bool insideControlPointRadius(const Vec3& controlPoint, const Vec3& position, float radiusSq)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const c = &controlPoint;
    const Vec3* const p = &position;
    float deltaX, deltaY, deltaZ;
    bool inside = false;
    __asm {
        mov eax, c
        mov edx, p
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fstp deltaY
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fstp deltaZ
        fld deltaX
        fmul st(0), st(0)
        fld deltaY
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld deltaZ
        fmul st(0), st(0)
        faddp st(1), st(0)
        fcomp radiusSq
        fnstsw ax
        test ah, 5
        jp outside
        mov inside, 1
    outside:
    }
    return inside;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(controlPoint.x) - position.x);
    const long double dy = static_cast<float>(static_cast<long double>(controlPoint.y) - position.y);
    const long double dz = static_cast<float>(static_cast<long double>(controlPoint.z) - position.z);
    return (dx * dx + dy * dy) + dz * dz < static_cast<long double>(radiusSq);
#else
    const long double dx = static_cast<long double>(position.x) - controlPoint.x;
    const long double dy = static_cast<long double>(position.y) - controlPoint.y;
    const long double dz = static_cast<long double>(position.z) - controlPoint.z;
    return (dy * dy + dx * dx) + dz * dz < static_cast<long double>(radiusSq);
#endif
#endif
}

// [INFERRED]
// void return: the manager discards it and exits do not materialize a common result.
// Timestamp and mission-2 assignment checks precede even the empty-member return.
void SquadCoordinator::update(double time)
{
    m_d124 = time;
    if (m_i120 == 2 && !IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander())
    {
        if (m_i40 == 0)
        {
            if (m_strategicAreaAssignment &&
                m_strategicAreaAssignment->getStatus(m_side + 1) != 0)
            {
                if (!m_strategicAreaAssignment->isAreaTakeable())
                {
                    // Both the main singleton and side are fresh after the area callback;
                    // the second side read is later still, after getSAI has returned.
                    SAI* const sai = IAIMain::m_instance->getSAI(m_side + 1);
                    if (!sai->ownAllTakeableCps(m_side + 1))
                        unAssignSquad();
                }
            }
            else if (m_strategicAreaAssignment &&
                     m_strategicAreaAssignment->getStatus(m_side + 1) == 0)
            {
                unAssignSquad();
            }
        }
        else if (m_i40 == 1 && m_strategicAreaAssignment &&
                 m_strategicAreaAssignment->getStatus(m_side + 1) == 0)
        {
            // Opposite one-based side is captured before getSAI; it is not reloaded
            // after that callback, unlike the ownAllTakeableCps argument above.
            const int otherSide = (m_side == 0) + 1;
            if (IAIMain::m_instance->getSAI(m_side + 1)->haveAnyOwnedAndTakeableCps(otherSide))
                unAssignSquad();
        }
    }

    if (m_squadMembers.empty())
        return;

    // Both images reload the member vector after each call, without a null-member guard.
    for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
        m_squadMembers[index]->updateFormationStatus();
    m_formation->updateClosestSlots();

    // The second live loop forwards the original double, not the mutable stored time.
    // It is skipped if the preceding callbacks emptied the vector; maneuver still follows.
    for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
        m_squadMembers[index]->update(time);

    updateSquadManeuver();

    m_formation->update(time);
    updateSquadPosition();
    updateShotsFired();
    // Each timed tail receives the original double argument, not the stored m_d124.
    updateRepairRequest(time);
    updateNeedOrdersMessage(time);
    updateIncomingMessages();
}

// [INFERRED] Updates the squad position from its living anchor, otherwise its first
// living member. Retains the old position when nobody is alive. Anchor/leader indices
// must be valid; lookups are unchecked and position-validity results are ignored.
void SquadCoordinator::updateSquadPosition()
{
    Vec3 position = {};
    const int anchorIndex = m_formation->getSquadMemberIndexAtSlot(0);
    SquadMember* member = anchorIndex == -1 ? getSquadLeader() : m_squadMembers[anchorIndex];
    if (!member->getPlayer()->getIsAlive())
    {
        member = 0;
        for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
        {
            if (m_squadMembers[index]->getPlayer()->getIsAlive())
            {
                member = m_squadMembers[index];
                break;
            }
        }
    }
    if (member)
        member->getPosition(position);
    else
        position = m_v108;
    m_v108 = position;
}

// [INFERRED] Removes fire records with invalid shooter positions. Expired records or
// shooters more than five metres away also clear member paths and the movement flag.
// Reuses the position local, deletes records before nodes, and re-reads the list end.
void SquadCoordinator::updateShotsFired()
{
    Vec3 position = {};
    for (std::list<SquadMemberFire*>::iterator it = m_list12.begin(); it != m_list12.end(); )
    {
        SquadMemberFire* const fire = *it;
        if (!fire->m_member->getPosition(position))
        {
            delete *it;
            m_list12.erase(it++);
        }
        else if (m_d124 > fire->m_time || shooterMovedAway(fire->m_position, position))
        {
            delete *it;
            m_list12.erase(it++);
            // Not updateSquadPaths: this inline walk has no null-member test.
            for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
                m_squadMembers[index]->setHavePath(false);
            m_formation->updateMovementFlag(true);
        }
        else
            ++it;
    }
}

// Strict ordered expiry clears the request and
// its confirmation together; the position and deadline are left as they were.
void SquadCoordinator::updateRepairRequest(double time)
{
    if (m_b172 && time > m_d188)
    {
        m_b172 = false;
        m_b196 = false;
    }
}

// Requests orders for an idle bot squad leader after twenty seconds without orders.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding SquadCoordinator operation.
void SquadCoordinator::updateNeedOrdersMessage(double time)
{
    if (IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander() && !isSquadTakingFlag())
    {
        const int orderState = m_i40;
        if (orderState == 0 || orderState == 2 || orderState == 4)
        {
            SquadMember* const leader = getSquadLeader();
            float distanceSq = 1000.0f;
            Vec3 position = {};
            if (leader)
            {
                if (leader->getPosition(position))
                    distanceSq = leaderGotoDistanceSq(position, m_v28);
                if (leader->isBot() && !leader->isMoving() &&
                    leader->getPlayer()->getIsAlive() && !leader->getPlayer()->isManDown() &&
                    (distanceSq < 225.0f ||
                     (m_strategicAreaAssignment &&
                      m_strategicAreaAssignment->getStatus(m_side + 1) == 0)))
                {
                    // The one path that keeps the running timer.
                    if (!(time > m_needOrderMessageTimer + 20.0))
                        return;
                    static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad)
                        ->requestNewCommanderOrder(m_squadIndex + 1, leader->getPlayer());
                    CommunicationAgent* const radio = leader->getBot()->getCommunicationAgent();
                    (void)radio;
                    BF2_AI_FRONTIER("dice::hfe::ai::SquadCoordinator::updateNeedOrdersMessage (CommunicationAgent::enqueueRadioMessage)");
                }
            }
        }
    }
    resetNeedOrderMessageTimer(time);
}

// Routes a human leader's call to one bot when the formation has left the leader.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding SquadCoordinator operation.
void SquadCoordinator::updateIncomingMessages()
{
    SquadMember* const leader = getSquadLeader();
    if (!leader || leader->isBot())
        return;
    if (!m_formation->shouldLeaveSquadLeader())
        return;

    bool found = false;
    SquadMember* member = 0;
    for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
    {
        member = m_squadMembers[index];
        if (member->isBot() && member->getPlayer()->getIsAlive())
        {
            found = true;
            break;
        }
    }
    if (!found)
        return;

    CommunicationAgent* const radio = member->getBot()->getCommunicationAgent();
    const std::list<AIRadioMessageInfo>& messages =
        radio->getReceivedMessages(static_cast<AIRadioMessageType>(12));
    if (messages.empty())
        return;
    if (messages.front().m_senderPlayerId != leader->getPlayerId())
        return;

    m_formation->joinSquadLeader();
    BF2_AI_FRONTIER("dice::hfe::ai::SquadCoordinator::updateIncomingMessages (CommunicationAgent::enqueueRadioMessage)");
}

// Returns whether a living, standing squad member is inside the assigned control point
// for an area this side does not own. Requires a valid member position and a successful
// control-point query; squares the radius in float.
bool SquadCoordinator::isSquadTakingFlag()
{
    bool taking = false;
    if (m_strategicAreaAssignment && m_strategicAreaAssignment->getStatus(m_side + 1) != 0)
    {
        Vec3 controlPoint = {};
        Vec3 position = {};
        float radius;
        if (m_strategicAreaAssignment->getControlPointPosAndRadius(controlPoint, radius))
        {
            radius = radius * radius;
            for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
            {
                SquadMember* const member = m_squadMembers[index];
                if (member->getPlayer()->getIsAlive() && !member->getPlayer()->isManDown() &&
                    member->getPosition(position) &&
                    insideControlPointRadius(controlPoint, position, radius))
                {
                    taking = true;
                    break;
                }
            }
        }
    }
    return taking;
}

// [INFERRED] void result: discarded by update, no common return materialization.
void SquadCoordinator::updateSquadManeuver()
{
    m_i148 = chooseSquadManeuver();
    switch (m_i148)
    {
    case 0:
        for (unsigned int index = 0; index < m_squadMembers.size(); ++index)
        {
            SquadMember* const member = m_squadMembers[index];
            if (member->isBot())
                member->getBot()->setAllowFire(true);
        }
        break;
    case 1:
        // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
        BF2_AI_FRONTIER("dice::hfe::ai::SquadCoordinator::updateSquadManeuver (squadManeuverAlternateFire)");
    case 2:
        // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
        BF2_AI_FRONTIER("dice::hfe::ai::SquadCoordinator::updateSquadManeuver (squadManeuverStaticFlank)");
    }
}

// Return carries the full integer selector; the source enum name remains unknown.
int SquadCoordinator::chooseSquadManeuver()
{
    int firingMembers = 0;
    Vec3 position = {};
    const unsigned int count = static_cast<unsigned int>(m_squadMembers.size());
    for (unsigned int index = 0; index < count; ++index)
    {
        SquadMember* const member = m_squadMembers[index];
        if (member->isBot() && member->getPlayer()->getIsAlive())
        {
            Bot* const bot = member->getBot();
            // [PLATFORM] ELF captures the manager before the equipment callback.
            // PE loads it afterward, and only when the low handle word is nonzero.
#if defined(_WIN32)
            const unsigned int handle = bot->getCurrentEquipment();
            Information* const information = (handle & 0xffffu)
                ? informationManager->getInformation(handle) : nullptr;
#else
            InformationManager* const manager = informationManager;
            const unsigned int handle = bot->getCurrentEquipment();
            Information* const information = manager->getInformation(handle);
#endif
            if (bot->isFiring())
            {
                // No information-null guard exists in either image.
                const unsigned int flags = information->getObjectTypeFlags();
                if (!(flags & 0x10u) && !(flags & 0x20u) && !(flags & 0x80u) &&
                    member->getPosition(position) &&
                    maneuverMemberWithinRange(position, m_v108))
                    ++firingMembers;
            }
        }
    }
    if (firingMembers > 1)
        return 1;
    if (m_i120 != 2)
        return 0;

    Vec3 target = m_v28;
    (void)target;
    // The remaining terrain-altitude/distance/assignment policy is evidenced but
    // not implemented here. Preserve its entry before any height call or flag store.
    // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
    BF2_AI_FRONTIER("dice::hfe::ai::SquadCoordinator::chooseSquadManeuver (terrain altitude and flank policy)");
}

// [INFERRED] void return, as above.
// Null entries are skipped; a live unsigned index walk invalidates each real member.
void SquadCoordinator::updateSquadPaths()
{
    for (std::size_t index = 0; index < m_squadMembers.size(); ++index)
    {
        SquadMember* const member = m_squadMembers[index];
        if (member)
            member->setHavePath(false);
    }
}

// Borrowed formation pointer, unchanged.
SquadFormation* SquadCoordinator::getSquadFormation()
{
    return m_formation;
}

// Return the address of the stored
// destination. [INFERRED] const-reference spelling; callers read the borrowed Vec3.
const Vec3& SquadCoordinator::getSquadGotoPos()
{
    return m_v28;
}

// Full-width field read; the original
// return enum spelling is unresolved, so int records the observed payload without new labels.
int SquadCoordinator::getMissionStatus()
{
    return m_i120;
}

// The assignment is borrowed and nullable.
bool SquadCoordinator::isSquadAssignedToArea(int areaIndex)
{
    return m_strategicAreaAssignment && m_strategicAreaAssignment->getIndex() == areaIndex;
}

// Each qualifying
// live member needs an IAIObject information-handle query before any strength can be added.
int SquadCoordinator::getStrategicStrength(StrategicStrengthType type)
{
    float total = 0.0f;
    for (std::size_t index = 0; index < m_squadMembers.size(); ++index)
    {
        IAIPlayer* const player = m_squadMembers[index]->getPlayer();
        if (!player->getIsAlive() || player->isManDown())
            continue;
        IAIObject* const vehicle = player->getCurrentVehicle();
        if (!vehicle)
            continue;

        // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
        // Both ports next call vehicle's unknown slot (ELF +0xB8 / PE +0xB4), passing
        // m_side + 1, then resolve its unsigned handle and sum unit strategic strengths[type].
        // ELF captures informationManager before that call; PE captures it afterwards for a
        // nonzero low handle word. Stop before the untyped call; do not invent an owner ABI.
        BF2_AI_FRONTIER("dice::hfe::ai::SquadCoordinator::getStrategicStrength (IAIObject information query)");
    }
    // The shipped empty/filtered paths reach this rounding tail with the unchanged zero sum.
    // Every possible contributor above still stops before its unrecovered object query.
    return static_cast<int>(total + 0.5f);
}

// Stores side and squad identity before constructing the formation, which queries this coordinator.

SquadCoordinator::SquadCoordinator(int side, int squadIndex)
    : m_squadMembers()
    , m_list12()
    , m_squadIndex(squadIndex)
    , m_side(side)
    , m_strategicAreaAssignment(0)
    , m_i40(4)
    , m_i44(0)
    , m_d48(-20.0)
    , m_i60(0)
    , m_i76(4)
    , m_i80(0)
    , m_i96(-1)
    , m_list104()
    , m_i120(0)
    , m_d124(0.0)
    , m_d132(0.0)
    , m_d140(0.0)
    , m_i148(0)
    , m_i152(0)
    , m_b156(false)
    , m_b157(false)
    , m_positions()
    , m_b172(false)
    , m_b196(false)
    , m_b220(false)
{
    m_v28.x = 0.0f;
    m_v28.y = 0.0f;
    m_v28.z = 0.0f;
    m_v64.x = 0.0f;
    m_v64.y = 0.0f;
    m_v64.z = 0.0f;
    m_v108.x = 0.0f;
    m_v108.y = 0.0f;
    m_v108.z = 0.0f;
    m_v176.x = 0.0f;
    m_v176.y = 0.0f;
    m_v176.z = 0.0f;
    m_v200.x = 0.0f;
    m_v200.y = 0.0f;
    m_v200.z = 0.0f;

    m_formation = new SquadFormation(this);

    m_v108.x = 0.0f;
    m_v108.y = 0.0f;
    m_v108.z = 0.0f;

    // two of them, pushed one after the other. See the header.
    Vec3 zero;
    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    m_positions.push_back(zero);
    m_positions.push_back(zero);

    m_v200.x = 0.0f;
    m_v200.y = 0.0f;
    m_v200.z = 0.0f;

    resetNeedOrderMessageTimer(
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime());

    m_b220 = false;
}

int SquadCoordinator::getSide()
{
    return m_side;
}

// Copy the stored squad position
// (retail ELF +0x6C / PE +0x8C), without testing squadHasPosition. [INFERRED]
// void return: both callers discard it; EAX holds different scratch values per port.
void SquadCoordinator::getSquadPosition(Vec3& position)
{
    position = m_v108;
}

// The stored leader index is
// used directly, including the absence of an empty-vector or negative-index guard.
SquadMember* SquadCoordinator::getSquadLeader()
{
    return m_squadMembers[m_i96];
}

// Signed count consumed by the manager's comparisons.
int SquadCoordinator::getNumberOfSquadMembers()
{
    return static_cast<int>(m_squadMembers.size());
}

// non-const in the mangled signature.
int SquadCoordinator::getSquadId()
{
    return m_squadIndex;
}

// Zero-distance test with 0.001f, including true for NaN.
// PE sums z squared, y squared, then x squared. No square root or component-wise epsilon.
bool SquadCoordinator::squadHasPosition()
{
    const float x = -m_v108.x;
    const float y = -m_v108.y;
    const float z = -m_v108.z;
    return !(z * z + y * y + x * x < 0.001f);
}

// PE inlined at : four stores in this order.
void SquadCoordinator::unAssignSquad()
{
    m_strategicAreaAssignment = 0;
    m_i40 = 4;
    m_i44 = 0;
    m_i120 = 0;
}

// Allocation is not checkPointer-wrapped. The human arm
// queries the one-based side's commander before publishing the member in the vector.
void SquadCoordinator::addSquadMember(IAIPlayer* player)
{
    SquadMember* member = new SquadMember(this, player, m_squadIndex, m_side,
                                         static_cast<int>(m_squadMembers.size()));
    if (!member->isBot())
    {
        if (!IAIMain::m_instance->getSAI(m_side + 1)->isPlayerCommander())
            unAssignSquad();
    }
    m_squadMembers.push_back(member);
    if (member->isAllowedInFormation())
    {
        m_formation->joinFormation(member->getMemberIndex());
        m_formation->updateMovementFlag(true);
    }
    member->updateMemberPos();
}

// Stop on the first engine-recognized leader; later members
// are not visited. No found leader leaves the old index intact. A first leader skips the
// change arm, but the single-member trailing-position reset still runs.
void SquadCoordinator::updateSquadLeader()
{
    const int oldLeader = m_i96;
    bool found = false;
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
    {
        m_squadMembers[i]->setIsSquadLeader(
            static_cast<IBFSquad*>(CommonSingletonServer::m_iBFSquad)->isSquadLeader(
                m_squadIndex + 1, m_squadMembers[i]->getPlayer()));
        if (m_squadMembers[i]->isSquadLeader())
        {
            m_i96 = static_cast<int>(i);
            found = true;
            break;
        }
    }
    if (!found)
        return;

    if (oldLeader != m_i96 && oldLeader != -1 && !m_squadMembers.empty())
    {
        m_squadMembers[m_i96]->updateMemberPos();
        // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
        // Next: formation->joinSquadLeader, updateSquadPosition, updateSquadPaths
        // then the human-leader/commander policy. Those movement bodies remain unrecovered.
        BF2_AI_FRONTIER("dice::hfe::ai::SquadCoordinator::updateSquadLeader (SquadFormation::joinSquadLeader)");
    }
    if (m_squadMembers.size() == 1)
        m_formation->resetTrailingPositions();
}

// - one indexed load through the vector's data pointer.
// no bounds check. Same shape as `BotManager::getBot`, and the same reason to leave it alone.
SquadMember* SquadCoordinator::getSquadMember(int index)
{
    return m_squadMembers[index];
}

// First matching player wins. The output index is written
// only on success, and the vector/member is fetched again after the player-id call.
SquadMember* SquadCoordinator::getSquadMember(int playerId, int& index)
{
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
    {
        if (m_squadMembers[i]->getPlayerId() == playerId)
        {
            index = static_cast<int>(i);
            return m_squadMembers[i];
        }
    }
    return 0;
}

// Skip null members, but do not guard their player.
// Compare the complete returned integer; no alive/bot/leader filter exists. Both images
// refresh the member vector after a nonmatching virtual getSpawnKit callback.
bool SquadCoordinator::hasMemberWithKit(KitType type)
{
    for (unsigned int i = 0; i < m_squadMembers.size(); ++i)
    {
        if (m_squadMembers[i] && m_squadMembers[i]->getPlayer()->getSpawnKit() == type)
            return true;
    }
    return false;
}

// Only pointer presence is tested;
// there is no area validity, membership, side or mission-state query.
bool SquadCoordinator::isSquadAssignedToAnyArea()
{
    return m_strategicAreaAssignment != 0;
}

// Return the stored pointer unchanged.
// ELF callers use it directly as AIStrategicArea::getRouteList/orderNormalBot's this;
// the paired kit callers read its area index at +0xF0. No ownership is transferred.
AIStrategicArea* SquadCoordinator::getSquadStrategicAreaAssignment()
{
    return m_strategicAreaAssignment;
}

// - one store of a double.
void SquadCoordinator::resetNeedOrderMessageTimer(double now)
{
    m_needOrderMessageTimer = now;
}

} // namespace ai
} // namespace hfe
} // namespace dice
