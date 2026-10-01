// [TEMP] Strict x86 adapter fixture. Real declarations; no production test hooks.
#ifndef BF_GAME_COUNTS_RUNTIME_H
#define BF_GAME_COUNTS_RUNTIME_H
#include "../../BF2/AI/AIInterface/BFKnowledge/BFGame.h"
#include "../../BF2/Game/IGameLogic.h"
#include "../../BF2/Game/GameLogic/ScoreSystem/ScoreManager.h"
#include "../../BF2/Game/Objects/Game/ControlPoint.h"
#include "../../BF2/IO/Input/PlayerInput.h"
#include <cstdlib>
#include <cstdio>
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
namespace dice { namespace hfe { namespace world { extern IBase* scoreManager; } } }
#endif
