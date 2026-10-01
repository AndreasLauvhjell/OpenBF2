// [INFERRED] name/path; native owner of complete PE A1781C client binding list.
#ifndef MENU_HUDCONSOLEBINDINGS_H
#define MENU_HUDCONSOLEBINDINGS_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include <list>
namespace dice { namespace hfe {
class ConsoleObject;
namespace hud {
std::list<ConsoleObject*>& hudConsoleObjects();
} } }
#endif
#endif
