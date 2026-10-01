// Music playback service shared by sound-engine entry points.

// Streams music on a worker owning one source and twelve buffers.
// Stop may join immediately or request a fade; errors are reported after joining because tracing is not thread-safe.


#ifndef SOUND_SOUNDENGINE_MUSIC_H
#define SOUND_SOUNDENGINE_MUSIC_H

namespace dice {
namespace hfe {

bool musicLoad(const char* name);       // stop, drop the old file, take the new name
bool musicPlay();                       // load the file and start the worker
bool musicStop(float fadeSeconds);      // join, or fade out; answers "was playing"
void musicUpdateVolume();               // group 1's volume into the worker's word

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_MUSIC_H
