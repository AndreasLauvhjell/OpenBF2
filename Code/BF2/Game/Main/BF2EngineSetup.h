// Engine module and service setup. [INFERRED] source path. Holds the engine,
// client movie-module handle, AI-initialized flag, and one unresolved field.

#ifndef BF2ENGINESETUP_H            // [INFERRED] guard style
#define BF2ENGINESETUP_H

namespace dice {
namespace hfe {

class BF2Engine;

class BF2EngineSetup
{
public:
    explicit BF2EngineSetup(BF2Engine* engine);
    ~BF2EngineSetup();

    // Builds the image's module list, registers class servers, and initializes modules.
    // These setup routines are instance methods.
    bool        initModules();
    bool        initLateModules();
    // return type from void. shutdownModules materialises `mov eax, 1`
    // immediately before its epilogue (0x813BCF6) on the single exit - the same shape as
    // initModules and initLateModules, both already bool. Every one of the three call sites
    // discards it, so this is invisible at the call but it is what the binary returns.
    bool        shutdownModules();

    bool        initFileSystem();
    bool        initRenderer();
    bool        initConsole();
    bool        initNetwork();
    // Instance method, even though its current body does not read this.
    void        shutdownNetwork();
    bool        initAI();
    void        shutdownAI();

    // [PLATFORM] Swiff (Flash UI) is client-only; both are stubs in the dedicated server build
    // (initSwiff 10 bytes, shutdownSwiff 5 bytes).
    bool initSwiff();               // BF2.exe 0x446CB0 (loadSwiffPlayerModule): NOT static - stores the module
                                    //   handle at +4; `return true` behind isDedicated() everywhere else
    void shutdownSwiff();             // BF2.exe 0x446CE0: unloads +4 with diagnostic name "no name"

private:
    BF2Engine* m_engine;            // offset 0   [INFERRED] name; set from the ctor argument
    void*      m_swiffPlayerDll;    // offset 4   SwiffPlayer.dll's module handle (BF2.exe 0x446CB0); [INFERRED] name
    int        m_unknown8;          // offset 8   [UNRESOLVED]
    bool       m_aiInitialised;     // offset 12  [INFERRED] name; written 1 by initAI and 0 by
                                    //            shutdownAI, which is all the evidence there is
};

} // namespace hfe
} // namespace dice

#endif // BF2ENGINESETUP_H
