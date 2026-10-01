// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SquadFormation operations.

#include "SquadFormation.h"

#include <list>

#include "SquadCoordinator.h"
#include "SquadMember.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"   // IAIPlayer::getCurrentVehicle
#include "../../../AIInterface/BFKnowledge/BFGame.h"     // IAIGame::getMaxNumberOfSquadMembers
#include "../../../AIInterface/BFKnowledge/BFTimer.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

// attested type name: `std::list<dice::hfe::ai::(anonymous namespace)::SquadMemberDistanceObj>`
// is instantiated from this translation unit (the anonymous namespace carries its path). Eight
// bytes, from the list's 16-byte header node. Member names are [INFERRED].
struct SquadMemberDistanceObj
{
    // Both images' merge instantiations order by the float at +4, ascending, strict; an
    // unordered pair compares false and keeps the first list's element in front.
    bool operator<(const SquadMemberDistanceObj& other) const
    {
        return m_distanceSq < other.m_distanceSq;
    }

    SquadMember* m_member;
    float m_distanceSq;
};

} // anonymous namespace

// [INFERRED] local name. Both images retain every delta/product through the
// ordered comparison. PE adds (z*z+y*y)+x*x; ELF adds (y*y+x*x)+z*z.
// Native MSVC x86 is the validated target; portable arms are not ELF/x64 proof.
static bool formationLeaderWithinRange(const Vec3& leader, const Vec3& squad)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const leaderPosition = &leader;
    const Vec3* const squadPosition = &squad;
    const float threshold = 2500.0f;
    bool within = false;
    __asm {
        mov eax, leaderPosition
        mov edx, squadPosition
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fmul st(0), st(0)
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld dword ptr [eax]
        fsub dword ptr [edx]
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
    const long double deltaX = static_cast<long double>(leader.x) - squad.x;
    const long double deltaY = static_cast<long double>(leader.y) - squad.y;
    const long double deltaZ = static_cast<long double>(leader.z) - squad.z;
#if defined(_WIN32)
    return (deltaZ * deltaZ + deltaY * deltaY) + deltaX * deltaX < 2500.0L;
#else
    return (deltaY * deltaY + deltaX * deltaX) + deltaZ * deltaZ < 2500.0L;
#endif
#endif
}

// [INFERRED] local name. Squared length ordered strictly above 0.001f; equality and NaN refuse.
// PE squares the stored components and sums (z*z+y*y)+x*x. ELF first subtracts a zero vector
// (exact) and sums (y*y+x*x)+z*z. Both keep the sum wide through the comparison.
// Native MSVC x86 is the validated target; portable arms are not ELF/x64 proof.
static bool trailPointIsSet(const Vec3& point)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const p = &point;
    const float threshold = 0.001f;
    bool set = false;
    __asm {
        mov eax, p
        fld dword ptr [eax+8]
        fmul st(0), st(0)
        fld dword ptr [eax+4]
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld dword ptr [eax]
        fmul st(0), st(0)
        faddp st(1), st(0)
        fcomp threshold
        fnstsw ax
        test ah, 41h
        jnz notSet
        mov set, 1
    notSet:
    }
    return set;
#else
    const long double x = point.x;
    const long double y = point.y;
    const long double z = point.z;
#if defined(_WIN32)
    return (z * z + y * y) + x * x > static_cast<long double>(0.001f);
#else
    return (y * y + x * x) + z * z > static_cast<long double>(0.001f);
#endif
#endif
}

