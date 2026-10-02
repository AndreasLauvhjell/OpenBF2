// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: bf2audio-smoke]
// Loads the rebuilt audio DLL, opens a provider, and plays one WAV with trace output.
// Preloads reference dependencies by full path without modifying the game directory.
// Usage: smoke.exe <BF2Audio.dll> <game dir> <file.wav> [provider display name] [seconds]
// Build: Code\Build\bf2audio_smoke.cmd. Requires the DLL image define for its interface layout.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "../../BF2/Sound/NullSound.h"
#include "../../BF2/Sound/ISoundTrace.h"
#include "../../BF2/Sound/ISoundFileIO.h"

using namespace dice::hfe;

namespace {

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
        std::fflush(stdout);
    }
};

// The block ISoundEngine::getStats answers (Sound/SoundEngine/BfSoundEngine.h SoundStats).
struct Stats
{
    const char* providerInfo;
    int playing, waiting, delayed, loadedSampleBytes, streamedBytes, streamingBufferBytes;
    int alBuffers, streamingBuffers, loadedFiles, sounds, samples, sources, maxVoices;
};

typedef ISoundEngine* (*GetSoundEngineFn)();
typedef void (*ShutdownSoundEngineFn)();

void describeAddress(const char* label, void* addr)
{
    HMODULE mod = 0;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR) addr, &mod);
    char name[MAX_PATH] = "";
    if (mod)
        GetModuleFileNameA(mod, name, MAX_PATH);
    const char* base = std::strrchr(name, '\\');
    std::printf("  %s %p = %s + 0x%X\n", label, addr, base ? base + 1 : name,
                mod ? (unsigned int) ((char*) addr - (char*) mod) : 0u);
}

// The crash reporter: exception code, faulting address as module + RVA (map it with the DLL's
// .map file at its preferred base 0x10000000), and the EBP chain's return addresses.
LONG WINAPI onCrash(EXCEPTION_POINTERS* ep)
{
    std::printf("CRASH: exception %08lX\n", ep->ExceptionRecord->ExceptionCode);
    describeAddress("at   ", ep->ExceptionRecord->ExceptionAddress);
    void** ebp = (void**) ep->ContextRecord->Ebp;
    for (int i = 0; i < 24 && ebp && !IsBadReadPtr(ebp, 8); ++i)
    {
        describeAddress("frame", ebp[1]);
        void** next = (void**) ebp[0];
        if (next <= ebp)
            break;
        ebp = next;
    }
    std::fflush(stdout);
    return EXCEPTION_EXECUTE_HANDLER;
}

