// [TEMP] Untouched native declarations; fixture lifetimes do not publish production objects.
#pragma once
#include <list>
#include "../../BF2/AI/AIDLL/AI/AIBot/SquadFormation.h"
#include "../../BF2/AI/AIDLL/AI/AIBot/SquadCoordinator.h"
#include "../../BF2/AI/AIDLL/AI/AIBot/SquadMember.h"
#include "../../BF2/AI/AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../BF2/AI/AIDLL/AI/Main/AIDllFrontier.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/AIPlayer.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/BFTimer.h"

namespace dice { namespace hfe { namespace ai {
bool fixtureFormationRange(const Vec3&, const Vec3&);
bool fixtureTrailPointIsSet(const Vec3&);
float fixtureTrailCrumbDistanceSq(const Vec3&, const Vec3&);
float fixtureTrailMemberDistanceSq(const Vec3&, const Vec3&);
} } }