// [INFERRED] local name. Squared distance between the anchor and the newest crumb, narrowed
// to float before the caller's strict 36 test. PE subtracts through its float-returning Vec3
// operator (anchor - crumb) and sums (dx*dx+dy*dy)+dz*dz; ELF keeps crumb - anchor wide and
// sums (dy*dy+dx*dx)+dz*dz. The sign of each difference cannot matter once squared.
static float trailCrumbDistanceSq(const Vec3& anchor, const Vec3& crumb)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const a = &anchor;
    const Vec3* const c = &crumb;
    float deltaX, deltaY, deltaZ, distance;
    __asm {
        mov eax, a
        mov edx, c
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
    const long double dx = static_cast<float>(static_cast<long double>(anchor.x) - crumb.x);
    const long double dy = static_cast<float>(static_cast<long double>(anchor.y) - crumb.y);
    const long double dz = static_cast<float>(static_cast<long double>(anchor.z) - crumb.z);
    return static_cast<float>((dx * dx + dy * dy) + dz * dz);
#else
    const long double dx = static_cast<long double>(crumb.x) - anchor.x;
    const long double dy = static_cast<long double>(crumb.y) - anchor.y;
    const long double dz = static_cast<long double>(crumb.z) - anchor.z;
    return static_cast<float>((dy * dy + dx * dx) + dz * dz);
#endif
#endif
}

// [INFERRED] local name. A claimed member's squared distance to the anchor, stored as the sort
// key. PE spills only dx to float and sums (dz*dz+dy*dy)+dx*dx; ELF keeps all three
// differences wide and sums (dy*dy+dx*dx)+dz*dz. Both narrow the sum to float for the key.
static float trailMemberDistanceSq(const Vec3& position, const Vec3& anchor)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const p = &position;
    const Vec3* const a = &anchor;
    float deltaX, distance;
    __asm {
        mov eax, p
        mov edx, a
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
        fstp distance
    }
    return distance;
#else
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(position.x) - anchor.x);
#else
    const long double dx = static_cast<long double>(position.x) - anchor.x;
#endif
    const long double dy = static_cast<long double>(position.y) - anchor.y;
    const long double dz = static_cast<long double>(position.z) - anchor.z;
#if defined(_WIN32)
    return static_cast<float>((dz * dz + dy * dy) + dx * dx);
#else
    return static_cast<float>((dy * dy + dx * dx) + dz * dz);
#endif
#endif
}

// [INFERRED] Runs the formation helpers, then advances a signed six-tick cycle: tick 3
// selects a formation and tick 6 resets the counter and applies it. [PLATFORM] The PE
// inlines selection; ELF calls it. Both use the same integer policy.
void SquadFormation::update(double time)
{
    updateSlotZeroPos();
    updateMovementFlag(false);
    updateFormationMaxSpeed();
    updateFormation();
    updateWaitForSquadMembers(time);
    updateFormationForwardVector();
    updateTrailingPositions();
    updateTrailingSlots();

    if (++m_i88 > 5)
    {
        m_i88 = 0;
        setFormation();
    }
    else if (m_i88 == 3)
        selectFormation();
}

// Selects the line formation unconditionally. The next cycle wrap applies it, replacing
// any shape selected by changeFormation.
void SquadFormation::selectFormation()
{
    m_formation = FormationLine;
}

// Updates the formation direction from its anchor member. An empty anchor returns immediately.
// [FRONTIER] The occupied-anchor path requires the remaining information queries.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding SquadFormation operation.
void SquadFormation::updateFormationForwardVector()
{
    Vec3 speed = {};
    Slot* const anchor = m_slots[0];
    if (!anchor->m_occupied)
        return;

    IAIPlayer* const player = m_coordinator->getSquadMember(anchor->m_memberIndex)->getPlayer();
    IAIObject* const vehicle = player->getCurrentVehicle();
    (void)speed;
    (void)vehicle;
    BF2_AI_FRONTIER("dice::hfe::ai::SquadFormation::updateFormationForwardVector (IAIObject information query)");
}

