// [TEMP] Unmodified production declarations; chosen native lifetimes in the fixture.
#pragma once
#include <list>
#include <string>
#include "../../BF2/AI/AIDLL/AI/AIBot/SquadCoordinator.h"
#include "../../BF2/AI/AIDLL/AI/AIBot/SquadFormation.h"
#include "../../BF2/AI/AIDLL/AI/AIBot/SquadMember.h"
#include "../../BF2/AI/AIDLL/AI/AIBot/BotMain.h"
#include "../../BF2/AI/AIDLL/AI/AIBot/CommunicationAgent.h"
#include "../../BF2/AI/AIDLL/AI/AIStrategic/SAI.h"
#include "../../BF2/AI/AIDLL/AI/AIStrategicEntity/AIStrategicArea.h"
#include "../../BF2/AI/AIDLL/AI/Main/AIMain.h"
#include "../../BF2/AI/AIDLL/AI/Main/AIDllFrontier.h"
#include "../../BF2/AI/AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/AIPlayer.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/BFGame.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/BFSquad.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/BFTimer.h"

namespace dice { namespace hfe { namespace ai {
bool fixtureShooterMovedAway(const Vec3&, const Vec3&);
float fixtureLeaderGotoDistanceSq(const Vec3&, const Vec3&);
bool fixtureInsideControlPointRadius(const Vec3&, const Vec3&, float);
} } }
