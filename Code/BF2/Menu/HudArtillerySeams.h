// [INFERRED] split beside the attested Menu/BF2HudManager.cpp family.
#ifndef MENU_HUDARTILLERYSEAMS_H
#define MENU_HUDARTILLERYSEAMS_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include "../Math/Vec3.h"
namespace dice { namespace hfe { namespace ai {
class IAIEventCourier;
// [FRONTIER] [CHOSEN, not recovered] Artillery request dispatch with team and target position.
// [RETIRE-WHEN: HUD-ARTILLERY-COURIER] Define after the courier's complete typed interface and request path are validated.
void dispatchArtilleryRequest(IAIEventCourier*, int team, Vec2 target);
} } }
#endif
#endif