// Adds a trail position when the anchor moves more than six metres from the last position.
void SquadFormation::updateTrailingPositions()
{
    Slot* const anchor = m_slots[0];
    if (!trailPointIsSet(m_v72))
        return;
    if (!trailPointIsSet(m_trailingPosition))
    {
        m_trailingPosition = m_v72;
        return;
    }
    if (!anchor->m_occupied)
        return;

    const float distanceSq = trailCrumbDistanceSq(m_v72, m_trailingPosition);
    SquadMember* const member = m_coordinator->getSquadMember(anchor->m_memberIndex);
    (void)member;
    if (!(distanceSq > 36.0f))
        return;

    for (unsigned int index = m_maxSquadMembers - 1; index != 0; --index)
    {
        // Two statements: both images reload the vector's storage after the byte store.
        m_slotTargets[index].first = m_slotTargets[index - 1].first;
        m_slotTargets[index].second = m_slotTargets[index - 1].second;
    }
    m_slotTargets[0].first = true;
    m_slotTargets[0].second = m_trailingPosition;
    m_trailingPosition = m_v72;
}

// Reassigns occupied trailing slots according to distance.
void SquadFormation::updateTrailingSlots()
{
    std::list<SquadMemberDistanceObj> members;
    Vec3 position = {};

    for (unsigned int index = 0; index < m_maxSquadMembers; ++index)
    {
        Slot* const slot = m_slots[index];
        if (slot->m_i20 != -1 && slot->m_occupied)
        {
            SquadMember* const member = m_coordinator->getSquadMember(slot->m_memberIndex);
            member->getPosition(position);
            const SquadMemberDistanceObj entry = { member, trailMemberDistanceSq(position, m_v72) };
            members.push_back(entry);
        }
    }

    if (members.empty())
        return;

    members.sort();

    Slot* slot = 0;
    for (unsigned int index = 0; index < m_maxSquadMembers; ++index)
    {
        slot = m_slots[index];
        slot->m_i20 = -1;
    }

    int trailIndex = 0;
    for (std::list<SquadMemberDistanceObj>::iterator it = members.begin(); it != members.end(); ++it)
    {
        SquadMember* const member = it->m_member;
        for (unsigned int index = 1; index < m_maxSquadMembers; ++index)
        {
            slot = m_slots[index];
            if (slot->m_memberIndex == member->getMemberIndex())
                break;
        }
        slot->m_i20 = trailIndex++;
    }
}

// No bounds, null or occupied test: an empty
// slot answers its stored index, -1 after construction or reset.
int SquadFormation::getSquadMemberIndexAtSlot(int slotIndex)
{
    return m_slots[slotIndex]->m_memberIndex;
}

bool SquadFormation::shouldLeaveSquadLeader()
{
    return m_b84;
}

// (, updateMovementFlag(true) inlined).
// The joined flag is set before the leave flag is cleared, in both images.
void SquadFormation::joinSquadLeader()
{
    m_b85 = true;
    m_b84 = false;
    updateMovementFlag(true);
}

