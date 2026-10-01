// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: bf2audio-smoke]
// Runs one audio DLL through a named scenario using the OpenAL logging shim.
// Compare stdout and shim logs from identical scenarios to check behavior.
// Usage: host.exe <BF2Audio.dll> <game dir> <shim BF2OpenAL.dll> <signals dir> <scenario> [provider] [quality] [voices]
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>

#include "../../BF2/Sound/NullSound.h"
#include "../../BF2/Sound/ISoundTrace.h"
#include "../../BF2/Sound/ISoundFileIO.h"
#include "../../BF2/Sound/ISoundNode.h"
#include "../../BF2/Sound/ISoundEvaluator.h"
#include "../../BF2/Sound/ISoundObstructionEvaluator.h"
#include "../../BF2/Math/Mat4.h"

namespace dice { namespace hfe {
struct SoundEnvelopePoint { float x, yMin, yMax; };      // the client's definition (12 bytes)
} }

using namespace dice::hfe;

namespace {

typedef void (__cdecl* MarkFn)(const char*);
MarkFn g_mark = 0;

void mark(const char* fmt, ...)
{
    char buf[512];
    va_list va;
    va_start(va, fmt);
    std::vsprintf(buf, fmt, va);
    va_end(va);
    std::printf("== %s\n", buf);
    if (g_mark)
        g_mark(buf);
}

struct StdoutTrace : public ISoundTrace
{
    virtual void error(const char* msg, const char* file, int line)        { report("ERR", msg, file, line); }
    virtual void warning(const char* msg, const char* file, int line)      { report("WRN", msg, file, line); }
    virtual void info(const char* msg, const char* file, int line)         { report("INF", msg, file, line); }
    virtual void assertFailed(const char* msg, const char* file, int line) { report("ASSERT", msg, file, line); std::exit(3); }
    virtual void debug(const char* msg, const char* file, int line)        { report("DBG", msg, file, line); }
    static void report(const char* kind, const char* msg, const char* file, int line)
    {
        const char* base = file ? std::strrchr(file, '\\') : 0;
        std::printf("  SND-%s %s(%d): %s\n", kind, base ? base + 1 : (file ? file : "?"), line, msg ? msg : "");
    }
};

struct Stats
{
    const char* providerInfo;
    int playing, waiting, delayed, loadedSampleBytes, streamedBytes, streamingBufferBytes;
    int alBuffers, streamingBuffers, loadedFiles, sounds, samples, sources, maxVoices;
};

// A parent node the host can move: the transform's row 3 is the position.
struct MovingNode : public ISoundNode
{
    Mat4 m;
    Vec3 v;
    MovingNode()
    {
        for (int i = 0; i < 16; ++i) m.m[i] = 0.0f;
        m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
        v.x = v.y = v.z = 0.0f;
    }
    virtual const Mat4& getTransform() { return m; }
    virtual const Math::Vec3* getVelocity() { return &v; }
};

struct RampEvaluator : public ISoundEvaluator
{
    virtual float getValue(float x, int) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
};

struct HalfObstruction : public ISoundObstructionEvaluator
{
    virtual float evaluate(ISound*, const Math::Vec3*) { return 0.5f; }
};

struct Ctx
{
    ISoundEngine* e;
    std::string   signals;
    const char*   provider;
    int           quality;
    int           voices;
    int           frame;
    Vec3          listenerPos, listenerFacing, listenerUp, listenerVel;   // the engine keeps pointers to these
    Ctx() : e(0), provider("Software Audio"), quality(0), voices(16), frame(0)
    {
        listenerPos.x = listenerPos.y = listenerPos.z = 0.0f;
        listenerFacing.x = 0.0f; listenerFacing.y = 0.0f; listenerFacing.z = 1.0f;
        listenerUp.x = 0.0f; listenerUp.y = 1.0f; listenerUp.z = 0.0f;
        listenerVel.x = listenerVel.y = listenerVel.z = 0.0f;
    }
    std::string path(const char* file) const { return signals + "\\" + file; }
};

Vec3 v3(float x, float y, float z) { Vec3 v; v.x = x; v.y = y; v.z = z; return v; }

void stats(Ctx& c, const char* when)
{
    const Stats* s = (const Stats*) c.e->getStats();
    std::printf("  [%s] playing=%d waiting=%d delayed=%d files=%d bytes=%d alBuf=%d strBuf=%d sounds=%d samples=%d sources=%d maxVoices=%d\n",
                when, s->playing, s->waiting, s->delayed, s->loadedFiles, s->loadedSampleBytes, s->alBuffers,
                s->streamingBuffers, s->sounds, s->samples, s->sources, s->maxVoices);
}

// Frames are paced on the performance counter at exactly 30 per second from the first frame, so
// that a sample of N.5 frames ends mid-frame in both runs and the run's own work does not drift the
// schedule (timeBeginPeriod(1) makes the sleep fine-grained; the last two milliseconds spin).
LARGE_INTEGER g_qpcFreq, g_qpcStart;
void waitForFrame(int frame)
{
    LARGE_INTEGER now;
    for (;;)
    {
        QueryPerformanceCounter(&now);
        const double due = (double) frame * (1.0 / 30.0);
        const double elapsed = (double) (now.QuadPart - g_qpcStart.QuadPart) / (double) g_qpcFreq.QuadPart;
        if (elapsed >= due)
            return;
        const double remaining = due - elapsed;
        if (remaining > 0.003)
            Sleep((DWORD) ((remaining - 0.002) * 1000.0));
    }
}

void frames(Ctx& c, int n, int statsEvery = 5)
{
    for (int i = 0; i < n; ++i)
    {
        waitForFrame(c.frame);
        mark("frame %d", c.frame);
        c.e->update(1);
        if (c.frame % statsEvery == 0)
        {
            char when[32];
            std::sprintf(when, "f%d", c.frame);
            stats(c, when);
        }
        ++c.frame;
    }
}

int init(Ctx& c, int quality, const char* provider, int voices)
{
    mark("initialize(%d, %s, %d)", quality, provider, voices);
    const int err = c.e->initialize((SoundQuality) quality, provider, voices);
    std::printf("  initialize -> %d, isInitialized=%d, eax=%d, quality=%d\n", err, c.e->isInitialized() ? 1 : 0,
                c.e->getEaxVersion(), c.e->getQualityLevel());
    return err;
}

ISoundTemplate* tmpl(Ctx& c, const char* file, bool is3d, int loops)
{
    ISoundTemplate* t = c.e->createTemplate();
    const std::string p = c.path(file);
    t->setFilename(p.c_str());
    t->setIs3dSound(is3d);
    t->setLoopCount((unsigned int) loops);
    t->setIsUsed();
    return t;
}

// scenarios
void oneshot2d(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    ISoundTemplate* t = tmpl(c, "mono22k.wav", false, 1);
    c.e->loadSoundFiles();
    stats(c, "loaded");
    ISound* s = t->createSound();
    mark("play");
    s->play();
    std::printf("  isPlaying=%d\n", s->isPlaying() ? 1 : 0);
    frames(c, 40);
    s->release();
    t->release();
}

void stereo44(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    ISoundTemplate* t = tmpl(c, "stereo44k.wav", false, 2);
    t->setVolume(0.5f);
    t->setPitch(1.2f);
    t->setPan(0.25f);
    t->setVolumeGroup(2);
    c.e->setVolumeForGroup(2, 0.7f);
    c.e->loadSoundFiles();
    ISound* s = t->createSound();
    mark("play(2 loops)");
    s->play();
    frames(c, 20);
    mark("setMasterVolume(0.6)");
    c.e->setMasterVolume(0.6f);
    frames(c, 20);
    mark("stop");
    s->stop();
    frames(c, 40);
    mark("play again then forceStop");
    s->play();
    frames(c, 10);
    s->forceStop();
    frames(c, 10);
    s->release();
    t->release();
}

void loop3d(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    c.e->setListenerPosition(&c.listenerPos);
    c.e->setListenerOrientation(&c.listenerFacing, &c.listenerUp);
    c.e->setListenerVelocity(&c.listenerVel);
    c.e->setDopplerEffect(1.0f);
    ISoundTemplate* t = tmpl(c, "loop_plst.wav", true, 0);
    t->setMinDistance(4.0f);
    t->setHalfVolumeDistance(15.0f);
    t->setPosition(v3(12.0f, 0.0f, 0.0f));
    t->setDirection(v3(1.0f, 0.0f, 0.0f));
    t->setConeInnerAngle(60.0f);
    t->setConeOuterAngle(120.0f);
    t->setConeOuterVolume(0.3f);
    t->setStopType(1);
    ISoundTemplate* t2 = tmpl(c, "loop_ltxt.wav", true, 2);
    t2->setMinDistance(2.0f);
    t2->setHalfVolumeDistance(8.0f);
    t2->setStopType(2);
    c.e->loadSoundFiles();
    stats(c, "loaded");
    ISound* s = t->createSound();
    mark("play (infinite loop, cone, listener moving)");
    s->play();
    c.listenerVel = v3(9.0f, 0.0f, 0.0f);
    for (int i = 0; i < 45; ++i)
    {
        c.listenerPos.x += 0.3f;
        if (i == 20) { c.listenerFacing = v3(1.0f, 0.0f, 0.0f); mark("listener faces +x"); }
        frames(c, 1);
    }
    mark("stop (stopType 1: finish loop, outro)");
    s->stop();
    frames(c, 50);
    MovingNode node;
    ISound* s2 = t2->createSound();
    s2->setParent(&node);
    node.m.m[12] = 3.0f; node.m.m[13] = 1.0f; node.m.m[14] = 5.0f;
    node.v = v3(0.0f, 0.0f, -2.0f);
    mark("play(2 loops) at a moving parent, then forceStop");
    s2->play(2);
    for (int i = 0; i < 25; ++i)
    {
        node.m.m[14] -= 0.2f;
        frames(c, 1);
    }
    s2->forceStop();
    frames(c, 10);
    mark("playAt offset");
    s2->playAt(v3(0.0f, 2.0f, 0.0f));
    frames(c, 20);
    mark("stop (stopType 2: cut to the outro now)");
    s2->stop();
    frames(c, 25);
    s2->release();
    s->release();
    t->release();
    t2->release();
}

void cull(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    c.e->setListenerPosition(&c.listenerPos);
    ISoundTemplate* t = tmpl(c, "mono44k_long.wav", true, 0);
    t->setMinDistance(2.0f);
    t->setHalfVolumeDistance(6.0f);
    t->setPosition(v3(500.0f, 0.0f, 0.0f));
    c.e->loadSoundFiles();
    ISound* s = t->createSound();
    mark("play far away (culled)");
    std::printf("  playSample -> %d\n", s->playSample(0) & 0xFF);      // the DLL answers in AL only
    frames(c, 10);
    mark("play near");
    t->setPosition(v3(3.0f, 0.0f, 0.0f));
    std::printf("  playSample -> %d\n", s->playSample(0) & 0xFF);
    frames(c, 20);
    mark("enable-culling off, far again");
    c.e->setProperty("enable-culling", 0.0f);
    t->setPosition(v3(500.0f, 0.0f, 0.0f));
    s->forceStop();
    frames(c, 5);
    std::printf("  playSample -> %d\n", s->playSample(0) & 0xFF);
    frames(c, 15);
    s->release();
    t->release();
}

void quality(Ctx& c)
{
    init(c, 2, c.provider, c.voices);
    ISoundTemplate* t = tmpl(c, "stereo44k.wav", false, 1);
    ISoundTemplate* tf = tmpl(c, "stereo44k.wav", false, 1);
    tf->setFlags(1);                             // keeps the 22 kHz floor
    ISoundTemplate* tm = tmpl(c, "mono11k_info.wav", false, 1);
    c.e->loadSoundFiles();
    stats(c, "loaded@2");
    ISound* s = t->createSound();
    ISound* sf = tf->createSound();
    ISound* sm = tm->createSound();
    mark("play at quality 2");
    s->play(); sf->play(); sm->play();
    frames(c, 35);
    mark("re-initialize at quality 1");
    init(c, 1, c.provider, c.voices);
    stats(c, "loaded@1");
    s->play(); sf->play();
    frames(c, 35);
    mark("re-initialize at quality 0");
    init(c, 0, c.provider, c.voices);
    s->play();
    frames(c, 35);
    s->release(); sf->release(); sm->release();
    t->release(); tf->release(); tm->release();
}

void voices(Ctx& c)
{
    init(c, c.quality, c.provider, 2);          // limit = min-free-sources (4) + 2
    c.e->setListenerPosition(&c.listenerPos);
    std::vector<ISoundTemplate*> ts;
    std::vector<ISound*> ss;
    for (int i = 0; i < 10; ++i)
    {
        ISoundTemplate* t = tmpl(c, "mono44k_long.wav", true, 0);
        t->setMinDistance(3.0f);
        t->setHalfVolumeDistance(20.0f);
        t->setPosition(v3(2.0f + 3.0f * i, 0.0f, 0.0f));
        t->setVolume(1.0f - 0.05f * i);
        ts.push_back(t);
    }
    c.e->loadSoundFiles();
    for (int i = 0; i < 10; ++i)
        ss.push_back(ts[i]->createSound());
    mark("play ten looping 3D sounds with a limit of six voices");
    for (int i = 0; i < 10; ++i)
        ss[i]->play();
    frames(c, 60, 1);
    mark("listener moves to the far end");
    c.listenerPos.x = 30.0f;
    frames(c, 30, 1);
    mark("stopAllSamples");
    c.e->stopAllSamples();
    frames(c, 10, 1);
    for (int i = 0; i < 10; ++i) { ss[i]->release(); ts[i]->release(); }
}

void retrigger(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    c.e->setListenerPosition(&c.listenerPos);
    c.e->setListenerOrientation(&c.listenerFacing, &c.listenerUp);
    ISoundTemplate* t = tmpl(c, "mono22k.wav", true, 1);
    t->setMinDistance(2.0f);
    t->setHalfVolumeDistance(10.0f);
    t->setPosition(v3(3.0f, 0.0f, 4.0f));
    c.e->loadSoundFiles();
    ISound* s = t->createSound();
    for (int i = 0; i < 60; ++i)
    {
        if (i % 5 == 0)
        {
            mark("retrigger play at frame %d", c.frame);
            s->play();
        }
        frames(c, 1, 1);
    }
    mark("release the sound while it plays (snapshot node)");
    s->release();
    frames(c, 40, 1);
    ISoundTemplate* t2 = tmpl(c, "mono22k.wav", true, 1);
    t2->setPosition(v3(-3.0f, 0.0f, 4.0f));
    c.e->loadSoundFiles();
    ISound* s2 = t2->createSound();
    s2->setSoundFlags(0x100);
    mark("flag 0x100: play twice in one frame");
    s2->play(); s2->play();
    frames(c, 20, 1);
    s2->release();
    t->release();
    t2->release();
}

void tinnitus(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    c.e->setListenerPosition(&c.listenerPos);
    const std::string p = c.path("mono44k_long.wav");
    const char* names[1] = { p.c_str() };
    mark("setTinnitusSound");
    c.e->setTinnitusSound(names, 1);
    ISoundTemplate* t = tmpl(c, "loop_plst.wav", true, 0);
    t->setPosition(v3(4.0f, 0.0f, 0.0f));
    t->setMinDistance(3.0f);
    t->setHalfVolumeDistance(12.0f);
    c.e->loadSoundFiles();
    ISound* s = t->createSound();
    s->play();
    frames(c, 10);
    mark("startTinnitus(0.2, 0.5, 0.3, 0.8, 0)");
    c.e->startTinnitus(0.2f, 0.5f, 0.3f, 0.8f, 0);
    frames(c, 50);
    mark("startTinnitus(0.2, -1, 0.5, 1.0, 0) - held");
    c.e->startTinnitus(0.2f, -1.0f, 0.5f, 1.0f, 0);
    frames(c, 30);
    mark("stopTinnitus(false)");
    c.e->stopTinnitus(false);
    frames(c, 40);
    mark("stopTinnitus(true) on the next one");
    c.e->startTinnitus(0.1f, 0.1f, 0.1f, 0.5f, 0);
    frames(c, 5);
    c.e->stopTinnitus(true);
    frames(c, 10);
    s->release();
    t->release();
}

void triggers(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    c.e->setListenerPosition(&c.listenerPos);
    c.e->setProperty("ambient-fade-time", 0.5f);
    c.e->setProperty("ambient-forced-fade-time", 0.2f);
    const char* files[3] = { "mono22k.wav", "stereo44k.wav", "mono11k_info.wav" };
    const float positions[3] = { 0.0f, 30.0f, 60.0f };
    const float minDist[3] = { 10.0f, 5.0f, 8.0f };
    const float radius[3] = { 0.0f, 2.0f, 0.0f };
    ISoundTemplate* ts[3];
    for (int i = 0; i < 3; ++i)
    {
        ts[i] = tmpl(c, files[i], true, 0);
        ts[i]->setPosition(v3(positions[i], 0.0f, 0.0f));
        ts[i]->setMinDistance(minDist[i]);
        ts[i]->setRadius(radius[i]);
        c.e->addTrigger(ts[i]);
    }
    c.e->loadSoundFiles();
    std::printf("  numTriggers=%d\n", c.e->numTriggers());
    c.listenerPos.x = -5.0f;
    mark("walk through the zones");
    for (int i = 0; i < 75; ++i)
    {
        c.listenerPos.x += 1.0f;
        frames(c, 1, 5);
    }
    mark("listener 1.5 above the second zone (radius 2)");
    c.listenerPos = v3(30.0f, 1.5f, 0.0f);
    frames(c, 20);
    c.listenerPos = v3(30.0f, 3.0f, 0.0f);
    mark("listener 3 above it (outside the radius)");
    frames(c, 20);
    mark("indoors: listener reverb on, the ambient obstruction smooths in");
    c.e->setListenerReverb(0.5f);
    frames(c, 25);
    mark("explicit setTrigger of zone 0, then setTrigger(null)");
    ISound* zone0 = ts[0]->createSound();
    c.e->setTrigger(zone0);
    std::printf("  getTrigger is zone0=%d\n", c.e->getTrigger() == zone0 ? 1 : 0);
    frames(c, 25);
    c.e->setTrigger(0);
    frames(c, 25);
    zone0->release();
    c.e->setListenerReverb(0.0f);
    mark("explicit setTrigger / playAmbient / stopAmbient");
    ISound* trig = c.e->getTrigger();
    std::printf("  getTrigger non-null=%d\n", trig ? 1 : 0);
    ISoundTemplate* ta = tmpl(c, "mono44k_long.wav", false, 0);
    c.e->loadSoundFiles();
    ISound* amb = ta->createSound();
    c.e->playAmbient(amb, 0.5f);
    frames(c, 25);
    c.e->stopAmbient(0.3f);
    frames(c, 15);
    c.e->removeTrigger(ts[1]);
    std::printf("  numTriggers after remove=%d\n", c.e->numTriggers());
    c.e->removeAllTriggers();
    std::printf("  numTriggers after removeAll=%d\n", c.e->numTriggers());
    frames(c, 10);
    amb->release();
    ta->release();
    for (int i = 0; i < 3; ++i)
        ts[i]->release();
}

void reverb(Ctx& c)
{
    init(c, c.quality, "Software Audio + EAX", c.voices);
    c.e->setListenerPosition(&c.listenerPos);
    const std::string preset = c.path("preset.rev");
    mark("setReverb(preset)");
    c.e->setReverb(preset.c_str());
    c.e->setListenerReverb(0.7f);
    ISoundTemplate* t = tmpl(c, "loop_plst.wav", true, 0);
    t->setPosition(v3(5.0f, 0.0f, 0.0f));
    t->setReverbLevel(0.6f);
    c.e->loadSoundFiles();
    ISound* s = t->createSound();
    HalfObstruction obstruction;
    s->setObstructionEvaluator(&obstruction);
    s->play();
    frames(c, 30);
    mark("listener reverb 0.2, underwater");
    c.e->setListenerReverb(0.2f);
    c.e->setIsUnderwater(true);
    c.e->setUnderwaterOcclusion(0.5f);
    frames(c, 20);
    mark("setReverb(missing)");
    c.e->setReverb("nonexistent.rev");
    frames(c, 10);
    s->release();
    t->release();
}

void props(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    int n = 0;
    const char** names = (const char**) c.e->get3dProviders(&n);
    for (int i = 0; i < n; ++i)
        std::printf("  provider[%d]=%s\n", i, names[i]);
    std::printf("  max-distance-cutoff=%.7g enable-culling=%.7g\n", c.e->getProperty("max-distance-cutoff"), c.e->getProperty("enable-culling"));
    c.e->setProperty("bogus-property", 1.0f);
    std::printf("  bogus=%.7g\n", c.e->getProperty("bogus-property"));
    c.e->setProperty("ambient-fade-time", 500.0f);
    std::printf("  ambient-fade-time=%.7g\n", c.e->getProperty("ambient-fade-time"));
    c.e->setProperty("ambient-indoor-obstruction", -3.0f);
    std::printf("  ambient-indoor-obstruction=%.7g\n", c.e->getProperty("ambient-indoor-obstruction"));
    c.e->setVolumeForGroup(9, 0.5f);
    c.e->setVolumeForGroup(3, 1.7f);
    std::printf("  group9=%.7g group3=%.7g\n", c.e->getVolumeForGroup(9), c.e->getVolumeForGroup(3));
    c.e->setMasterVolume(2.0f);
    std::printf("  master=%.7g\n", c.e->getMasterVolume());
    c.e->setDopplerEffect(-1.0f);
    std::printf("  doppler=%.7g\n", c.e->getDopplerEffect());
    c.e->setDopplerEffect(2.5f);
    std::printf("  doppler=%.7g\n", c.e->getDopplerEffect());
    c.e->setListenerReverb(2.0f);
    std::printf("  listenerReverb=%.7g\n", c.e->getListenerReverb());
    c.e->setUnderwaterOcclusion(0.4f);
    c.e->setIsUnderwater(true);
    std::printf("  underwater=%d occlusion=%.7g interactive=%d\n", c.e->getIsUnderwater() ? 1 : 0, c.e->getUnderwaterOcclusion(), c.e->getInteractiveMode() ? 1 : 0);
    int count = -1;
    std::printf("  queryPlayingSamples -> %d (count %d)\n", c.e->queryPlayingSamples(&count), count);
    ISoundTemplate* t = tmpl(c, "mono22k.wav", false, 1);
    c.e->getSoundFiles(&count);
    std::printf("  getSoundFiles before load: %d\n", count);
    c.e->loadSoundFiles();
    c.e->getSoundFiles(&count);
    std::printf("  getSoundFiles after load: %d\n", count);
    ISound* s = t->createSound();
    void* info = s->getSampleInfo(0);
    if (info)
    {
        const float* f = (const float*) info;
        std::printf("  sampleInfo intro=%.7g loop=%.7g outro=%.7g\n", f[0], f[1], f[2]);
    }
    std::printf("  template getters: loop=%u vol=%.7g pitch=%.7g pan=%.7g min=%.7g half=%.7g 3d=%d stop=%d reverb=%.7g\n",
                t->getLoopCount(), t->getVolume(), t->getPitch(), t->getPan(), t->getMinDistance(), t->getHalfVolumeDistance(),
                t->getIs3dSound() ? 1 : 0, t->getStopType(), t->getReverbLevel());
    std::printf("  checkSampleAvailable(0)=%d (1)=%d\n", t->checkSampleAvailable(0) ? 1 : 0, t->checkSampleAvailable(1) ? 1 : 0);
    stats(c, "props");
    s->release();
    t->release();
}

void reinit(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    ISoundTemplate* t = tmpl(c, "mono44k_long.wav", false, 0);
    c.e->loadSoundFiles();
    ISound* s = t->createSound();
    s->play();
    frames(c, 15);
    mark("initialize again with the same arguments");
    init(c, c.quality, c.provider, c.voices);
    frames(c, 10);
    mark("initialize with 8 voices while playing");
    init(c, c.quality, c.provider, 8);
    frames(c, 25);
    mark("shutdown");
    c.e->shutdown();
    std::printf("  isInitialized=%d\n", c.e->isInitialized() ? 1 : 0);
    stats(c, "after shutdown");
    mark("initialize after shutdown and play");
    init(c, c.quality, c.provider, c.voices);
    c.e->loadSoundFiles();
    s->play();
    frames(c, 25);
    s->release();
    t->release();
}

void interactive(Ctx& c)
{
    c.e->setInteractiveMode(true);
    init(c, c.quality, c.provider, c.voices);
    ISoundTemplate* t = c.e->createTemplate();
    const std::string p = c.path("mono22k.wav");
    t->setFilename(p.c_str());
    t->setIs3dSound(false);
    mark("setIsUsed in interactive mode (loads now)");
    t->setIsUsed();
    stats(c, "after setIsUsed");
    ISound* s = t->createSound();
    s->play();
    frames(c, 25);
    ISoundTemplate* t2 = c.e->createTemplate();
    const std::string p2 = c.path("stereo44k.wav");
    t2->setFilename(p2.c_str());
    t2->setIs3dSound(false);
    mark("play without setIsUsed (loads on demand)");
    ISound* s2 = t2->createSound();
    s2->play();
    frames(c, 25);
    s->release(); s2->release();
    t->release(); t2->release();
}

void errors(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    ISoundTemplate* t = tmpl(c, "nofile.wav", false, 1);
    const std::string a = c.path("mono22k.wav");
    const std::string b = c.path("nofile.wav");
    const char* names[3] = { a.c_str(), "", b.c_str() };
    ISoundTemplate* t3 = c.e->createTemplate();
    t3->setFilenames(names, 3);
    t3->setIs3dSound(false);
    t3->setIsUsed();
    mark("loadSoundFiles with a missing file");
    c.e->loadSoundFiles();
    stats(c, "loaded");
    ISound* s = t->createSound();
    std::printf("  play missing: playSample -> %d isPlaying=%d\n", s->playSample(0) & 0xFF, s->isPlaying() ? 1 : 0);
    ISound* s3 = t3->createSound();
    std::printf("  playSample(0) -> %d, (1) -> %d, (2) -> %d\n", s3->playSample(0) & 0xFF, s3->playSample(1) & 0xFF, s3->playSample(2) & 0xFF);
    int n = 0;
    const char** fn = t3->getFilenames(&n);
    std::printf("  getFilenames n=%d [0]=%s [1]=%s [2]=%s\n", n, fn[0] ? "ok" : "null", fn[1] ? "ok" : "null", fn[2] ? "ok" : "null");
    frames(c, 20);
    s->release(); s3->release();
    t->release(); t3->release();
}

void ogg(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    ISoundTemplate* t = tmpl(c, "stream.ogg", false, 0);
    c.e->loadSoundFiles();
    stats(c, "loaded");
    ISound* s = t->createSound();
    mark("play streamed ogg (infinite loop)");
    s->play();
    frames(c, 90);
    mark("stop");
    s->stop();
    frames(c, 30);
    const std::string music = c.path("music.ogg");
    mark("loadMusic/playMusic");
    std::printf("  loadMusic -> %d\n", c.e->loadMusic(music.c_str()) ? 1 : 0);
    std::printf("  playMusic -> %d\n", c.e->playMusic() ? 1 : 0);
    c.e->setVolumeForGroup(1, 0.8f);
    frames(c, 60);
    mark("stopMusic(1.0)");
    std::printf("  stopMusic -> %d\n", c.e->stopMusic(1.0f) ? 1 : 0);
    frames(c, 45);
    mark("music that does not exist, then playMusic twice and stopMusic(0)");
    const std::string missing = c.path("nofile.ogg");
    std::printf("  loadMusic(missing) -> %d\n", c.e->loadMusic(missing.c_str()) ? 1 : 0);
    std::printf("  playMusic -> %d\n", c.e->playMusic() ? 1 : 0);
    frames(c, 10);
    std::printf("  loadMusic -> %d\n", c.e->loadMusic(music.c_str()) ? 1 : 0);
    std::printf("  playMusic -> %d, again -> %d\n", c.e->playMusic() ? 1 : 0, c.e->playMusic() ? 1 : 0);
    frames(c, 20);
    std::printf("  stopMusic(0) -> %d, again -> %d\n", c.e->stopMusic(0.0f) ? 1 : 0, c.e->stopMusic(0.0f) ? 1 : 0);
    frames(c, 5);
    s->release();
    t->release();
}

// Envelopes (randomised and not), host evaluators, first-person sounds, multi-file templates with
// per-quality sample limits, user data, and a multi-step update.
void envelopes(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    c.e->setListenerPosition(&c.listenerPos);
    SoundEnvelopePoint vol[4] = { { 0.0f, 0.0f, 0.0f }, { 0.25f, 1.0f, 0.5f }, { 0.75f, 1.0f, 0.5f }, { 1.0f, 0.0f, 0.0f } };
    SoundEnvelopePoint pit[3] = { { 0.0f, 0.8f, 0.0f }, { 0.5f, 1.3f, 0.0f }, { 1.0f, 0.9f, 0.0f } };
    ISoundTemplate* t = tmpl(c, "loop_plst.wav", true, 0);
    t->setPosition(v3(3.0f, 0.0f, 2.0f));
    t->setVolumeEnvelope(0.0f, 1.0f, 0.0f, 1.0f, true, 4, vol);
    t->setPitchEnvelope(0.0f, 1.0f, 0.3f, 2.0f, false, 3, pit);
    float xMin, xMax, yMin, yMax; bool randomize; unsigned int count;
    const SoundEnvelopePoint* got = t->getVolumeEnvelope(&xMin, &xMax, &yMin, &yMax, &randomize, &count);
    std::printf("  volume envelope back: %.7g %.7g %.7g %.7g rnd=%d n=%u p1=(%.7g,%.7g,%.7g)\n", xMin, xMax, yMin, yMax,
                randomize ? 1 : 0, count, got ? got[1].x : -1.0f, got ? got[1].yMin : -1.0f, got ? got[1].yMax : -1.0f);
    const std::string a = c.path("mono22k.wav"), b = c.path("mono11k_info.wav"), d = c.path("stereo44k.wav");
    const char* names[3] = { a.c_str(), b.c_str(), d.c_str() };
    ISoundTemplate* tm = c.e->createTemplate();
    tm->setFilenames(names, 3);
    tm->setIs3dSound(false);
    tm->setMaxSamples((SoundQuality) 0, 3);
    tm->setMaxSamples((SoundQuality) 1, 2);
    tm->setMaxSamples((SoundQuality) 2, 1);
    tm->setIsUsed();
    c.e->loadSoundFiles();
    stats(c, "loaded");
    ISound* s = t->createSound();
    s->setUserData(0, (void*) 0x1234);
    s->setUserData(2, (void*) 0x5678);
    std::printf("  userData 0=%p 2=%p is1P=%d\n", s->getUserData(0), s->getUserData(2), s->getIs1PSound() ? 1 : 0);
    mark("play with envelopes");
    s->play();
    frames(c, 40);
    mark("host evaluators take over, pitch param ramps");
    RampEvaluator ramp;
    float param = 0.0f;
    s->setVolumeEvaluator(&ramp, &param);
    s->setPitchEvaluator(&ramp, &param);
    for (int i = 0; i < 20; ++i)
    {
        param += 0.05f;
        frames(c, 1);
    }
    std::printf("  getVolumeEvaluator is host=%d\n", s->getVolumeEvaluator() == &ramp ? 1 : 0);
    mark("first-person: position and velocity collapse to the listener");
    s->setIs1PSound(true);
    frames(c, 10);
    s->setIs1PSound(false);
    s->forceStop();
    frames(c, 5);
    mark("multi-file template: random index plays, then explicit samples");
    ISound* sm = tm->createSound();
    for (int i = 0; i < 6; ++i)
    {
        sm->play();
        frames(c, 5);
    }
    std::printf("  playSample(2)=%d checkSampleAvailable(1)=%d\n", sm->playSample(2) & 0xFF, tm->checkSampleAvailable(1) ? 1 : 0);
    frames(c, 20);
    mark("update(3) steps, then update(0)");
    c.e->update(3);
    c.e->update(0);
    stats(c, "after update(3)");
    frames(c, 5);
    sm->release();
    s->release();
    t->release();
    tm->release();
}

// Every provider row, including the ones this machine cannot open: the codes, the EAX probe, the
// re-open by index ("0".."6") and the failure paths of openProvider.
void providers(Ctx& c)
{
    int n = 0;
    const char** names = (const char**) c.e->get3dProviders(&n);
    for (int i = 0; i < n; ++i)
    {
        const int err = init(c, c.quality, names[i], c.voices);
        if (!err)
        {
            ISoundTemplate* t = tmpl(c, "mono22k.wav", true, 1);
            c.e->loadSoundFiles();
            ISound* s = t->createSound();
            s->play();
            frames(c, 8);
            s->release();
            t->release();
        }
        mark("shutdown after row %d", i);
        c.e->shutdown();
    }
    mark("rows by index, then a name that does not exist");
    std::printf("  initialize(\"6\") -> %d\n", c.e->initialize((SoundQuality) 0, "6", 16));
    c.e->shutdown();
    std::printf("  initialize(\"9\") -> %d\n", c.e->initialize((SoundQuality) 0, "9", 16));
    c.e->shutdown();
    std::printf("  initialize(\"No Such Provider\") -> %d\n", c.e->initialize((SoundQuality) 0, "No Such Provider", 16));
    c.e->shutdown();
    std::printf("  initialize(null) -> %d\n", c.e->initialize((SoundQuality) 0, 0, 16));
    frames(c, 3);
}

// The sanity clamps: NaN and out-of-range positions, directions and velocities, degenerate
// listener vectors, a negative quality, out-of-range group and property values.
void sanity(Ctx& c)
{
    init(c, c.quality, c.provider, c.voices);
    c.e->setListenerPosition(&c.listenerPos);
    c.e->setListenerOrientation(&c.listenerFacing, &c.listenerUp);
    c.e->setListenerVelocity(&c.listenerVel);
    const float nan = std::strtof("nan", 0);
    ISoundTemplate* t = tmpl(c, "mono44k_long.wav", true, 0);
    t->setMinDistance(1.0f);
    t->setHalfVolumeDistance(5.0f);
    t->setPosition(v3(2.0f, 0.0f, 0.0f));
    t->setDirection(v3(0.05f, 0.0f, 0.0f));          // shorter than the 0.01 threshold: no direction
    t->setConeInnerAngle(90.0f);
    c.e->loadSoundFiles();
    ISound* s = t->createSound();
    MovingNode node;
    s->setParent(&node);
    s->play();
    frames(c, 5);
    mark("parent flies past 20000 m, then to NaN");
    node.m.m[12] = 25000.0f;
    frames(c, 5);
    node.m.m[12] = nan;
    frames(c, 5);
    node.m.m[12] = 2.0f;
    mark("parent velocity above 300 m/s, then NaN");
    node.v = v3(0.0f, 0.0f, 400.0f);
    frames(c, 5);
    node.v = v3(nan, 0.0f, 0.0f);
    frames(c, 5);
    node.v = v3(0.0f, 0.0f, 0.0f);
    mark("a real direction, then a NaN direction through a scaled parent");
    t->setDirection(v3(0.0f, 0.0f, 1.0f));
    frames(c, 5);
    node.m.m[0] = 3.0f; node.m.m[10] = 3.0f;         // |dir| >= 1.1 after the parent's scale
    frames(c, 5);
    node.m.m[0] = 1.0f; node.m.m[10] = 1.0f;
    mark("listener at 400 m/s, then facing == up (degenerate frame)");
    c.listenerVel = v3(400.0f, 0.0f, 0.0f);
    frames(c, 5);
    c.listenerVel = v3(0.0f, 0.0f, 0.0f);
    c.listenerFacing = v3(0.0f, 1.0f, 0.0f);
    frames(c, 5);
    c.listenerFacing = v3(0.0f, 0.0f, 1.0f);
    mark("out-of-range setters");
    t->setVolume(3.0f); t->setPitch(9.0f); t->setPan(-1.0f); t->setReverbLevel(2.0f);
    t->setConeOuterVolume(-2.0f); t->setGlobalEffectLevel(5.0f); t->setMinDistance(0.0f); t->setHalfVolumeDistance(-1.0f);
    std::printf("  vol=%.7g pitch=%.7g pan=%.7g reverb=%.7g outer=%.7g effect=%.7g min=%.7g half=%.7g\n",
                t->getVolume(), t->getPitch(), t->getPan(), t->getReverbLevel(), t->getConeOuterVolume(),
                t->getGlobalEffectLevel(), t->getMinDistance(), t->getHalfVolumeDistance());
    frames(c, 5);
    t->setStopType(7);
    std::printf("  stopType(7)=%d\n", t->getStopType());
    t->setMaxSamples((SoundQuality) 1, -5);
    std::printf("  maxSamples(1, -5)=%d\n", t->getMaxSamples((SoundQuality) 1));
    s->forceStop();
    frames(c, 3);
    s->release();
    t->release();
}

struct Scenario { const char* name; void (*fn)(Ctx&); };
const Scenario kScenarios[] =
{
    { "providers", providers }, { "sanity", sanity },
    { "oneshot2d", oneshot2d }, { "stereo44", stereo44 }, { "loop3d", loop3d }, { "cull", cull },
    { "quality", quality }, { "voices", voices }, { "retrigger", retrigger }, { "tinnitus", tinnitus },
    { "triggers", triggers }, { "reverb", reverb }, { "props", props }, { "reinit", reinit },
    { "interactive", interactive }, { "errors", errors }, { "envelopes", envelopes }, { "ogg", ogg },
};

} // namespace

