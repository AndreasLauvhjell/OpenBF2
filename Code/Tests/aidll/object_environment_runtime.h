// [TEMP]
// Strict x86 fixture using real declarations; chosen lifetimes and observer hooks live in the .cpp.
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "../../BF2/AI/AIInterface/BFKnowledge/BFObjectEnvironment.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/BFTimer.h"
#include "../../BF2/AI/AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../BF2/Core/Ids.h"
#include "../../BF2/Game/IGameLogic.h"
#include "../../BF2/Game/Objects/IObject.h"
#include "../../BF2/Game/Objects/IObjectTemplate.h"
#include "../../BF2/Game/Objects/IObjectComponent.h"
#include "../../BF2/Game/Objects/IPlayer.h"
#include "../../BF2/Game/Objects/Object.h"
#include "../../BF2/Game/Objects/Base/SimpleObject.h"
#include "../../BF2/Game/Objects/Base/Bundle.h"
#include "../../BF2/Game/Objects/Game/PlayerControlObject.h"
#include "../../BF2/Game/Objects/GameLogicComp/IArmorGLComp.h"
#include "../../BF2/Game/Objects/GameLogicComp/ArmorGLCompTemplate.h"
#include "../../BF2/Scene/Object/PlayerManager.h"
#include "../../BF2/Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

[[noreturn]] void unexpected(const char* name);
