// [TEMP] Exact small production bodies, unchanged native headers.
#pragma once
#include "../../BF2/AI/AIDLL/AI/AIBot/BotMain.h"
#include "../../BF2/AI/AIDLL/AI/AIStrategic/SAI.h"
#include "../../BF2/AI/AIDLL/AI/Main/AIMain.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/AIPlayer.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/KitTemplateManager.h"
#include "../../BF2/IO/Input/PlayerInput.h"
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <string>
#include <vector>
__declspec(noreturn) inline void unexpected(const char* message)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", message);
    std::fflush(stderr);
    std::abort();
}
inline void require(bool condition, const char* message)
{
    if (!condition) unexpected(message);
}