// [INFERRED] void result. No slot-vector, leader or member null guards exist.
void SquadFormation::updateFormation()
{
    Slot* slot = m_slots[0];
    SquadMember* const leader = m_coordinator->getSquadLeader();
    const int leaderIndex = leader->getMemberIndex();
    const bool leaderIsBot = leader->isBot();
    bool leaderPresent = false;
    Vec3 squadPosition = {};
    m_coordinator->getSquadPosition(squadPosition);

    unsigned int leaderSlotIndex = 0;
    for (; leaderSlotIndex < m_maxSquadMembers; ++leaderSlotIndex)
    {
        slot = m_slots[leaderSlotIndex];
        if (slot->m_occupied && slot->m_memberIndex == leaderIndex)
        {
            leaderPresent = true;
            break;
        }
    }

    if (leaderPresent)
    {
        if (m_b84)
        {
            if (leaderSlotIndex != 0)
                return;
            for (unsigned int index = m_maxSquadMembers - 1; index > 0; --index)
            {
                Slot* const other = m_slots[index];
                if (!other->m_occupied)
                    continue;
                const int otherIndex = other->m_memberIndex;
                const int oldIndex = slot->m_memberIndex;
                slot->m_b24 = false;
                slot->m_i28 = 0.0f;
                slot->m_memberIndex = otherIndex;
                other->m_b24 = false;
                other->m_i28 = 0.0f;
                other->m_memberIndex = oldIndex;

                // This arm resets trailing positions before either path clear.
                resetTrailingPositions();
                m_coordinator->getSquadMember(slot->m_memberIndex)->setHavePath(false);
                m_coordinator->getSquadMember(other->m_memberIndex)->setHavePath(false);
                return;
            }
            return;
        }

        if (leaderSlotIndex == 0)
            return;
        Vec3 leaderPosition = {};
        leader->getPosition(leaderPosition); // Retail deliberately ignores validity.
        if (!formationLeaderWithinRange(leaderPosition, squadPosition) && !m_b85)
            return;

        m_b85 = false;
        Slot* const zero = m_slots[0];
        const bool oldOccupied = zero->m_occupied;
        const int oldIndex = zero->m_memberIndex;
        zero->m_occupied = true;
        zero->m_memberIndex = leaderIndex;
        zero->m_b24 = false;
        zero->m_i28 = 0.0f;
        m_coordinator->getSquadMember(leaderIndex)->setHavePath(false);

        // The old leader-slot pointer is reloaded after that callback.
        slot = m_slots[leaderSlotIndex];
        slot->m_occupied = oldOccupied;
        slot->m_memberIndex = oldIndex;
        slot->m_b24 = false;
        slot->m_i28 = 0.0f;
        // Even an unoccupied former anchor gets this unchecked member lookup.
        m_coordinator->getSquadMember(oldIndex)->setHavePath(false);
        resetTrailingPositions();
        return;
    }

    Slot* const zero = m_slots[0];
    if (!zero->m_occupied)
        return;
    if (m_coordinator->getSquadMember(zero->m_memberIndex)->isBot() == leaderIsBot)
        return;

    // Find the first occupied slot whose bot/human kind matches the captured leader.
    // The bound, vector and coordinator are fresh after each member callback.
    for (unsigned int index = 1; index < m_maxSquadMembers; ++index)
    {
        Slot* const other = m_slots[index];
        if (!other->m_occupied ||
            m_coordinator->getSquadMember(other->m_memberIndex)->isBot() != leaderIsBot)
            continue;
        const int oldIndex = zero->m_memberIndex;
        const int otherIndex = other->m_memberIndex;
        other->m_memberIndex = oldIndex;
        other->m_i28 = 0.0f;
        other->m_b24 = false;
        zero->m_memberIndex = otherIndex;
        zero->m_b24 = false;
        zero->m_i28 = 0.0f;
        m_coordinator->getSquadMember(otherIndex)->setHavePath(false);
        resetTrailingPositions();
        return;
    }
}

// [INFERRED] void result. The first slot is deliberately excluded from this scan.
void SquadFormation::updateWaitForSquadMembers(double time)
{
    m_b36 = true;
    Vec3 memberPosition = {};
    Vec3 squadPosition = {};
    (void)memberPosition;
    (void)squadPosition;
    (void)time;
    for (unsigned int index = 1; index < m_maxSquadMembers; ++index)
    {
        Slot* const slot = m_slots[index];
        if (slot->m_occupied)
        {
            // The real member lookup begins an occupied-slot timing policy with
            // platform-specific time/RNG precision; no deadline or wait is invented.
            // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
            BF2_AI_FRONTIER("dice::hfe::ai::SquadFormation::updateWaitForSquadMembers (occupied slot timing)");
        }
    }
}

