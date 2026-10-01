// [INFERRED] name/path. PE759E50 allocates the category/string record in events
// 38/39 and destroys it in event73. Native std::string, no retail32-byte overlay.
#ifndef MENU_HUDLOCALRADIOMESSAGE_H
#define MENU_HUDLOCALRADIOMESSAGE_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include "../Game/IGame.h"
namespace dice { namespace hfe { namespace hud {
struct HudLocalRadioMessage {
    TargetChannel channel; // untouched during default construction, then assigned
    std::string message;
};
} } }
#endif
#endif
