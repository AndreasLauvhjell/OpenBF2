// Demo runtime interface.
// Coordinates demo playback, capture and engine timing through owned recorder/tick state.
// Timedemos also own statistics and write reports on completion.
// [CHOSEN] Return types without direct caller evidence remain provisional.

#ifndef GAME_MAIN_DEMO_H            // [INFERRED] guard style
#define GAME_MAIN_DEMO_H

#include "../IEventListener.h"      // EventCategory, EventNode, IEventListener
#include <string>

namespace dice {
namespace hfe {

class IGame;                        // Game/IGame.h
class IGameClient;                  // no vtable in this binary - see BF2Engine.cpp
class BF2Render;
class BF2Console;                   // Game/Main/BF2Console.h
class TickCalculator;
class DemoRecorder;                 // Game/DemoRecorder/ (path attested)
class DemoConfig;                   // Game/DemoRecorder/DemoConfig.cpp (path attested)
struct TimeDemoStats;               // [INFERRED name] defined in demo.cpp; 0x30-byte stats block

namespace io { class PlayerInput; class IStream; }

// _ZTVN4dice3hfe4DemoE 0x8893430: one virtual slot (handleEvent). Base: IEventListener, offset 0.
class Demo : public IEventListener
{
public:
    Demo();
    ~Demo();                                                    // /0x81415D0, 21 B - NON-virtual

    // setup / teardown
    bool playDemo(IGameClient*, const DemoConfig&);             //   [CHOSEN bool]
    bool playDemo(IGameClient*, const std::string& configFile); //   [CHOSEN bool]
    void recordDemo(IGame*, const std::string& name, BF2Console*);
    void stop(IGame*);
    void killDemoRecorder();

    // the per-frame hooks mainLoop calls (see BF2Engine.cpp's dominance table)
    void   beginMainLoop(BF2Render*, BF2Console*);              //    depth 0
    double overridePreferredFrameTime(double);                  //    depth 2 - return double proven
    bool   allowLockFps() const;                                //    depth 2  [CHOSEN bool]
    bool   overrideAdvanceTime();                               //    depth 3  [CHOSEN bool]
    void   updateTicks(float* timeStep, int* tickCount,         //    depth 5  [CHOSEN void]
                       int* playbackTicks, float* playbackStep, //   out-param names [INFERRED]
                       const TickCalculator* engineCalc);
    void   handlePlayerInput(int simulationRate);               //    depth 7  [CHOSEN void]
    bool   endMainLoop(BF2Render*, bool quit);                  //   depth 8  [CHOSEN bool]
                                                                //   returns true when the demo wants shutdown

    // the rest
    void addPlayerInput(const io::PlayerInput&);
    void visualUpdate(float, float, float, int);                // client-side path
    bool allowRendering() const;                                //    [CHOSEN bool]
    DemoRecorder* recorder() const { return demoRecorder; }     // [CHOSEN] accessor: BF2Engine::mainLoop hands +0x08 to BF2Render (BF2.exe 0x40CA80)
    io::IStream* openFileForWriting(const char*);                 // timedemo report files

    // IEventListener - the only virtual. Registered by BF2Engine::init on category 2 (ECHud) with
    // priority 100. Sets the TU-static frame flag on (category 2, id 8).
    virtual void handleEvent(EventCategory, unsigned int, EventNode*, void*);

    // [INFERRED] Friendship exposes recorder state used directly by BF2Engine.
    // [UNRESOLVED] Original source visibility.
    friend class BF2Engine;
    friend class GameClient;          // BF2.exe 4D2630 reads isPlayback (+0x10) through the engine's demo

private:
    // Layout proven by ctor 0x8141550 + operator new(0x14) at the construction site. Field names
    // [INFERRED] except demoRecorder (pinned by the killDemoRecorder symbol name).
    TickCalculator* tickCalculator;     // +0x04  own calculator, new TickCalculator(10, 30.0f)
    DemoRecorder*   demoRecorder;       // +0x08  null when idle - every member is a no-op then
    TimeDemoStats*  timeDemoStats;      // +0x0C  only non-null during a timedemo playback
    bool            isPlayback;         // +0x10  set only by playDemo(config); makes stop() skip
                                        //        writeEndOfStream
};

} // namespace hfe
} // namespace dice

#endif // GAME_MAIN_DEMO_H