// [INFERRED] local helper name. Preserve leaveFormation's x87 comparison before its
// conditional float store. [PLATFORM] PE spills only dx to float; ELF keeps all three differences
// in x87 registers. The MSVC x86 block avoids modern SSE narrowing of dy/dz and the sum.
static bool nearerFormationMember(const Vec3& position, const Vec3& destination, float& minimum)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    const Vec3* const positionPointer = &position;
    const Vec3* const destinationPointer = &destination;
    float* const minimumPointer = &minimum;
    float deltaX;
    bool nearer = false;
    __asm {
        mov eax, positionPointer
        mov edx, destinationPointer
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fstp deltaX
        fld dword ptr [eax+4]
        fsub dword ptr [edx+4]
        fmul st(0), st(0)
        fld deltaX
        fmul st(0), st(0)
        faddp st(1), st(0)
        fld dword ptr [eax+8]
        fsub dword ptr [edx+8]
        fmul st(0), st(0)
        faddp st(1), st(0)
        mov ecx, minimumPointer
        fcom dword ptr [ecx]
        fnstsw ax
        test ah, 5
        jp notNearer
        fstp dword ptr [ecx]
        mov nearer, 1
        jmp compared
    notNearer:
        fstp st(0)
    compared:
    }
    return nearer;
#else
    // Native long double retains the ELF x87 intermediate width. The separate MSVC
    // x64 port has no x87 inline assembly; it is not validated as retail x86 here.
#if defined(_WIN32)
    const long double dx = static_cast<float>(static_cast<long double>(position.x) - destination.x);
#else
    const long double dx = static_cast<long double>(position.x) - destination.x;
#endif
    const long double dy = static_cast<long double>(position.y) - destination.y;
    const long double dz = static_cast<long double>(position.z) - destination.z;
    const long double distance = (dy * dy + dx * dx) + dz * dz;
    if (distance < minimum)
    {
        minimum = static_cast<float>(distance);
        return true;
    }
    return false;
#endif
}

// Captured unsigned slot count, full signed member
// comparison, no occupied/null/vector-size test. Even an empty slot's -1 can match.
bool SquadFormation::isInFormation(int memberIndex)
{
    const unsigned int count = m_maxSquadMembers;
    for (unsigned int i = 0; i < count; ++i)
        if (m_slots[i]->m_memberIndex == memberIndex)
            return true;
    return false;
}

// Preserve offset
// reference-slot and target-vector state. [INFERRED] void return from discarded uses.
void SquadFormation::resetSlot(int slotIndex)
{
    Slot* const slot = m_slots[slotIndex];
    slot->m_occupied = false;
    slot->m_memberIndex = -1;
    slot->m_i20 = -1;
    slot->m_b24 = false;
    slot->m_i28 = 0.0f;
}

