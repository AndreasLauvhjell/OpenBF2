// Bot-order interface; queries take a player ID. Preserve virtual declaration order.
// The destructor occupies two ELF slots and one MSVC slot.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_IAIPLAYERCOURIER_H     // [INFERRED] guard style
#define AI_AIINTERFACE_IAIPLAYERCOURIER_H

namespace dice {
namespace hfe {
namespace world { class IObject; }
namespace ai {

class IAIPlayerCourier
{
public:
    virtual ~IAIPlayerCourier();                 // aidll-side body; ELF slots +0x00/+0x04

    // [CHOSEN] Returns the vehicle requested for this player, or null when any seat is
    // acceptable. Seat selection compares the exact IObject pointer.
    virtual world::IObject* getRequestedVehicle(int playerId) = 0;
    virtual world::IObject* getFirstAbilityTarget(int playerId) = 0;          // ELF +0x0C / PE +0x08
    virtual world::IObject* getResurrectTarget(int playerId) = 0;             // ELF +0x10 / PE +0x0C
    virtual unsigned int getSecondAbilityTargetGridId(int playerId) = 0;      // ELF +0x14 / PE +0x10
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_IAIPLAYERCOURIER_H
