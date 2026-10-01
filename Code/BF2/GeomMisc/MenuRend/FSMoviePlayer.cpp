// Full-screen movie playback through Video. [INFERRED] Source placement.

#ifdef BF2_IMAGE_RENDDX9

#include "../../../RendDX9Abi/HfeAbi.h"
#include "../../../RendDX9Abi/ModuleCommon.h"
#include "../../../RendDX9Abi/BfMemory.h"
#include "../../../RendDX9Abi/Frontier.h"
#include "../../../RendDX9Abi/PluginEntry.h"
#include "../../RendDX9/RendDX9Base.h"

#include <new>
#include <cstdio>
#include <cstdlib>

namespace dice {
namespace hfe {
namespace dx9rend {
// ShaderManager.cpp / Effect.cpp - the same seams DebugLineGraphD3d.cpp declares.
extern void* g_shaderManager;                       // dword_1023B97C
void* shaderManagerGetEffect(void*, const char*);   // sub_10022EF0
class Effect;
void* effectGetShaderParameter(Effect*, const vc71::string&);   // sub_1001E420
}
namespace geom {

// From getClassList's own registration at 0x1007DF47/0x1020AF2C, and queryInterface (0x100A2410).
extern const unsigned int CID_FSMoviePlayer  = 81046u;
extern const unsigned int IID_IFSMoviePlayer = 80047u;


// Owns video playback, quad and shader resources. Device listener callbacks are empty.
// [UNRESOLVED] The additional stored handle's type and ownership.

class FSMoviePlayer
{
public:
    FSMoviePlayer();

    void  addRef() { ++m_refCount; }
    int   getRef() const { return m_refCount; }
    int   release();
    void* queryInterface(unsigned int iid) const;

