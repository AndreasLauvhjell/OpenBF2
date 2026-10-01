// Shared module declarations; this combined header is a reconstruction convenience [INFERRED].
// Keep concrete factory definitions out of this header to avoid conflicting partial class declarations.

#ifndef CORE_MODULES_H          // [INFERRED] guard style
#define CORE_MODULES_H

#include <vector>

#include "IModule.h"            // IModule -> IClassServer -> IBase, plus the two RegInfo structs
#include "../Game/IEventListener.h"   // two of the 29 are also event listeners - see below

namespace dice {
namespace hfe {

// Module classes implement IModule's eight slots; close returns bool.
// Every module starts with one reference so registration and unregistration leave the engine's
// ownership balanced. Event-listener modules carry an additional base pointer before that counter.
#define BF2_MODULE(name)                                                \
    class name : public IModule                                         \
    {                                                                   \
    public:                                                             \
        name() : m_refCount(1) {}       /* born with one reference */   \
        virtual void  addRef();                                         \
        virtual int   getRef();                                         \
        virtual int   release();                                        \
        virtual void* queryInterface(unsigned int) const;               \
        virtual void  getClassList(std::vector<ClassRegInfo>&);         \
        virtual void  getSingletonList(std::vector<SingletonRegInfo>&); \
        virtual bool  init();                                           \
        virtual bool  close();                                          \
    private:                                                            \
        int m_refCount;                 /* +0x04 — see the IBase quartet in ModuleRegistry.cpp */ \
    }

// Event-listening modules have IModule and IEventListener bases before their one-reference counter.
// Keep the second base: omitting it would shift the refcount into the listener's virtual-pointer slot.
#define BF2_MODULE_EV(name)                                             \
    class name : public IModule, public IEventListener                  \
    {                                                                   \
    public:                                                             \
        name() : m_refCount(1) {}       /* born with one reference */   \
        virtual void  addRef();                                         \
        virtual int   getRef();                                         \
        virtual int   release();                                        \
        virtual void* queryInterface(unsigned int) const;               \
        virtual void  getClassList(std::vector<ClassRegInfo>&);         \
        virtual void  getSingletonList(std::vector<SingletonRegInfo>&); \
        virtual bool  init();                                           \
        virtual bool  close();                                          \
        virtual void  handleEvent(EventCategory, unsigned int, EventNode*, void*); \
    private:                                                            \
        int m_refCount;                 /* +0x08 for these two — see above */ \
    }

// dice::hfe — `Module` itself is declared in Core/Module.h and is not repeated here.
BF2_MODULE(ModuleBF2);
// declared explicitly, not via the macro: ModuleGame carries a flag at +0x0C that no other
// module has (init sets it, close does not clear it). Adding it to the macro would give all 27
// plain modules a member the binary says they do not have.
class ModuleGame : public IModule, public IEventListener
{
public:
    // Starts with one reference. init() initializes m_appActive before it may be read.
    ModuleGame() : m_refCount(1) {}
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual void  getClassList(std::vector<ClassRegInfo>&);
    virtual void  getSingletonList(std::vector<SingletonRegInfo>&);
    virtual bool  init();
    virtual bool  close();
    virtual void  handleEvent(EventCategory, unsigned int, EventNode*, void*);
private:
    int  m_refCount;            // +0x08 - see the layout note above
    // [INFERRED] app-active field name. Initialization seeds true; CEActivateApp overwrites it
    // and loading-screen updates test it before rendering.
    bool m_appActive;
};
BF2_MODULE(ModuleGameLogic);
BF2_MODULE(ModuleNetworkables);
BF2_MODULE(MathModule);
BF2_MODULE(BfSoundModule);
BF2_MODULE(SpawnSystemModule);

namespace io             { BF2_MODULE(ModuleIO); BF2_MODULE(FileModule); BF2_MODULE(NetworkModule);
                           BF2_MODULE(InputDX8Module); }
namespace console        { BF2_MODULE(ModuleConsole); }
namespace tool           { BF2_MODULE(ModuleToolObject); }
namespace animationTools { BF2_MODULE_EV(AnimModule); }   // also an IEventListener
namespace ai {
// also explicit. Its flag at +0x0C guards a one-shot console registration, and it is the only
// module whose init can return false.
class AIInterfaceModule : public IModule
{
public:
    // Born with one reference - see the note on BF2_MODULE. Refcount at +0x04.
    AIInterfaceModule() : m_refCount(1), m_needsConsoleRegister(true) {}
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual void  getClassList(std::vector<ClassRegInfo>&);
    virtual void  getSingletonList(std::vector<SingletonRegInfo>&);
    virtual bool  init();
    virtual bool  close();
    // Resets the AI module and returns true.
    virtual bool  reset();

    bool initAIConnection();
    bool closeAIConnection();
private:
    int  m_refCount;                // +0x04
    int  m_field08;                 // +0x08, deliberately ctor-uninitialized in both retail ports
    bool m_needsConsoleRegister;    // +0x0C
};
}

namespace world {
BF2_MODULE(Module);              // world::Module, distinct from dice::hfe::Module
BF2_MODULE(ObjectModule);
BF2_MODULE(ObjTemplPortalModule);
BF2_MODULE(ObjTemplBaseModule);
BF2_MODULE(PhysicsModule);
BF2_MODULE(BFMaterialSystemModule);
BF2_MODULE(ScoreSystemModule);
BF2_MODULE(HudCompModule);
BF2_MODULE(GameLogicCompModule);
BF2_MODULE(GameLogicObjTemplBaseModule);
BF2_MODULE(GameLogicObjTemplFxModule);
BF2_MODULE(GameLogicObjTemplVehicleModule);
BF2_MODULE(GameLogicObjTemplBFModule);
} // namespace world

} // namespace hfe
} // namespace dice

#endif // CORE_MODULES_H
