// [CHOSEN, not recovered] Selects the host display-mode ABI: three VC7.1 vector records
// in a 0x40-byte retail carrier, or native STL vectors for the rebuilt pair.
// Renderer-internal VC7.1 storage remains unchanged.

#ifndef RENDDX9ABI_DISPLAYMODEABI_H
#define RENDDX9ABI_DISPLAYMODEABI_H

#include "PluginAbi.h"

#include <new>

namespace renddx9abi {

struct DisplayModeMultiSample
{
    unsigned int type;
    unsigned int quality;
};

static_assert(sizeof(DisplayModeMultiSample) == 8,
              "the display-mode multisample record is two dwords");

#if BF2_PLUGIN_ABI_NATIVE

// [PLATFORM] Native rebuilt-client carrier.  Both images compile this definition with /MD and
// _ITERATOR_DEBUG_LEVEL=0, as enforced by PluginAbi.h.  It deliberately does not reuse the
// renderer's internal fixed-layout vectors.
struct HostDisplayMode
{
    unsigned char fullscreen;
    unsigned char pad01[3];
    unsigned short width;
    unsigned short height;
    unsigned short refreshHz;
    unsigned char bpp;
    unsigned char pad0B;
    std::vector<unsigned int> depthFormats;
    std::vector<DisplayModeMultiSample> multiSamples;
    unsigned char vsync;
    unsigned char pad25[3];
    std::vector<void*> windows;

    HostDisplayMode()
        : fullscreen(0), width(0), height(0), refreshHz(0), bpp(0), vsync(0)
    {
        pad01[0] = pad01[1] = pad01[2] = 0;
        pad0B = 0;
        pad25[0] = pad25[1] = pad25[2] = 0;
    }
};

static_assert(sizeof(void*) == 4, "the native plugin boundary is x86-only");
static_assert(sizeof(std::vector<unsigned int>) == 12,
              "native display-mode ABI requires the pinned release MSVC vector layout");
static_assert(sizeof(HostDisplayMode) == 0x34,
              "native x86 carrier is scalars plus three 12-byte std::vectors");

#else

// Retail BF2 1.50 carrier, read from BF2.exe 0x454370 and RendDX9.dll 0x1000CA40.
struct HostDisplayMode
{
    /* 0x00 */ unsigned char       fullscreen;
    /* 0x01 */ unsigned char       pad01[3];
    /* 0x04 */ unsigned short      width;
    /* 0x06 */ unsigned short      height;
    /* 0x08 */ unsigned short      refreshHz;
    /* 0x0A */ unsigned char       bpp;
    /* 0x0B */ unsigned char       pad0B;
    /* 0x0C */ vc71::vector_layout depthFormats;
    /* 0x1C */ vc71::vector_layout multiSamples;
    /* 0x2C */ unsigned char       vsync;
    /* 0x2D */ unsigned char       pad2D[3];
    /* 0x30 */ vc71::vector_layout windows;
};

static_assert(sizeof(HostDisplayMode) == 0x40,
              "BF2Render::init allocates the retail 0x40-byte carrier");

#endif

inline const unsigned int* hostDepthFirst(const HostDisplayMode& mode)
{
#if BF2_PLUGIN_ABI_NATIVE
    return mode.depthFormats.empty() ? 0 : &mode.depthFormats[0];
#else
    return reinterpret_cast<const unsigned int*>(mode.depthFormats.first);
#endif
}

inline const unsigned int* hostDepthLast(const HostDisplayMode& mode)
{
#if BF2_PLUGIN_ABI_NATIVE
    return mode.depthFormats.empty() ? 0 : (&mode.depthFormats[0] + mode.depthFormats.size());
#else
    return reinterpret_cast<const unsigned int*>(mode.depthFormats.last);
#endif
}

inline const DisplayModeMultiSample* hostMultiSampleFirst(const HostDisplayMode& mode)
{
#if BF2_PLUGIN_ABI_NATIVE
    return mode.multiSamples.empty() ? 0 : &mode.multiSamples[0];
#else
    return reinterpret_cast<const DisplayModeMultiSample*>(mode.multiSamples.first);
#endif
}

inline const DisplayModeMultiSample* hostMultiSampleLast(const HostDisplayMode& mode)
{
#if BF2_PLUGIN_ABI_NATIVE
    return mode.multiSamples.empty() ? 0 : (&mode.multiSamples[0] + mode.multiSamples.size());
#else
    return reinterpret_cast<const DisplayModeMultiSample*>(mode.multiSamples.last);
#endif
}

inline void* const* hostWindowFirst(const HostDisplayMode& mode)
{
#if BF2_PLUGIN_ABI_NATIVE
    return mode.windows.empty() ? 0 : &mode.windows[0];
#else
    return reinterpret_cast<void* const*>(mode.windows.first);
#endif
}

inline void* const* hostWindowLast(const HostDisplayMode& mode)
{
#if BF2_PLUGIN_ABI_NATIVE
    return mode.windows.empty() ? 0 : (&mode.windows[0] + mode.windows.size());
#else
    return reinterpret_cast<void* const*>(mode.windows.last);
#endif
}

// The three destination vectors contain only POD records.  Their buffers are renderer-owned and
// therefore live on the plugin's operator-new heap (Memory.dll in retail mode, shared /MD CRT in
// native mode), exactly like the original vector assignments.
template <typename T>
inline void clearOwnedDisplayVector(vc71::vector_layout& dst)
{
    if (dst.first)
        ::operator delete(dst.first);
    dst.first = 0;
    dst.last  = 0;
    dst.end   = 0;
}

template <typename T>
inline void assignOwnedDisplayVector(vc71::vector_layout& dst,
                                     const T* first,
                                     const T* last)
{
    vc71::vector_layout replacement = { 0, 0, 0, 0 };
    try
    {
        vc71::vectorRef<T> writer(&replacement);
        for (const T* p = first; p != last; ++p)
            writer.push_back(*p);
    }
    catch (...)
    {
        clearOwnedDisplayVector<T>(replacement);
        throw;
    }

    clearOwnedDisplayVector<T>(dst);
    dst.first = replacement.first;
    dst.last  = replacement.last;
    dst.end   = replacement.end;
}

inline void assignOwnedDisplayModeVectors(vc71::vector_layout& depth,
                                          vc71::vector_layout& multiSample,
                                          vc71::vector_layout& windows,
                                          const HostDisplayMode& source)
{
    assignOwnedDisplayVector<unsigned int>(depth,
                                            hostDepthFirst(source),
                                            hostDepthLast(source));
    assignOwnedDisplayVector<DisplayModeMultiSample>(multiSample,
                                                      hostMultiSampleFirst(source),
                                                      hostMultiSampleLast(source));
    assignOwnedDisplayVector<void*>(windows,
                                    hostWindowFirst(source),
                                    hostWindowLast(source));
}

inline void clearOwnedDisplayModeVectors(vc71::vector_layout& depth,
                                         vc71::vector_layout& multiSample,
                                         vc71::vector_layout& windows)
{
    // sub_1000C9B0 destroys them in reverse member order.
    clearOwnedDisplayVector<void*>(windows);
    clearOwnedDisplayVector<DisplayModeMultiSample>(multiSample);
    clearOwnedDisplayVector<unsigned int>(depth);
}

} // namespace renddx9abi

#endif // RENDDX9ABI_DISPLAYMODEABI_H
