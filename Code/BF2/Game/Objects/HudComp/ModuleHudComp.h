// [INFERRED] provider declaration beside the attested ModuleHudComp.cpp.
#ifndef GAME_OBJECTS_HUDCOMP_MODULEHUDCOMP_H
#define GAME_OBJECTS_HUDCOMP_MODULEHUDCOMP_H
#include "../../../Core/BuildConfig.h"
#include <list>
namespace dice { namespace hfe {
class ConsoleObject;
namespace world {
std::list<ConsoleObject*>& hudCompConsoleObjects();
}
} }
#endif
