// [TEMP] Native typed observer fixture; no production test hooks or retail layout claims.
#ifndef SPAWN_QUERIES_RUNTIME_H
#define SPAWN_QUERIES_RUNTIME_H
#include "../../BF2/AI/AIInterface/BFKnowledge/BFGame.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/AIPlayer.h"
#include "../../BF2/Game/GameLogic/SpawnSystem/SpawnManager.h"
#include "../../BF2/Game/GameLogic/SpawnSystem/SpawnGroup.h"
#include "../../BF2/Game/Objects/IPlayer.h"
#include "../../BF2/IO/Input/PlayerInput.h"
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <string>
#include <vector>

__declspec(noreturn) inline void unexpected(const char* name)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", name);
    std::fflush(stderr);
    std::abort();
}
inline void require(bool condition, const char* name)
{
    if (!condition) unexpected(name);
}
namespace dice { namespace hfe { extern IBase* spawnManager; } }
#endif
