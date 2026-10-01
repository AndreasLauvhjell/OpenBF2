// Team/player scoring and replication. Source path is attested.
#include "ScoreManager.h"
#include "../../../Core/Debug.h"   // getTeamScore's fatal range site
#include "../../../IO/BitStream.h"
#include "../../../IO/Network/NetworkStats.h"
#include "../../IGameLogic.h"      // setWinner's gameLogic->getGameStatus() gate

#include <cstdio>
#include <cstdlib>
#include <cstring>                 // std::memcpy

namespace dice {
namespace hfe {
namespace world {

namespace {

Vec3 scoreManagerStatsColour()
{
    Vec3 colour;
    colour.x = 240.0f;
    colour.y = 255.0f;
    colour.z = 100.0f;
    return colour;
}

// TU static at ds:0x8AAC73C; getNetUpdate accumulates into its +4 dword (0x8AAC740).
io::RegistredNetworkStats g_scoreManagerStats("ScoreManager", scoreManagerStatsColour());

// [CHOSEN] source factoring of the repeated sign-plus-17-bit blocks. The ELF spells all 38 copies
// out inline, but every copy has the same branch, sign bit and magnitude width.
void writeSigned17(io::BitStream& stream, int value)
{
    const unsigned int sign = value < 0 ? 1u : 0u;
    const unsigned int magnitude = value < 0
        ? 0u - static_cast<unsigned int>(value)
        : static_cast<unsigned int>(value);
    stream.writeBits(&sign, 1);
    stream.writeBits(&magnitude, 17);
}

int scoreRecordInt(const unsigned char* record, unsigned int offset)
{
    return *reinterpret_cast<const int*>(record + offset);
}

int& scoreRecordIntRef(unsigned char* record, unsigned int offset)
{
    return *reinterpret_cast<int*>(record + offset);
}

// Reads signed magnitude; bits includes the sign bit. Score fields use 18 bits, round results use 3.
int readSignedMagnitude(io::BitStream& stream, unsigned int bits)
{
    unsigned int sign = 0;
    stream.readBits(&sign, 1);
    unsigned int magnitude = 0;
    stream.readBits(&magnitude, bits - 1);
    return sign ? -static_cast<int>(magnitude) : static_cast<int>(magnitude);
}

// The apply side writes straight into the embedded TeamScore objects by raw offset, exactly as
// updateStateMask above reads them by raw offset. Keeping the offset form is deliberate: it
// is what makes the shipped +0x1C-into-+0x18 defect below visible as a wrong constant rather than
// looking like a duplicated line someone fat-fingered.
int& teamInt(unsigned char* team, unsigned int offset)
{
    return *reinterpret_cast<int*>(team + offset);
}

} // anonymous namespace

int ScoreManager::getDeath()                  { return m_death; }                  // +0x44
int ScoreManager::getKill()                   { return m_kill; }                   // +0x48
int ScoreManager::getAssistKill()             { return m_assistKill; }             // +0x4C
int ScoreManager::getInDirectKill()           { return m_inDirectKill; }           // +0x50
int ScoreManager::getTK()                     { return m_TK; }                     // +0x54
int ScoreManager::getHeal()                   { return m_heal; }                   // +0x58
int ScoreManager::getSoldierRevive()          { return m_soldierRevive; }          // +0x5C
int ScoreManager::getRepair()                 { return m_repair; }                 // +0x60
int ScoreManager::getVehicleRevive()          { return m_vehicleRevive; }          // +0x64
int ScoreManager::getAmmo()                   { return m_ammo; }                   // +0x68
int ScoreManager::getFlagCapture()            { return m_flagCapture; }            // +0x6C
int ScoreManager::getFlagReturn()             { return m_flagReturn; }             // +0x70
int ScoreManager::getControlPointCapture()    { return m_controlPointCapture; }    // +0x74
int ScoreManager::getObjective()              { return m_objective; }              // +0x78
int ScoreManager::getDriverPassenger()        { return m_driverPassenger; }        // +0x7C
int ScoreManager::getTeamDamage()             { return m_teamDamage; }             // +0x80
int ScoreManager::getTeamVehicleDamage()      { return m_teamVehicleDamage; }      // +0x84

// The five limits - `fld dword ptr [eax+N]`, leaving the value in ST0.
float ScoreManager::getHealScoreLimit()          { return m_healScoreLimit; }          // +0x88
float ScoreManager::getRepairScoreLimit()        { return m_repairScoreLimit; }        // +0x8C
float ScoreManager::getAmmoScoreLimit()          { return m_ammoScoreLimit; }          // +0x90
float ScoreManager::getTeamDamageLimit()         { return m_teamDamageLimit; }         // +0x94
float ScoreManager::getTeamVehicleDamageLimit()  { return m_teamVehicleDamageLimit; }  // +0x98

// forceSetWinner - 17 bytes: `mov edx,[ebp+arg_4]; mov eax,[ebp+this]; mov [eax+9Ch], dl`.
// Note it stores only the LOW byte, which is why m_winner is an unsigned char.
void ScoreManager::forceSetWinner(unsigned char winner)  { m_winner = winner; }           // +0x9C

// Snapshots both teams and sets shared dirty bits for changed score fields.
// Several fields share a bit, including tickets/state/bleed, so receivers must decode the entire group.
// [UNRESOLVED] Individual unnamed TeamScore block fields retain their existing wire positions.
void ScoreManager::updateStateMask(unsigned int* mask)
{
    // NetworkableBase is the original +4 secondary base, hence these inherited members land at
    // object offsets +0x2C / +0x38 / +0x3C exactly as the ELF accesses them.
    m_prev = m_head;
    m_head = (m_head + 1) & 3;
    m_next = m_head;

    unsigned char* const next = m_scoreBuffer + 164 * m_next;
    unsigned char* const prev = m_scoreBuffer + 164 * m_prev;

    // The ELF walks team one, then team two, then the four trailing fields. Each entry is
    // { TeamScore offset, record offset, bit }.
    struct Field { unsigned int src; unsigned int rec; unsigned int bit; bool byte; };

    static const Field TEAM1[] = {
        { 0x04, 0x04, 0x00000001u, false }, { 0x08, 0x08, 0x00000002u, false },
        { 0x0C, 0x0C, 0x01000000u, false }, { 0x40, 0x40, 0x01000000u, false },
        { 0x10, 0x10, 0x04000000u, false }, { 0x14, 0x14, 0x10000000u, false },
        { 0x18, 0x18, 0x00000004u, false }, { 0x1C, 0x1C, 0x00000008u, false },
        { 0x20, 0x20, 0x00000010u, false }, { 0x3C, 0x3C, 0x00000010u, false },
        { 0x24, 0x24, 0x00000020u, false }, { 0x28, 0x28, 0x00000040u, false },
        { 0x2C, 0x2C, 0x00000080u, false }, { 0x30, 0x30, 0x00000100u, false },
        { 0x34, 0x34, 0x00000200u, false },
        { 0x84, 0x44, 0x00000400u, false },          // m_tickets
        { 0x90, 0x50, 0x00000400u, true  },          // m_ticketBleed byte - same bit
        { 0x8C, 0x4C, 0x00000400u, false },          // m_ticketState - same bit
        { 0x88, 0x48, 0x00000800u, false }           // m_roundsWon (named by setWinner)
    };
    static const Field TEAM2[] = {
        { 0x04, 0x54, 0x00001000u, false }, { 0x08, 0x58, 0x00002000u, false },
        { 0x0C, 0x5C, 0x02000000u, false }, { 0x40, 0x90, 0x02000000u, false },
        { 0x10, 0x60, 0x08000000u, false }, { 0x14, 0x64, 0x20000000u, false },
        { 0x18, 0x68, 0x00004000u, false }, { 0x1C, 0x6C, 0x00008000u, false },
        { 0x20, 0x70, 0x00010000u, false }, { 0x3C, 0x8C, 0x00010000u, false },
        { 0x24, 0x74, 0x00020000u, false }, { 0x28, 0x78, 0x00040000u, false },
        { 0x2C, 0x7C, 0x00080000u, false }, { 0x30, 0x80, 0x00100000u, false },
        { 0x34, 0x84, 0x00200000u, false },
        { 0x84, 0x94, 0x00400000u, false },
        { 0x90, 0xA0, 0x00400000u, true  },          // m_ticketBleed byte
        { 0x8C, 0x9C, 0x00400000u, false },
        { 0x88, 0x98, 0x00800000u, false }
    };

    const unsigned char* const team[2] = {
        reinterpret_cast<const unsigned char*>(&m_teamScore1),
        reinterpret_cast<const unsigned char*>(&m_teamScore2)
    };
    const Field* const table[2] = { TEAM1, TEAM2 };
    const unsigned int counts[2] = { sizeof(TEAM1) / sizeof(TEAM1[0]),
                                     sizeof(TEAM2) / sizeof(TEAM2[0]) };

    for (int t = 0; t < 2; ++t)
    {
        for (unsigned int i = 0; i < counts[t]; ++i)
        {
            const Field& f = table[t][i];
            if (f.byte)
            {
                next[f.rec] = team[t][f.src];
                if (prev[f.rec] != next[f.rec])
                    *mask |= f.bit;
            }
            else
            {
                *reinterpret_cast<int*>(next + f.rec) =
                    *reinterpret_cast<const int*>(team[t] + f.src);
                if (*reinterpret_cast<const int*>(prev + f.rec)
                    != *reinterpret_cast<const int*>(next + f.rec))
                    *mask |= f.bit;
            }
        }
    }

    // the four trailing fields, in the ELF's order
    // TWO bytes OFF THE ScoreManager itself, not off either TeamScore.
    next[0x00] = m_winner;                           // +0x9C, a named member
    if (prev[0x00] != next[0x00])
        *mask |= 0x40000000u;

    next[0x01] = m_victoryType;                      // +0x9D, a named member
    if (prev[0x01] != next[0x01])
        *mask |= 0x80000000u;

    // and both teams' m_block[13], sharing bit 31 with the byte above.
    *reinterpret_cast<int*>(next + 0x38) = *reinterpret_cast<const int*>(team[0] + 0x38);
    if (*reinterpret_cast<const int*>(prev + 0x38) != *reinterpret_cast<const int*>(next + 0x38))
        *mask |= 0x80000000u;

    *reinterpret_cast<int*>(next + 0x88) = *reinterpret_cast<const int*>(team[1] + 0x38);
    if (*reinterpret_cast<const int*>(prev + 0x88) != *reinterpret_cast<const int*>(next + 0x88))
        *mask |= 0x80000000u;
}

// init - ELF 0x8107B30, 62 B. The argument is ignored: ScoreManager is a static, object-less
// networkable. The four 164-byte records are deliberately not zeroed, as shipped.
bool ScoreManager::init(void* /*object*/)
{
    if (!m_scoreBuffer)
        m_scoreBuffer = new unsigned char[0x290];
    return m_scoreBuffer != 0;
}

// getNetUpdate - ELF 0x81086A0, 7,974 B. Sender-side ScoreManager wire. The mask is always 32 bits;
// ordinary score fields use sign + 17-bit magnitude. Ticket count, ticket state and ticket bleed
// share one mask bit per team and are emitted together, which is why Python's ticket setters become
// visible to clients on the next static-networkable update.
bool ScoreManager::getNetUpdate(io::BitStream& stream, io::BaseLineData* /*baseline*/,
                                unsigned int stateMask, int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();
    m_next = m_head;

    // The original makes this conservative preflight only for a nonzero mask: 32 header bits plus
    // 16 per set mask bit. It then relies on BitStream's exact checks for the variable-sized arms.
    if (stateMask != 0)
    {
        int requiredBits = 32;
        for (unsigned int bit = 0; bit < 32; ++bit)
        {
            if (stateMask & (1u << bit))
                requiredBits += 16;
        }
        // Both shipped ports use a signed comparison here. Preserve it even though ordinary stream
        // capacities are small and positive; an underflow/high-bit residue follows signed retail
        // behaviour rather than an unsigned `ja`.
        if (requiredBits > static_cast<int>(stream.getRemainingBitCapacity()))
            return false;
    }

    stream.writeBits(&stateMask, 32);
    const unsigned char* const record = m_scoreBuffer + 164 * m_next;

    // Team one. This is the ELF's wire order, which intentionally differs from numeric bit order.
    if (stateMask & 0x00000001u) writeSigned17(stream, scoreRecordInt(record, 0x04));
    if (stateMask & 0x00000002u) writeSigned17(stream, scoreRecordInt(record, 0x08));
    if (stateMask & 0x01000000u)
    {
        writeSigned17(stream, scoreRecordInt(record, 0x0C));
        writeSigned17(stream, scoreRecordInt(record, 0x40));
    }
    if (stateMask & 0x04000000u) writeSigned17(stream, scoreRecordInt(record, 0x10));
    if (stateMask & 0x10000000u) writeSigned17(stream, scoreRecordInt(record, 0x14));
    if (stateMask & 0x00000004u) writeSigned17(stream, scoreRecordInt(record, 0x18));
    if (stateMask & 0x00000008u) writeSigned17(stream, scoreRecordInt(record, 0x1C));
    if (stateMask & 0x00000010u)
    {
        writeSigned17(stream, scoreRecordInt(record, 0x20));
        writeSigned17(stream, scoreRecordInt(record, 0x3C));
    }
    if (stateMask & 0x00000020u) writeSigned17(stream, scoreRecordInt(record, 0x24));
    if (stateMask & 0x00000040u) writeSigned17(stream, scoreRecordInt(record, 0x28));
    if (stateMask & 0x00000080u) writeSigned17(stream, scoreRecordInt(record, 0x2C));
    if (stateMask & 0x00000100u) writeSigned17(stream, scoreRecordInt(record, 0x30));
    if (stateMask & 0x00000200u) writeSigned17(stream, scoreRecordInt(record, 0x34));
    if (stateMask & 0x00000400u)
    {
        writeSigned17(stream, scoreRecordInt(record, 0x44));       // tickets
        const unsigned int bleed = record[0x50] ? 1u : 0u;
        stream.writeBits(&bleed, 1);
        writeSigned17(stream, scoreRecordInt(record, 0x4C));       // ticket state
    }
    if (stateMask & 0x00000800u) writeSigned17(stream, scoreRecordInt(record, 0x48));

    // Team two, in the same field order with the second record block's offsets/masks.
    if (stateMask & 0x00001000u) writeSigned17(stream, scoreRecordInt(record, 0x54));
    if (stateMask & 0x00002000u) writeSigned17(stream, scoreRecordInt(record, 0x58));
    if (stateMask & 0x02000000u)
    {
        writeSigned17(stream, scoreRecordInt(record, 0x5C));
        writeSigned17(stream, scoreRecordInt(record, 0x90));
    }
    if (stateMask & 0x08000000u) writeSigned17(stream, scoreRecordInt(record, 0x60));
    if (stateMask & 0x20000000u) writeSigned17(stream, scoreRecordInt(record, 0x64));
    if (stateMask & 0x00004000u) writeSigned17(stream, scoreRecordInt(record, 0x68));
    if (stateMask & 0x00008000u) writeSigned17(stream, scoreRecordInt(record, 0x6C));
    if (stateMask & 0x00010000u)
    {
        writeSigned17(stream, scoreRecordInt(record, 0x70));
        writeSigned17(stream, scoreRecordInt(record, 0x8C));
    }
    if (stateMask & 0x00020000u) writeSigned17(stream, scoreRecordInt(record, 0x74));
    if (stateMask & 0x00040000u) writeSigned17(stream, scoreRecordInt(record, 0x78));
    if (stateMask & 0x00080000u) writeSigned17(stream, scoreRecordInt(record, 0x7C));
    if (stateMask & 0x00100000u) writeSigned17(stream, scoreRecordInt(record, 0x80));
    if (stateMask & 0x00200000u) writeSigned17(stream, scoreRecordInt(record, 0x84));
    if (stateMask & 0x00400000u)
    {
        writeSigned17(stream, scoreRecordInt(record, 0x94));       // tickets
        const unsigned int bleed = record[0xA0] ? 1u : 0u;
        stream.writeBits(&bleed, 1);
        writeSigned17(stream, scoreRecordInt(record, 0x9C));       // ticket state
    }
    if (stateMask & 0x00800000u) writeSigned17(stream, scoreRecordInt(record, 0x98));

    if (stateMask & 0x40000000u)
    {
        const unsigned int sign = 0;
        const unsigned int winner = record[0x00];
        stream.writeBits(&sign, 1);
        stream.writeBits(&winner, 2);
    }
    if (stateMask & 0x80000000u)
    {
        const unsigned int sign = 0;
        const unsigned int victoryType = record[0x01];
        stream.writeBits(&sign, 1);
        stream.writeBits(&victoryType, 2);
        writeSigned17(stream, scoreRecordInt(record, 0x38));
        writeSigned17(stream, scoreRecordInt(record, 0x88));
    }

    if (stream.hasError())
    {
        stream.clearError();
        return false;
    }

    g_scoreManagerStats.m_writeBits +=
        static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    return true;
}

// Decodes score changes into the next record, then applies them to both teams.
// A failed mask read returns early; later read failures still apply the decoded portion.
// The application step intentionally writes m_block[6] into m_block[5]'s destination,
// overwriting that field while leaving the live m_block[6] unchanged.
void ScoreManager::setNetUpdate(io::BitStream& stream, float /*time*/, bool ghostActive,
                                io::BaseLineData* /*baseline*/, int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();   // +0x34 == 1 is getBitPosition inlined,
                                                     // not an error test - see Player.cpp
    makeRoomForNextUpdate(ghostActive);              // once

    if (m_head != m_next)                            // carry the previous slot forward
        std::memcpy(m_scoreBuffer + 164 * m_next,
                    m_scoreBuffer + 164 * m_head, 164);   // rep movsd 0x29 = 41 dwords

    unsigned char* const record = m_scoreBuffer + 164 * m_next;

    unsigned int stateMask = 0;
    if (!stream.readBits(&stateMask, 32))
    {
        // retail clears the latch and then reports - Debug(6, <file>, 832, "Failed to read
        // mask"). The report is named, not written: Debug(DebugTypeError, ...) aborts in this
        // tree and retail plainly continues past it. [UNRESOLVED] the overload behind PE 0x454B90.
        stream.clearError();
        return;                                      // the only early return in the body
    }

    // the read, in the writer'S order, which is not numeric bit order
    if (stateMask & 0x00000001u) scoreRecordIntRef(record, 0x04) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00000002u) scoreRecordIntRef(record, 0x08) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x01000000u)
    {
        scoreRecordIntRef(record, 0x0C) = readSignedMagnitude(stream, 18);
        scoreRecordIntRef(record, 0x40) = readSignedMagnitude(stream, 18);
    }
    if (stateMask & 0x04000000u) scoreRecordIntRef(record, 0x10) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x10000000u) scoreRecordIntRef(record, 0x14) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00000004u) scoreRecordIntRef(record, 0x18) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00000008u) scoreRecordIntRef(record, 0x1C) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00000010u)
    {
        scoreRecordIntRef(record, 0x20) = readSignedMagnitude(stream, 18);
        scoreRecordIntRef(record, 0x3C) = readSignedMagnitude(stream, 18);
    }
    if (stateMask & 0x00000020u) scoreRecordIntRef(record, 0x24) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00000040u) scoreRecordIntRef(record, 0x28) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00000080u) scoreRecordIntRef(record, 0x2C) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00000100u) scoreRecordIntRef(record, 0x30) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00000200u) scoreRecordIntRef(record, 0x34) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00000400u)
    {
        scoreRecordIntRef(record, 0x44) = readSignedMagnitude(stream, 18);   // tickets
        unsigned int bleed = 0;
        stream.readBits(&bleed, 1);
        record[0x50] = static_cast<unsigned char>(bleed);                    // ticket bleed, a byte
        scoreRecordIntRef(record, 0x4C) = readSignedMagnitude(stream, 18);   // ticket state
    }
    if (stateMask & 0x00000800u) scoreRecordIntRef(record, 0x48) = readSignedMagnitude(stream, 18);

    if (stateMask & 0x00001000u) scoreRecordIntRef(record, 0x54) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00002000u) scoreRecordIntRef(record, 0x58) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x02000000u)
    {
        scoreRecordIntRef(record, 0x5C) = readSignedMagnitude(stream, 18);
        scoreRecordIntRef(record, 0x90) = readSignedMagnitude(stream, 18);
    }
    if (stateMask & 0x08000000u) scoreRecordIntRef(record, 0x60) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x20000000u) scoreRecordIntRef(record, 0x64) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00004000u) scoreRecordIntRef(record, 0x68) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00008000u) scoreRecordIntRef(record, 0x6C) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00010000u)
    {
        scoreRecordIntRef(record, 0x70) = readSignedMagnitude(stream, 18);
        scoreRecordIntRef(record, 0x8C) = readSignedMagnitude(stream, 18);
    }
    if (stateMask & 0x00020000u) scoreRecordIntRef(record, 0x74) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00040000u) scoreRecordIntRef(record, 0x78) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00080000u) scoreRecordIntRef(record, 0x7C) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00100000u) scoreRecordIntRef(record, 0x80) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00200000u) scoreRecordIntRef(record, 0x84) = readSignedMagnitude(stream, 18);
    if (stateMask & 0x00400000u)
    {
        scoreRecordIntRef(record, 0x94) = readSignedMagnitude(stream, 18);   // tickets
        unsigned int bleed = 0;
        stream.readBits(&bleed, 1);
        record[0xA0] = static_cast<unsigned char>(bleed);
        scoreRecordIntRef(record, 0x9C) = readSignedMagnitude(stream, 18);   // ticket state
    }
    if (stateMask & 0x00800000u) scoreRecordIntRef(record, 0x98) = readSignedMagnitude(stream, 18);

    // Round-result bytes also use signed magnitude with a zero sign and two magnitude bits.
    if (stateMask & 0x40000000u)
        record[0x00] = static_cast<unsigned char>(readSignedMagnitude(stream, 3));   // winner
    if (stateMask & 0x80000000u)                     // PE `test eax, eax / jns` = bit 31
    {
        record[0x01] = static_cast<unsigned char>(readSignedMagnitude(stream, 3));   // victoryType
        scoreRecordIntRef(record, 0x38) = readSignedMagnitude(stream, 18);   // team 1 m_block[13]
        scoreRecordIntRef(record, 0x88) = readSignedMagnitude(stream, 18);   // team 2 m_block[13]
    }

    if (stream.hasError())
    {
        // Debug(6, <file>, 966, "read error") - named, not written; see the block above.
        // AND this ONE does NOT return: it clears and falls through to the stats and apply.
        stream.clearError();
    }

    g_scoreManagerStats.m_readBits +=
        static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);

    if (stateMask == 0)                              // nothing changed - no apply
        return;

    // the apply: the same mask in the same order, into the two embedded TeamScores
    // Unlike SpawnGroup::setNetUpdate, whose apply re-orders its arms and makes one pair
    // mutually exclusive, this one walks the mask in exactly the read's order. Checked, not assumed.
    unsigned char* const t1 = reinterpret_cast<unsigned char*>(&m_teamScore1);
    unsigned char* const t2 = reinterpret_cast<unsigned char*>(&m_teamScore2);

    if (stateMask & 0x00000001u) teamInt(t1, 0x04) = scoreRecordInt(record, 0x04);
    if (stateMask & 0x00000002u) teamInt(t1, 0x08) = scoreRecordInt(record, 0x08);
    if (stateMask & 0x01000000u)
    {
        teamInt(t1, 0x0C) = scoreRecordInt(record, 0x0C);
        teamInt(t1, 0x40) = scoreRecordInt(record, 0x40);
    }
    if (stateMask & 0x04000000u) teamInt(t1, 0x10) = scoreRecordInt(record, 0x10);
    if (stateMask & 0x10000000u) teamInt(t1, 0x14) = scoreRecordInt(record, 0x14);
    if (stateMask & 0x00000004u) teamInt(t1, 0x18) = scoreRecordInt(record, 0x18);
    // shipped defect, reproduced: the destination is +0x18, not +0x1C. PE 0x5C9EA5
    // stores to [ebx+0xB4], the same slot the line above just wrote. m_block[6] never reaches the
    // live TeamScore and m_block[5] is clobbered by it. Present in the ELF too (+0xB8 twice), so
    // it is dice's source. Do not "fix" this to 0x1C.
    if (stateMask & 0x00000008u) teamInt(t1, 0x18) = scoreRecordInt(record, 0x1C);
    if (stateMask & 0x00000010u)
    {
        teamInt(t1, 0x20) = scoreRecordInt(record, 0x20);
        teamInt(t1, 0x3C) = scoreRecordInt(record, 0x3C);
    }
    if (stateMask & 0x00000020u) teamInt(t1, 0x24) = scoreRecordInt(record, 0x24);
    if (stateMask & 0x00000040u) teamInt(t1, 0x28) = scoreRecordInt(record, 0x28);
    if (stateMask & 0x00000080u) teamInt(t1, 0x2C) = scoreRecordInt(record, 0x2C);
    if (stateMask & 0x00000100u) teamInt(t1, 0x30) = scoreRecordInt(record, 0x30);
    if (stateMask & 0x00000200u) teamInt(t1, 0x34) = scoreRecordInt(record, 0x34);
    if (stateMask & 0x00000400u)
    {
        m_teamScore1.setTickets(scoreRecordInt(record, 0x44));
        m_teamScore1.setTicketBleed(record[0x50] != 0);
        m_teamScore1.setTicketState(scoreRecordInt(record, 0x4C));
    }
    if (stateMask & 0x00000800u) teamInt(t1, 0x88) = scoreRecordInt(record, 0x48);   // m_roundsWon

    if (stateMask & 0x00001000u) teamInt(t2, 0x04) = scoreRecordInt(record, 0x54);
    if (stateMask & 0x00002000u) teamInt(t2, 0x08) = scoreRecordInt(record, 0x58);
    if (stateMask & 0x02000000u)
    {
        teamInt(t2, 0x0C) = scoreRecordInt(record, 0x5C);
        teamInt(t2, 0x40) = scoreRecordInt(record, 0x90);
    }
    if (stateMask & 0x08000000u) teamInt(t2, 0x10) = scoreRecordInt(record, 0x60);
    if (stateMask & 0x20000000u) teamInt(t2, 0x14) = scoreRecordInt(record, 0x64);
    if (stateMask & 0x00004000u) teamInt(t2, 0x18) = scoreRecordInt(record, 0x68);
    // Preserve the original overwrite: both team-two fields target the same destination.
    if (stateMask & 0x00008000u) teamInt(t2, 0x18) = scoreRecordInt(record, 0x6C);
    if (stateMask & 0x00010000u)
    {
        teamInt(t2, 0x20) = scoreRecordInt(record, 0x70);
        teamInt(t2, 0x3C) = scoreRecordInt(record, 0x8C);
    }
    if (stateMask & 0x00020000u) teamInt(t2, 0x24) = scoreRecordInt(record, 0x74);
    if (stateMask & 0x00040000u) teamInt(t2, 0x28) = scoreRecordInt(record, 0x78);
    if (stateMask & 0x00080000u) teamInt(t2, 0x2C) = scoreRecordInt(record, 0x7C);
    if (stateMask & 0x00100000u) teamInt(t2, 0x30) = scoreRecordInt(record, 0x80);
    if (stateMask & 0x00200000u) teamInt(t2, 0x34) = scoreRecordInt(record, 0x84);
    if (stateMask & 0x00400000u)
    {
        m_teamScore2.setTickets(scoreRecordInt(record, 0x94));
        m_teamScore2.setTicketBleed(record[0xA0] != 0);
        m_teamScore2.setTicketState(scoreRecordInt(record, 0x9C));
    }
    if (stateMask & 0x00800000u) teamInt(t2, 0x88) = scoreRecordInt(record, 0x98);

    if (stateMask & 0x40000000u)
        m_winner = record[0x00];                     // PE +0x98
    if (stateMask & 0x80000000u)
    {
        m_victoryType   = record[0x01];              // PE +0x99
        teamInt(t1, 0x38) = scoreRecordInt(record, 0x38);   // team 1 m_block[13]
        teamInt(t2, 0x38) = scoreRecordInt(record, 0x88);   // team 2 m_block[13]
    }
}