// [INFERRED]
// void return from discarded calls and inconsistent exit registers. Zero slots, null slots
// and bad member indices retain the retail missing guards. A missing member still updates
// movement; duplicate member indices stop at their first slot.
void SquadFormation::leaveFormation(int memberIndex)
{
    unsigned int removed = 0;
    for (; removed < m_maxSquadMembers; ++removed)
    {
        if (m_slots[removed]->m_memberIndex == memberIndex)
        {
            resetSlot(static_cast<int>(removed));
            break;
        }
    }

    if (removed < m_maxSquadMembers - 1 && m_slots[removed + 1]->m_occupied)
    {
        unsigned int last = m_maxSquadMembers - 1;
        while (last > 0)
        {
            Slot* const lastSlot = m_slots[last];
            if (lastSlot->m_occupied)
            {
                if (removed == 0 && last > 1)
                {
                    unsigned int selected = last;
                    float minimum = 3.402823466e+38F;
                    Vec3 position = {};
                    for (unsigned int candidate = last; candidate > 0; --candidate)
                    {
                        Slot* const slot = m_slots[candidate];
                        // Retail obtains the member even for an unoccupied candidate.
                        SquadMember* const member = m_coordinator->getSquadMember(slot->m_memberIndex);
                        if (slot->m_occupied && !member->isSquadLeader() && member->isBot() &&
                            member->getPosition(position))
                        {
                            const Vec3& destination = m_coordinator->getSquadGotoPos();
                            if (nearerFormationMember(position, destination, minimum))
                                selected = candidate;
                        }
                    }

                    Slot* destination = m_slots[0];
                    Slot* source = m_slots[selected];
                    destination->m_occupied = source->m_occupied;
                    destination->m_b24 = source->m_b24;
                    destination->m_offset = source->m_offset;
                    destination->m_memberIndex = source->m_memberIndex;
                    destination->m_i20 = source->m_i20;
                    destination->m_i28 = source->m_i28;
                    m_coordinator->getSquadMember(destination->m_memberIndex)->setHavePath(false);
                    resetSlot(static_cast<int>(selected));

                    if (selected < m_maxSquadMembers - 1 && m_slots[selected + 1]->m_occupied)
                    {
                        unsigned int replacement = m_maxSquadMembers - 1;
                        while (replacement > 0)
                        {
                            source = m_slots[replacement];
                            if (source->m_occupied)
                            {
                                destination = m_slots[selected];
                                destination->m_occupied = source->m_occupied;
                                destination->m_b24 = source->m_b24;
                                destination->m_offset = source->m_offset;
                                destination->m_memberIndex = source->m_memberIndex;
                                destination->m_i20 = source->m_i20;
                                destination->m_i28 = source->m_i28;
                                m_coordinator->getSquadMember(destination->m_memberIndex)->setHavePath(false);
                                resetSlot(static_cast<int>(replacement));
                                break;
                            }
                            --replacement;
                        }
                    }
                }
                else
                {
                    const int movedMember = lastSlot->m_memberIndex;
                    const int oldIndex = lastSlot->m_i20;
                    const bool oldFlag = lastSlot->m_b24;
                    const float oldValue = lastSlot->m_i28;
                    resetSlot(static_cast<int>(last));
                    Slot* const slot = m_slots[removed];
                    slot->m_occupied = true;
                    slot->m_memberIndex = movedMember;
                    slot->m_i20 = oldIndex;
                    slot->m_b24 = oldFlag;
                    slot->m_i28 = oldValue;
                    m_coordinator->getSquadMember(movedMember)->setHavePath(false);
                }
                break;
            }
            --last;
        }
    }
    if (removed == 0)
        resetTrailingPositions();
    updateMovementFlag(true);
}

// Slot zero is never considered. Fresh bounds after
// callbacks; occupied candidates only, with -1 and identical-slot results skipped.
void SquadFormation::updateClosestSlots()
{
    for (unsigned int slot = 1; slot < m_maxSquadMembers; ++slot)
    {
        if (m_slots[slot]->m_occupied)
        {
            const int closest = getClosestSlotIndex(static_cast<int>(slot));
            if (closest != -1 && closest != static_cast<int>(slot))
                swapSlots(static_cast<int>(slot), closest);
        }
    }
}

// The slot/member read precedes
// the first getSlotWorldPos call; no argument or null guard is added.
int SquadFormation::getClosestSlotIndex(int slotIndex)
{
    int best = -1;
    float minimum = 3.402823466e+38F;
    Vec3 target = {};
    Vec3 position = {};
    const int memberIndex = m_slots[slotIndex]->m_memberIndex;
    (void)best; (void)minimum; (void)target; (void)position; (void)memberIndex;
    // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
    // The bool getSlotWorldPos(int,Vec3&) movement/position chain remains unwritten.
    BF2_AI_FRONTIER("dice::hfe::ai::SquadFormation::getClosestSlotIndex (getSlotWorldPos)");
}

// Keep the real allocation and captured pointers;
// only these three payload fields move, even for identical or aliased slots.
void SquadFormation::swapSlots(int first, int second)
{
    Slot* const a = m_slots[first];
    Slot* const b = m_slots[second];
    Slot* const temporary = new Slot(Slot::SwapTemporary());
    temporary->m_memberIndex = a->m_memberIndex;
    temporary->m_b24 = a->m_b24;
    temporary->m_i28 = a->m_i28;
    a->m_memberIndex = b->m_memberIndex;
    a->m_b24 = b->m_b24;
    a->m_i28 = b->m_i28;
    b->m_memberIndex = temporary->m_memberIndex;
    b->m_b24 = temporary->m_b24;
    b->m_i28 = temporary->m_i28;
    delete temporary;
}