typedef ISoundEngine* (*GetSoundEngineFn)();
typedef void (*ShutdownSoundEngineFn)();

int main(int argc, char** argv)
{
    std::setvbuf(stdout, 0, _IONBF, 0);
    if (argc < 6)
    {
        std::printf("usage: host.exe <BF2Audio.dll> <game dir> <shim BF2OpenAL.dll> <signals dir> <scenario|list> [provider] [quality] [voices]\n");
        return 2;
    }
    const std::string dllPath = argv[1], gameDir = argv[2], shimPath = argv[3];
    Ctx c;
    c.signals = argv[4];
    const char* scenario = argv[5];
    if (argc > 6) c.provider = argv[6];
    if (argc > 7) c.quality = std::atoi(argv[7]);
    if (argc > 8) c.voices = std::atoi(argv[8]);
    if (!std::strcmp(scenario, "list"))
    {
        for (size_t i = 0; i < sizeof(kScenarios) / sizeof(kScenarios[0]); ++i)
            std::printf("%s\n", kScenarios[i].name);
        return 0;
    }
    const Scenario* chosen = 0;
    for (size_t i = 0; i < sizeof(kScenarios) / sizeof(kScenarios[0]); ++i)
        if (!std::strcmp(kScenarios[i].name, scenario))
            chosen = &kScenarios[i];
    if (!chosen)
    {
        std::printf("unknown scenario %s\n", scenario);
        return 2;
    }

    SetDllDirectoryA(gameDir.c_str());
    HMODULE memory = LoadLibraryA((gameDir + "\\Memory.dll").c_str());
    HMODULE shim   = LoadLibraryA(shimPath.c_str());
    HMODULE audio  = LoadLibraryA(dllPath.c_str());
    std::printf("Memory.dll=%s shim=%s audio=%s\n", memory ? "ok" : "FAILED", shim ? "ok" : "FAILED", audio ? "ok" : "FAILED");
    if (!memory || !shim || !audio)
        return 1;
    g_mark = (MarkFn) GetProcAddress(shim, "alshim_mark");
    GetSoundEngineFn getSoundEngine = (GetSoundEngineFn) GetProcAddress(audio, "getSoundEngine");
    ShutdownSoundEngineFn shutdownSoundEngine = (ShutdownSoundEngineFn) GetProcAddress(audio, "shutdownSoundEngine");
    if (!getSoundEngine || !shutdownSoundEngine || !g_mark)
    {
        std::printf("exports missing\n");
        return 1;
    }
    timeBeginPeriod(1);
    QueryPerformanceFrequency(&g_qpcFreq);
    QueryPerformanceCounter(&g_qpcStart);
    c.e = getSoundEngine();
    StdoutTrace trace;
    c.e->setSoundTrace(&trace);
    c.e->setSoundFileIO(0);
    c.e->setProperty("soundfile-use-usagerefcount", 1.0f);
    mark("scenario %s", chosen->name);
    chosen->fn(c);
    mark("shutdown");
    c.e->shutdown();
    stats(c, "end");
    shutdownSoundEngine();
    mark("done");
    return 0;
}
