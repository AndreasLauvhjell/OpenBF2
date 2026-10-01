// Shared sound tracing declarations. [CHOSEN, not recovered] Header split.

#ifndef SOUND_SOUNDENGINE_SOUNDTRACE_H
#define SOUND_SOUNDENGINE_SOUNDTRACE_H

#include "../ISoundTrace.h"
#include "../ISoundFileIO.h"

namespace dice {
namespace hfe {

// ds:where every "SND-" line goes. Never null after BfSoundEngine's constructor: the
// engine news the DLL's default (OutputDebugString) sink there and setSoundTrace(0) puts it back.
extern ISoundTrace*  g_soundTrace;

// ds:where every file byte comes from. Same lifetime rule, default = the CRT's fopen.
extern ISoundFileIO* g_soundFileIO;

// Returns a shared 2048-byte formatting buffer. Not reentrant or thread-safe.
char* sndFormat(const char* fmt, ...);

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_SOUNDTRACE_H
