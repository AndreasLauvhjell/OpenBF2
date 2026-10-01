// [TEMP] Real declarations for the strict x86 capture-counter adapter fixture.
#pragma once
#include "../../BF2/AI/AIInterface/BFKnowledge/BFGame.h"
#include "../../BF2/Game/IGameLogic.h"
#include "../../BF2/Game/Objects/Game/ControlPoint.h"
#include "../../BF2/IO/Input/PlayerInput.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

__declspec(noreturn) inline void unexpected(const char* name)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", name);
    std::fflush(stderr);
    std::abort();
}
static int checks;
inline void require(bool value, const char* name)
{
    ++checks;
    if (!value) unexpected(name);
}
