// [INFERRED] Default implementation names and source placement.

// Fallback host callbacks: CRT file I/O, debugger tracing and an inert profiler.
// Passing null to the callback setters restores these defaults.


#ifndef SOUND_SOUNDENGINE_SOUNDDEFAULTS_H
#define SOUND_SOUNDENGINE_SOUNDDEFAULTS_H

#include "../ISoundFileIO.h"
#include "../ISoundTrace.h"

namespace dice {
namespace hfe {

// Default file-access implementation.
class DefaultSoundFileIO : public ISoundFileIO
{
public:
    virtual void* open(const char* name);
    virtual int   close(void* handle);
    virtual int   seek(void* handle, int offset, int origin);
    virtual int   read(void* handle, void* buffer, int bytes);
};

// Default trace implementation.
class DefaultSoundTrace : public ISoundTrace
{
public:
    virtual void error(const char* msg, const char* file, int line);
    virtual void warning(const char* msg, const char* file, int line);
    virtual void info(const char* msg, const char* file, int line);
    virtual void assertFailed(const char* msg, const char* file, int line);  // exit(-1)
    virtual void debug(const char* msg, const char* file, int line);
};

// The host's profiler interface. Fifteen slots is all the image reveals (see the header block);
// the DLL stores the pointer and never calls through it, so it stays opaque here.
class IProfilerClient;

// Null profiler interface: preserve all fifteen virtual slots. [UNRESOLVED] Method names.
class NullProfilerClient
{
public:
    virtual int  slot0()                       { return 1; }
    virtual int  slot1()                       { return 1; }
    virtual int  slot2()                       { return 1; }
    virtual int  slot3(int)                    { return 0; }
    virtual void slot4()                       {}
    virtual void slot5()                       {}
    virtual void slot6(int, int, int)          {}
    virtual void slot7(int)                    {}
    virtual void slot8(int, int)               {}
    virtual void slot9(int)                    {}
    virtual bool slot10()                      { return false; }
    virtual void slot11(int)                   {}
    virtual bool slot12()                      { return false; }
    virtual void slot13(int)                   {}
    virtual bool slot14(int)                   { return false; }
};

extern NullProfilerClient g_nullProfilerClient;
extern IProfilerClient*   g_profilerClient;         // ds:never null

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_SOUNDDEFAULTS_H