unsigned int ScoreManager::getMinimumBitSize() const
{
    return 40;
}

// five bytes: an exact empty body. The dtor calls it before freeing the update ring.
void ScoreManager::removeAll()
{
}

} // namespace world
} // namespace hfe
} // namespace dice

namespace dice {
namespace hfe {
namespace world {

// Reference-counted score manager. release returns the remaining count after virtual deletion.
void ScoreManager::addRef()
{
    ++m_refCount;
}

int ScoreManager::getRef()
{
    return m_refCount;
}

int ScoreManager::release()
{
    //  saves EBX; PE 569A00 saves ESI before its secondary-base deletion.
    const int remaining = --m_refCount;

    if (remaining == 0)
        delete this;

    return remaining;
}

void* ScoreManager::queryInterface(unsigned int iid) const
{
    // IBase and IScoreManager return the primary +0 interface.
    if (iid == IID_IBase || iid == 0xC4B8)
        return static_cast<IScoreManager*>(const_cast<ScoreManager*>(this));

    // The third id in the 45-byte body. This adjusted pointer is the NetworkableBase subobject at
    // +4; returning the primary object would dispatch scoring slots as INetworkable methods.
    if (iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(const_cast<ScoreManager*>(this));

    return 0;
}

// Initializes default scoring, damage ceilings and team IDs 1 and 2.
// Death alone has no penalty; team damage, vehicle damage and team kills are separate penalties.
ScoreManager::ScoreManager()
    : m_refCount(1)
    , m_death(0)
    , m_kill(2)
    , m_assistKill(1)
    , m_inDirectKill(1)
    , m_TK(-4)
    , m_heal(1)
    , m_soldierRevive(2)
    , m_repair(1)
    , m_vehicleRevive(2)
    , m_ammo(1)
    , m_flagCapture(5)
    , m_flagReturn(2)
    , m_controlPointCapture(4)
    , m_objective(5)
    , m_driverPassenger(1)
    , m_teamDamage(-2)
    , m_teamVehicleDamage(-1)
    , m_healScoreLimit(100.0f)
    , m_repairScoreLimit(100.0f)
    , m_ammoScoreLimit(100.0f)
    , m_teamDamageLimit(50.0f)
    , m_teamVehicleDamageLimit(25.0f)
    , m_winner(0)
    , m_victoryType(4)
    , m_scoreBuffer(0)
{
    m_teamScore1.m_teamId = 1;                   // +0xA0
    m_teamScore2.m_teamId = 2;                   // +0x134
}

// Resets round counters and tickets while preserving persistent TeamScore fields,
// then sets victory type 4 and winner 0.
void ScoreManager::reset()
{
    TeamScore* teams[2] = { &m_teamScore1, &m_teamScore2 };   // lea esi,[ebx+0A0h] / [ebx+134h]
    for (int i = 0; i < 2; ++i)
    {
        TeamScore& t = *teams[i];
        t.m_block[0]  = 0;                       // +0x04
        t.m_block[1]  = 0;                       // +0x08
        t.m_block[2]  = 0;                       // +0x0C
        t.m_block[5]  = 0;                       // +0x18
        t.m_block[7]  = 0;                       // +0x20
        t.m_block[8]  = 0;                       // +0x24
        t.m_block[9]  = 0;                       // +0x28
        t.m_block[10] = 0;                       // +0x2C
        t.m_block[11] = 0;                       // +0x30
        t.setTickets(0);                         // clamps, then stores +0x84
        t.m_block[12] = 0;                       // +0x34
    }
    m_victoryType = 4;                           // +0x9D
    m_winner      = 0;                           // +0x9C
}

// ~ScoreManager - ELF D1 0x81077E0 (74 B) / D0 0x8107830 (78 B). Restore both vptrs, call the
// exact-empty removeAll(), free the update ring when present, then destroy NetworkableBase.
ScoreManager::~ScoreManager()
{
    removeAll();
    if (m_scoreBuffer)
        delete[] m_scoreBuffer;
}

// the setters, +112..+244. Each is 14 bytes in the ELF (`mov [ecx+N], edx`) except the last
// two pairs, which are 17 - the difference is a sign-extending load, not extra work.
void ScoreManager::setDeath(int v)               { m_death = v; }
void ScoreManager::setKill(int v)                { m_kill = v; }
void ScoreManager::setAssistKill(int v)          { m_assistKill = v; }
void ScoreManager::setInDirectKill(int v)        { m_inDirectKill = v; }
void ScoreManager::setTK(int v)                  { m_TK = v; }
void ScoreManager::setHeal(int v)                { m_heal = v; }
void ScoreManager::setSoldierRevive(int v)       { m_soldierRevive = v; }
void ScoreManager::setRepair(int v)              { m_repair = v; }
void ScoreManager::setAmmo(int v)                { m_ammo = v; }
void ScoreManager::setVehicleRevive(int v)       { m_vehicleRevive = v; }
void ScoreManager::setFlagCapture(int v)         { m_flagCapture = v; }
void ScoreManager::setFlagReturn(int v)          { m_flagReturn = v; }
void ScoreManager::setControlPointCapture(int v) { m_controlPointCapture = v; }
void ScoreManager::setObjective(int v)           { m_objective = v; }
void ScoreManager::setDriverPassenger(int v)     { m_driverPassenger = v; }
void ScoreManager::setTeamDamage(int v)          { m_teamDamage = v; }
void ScoreManager::setTeamVehicleDamage(int v)   { m_teamVehicleDamage = v; }

void ScoreManager::setHealScoreLimit(float v)         { m_healScoreLimit = v; }
void ScoreManager::setRepairScoreLimit(float v)       { m_repairScoreLimit = v; }
void ScoreManager::setAmmoScoreLimit(float v)         { m_ammoScoreLimit = v; }
void ScoreManager::setTeamDamageLimit(float v)        { m_teamDamageLimit = v; }
void ScoreManager::setTeamVehicleDamageLimit(float v) { m_teamVehicleDamageLimit = v; }

// +24 / +36, 15 bytes each - plain byte reads widened to int.
int ScoreManager::getWinner()      { return m_winner; }
int ScoreManager::getVictoryType() { return m_victoryType; }

// +32, 17 bytes.
void ScoreManager::setVictoryType(unsigned char t) { m_victoryType = t; }

// Records the winner and increments its rounds-won count for team 1 or 2. Ignores
// calls during EndGame or Restart, so endGame must call this before changing state.
// forceSetWinner bypasses the guard and does not increment the count.
void ScoreManager::setWinner(unsigned char w)
{
    if (gameLogic->getGameStatus() == GameStatus_EndGame)
        return;
    if (gameLogic->getGameStatus() == GameStatus_Restart)
        return;
    m_winner = w;                                                          // +0x9C
    if (static_cast<unsigned char>(w - 1) <= 1)                            // team 1 or 2
    {
        TeamScore& team = (w == 1) ? m_teamScore1 : m_teamScore2;          // +0xA0 / +0x134
        ++team.m_roundsWon;                                                // +0x88
    }
}

// ScoreManager::create - ELF 0x8559930, 89 bytes. The same idiom as SpawnManager::create and
// MaterialManager::create, to the byte: aggregation refused outright, then new / queryInterface /
// release-on-failure.
IBase* ScoreManager::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    ScoreManager* self = new ScoreManager();

    void* itf = self->queryInterface(iid);
    if (itf == 0)
        self->release();

    return static_cast<IBase*>(itf);
}


// Returns the score for team 1 or 2. Invalid ids raise a fatal diagnostic before
// the unchecked pointer arithmetic can be used.
TeamScore* ScoreManager::getTeamScore(unsigned int teamId)
{
    if (teamId - 1 > 1)
    {
        // The site that attests this file's line 222. fatal (type 7 folds to 6).
        Debug(DEBUGTYPE_7, "Game/GameLogic/ScoreSystem/ScoreManager.cpp", 222, __FILE__)
            << "TBM: Incorrect team number " << teamId;
    }

    // Written as the ELF computes it - a stride from a base, not a member pick - so an
    // out-of-range team produces the same out-of-range pointer dice's build produces.
    return reinterpret_cast<TeamScore*>(
        reinterpret_cast<char*>(&m_teamScore1) - 148 + 148 * static_cast<int>(teamId));
}

} // namespace world

} // namespace hfe
} // namespace dice
