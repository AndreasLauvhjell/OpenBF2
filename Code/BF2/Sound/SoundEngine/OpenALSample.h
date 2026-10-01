
// OpenAL sample voices and their streaming implementations.


#ifndef SOUND_SOUNDENGINE_OPENALSAMPLE_H
#define SOUND_SOUNDENGINE_OPENALSAMPLE_H

#include "Sample.h"
#include "BfOpenAL.h"

#include <list>

struct OggVorbis_File;

namespace dice {
namespace hfe {

class SoundFile;

class OpenALSample : public Sample
{
public:
    explicit OpenALSample(const Vec3* position);
    virtual ~OpenALSample();

    virtual void  releaseSource();
    virtual int   getState();
    virtual const Vec3* getPosition(Vec3* out);
    virtual const Vec3* getPositionRef() { return &m_position; }
    virtual int   start(int startSample, int loops);
    virtual void  update(int steps);
    virtual bool  stop(bool force);
    virtual bool  isStopped() { return m_stopped; }

    virtual int   getCurrentBlockStart() = 0;               // +0x28
    virtual bool  startBuffers(int startSample, int loops) = 0;   // +0x2C
    virtual int   updateBuffers() = 0;                      // +0x30
    virtual bool  endLoop() = 0;                            // +0x34
    virtual bool  isLoopingDone() = 0;                      // +0x38
    virtual int   releaseBuffers();                         // +0x3C unqueue processed

    void  setObstruction(float obstruction);
    void  setOcclusion(float occlusion);
    void  applyDistanceAndCone();
    float computeObstructionGain();

    bool                m_streaming;        // +0x20 StreamingSample's constructor sets it
    Vec3                m_position;         // +0x24 the offset play*At was given, or zero
    ALuint              m_source;           // +0x30 0 while no voice is held
    bool                m_stopped;          // +0x34 stop() was called
    float               m_obstruction;      // +0x38 slew-limited evaluator answer
    EAXBUFFERPROPERTIES m_eax;              // +0x3C sent whole every update when EAX is up
};

class StaticSample : public OpenALSample
{
public:
    explicit StaticSample(const Vec3* position);
    virtual ~StaticSample();

    virtual int   getCurrentBlockStart();
    virtual bool  startBuffers(int startSample, int loops);
    virtual int   updateBuffers();
    virtual bool  endLoop();
    virtual bool  isLoopingDone() { return true; }

    bool m_loopPending;                                     // +0x84 loop block queued behind an intro
};

class StreamingSample : public OpenALSample
{
public:
    explicit StreamingSample(const Vec3* position);
    virtual ~StreamingSample();

    virtual int   getCurrentBlockStart() { return 0; }
    virtual bool  startBuffers(int startSample, int loops);
    virtual int   updateBuffers();
    virtual bool  endLoop() { return false; }
    virtual bool  isLoopingDone() { return m_loopsLeft <= 0; }
    virtual int   releaseBuffers();

    OggVorbis_File* m_vorbisFile;   // +0x84 the memory-backed decoder, 0 until startBuffers
    ALuint          m_buffers[4];   // +0x88 from the streaming pool
    int             m_loopsLeft;    // +0x98 INT_MAX when the loop count is 0
    int             m_bufferIndex;  // +0x9C how many of the four have been queued so far
};

// The TU's free functions and pools

// linear gain (0..1) to EAX millibels: -10000 at or below 1e-4, 0 at or above
// 0.9999, else 2000 * log10(gain).
int  linearToMillibel(float gain);

// OpenAL source pool.
ALuint acquireSource();
void   releaseSourceToPool(ALuint source);
void   purgeSourcePool();
// Streaming-buffer pool.
ALuint allocateStreamingBuffer();
void   purgeStreamingBufferPool();
// Ogg seek-table cache.
void   purgeLoopInfoCache();

// The streaming helpers, shared with Music.cpp's thread. See the .cpp for the argument contract.
bool streamStart(OggVorbis_File** vf, SoundFile* file, char* buffer, int* loopsLeft,
                 int seekSample, ALuint source, ALuint alBuffer, bool report);
bool bufferData(OggVorbis_File* vf, SoundFile* file, char* buffer, int* loopsLeft,
                ALuint alBuffer, bool report);
bool updateSourceBuffers(OggVorbis_File* vf, SoundFile* file, char* buffer, int* loopsLeft,
                         ALuint source, ALuint newBuffer, bool report);

// Pool statistics and the configured voice limit.
extern int g_streamingBufferBytes;
extern int g_streamingBufferCount;
extern int g_sourceCount;
extern int g_maxSources;

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_OPENALSAMPLE_H
