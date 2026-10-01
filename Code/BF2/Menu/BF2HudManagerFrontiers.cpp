// Compatibility translation unit retained by the build manifests; implementations reside with their subsystems.
#include "BF2HudManager.h"
#if !DEDICATED
#include <cstdio>
#include <cstdlib>

namespace dice { namespace hfe { namespace hud {
// Commander ambience still needs the sound owner's object-returning interface.
// The audible start/stop fallback cannot provide those missing objects.
} } }
#endif
