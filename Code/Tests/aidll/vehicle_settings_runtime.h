// [TEMP] Native, real-header vehicle-roster fixture. No production declarations are rewritten.
#pragma once
#include "../../BF2/AI/AIDLL/AI/Main/AISettings.h"
#include "../../BF2/AI/AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../BF2/AI/AIDLL/AI/Main/AIDllFrontier.h"
#include "../../BF2/AI/AIDLL/AI/AIBot/BotMain.h"
#include "../../BF2/AI/AIDLL/AI/AIPathFinding/Material/MaterialManager.h"
#include "../../BF2/AI/AIDLL/AI/AIPathFinding/Material/Material.h"
#include "../../BF2/AI/AIDLL/AI/AIUtils/checkPointer.h"
#include "../../BF2/Core/ClassManager.h"
#include "../../BF2/Core/Debug.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <type_traits>

namespace dice { namespace hfe { extern ClassManager* classManager; } }
[[noreturn]] void unexpected(const char* method);