// Allocates formation slots and parallel position state for the configured squad size.

SquadFormation::SquadFormation(SquadCoordinator* coordinator)
    : m_slots()
    , m_coordinator(coordinator)
    , m_formation(0)
    , m_i48(0)
    , m_f52(0.0f)
    , m_i88(0)
    , m_d92(0.0)
    , m_slotTargets()
    , m_trailingPosition()
{
    m_direction.x = 0.0f;
    m_direction.y = 0.0f;
    m_direction.z = 0.0f;

    m_v60.x = 0.0f;
    m_v60.y = 0.0f;
    m_v60.z = 0.0f;
    m_v72.x = 0.0f;
    m_v72.y = 0.0f;
    m_v72.z = 0.0f;

    m_side = coordinator->getSide();

    m_maxSquadMembers = static_cast<unsigned int>(
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getMaxNumberOfSquadMembers());

    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        m_slots.push_back(new Slot());

        Vec3 zero;
        zero.x = 0.0f;
        zero.y = 0.0f;
        zero.z = 0.0f;
        m_slotTargets.push_back(std::pair<bool, Vec3>(false, zero));
    }

    // FLT_MAX, written as the bit pattern the binary stores.
    m_formationMaxSpeed = 3.402823466e+38F;
    updateFormationMaxSpeed();

    m_formation = FormationLine;
    setFormation();

    m_b36 = false;
    m_i48 = 0;
    m_f52 = 1.0f;
    m_v60.x = 0.0f;
    m_v60.y = 0.0f;
    m_v60.z = 0.0f;
    m_b56 = false;
    m_b84 = false;
    m_b85 = false;
    m_i88 = 0;
    m_d92 = 0.0;
    m_v72.x = 0.0f;
    m_v72.y = 0.0f;
    m_v72.z = 0.0f;
}

float SquadFormation::getFormationMaxSpeed()
{
    return m_formationMaxSpeed;
}

// Use the first free slot; neither a duplicate-member
// check nor a full-formation refusal exists. The movement update and path reset run
// even when the scan finds no free slot. PE inlines updateMovementFlag(true).
void SquadFormation::joinFormation(int memberIndex)
{
    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        Slot* const slot = m_slots[i];
        if (!slot->m_occupied)
        {
            slot->m_occupied = true;
            slot->m_memberIndex = memberIndex;
            slot->m_i20 = -1;
            slot->m_b24 = false;
            slot->m_i28 = 0;
            break;
        }
    }

    updateMovementFlag(true);
    m_coordinator->getSquadMember(memberIndex)->setHavePath(false);
}

// An unoccupied slot zero leaves the old movement
// flag untouched. Otherwise clear it before reading the clock, then set it only
// when forced or when both strict thresholds pass. No empty-slot-vector guard.
void SquadFormation::updateMovementFlag(bool force)
{
    if (!m_slots[0]->m_occupied)
        return;

    m_b56 = false;
    const double now =
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();

    bool update = force;
    if (!update)
    {
        const float dx = m_v60.x - m_v72.x;
        const float dy = m_v60.y - m_v72.y;
        const float dz = m_v60.z - m_v72.z;
        // The PE sums z squared, y squared, then x squared.
        update = dz * dz + dy * dy + dx * dx > 4.0f && now > m_d92;
    }

    if (update)
    {
        m_b56 = true;
        m_d92 = now + 0.2;
        m_v60 = m_v72;
    }
}

// Preserve both guards.
// The positioned-squad arm calls getSlotWorldPos(getSquadMemberIndexAtSlot(0), m_v72).
// Its navigation/position dependencies are not recovered here.
// [FRONTIER, narrow] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
void SquadFormation::updateSlotZeroPos()
{
    if (m_slots[0]->m_occupied && m_coordinator->squadHasPosition())
    {
        BF2_AI_FRONTIER("dice::hfe::ai::SquadFormation::updateSlotZeroPos (getSlotWorldPos)");
    }
}

