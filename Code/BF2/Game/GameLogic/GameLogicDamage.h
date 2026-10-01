// Static damage calculation and authoritative death/assist attribution. Apply
// friendly-fire and vehicle modifiers, update hit records, notify occupants and
// Python, then apply armor damage. Preserve notification order and observable
// virtual calls even when their results are discarded.

#ifndef GAME_GAMELOGIC_GAMELOGICDAMAGE_H     // [INFERRED] guard style
#define GAME_GAMELOGIC_GAMELOGICDAMAGE_H

#include <list>

#include "../../Core/WeakPtr.h"
#include "../../Math/Vec3.h"
#include "../GameEvents/KilledByEvent.h"         // dice::hfe::KilledByData

namespace dice {
namespace hfe {
namespace world {

class IObject;
class IPlayer;
class IArmorGLComp;

// . The kill record: victim id, the material (narrowed to 16 bits), the killer's
// id and "alive" when there is one, the weapon's template id unless `vehicleKill`, the vehicle's
// template id (the killer's default vehicle when their PCO's child soldier is active, else the
// root of `vehicle`), and the armor's last-hit bone index and direction.
void fillKilledByData(KilledByData& data, IPlayer* killer, IPlayer* victim, IObject* weapon,
                      IObject* vehicle, IArmorGLComp* armor, int material, bool vehicleKill);

class GameLogicDamage
{
public:
    // Applies vehicle and friendly-fire modifiers. splash selects splash percentages;
    // soldiers/ladders use soldier percentages. The local-player debug bypass returns zero.
    static float calcDamage(IObject* object, float damage, int team, bool splash,
                            bool noFriendlyFireMod);

    // . See the header block.
    static void serverGiveDamage(IObject* object, float damage, IPlayer* attacker,
                                 std::list<WeakPtr<IPlayer> >* assistList, IObject* sourceObject,
                                 int team, IObject* weapon, IObject* vehicle, Vec3 position,
                                 Vec3 direction, int material, bool lastDamageType, bool unused,
                                 bool canTK, bool vehicleKill);
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_GAMELOGICDAMAGE_H
