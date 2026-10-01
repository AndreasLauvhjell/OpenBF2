// Test scaffolding: provisional game declarations for isolated syntax checks.

#ifndef BF2_STUB_GAME_H
#define BF2_STUB_GAME_H

#include <string>
#include "stub_core.h"
#include "shadow/Game/IEventListener.h"

namespace dice {
namespace hfe {


// Game interfaces. Slot offsets in the comments are the recovered code's.

class IGame
{
public:
    virtual ~IGame();
    virtual void* queryInterface(unsigned int iid);   // vtable +0x0C
    virtual bool  init();                             // vtable +0x18
    virtual void  shutdown();                         // vtable +0x1C
};

class IGameLogic
{
public:
    virtual ~IGameLogic();
    virtual void release();                           // vtable +0x08
    virtual bool init();                              // vtable +0x10
};

class IGameServer
{
public:
    virtual ~IGameServer();
    // vtable +0x1C, cited as
    // _ZThn80_N4dice3hfe10GameServer11startServerERKSsii
    virtual bool startServer(const std::string&, int, int);
};

class IGameClient
{
public:
    virtual ~IGameClient();
    // The recovered startGame() calls this slot by the placeholder name below
    // and states the call-site signature as
    //   bool (const std::string&, int, const std::string&, float, bool)
    // while comparing the result against exactly 1. The placeholder name is
    // reproduced verbatim; no name is invented here.
    virtual int UNRESOLVED_slot0x1C(const std::string&, int, const std::string&, float, bool);
};

class IModInfo
{
public:
    virtual ~IModInfo();
    virtual bool init();                              // ModInfo vptr+0x10
};

// dice::hfe::profilerClient. BF2Engine.cpp names the slot it calls
// "slot40" and explicitly refuses to name it further; reproduced verbatim.
class IProfilerClient
{
public:
    virtual ~IProfilerClient();
    virtual void slot40();
};

// dice::hfe::serverSettings (ELF 0x8AAB7A0). The two field names are marked
// [INFERRED] by the recovered code itself (from IDA's applied struct type).
class ServerSettings
{
public:
    std::string serverIP;    // +0x1C
    int         serverPort;  // +0x20
};

// Demo is an event listener so initialization/shutdown can register and remove it.
class Demo : public IEventListener
{
public:
    Demo();
    void stop(IGame*);        // _ZN4dice3hfe4Demo4stopEPNS0_5IGameE (0x8141C00)
};

class StatusMonitor
{
public:
    StatusMonitor();
    ~StatusMonitor();
};

class PerformanceLogging
{
public:
    PerformanceLogging();
    ~PerformanceLogging();
};

class TickCalculator
{
public:
    TickCalculator(int, float);   // TickCalculator(10, 30.0f), operator new(0x20)
    ~TickCalculator();
};

class BF2Render
{
public:
    virtual ~BF2Render();         // deleted through "vtable +4"
    virtual bool init();          // vtable +8 - called by BF2EngineSetup::initRenderer (0x813be80)
};

// operator new(1) with NO constructor call -> an empty class, per BF2Engine.cpp.
class DebugDraw
{
};

// BF2Engine.cpp: "Concrete type is dice::hfe::CheckServerAliveThread, whose
// ctor at 0x812e760 takes BF2Engine*". Stored into a Thread* member.
class CheckServerAliveThread : public Thread
{
public:
    explicit CheckServerAliveThread(BF2Engine*);
};

// dice::hfe::loadStat (ELF 0x8ABBC98). BF2Engine.cpp: "NON-static, ONE argument".
class LoadStat
{
public:
    void registerAtIncLoadCallback(void (*)());
};

extern LoadStat* loadStat;

// dice::hfe::SimpleParser. add() is cited at 0x814a2f0 with the signature
// add(int, const std::string&, const std::string&). The ctor is fully inlined
// in the binary; the recovered parseParameters marks its `SimpleParser('+')`
// form [INFERRED].
class SimpleParser
{
public:
    explicit SimpleParser(char prefix);
    ~SimpleParser();
    void add(int, const std::string&, const std::string&);
};

// File-scope helpers whose ELF symbols BF2Engine.cpp cites in a comment but
// which it never declares.
void bf2ExitCallback(void*);            // _ZN4dice3hfe15bf2ExitCallbackEPv
void atIncLoadCallback();               // _ZN4dice3hfe17atIncLoadCallbackEv
bool HandleCommandHookPB(const char*, int);  // signature (char const*, int), cited

// Called by init(); PunkBuster server SDK.
void PbServerInitialize();

// Called by init()/shutdown(); BF2Engine.cpp notes they are "not declared in
// BF2Engine.h" and gives them no home.
void initSettingsRepostitory();
void shutdownSettingsRepostitory();


// dice::hfe::world

namespace world {

class Object
{
public:
    virtual ~Object();
    virtual void release();                                   // vtable +0x8
};

class ObjectManager
{
public:
    virtual ~ObjectManager();
    virtual Object* getObjectFromNameHash(const std::string&);  // vtable +0x24
};

class ObjectTemplateManager
{
public:
    virtual ~ObjectTemplateManager();
    virtual void deleteAll(bool);                               // vtable +0x38
};

class SupplyDropManager
{
public:
    static SupplyDropManager* Instance();
    static void Release();
};

void setMaterialManager(unsigned int);
void setScoreManager(unsigned int);

} // namespace world

} // namespace hfe
} // namespace dice

#endif // BF2_STUB_GAME_H