// Only the target-valid flags and slot indices are
// cleared; target coordinates and occupied/member-index fields remain unchanged.
void SquadFormation::resetTrailingPositions()
{
    updateSlotZeroPos();
    m_trailingPosition = m_v72;

    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        m_slotTargets[i].first = false;
        m_slots[i]->m_i20 = -1;
    }
}

// Sets the formation speed from the slowest occupied slot, adjusted for crowding.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding SquadFormation operation.
void SquadFormation::updateFormationMaxSpeed()
{
    int occupied = 0;

    m_formationMaxSpeed = 3.402823466e+38F;

    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        Slot* const slot = m_slots[i];

        if (slot->m_occupied)
        {
            ++occupied;
            BF2_AI_FRONTIER("dice::hfe::ai::SquadFormation::updateFormationMaxSpeed"
                            " - the per-member IPIMobile::getMaxSpeed lookup");
        }
    }

    if (occupied)
    {
        if (occupied > 1)
            m_formationMaxSpeed = 0.89999998f * m_formationMaxSpeed;
    }
    else
    {
        m_formationMaxSpeed = 5.0f;
    }
}

// Sets the formation-specific direction and dispatches layout. Echelons mirror the
// third direction component; unknown formation values leave the layout unchanged.
void SquadFormation::setFormation()
{
    if (m_formation == FormationWedge)
    {
        m_direction.x = 0.80000001f;
        m_direction.y = 0.0f;
        m_direction.z = -0.60000002f;
        setWedgeFormation();
        return;
    }

    if (m_formation > FormationWedge)
    {
        if (m_formation == FormationEchelonA)
        {
            m_direction.x = 0.60000002f;
            m_direction.y = 0.0f;
            m_direction.z = -0.80000001f;
        }
        else
        {
            if (m_formation != FormationEchelonB)
                return;

            m_direction.x = 0.60000002f;
            m_direction.y = 0.0f;
            m_direction.z = 0.80000001f;
        }

        setEchelonFormation();
        return;
    }

    if (m_formation == FormationLine)
    {
        m_direction.x = 1.0f;
        m_direction.y = 0.0f;
        m_direction.z = 0.0f;
        setLineFormation();
    }
}

// Places slot zero at the origin and alternates subsequent formation slots on either side.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
void SquadFormation::setLineFormation()
{
    const float spacing = 1.0f;

    bool  positiveSide  = true;
    float offset        = 0.0f;
    int   referenceSlot = 0;

    for (unsigned int i = 0; i < m_maxSquadMembers; ++i)
    {
        Slot* const slot = m_slots[i];

        if (i)
        {
            if (positiveSide)
            {
                offset = spacing * m_direction.x;
                slot->m_referenceSlot = referenceSlot;
                positiveSide = false;

                if (i >= 3)
                    ++referenceSlot;
            }
            else
            {
                offset = -spacing * m_direction.x;
                slot->m_referenceSlot = referenceSlot++;
                positiveSide = true;
            }
        }

        slot->m_offset.x = offset;
        slot->m_offset.y = 0.0f;
        slot->m_offset.z = 0.0f;

        if (slot->m_occupied)
        {
            BF2_AI_FRONTIER("dice::hfe::ai::SquadFormation::setLineFormation"
                            " - the occupied-slot vehicle-radius spread");
        }
    }
}

// [FRONTIER] Wedge and echelon layouts selected by changeFormation remain unimplemented.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace the stops when their layout behavior is recovered.
void SquadFormation::setWedgeFormation()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SquadFormation::setWedgeFormation");
}

void SquadFormation::setEchelonFormation()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SquadFormation::setEchelonFormation");
}

} // namespace ai
} // namespace hfe
} // namespace dice
