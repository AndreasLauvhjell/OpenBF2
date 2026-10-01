// [CHOSEN, not recovered] [TEMP] diagnostic tracing of projectile-to-particle explosion effects.
// BF2_TRACE_FX enables stderr and BF2FxTrace.log output: first 64 hits, then every 256th per call site.
// Compiled out of dedicated builds. [RETIRE-WHEN: fx-explosion-diagnosed]
// Remove after the missing-explosion lifecycle is diagnosed and verified in a live client.
#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#if DEDICATED
#define BF2_FX_TRACE(...) ((void)0)
#else
namespace dice {
namespace hfe {
namespace fxtrace {

inline bool armed()
{
    static int value = -1;
    if (value < 0)
        value = std::getenv("BF2_TRACE_FX") ? 1 : 0;
    return value != 0;
}

inline void write(int hit, const char* format, ...)
{
    static std::FILE* file = 0;
    static bool opened = false;
    if (!opened)
    {
        opened = true;
        file = std::fopen("BF2FxTrace.log", "a");
    }
    char line[640];
    va_list args;
    va_start(args, format);
    std::vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    std::fprintf(stderr, "[FXTRACE #%d] %s\n", hit, line);
    std::fflush(stderr);
    if (file)
    {
        std::fprintf(file, "[FXTRACE #%d] %s\n", hit, line);
        std::fflush(file);
    }
}

} // namespace fxtrace
} // namespace hfe
} // namespace dice

#define BF2_FX_TRACE(...)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if (::dice::hfe::fxtrace::armed())                                                         \
        {                                                                                          \
            static int s_fxTraceHits = 0;                                                          \
            ++s_fxTraceHits;                                                                       \
            if (s_fxTraceHits <= 64 || (s_fxTraceHits & 255) == 0)                                 \
                ::dice::hfe::fxtrace::write(s_fxTraceHits, __VA_ARGS__);                           \
        }                                                                                          \
    } while (0)
#endif
