
// Reads RIFF WAVE format, data and cue/playlist or adtl loop metadata without decoding audio.
// Loop points are sample indices; unknown chunks are skipped.


#ifndef SOUND_SOUNDENGINE_WAVFILEFORMAT_H
#define SOUND_SOUNDENGINE_WAVFILEFORMAT_H

#include <string>

namespace dice {
namespace hfe {

struct WavFormat
{
    int   formatTag;        // +0x00 the fmt chunk's low word
    char* data;             // +0x04 the data chunk, in the engine heap; the owner frees it
    int   dataBytes;        // +0x08
    int   samplesPerSecond; // +0x0C
    int   bitsPerSample;    // +0x10
    int   channels;         // +0x14
    int   numSamples;       // +0x18 dataBytes / blockAlign
    int   blockAlign;       // +0x1C
    int   reserved;         // +0x20 always 0
};

// Reads `name` through the engine's ISoundFileIO. True with the format block
// filled, the data owned by the caller and the loop points set; false (and nothing owned) when
// the file cannot be opened or is not RIFF/WAVE, or when the format chunk is short. A missing or
// unusable loop is not an error: the loop is then the whole file.
bool parseWavFile(const std::string& name, WavFormat* format, int* loopStart, int* loopEnd);

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_WAVFILEFORMAT_H