HMODULE loadByPath(const std::string& path, const char* what)
{
    HMODULE m = LoadLibraryA(path.c_str());
    std::printf("%-14s %s -> %p%s\n", what, path.c_str(), (void*) m, m ? "" : "  (LoadLibrary FAILED)");
    return m;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        std::printf("usage: smoke.exe <BF2Audio.dll> <game dir> <file.wav> [provider] [seconds]\n");
        return 2;
    }
    std::setvbuf(stdout, 0, _IONBF, 0);         // a crash must not eat the log
    SetUnhandledExceptionFilter(onCrash);
    const std::string dllPath  = argv[1];
    const std::string gameDir  = argv[2];
    const std::string wavPath  = argv[3];
    const char* provider       = argc > 4 ? argv[4] : "Software Audio";
    const int   seconds        = argc > 5 ? std::atoi(argv[5]) : 4;

    // The OpenAL provider is loaded from the content installation. The rebuilt audio DLL
    // uses the same dynamic CRT as this host and needs no retail Memory.dll or MSVCR71 heap.
    std::printf("SetDllDirectory(%s) -> %d\n", gameDir.c_str(), SetDllDirectoryA(gameDir.c_str()) ? 1 : 0);
    HMODULE openal = loadByPath(gameDir + "\\BF2OpenAL.dll", "BF2OpenAL.dll");
    HMODULE audio  = loadByPath(dllPath, "BF2Audio.dll");
    if (!audio)
    {
        std::printf("GetLastError = %lu\n", GetLastError());
        return 1;
    }
    GetSoundEngineFn getSoundEngine = (GetSoundEngineFn) GetProcAddress(audio, "getSoundEngine");
    ShutdownSoundEngineFn shutdownSoundEngine = (ShutdownSoundEngineFn) GetProcAddress(audio, "shutdownSoundEngine");
    std::printf("getSoundEngine=%p shutdownSoundEngine=%p\n", (void*) getSoundEngine, (void*) shutdownSoundEngine);
    if (!getSoundEngine || !shutdownSoundEngine)
        return 1;

    ISoundEngine* engine = getSoundEngine();
    std::printf("engine=%p (same on second call: %s)\n", (void*) engine, getSoundEngine() == engine ? "yes" : "NO");
    StdoutTrace trace;
    engine->setSoundTrace(&trace);
    engine->setSoundFileIO(0);                  // the DLL's own fopen-based reader
    engine->setProperty("soundfile-use-usagerefcount", 1.0f);   // what BfSound::init sets

    int count = 0;
    const char** names = (const char**) engine->get3dProviders(&count);
    std::printf("providers: %d\n", count);
    for (int i = 0; i < count; ++i)
        std::printf("  [%d] %s\n", i, names[i]);

    std::printf("initialize(quality 0, \"%s\", 16) ...\n", provider);
    const int err = engine->initialize((SoundQuality) 0, provider, 16);
    std::printf("  -> %d (%s), isInitialized=%d, eax=%d\n", err,
                err == 0 ? "ok" : err == 1 ? "noprovider" : err == 2 ? "noproviderdll" : err == 3 ? "openaldenied" : "openalerror",
                engine->isInitialized() ? 1 : 0, engine->getEaxVersion());
    if (err)
    {
        shutdownSoundEngine();
        return 1;
    }

    engine->setMasterVolume(1.0f);
    engine->setVolumeForGroup(0, 1.0f);

    ISoundTemplate* t = engine->createTemplate();
    t->setFilename(wavPath.c_str());
    t->setIs3dSound(false);
    t->setLoopCount(1);
    t->setVolume(1.0f);
    t->setIsUsed();                             // marks the file used (usage-refcount mode)
    int pending = 0;
    engine->getSoundFiles(&pending);
    std::printf("wanted+used+unloaded files before loadSoundFiles(): %d\n", pending);
    engine->loadSoundFiles();                   // what the client calls once a level's templates exist
    engine->getSoundFiles(&pending);
    std::printf("... after: %d; checkSampleAvailable(0)=%d\n", pending, t->checkSampleAvailable(0) ? 1 : 0);
    ISound* sound = t->createSound();
    std::printf("template=%p sound=%p\n", (void*) t, (void*) sound);

    const Stats* stats = (const Stats*) engine->getStats();
    std::printf("after load: files=%d loadedBytes=%d alBuffers=%d provider=\"%s\"\n",
                stats->loadedFiles, stats->loadedSampleBytes, stats->alBuffers, stats->providerInfo ? stats->providerInfo : "");

    sound->play();
    std::printf("play() -> isPlaying=%d\n", sound->isPlaying() ? 1 : 0);
    const int frames = seconds * 30;
    for (int frame = 0; frame < frames; ++frame)
    {
        engine->update(1);
        if (frame % 15 == 0)
        {
            stats = (const Stats*) engine->getStats();
            std::printf("  frame %3d: playing=%d waiting=%d delayed=%d samples=%d sources=%d isPlaying=%d\n",
                        frame, stats->playing, stats->waiting, stats->delayed, stats->samples, stats->sources,
                        sound->isPlaying() ? 1 : 0);
            std::fflush(stdout);
        }
        Sleep(33);
    }
    sound->release();
    t->release();
    engine->shutdown();
    std::printf("shutdown: isInitialized=%d\n", engine->isInitialized() ? 1 : 0);
    shutdownSoundEngine();
    std::printf("shutdownSoundEngine done\n");
    (void) openal;
    return 0;
}
