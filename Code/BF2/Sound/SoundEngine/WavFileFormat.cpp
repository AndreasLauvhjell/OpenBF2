
// BF2Audio.dll only. Without BF2_IMAGE_BF2AUDIO this file compiles to nothing: the recursive
// sweeps that build BF2.exe and bf2_w32ded.exe (CMake's bf2_client_objects glob, build.cmd's
// all-TU census) must not carry the DLL's engine: its DllMain, its two exports and its own null
// Sound/SoundTemplate pair would collide with the host's, which loads this image at run time.
// Same convention as
// Code/BF2/RendDX9/BF2Render.cpp's BF2_IMAGE_RENDDX9 guard. Build with Code\Build\bf2audio.cmd.
#if defined(BF2_IMAGE_BF2AUDIO)
#include "WavFileFormat.h"
#include "SoundTrace.h"
#include "BfSoundEngine.h"

#include "../../../RendDX9Abi/BfMemory.h"

namespace dice {
namespace hfe {

namespace {
const char* const kFile = "C:\\dice\\Projects\\BF2Branches\\Patch_1_50\\Code\\BF2\\Sound\\SoundEngine\\WavFileFormat.cpp";

inline unsigned int fourcc(const char* s)
{
    return (unsigned int) (unsigned char) s[0] | ((unsigned int) (unsigned char) s[1] << 8)
         | ((unsigned int) (unsigned char) s[2] << 16) | ((unsigned int) (unsigned char) s[3] << 24);
}

// cue point + playlist segment -> [offset, offset + length + 1).
bool loopFromPlaylist(const unsigned int* cuePoint, const unsigned int* segment, int* loopStart, int* loopEnd)
{
    if (!cuePoint || !segment || segment[0] != cuePoint[0])
        return false;
    const unsigned int offset = cuePoint[5];
    *loopStart = (int) offset;
    *loopEnd   = (int) (segment[1] + offset + 1);
    return true;
}

// cue point + ltxt -> [offset, offset + sampleLength).
bool loopFromLtxt(const unsigned int* cuePoint, const unsigned int* ltxt, int* loopStart, int* loopEnd)
{
    if (!cuePoint || !ltxt || ltxt[2] != cuePoint[0])
        return false;
    const unsigned int offset = cuePoint[5];
    *loopStart = (int) offset;
    *loopEnd   = (int) (offset + ltxt[3]);
    return true;
}
}

bool parseWavFile(const std::string& name, WavFormat* format, int* loopStart, int* loopEnd)
{
    ISoundFileIO* io = g_soundFileIO;
    void* handle = io->open(name.c_str());
    if (!handle)
        return false;
    unsigned int riff[3];
    if (io->read(handle, riff, 12) != 12 || riff[0] != fourcc("RIFF") || riff[2] != fourcc("WAVE"))
    {
        io->close(handle);
        return false;
    }
    unsigned int* cue      = 0;
    unsigned int* cuePoint = 0;
    unsigned int* plst     = 0;
    unsigned int* segment  = 0;
    unsigned int* list     = 0;
    unsigned int* ltxt     = 0;
    unsigned int chunk[2];
    int got = io->read(handle, chunk, 8);
    while (got)
    {
        const unsigned int id   = chunk[0];
        const int          size = (int) chunk[1];
        if (id == fourcc("fmt "))
        {
            unsigned int fmt[5] = { 0, 0, 0, 0, 0 };
            if (io->read(handle, fmt, size) != size)
            {
                g_soundTrace->warning(sndFormat("Error reading format chunk of file: %s", name.c_str()), kFile, 152);
                io->close(handle);
                return false;
            }
            format->formatTag        = (int) (fmt[0] & 0xFFFF);
            format->samplesPerSecond = (int) fmt[1];
            format->channels         = (int) (fmt[0] >> 16);
            format->bitsPerSample    = (int) (fmt[3] >> 16);
            format->blockAlign       = (int) (fmt[3] & 0xFFFF);
            format->data             = 0;
            format->dataBytes        = 0;
            format->numSamples       = 0;
            format->reserved         = 0;
        }
        else if (id == fourcc("cue "))
        {
            if (size != 28)
                g_soundTrace->warning(sndFormat("%s has multiple cue points in a cue list. Ignoring all but the first cue point.", name.c_str()), kFile, 164);
            unsigned int* block = (unsigned int*) renddx9abi::engineAlloc(size);
            if (io->read(handle, block, size) == size)
            {
                cue      = block;
                cuePoint = block + 1;
            }
            else
            {
                g_soundTrace->error(sndFormat("Wavefile cue list segment is corrupt (redo the loop points): %s", name.c_str()), kFile, 170);
                renddx9abi::engineFree(block);
            }
        }
        else if (id == fourcc("plst"))
        {
            unsigned int* block = (unsigned int*) renddx9abi::engineAlloc(16);
            if (io->read(handle, block, size) == size)
            {
                plst    = block;
                segment = block + 1;
            }
            else
            {
                g_soundTrace->error(sndFormat("Wavefile playlist chunk is corrupt (redo the loop points): %s", name.c_str()), kFile, 185);
                renddx9abi::engineFree(block);
            }
        }
        else if (id == fourcc("LIST") && !list)
        {
            unsigned int* block = (unsigned int*) renddx9abi::engineAlloc(size);
            if (io->read(handle, block, size) == size)
            {
                list = block;
                ltxt = block + 1;
                if (block[0] != fourcc("adtl") || block[1] != fourcc("ltxt"))
                {
                    renddx9abi::engineFree(block);
                    list = 0;
                    ltxt = 0;
                }
            }
            else
            {
                g_soundTrace->error(sndFormat("Wavefile list chunk is corrupt (redo the loop points): %s", name.c_str()), kFile, 200);
                renddx9abi::engineFree(block);
            }
        }
        else if (id == fourcc("data"))
        {
            format->dataBytes = size;
            char* block = (char*) renddx9abi::engineAlloc(size);
            if (io->read(handle, block, size) != size)
            {
                g_soundTrace->error(sndFormat("Wavefile DATA chunk is corrupt: %s", name.c_str()), kFile, 222);
                renddx9abi::engineFree(block);
            }
            else
            {
                format->data = block;
            }
        }
        else
        {
            io->seek(handle, size, 1);
        }
        got = io->read(handle, chunk, 8);
    }
    format->numSamples = format->dataBytes / format->blockAlign;
    bool result;
    if (cuePoint && segment)
        result = loopFromPlaylist(cuePoint, segment, loopStart, loopEnd);
    else if (cuePoint && ltxt)
        result = loopFromLtxt(cuePoint, ltxt, loopStart, loopEnd);
    else
    {
        *loopStart = 0;
        *loopEnd   = format->numSamples;
        result = format->data != 0;
    }
    renddx9abi::engineFree(cue);
    renddx9abi::engineFree(plst);
    renddx9abi::engineFree(list);
    io->close(handle);
    return result;
}

} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_BF2AUDIO