    bool  load(const vc71::string& path);   // +0x10
    void  releaseAll();                     // +0x14
    void  dtor(char flags);                 // +0x20

private:
    void*         m_vptr;         // +0x00
    void*         m_listenerVptr; // +0x04
    int           m_refCount;     // +0x08
    void*         m_video;        // +0x0C
    void*         m_videoHandle;  // +0x10
    vc71::string  m_path;         // +0x14
    void*         m_vertexBuffer; // +0x30
    void*         m_effect;       // +0x34
    void*         m_paramTransform; // +0x38
    void*         m_paramTexture; // +0x3C
    void*         m_stateObject;  // +0x40
};

static_assert(sizeof(FSMoviePlayer) == 0x44,
              "RefFactory_geom_FSMoviePlayer allocates 0x44 - see 0x1007D030");

namespace {

void __fastcall fsm_addRef(void* s, void*) { static_cast<FSMoviePlayer*>(s)->addRef(); }
int  __fastcall fsm_getRef(void* s, void*) { return static_cast<FSMoviePlayer*>(s)->getRef(); }
int  __fastcall fsm_release(void* s, void*) { return static_cast<FSMoviePlayer*>(s)->release(); }

void* __fastcall fsm_queryInterface(void* s, void*, unsigned int iid)
{
    return static_cast<FSMoviePlayer*>(s)->queryInterface(iid);
}

// +0x10 - the shipped signature takes the path by value (retn 0x1C); the copy lives inside.
#if BF2_PLUGIN_ABI_NATIVE
// [CHOSEN, not recovered] Converts the host's by-value string once at the native movie boundary.
// [RETIRE-WHEN: fsmovie-native-string-adapter] Replace with native internal storage while preserving the retail ABI adapter.
bool __fastcall fsm_load(void* s, void*, ::renddx9abi::HostString path)
{
    const vc71::string internalPath(path.data(), static_cast<unsigned int>(path.size()));
    return static_cast<FSMoviePlayer*>(s)->load(internalPath);
}
#else
bool __fastcall fsm_load(void* s, void*, const char* pathRaw0, int, int, int, int, int, int)
{
    // the by-value string's seven stack dwords: the first is the union/alloc dword, the buffer
    // or pointer follows. Reconstituting through the layout rather than guessing an overload:
    const vc71::string* path = reinterpret_cast<const vc71::string*>(&pathRaw0);
    const bool ok = static_cast<FSMoviePlayer*>(s)->load(*path);
    path->~string();                                       // callee-destroyed, as shipped
    return ok;
}
#endif

void __fastcall fsm_releaseAll(void* s, void*) { static_cast<FSMoviePlayer*>(s)->releaseAll(); }

// [FRONTIER] Movie drawing requires the video frame, shader parameters, state setup, and quad buffer.
// Successful loading must remain blocked until this path is implemented.
void __fastcall fsm_drawFrame(void* s, void*)
{
    RD9_FRONTIER_AT("dice::hfe::geom::FSMoviePlayer::drawFrame "
                    "(0x100A2300 - StateManager, effect passes and DrawPrimitive; the draw path)",
                    s, 0x18);
}

// Calls the video's playback query. Requires a successfully loaded video; no null guard is provided.
int __fastcall fsm_videoPassthrough(void* s, void*)
{
    void* video = *reinterpret_cast<void**>(static_cast<char*>(s) + 0x0C);
    void** vtbl = *reinterpret_cast<void***>(video);
    typedef int (__thiscall* Fn)(void*);
    return reinterpret_cast<Fn>(vtbl[0x30 / 4])(video);
}

void __fastcall fsm_dtor(void* s, void*, char flags) { static_cast<FSMoviePlayer*>(s)->dtor(flags); }

void* const kFSMoviePlayerVtable[9] =
{
    reinterpret_cast<void*>(&fsm_addRef),           // +0x00  0x1001A010 (folded at0x8 pair)
    reinterpret_cast<void*>(&fsm_getRef),           // +0x04
    reinterpret_cast<void*>(&fsm_release),          // +0x08
    reinterpret_cast<void*>(&fsm_queryInterface),   // +0x0C
    reinterpret_cast<void*>(&fsm_load),             // +0x10
    reinterpret_cast<void*>(&fsm_releaseAll),       // +0x14
    reinterpret_cast<void*>(&fsm_drawFrame),        // +0x18  0x100A2300  [FRONTIER]
    reinterpret_cast<void*>(&fsm_videoPassthrough), // +0x1C
    reinterpret_cast<void*>(&fsm_dtor)              // +0x20
};

void __fastcall fsm_listenerNoop(void*, void*) { /* all three shipped slots are no-ops */ }

void* const kFSMoviePlayerListenerVtable[3] =
{
    reinterpret_cast<void*>(&fsm_listenerNoop),     // +0x00
    reinterpret_cast<void*>(&fsm_listenerNoop),     // +0x04
    reinterpret_cast<void*>(&fsm_listenerNoop)      // +0x08  nullsub_39
};

} // anonymous namespace

// geom_FSMoviePlayer_ctor, 0x100A2290 - read in full. The listener vptr is double-written as
// usual; only the survivor is reproduced.
FSMoviePlayer::FSMoviePlayer()
    : m_vptr(const_cast<void**>(kFSMoviePlayerVtable))
    , m_listenerVptr(const_cast<void**>(kFSMoviePlayerListenerVtable))
    , m_refCount(1)
    , m_video(0)
    , m_videoHandle(0)
    , m_vertexBuffer(0)
    , m_effect(0)
    , m_paramTransform(0)
    , m_paramTexture(0)
    , m_stateObject(0)
{
}

// FSMoviePlayer_release, 0x100A2440 - `--ref; if zero vslot+0x20(this, 1)`, i.e. the virtual
// scalar-deleting destructor with the free bit set.
int FSMoviePlayer::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        dtor(1);
    return remaining;
}

// FSMoviePlayer_queryInterfaceById, 0x100A2410: iid 1 and iid 80047, nothing else.
void* FSMoviePlayer::queryInterface(unsigned int iid) const
{
    if (iid == RD9_IID_IBASE || iid == IID_IFSMoviePlayer)
        return const_cast<FSMoviePlayer*>(this);
    return 0;
}


// Loads movie shader resources and stores the path; returns false when required resources are missing.
// [FRONTIER] The state and quad setup following Video loading remains unimplemented.

bool FSMoviePlayer::load(const vc71::string& path)
{
    m_effect = ::dice::hfe::dx9rend::shaderManagerGetEffect(
        ::dice::hfe::dx9rend::g_shaderManager, "mods/xpack/Shaders/LoadingScreen");
    if (!m_effect)
        return false;

    m_paramTransform = ::dice::hfe::dx9rend::effectGetShaderParameter(
        static_cast<::dice::hfe::dx9rend::Effect*>(m_effect), vc71::string("TRANSFORM"));
    if (!m_paramTransform)
        return false;

    m_paramTexture = ::dice::hfe::dx9rend::effectGetShaderParameter(
        static_cast<::dice::hfe::dx9rend::Effect*>(m_effect), vc71::string("TEXTURE"));
    if (!m_paramTexture)
        return false;

    m_path = path;

#if BF2_PLUGIN_ABI_NATIVE
    // [TEMP] [CHOSEN, not recovered]
    // [RETIRE-WHEN: fsmovie-native-load-tail] Implement video creation/open, quad setup, teardown, and drawing before load can succeed.
    std::fprintf(stderr, "[FRONTIER] dice::hfe::geom::FSMoviePlayer::load reached "
                         "(PE 0x100A2086; Video/quad tail unresolved)\n");
    std::fflush(stderr);
    std::abort();
#else
    // [CHOSEN, not recovered] Existing retail-compatibility movie-skip policy is unchanged.
    // [RETIRE-WHEN: fsmovie-native-load-tail] This branch also retires with the complete movie path.
    return false;
#endif
}

// Releases parameters, effect, video handle, video, state wrapper, and vertex buffer in that order.
void FSMoviePlayer::releaseAll()
{
    typedef int (__thiscall* ReleaseFn)(void*);

    void* order[5] = { m_paramTransform, m_paramTexture, m_effect, m_videoHandle, m_video };
    for (int i = 0; i < 5; ++i)
    {
        if (order[i])
        {
            void** vtbl = *reinterpret_cast<void***>(order[i]);
            reinterpret_cast<ReleaseFn>(vtbl[0x08 / 4])(order[i]);
        }
    }
    m_paramTransform = 0;
    m_paramTexture   = 0;
    m_effect         = 0;
    m_videoHandle    = 0;
    m_video          = 0;

    if (m_stateObject || m_vertexBuffer)
    {
        // Only the unreachable post-Video tail of load() can populate these; their teardown
        // (sub_10014790 / sub_10014510) is written together with that tail.
        RD9_FRONTIER("dice::hfe::geom::FSMoviePlayer::releaseAll "
                     "(a state object or quad buffer exists - the wrapper teardown is unwritten)",
                     0x14);
    }
}

// +0x20, 0x100A2460 - restore both vptrs, releaseAll, destroy the path, free when flags & 1.
void FSMoviePlayer::dtor(char flags)
{
    m_vptr         = const_cast<void**>(kFSMoviePlayerVtable);
    m_listenerVptr = const_cast<void**>(kFSMoviePlayerListenerVtable);
    releaseAll();
    m_path.~string();
    if (flags & 1)
        ::renddx9abi::engineFree(this);
}

// RefFactory_geom_FSMoviePlayer, 0x1007D030. Same shape and __fastcall convention as the others.
IBase* __fastcall createFSMoviePlayer(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;

    void* storage = ::renddx9abi::engineAlloc(sizeof(FSMoviePlayer));
    if (!storage)
        return 0;

    FSMoviePlayer* self = new (storage) FSMoviePlayer();

    void* itf = self->queryInterface(iid);
    if (!itf)
        self->release();
    return static_cast<IBase*>(itf);
}

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9
