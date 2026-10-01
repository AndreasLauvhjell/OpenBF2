// Clock and FPU-control service, constructed before module registration and registered by address.
// Header path [INFERRED]. Preserve the IBase quartet before the virtual destructor and clock methods.
// Clock getters return double. Simulation-rate mutation and exact-time advancement are nonvirtual.

#ifndef CORE_SYSTEM_H               // [INFERRED] guard style
#define CORE_SYSTEM_H

#if defined(_WIN32)
#include <string>
struct _EXCEPTION_POINTERS;
#endif

namespace dice {
namespace hfe {

#if defined(_WIN32)
// [PLATFORM] PE-only helpers in System.cpp; names [INFERRED]. The dump worker keeps
// native pointers/handles; exception records and stream sizes use Windows SDK types.
const wchar_t* crashDumpTargetName(const wchar_t* baseFileName);
void writeCrashDump(_EXCEPTION_POINTERS* exception, const wchar_t* fileName);
// Dedicated PE 40F0D0: prepend the entry and retain at most four strings. Return
// value is unused by its caller; void is [CHOSEN call-use], not a recovered name.
void addCrashDumpAssertInfo(const std::string& entry);
#endif

// World clock and fixed-step tick operations shared by simulation and prediction.
namespace WorldPref
{
    double getWorldTime();                          // body in System.cpp

    // Advances the tick count and derives world time from the updated count and fixed interval.
    void incTickCount(unsigned int count);

    // `mov eax, ds:8AC0D68h` - the 30 Hz tick counter, sitting right after the
    // world-time double in the same .bss run. `int` return joins the form demo.cpp:105 and
    // StatusMonitor.cpp:168 already committed to (MSVC mangles the return; two forms would mint
    // two externals). TargetObject::handleUpdate compares ticks unsigned (`jnb`) - callers cast.
    int getTickCount();                             // body in System.cpp

    // Shared world tick count.
    extern unsigned int mTickCount;                 // ds:0x8AC0D68

    // Returns the fixed simulation interval in seconds.
    double getTickTime();                           // body in System.cpp
    extern double mTickTime;                        // ds:0x898BFD8 = 1.0 / 30.0

    // Sets world time directly, or resets the tick count and derives its corresponding world time.
    void resetWorldTime(double time);               // body in System.cpp
    void resetTickCount(unsigned int count);        // body in System.cpp

    // Advances prediction time by count ticks. [INFERRED] Helper name.
    void advanceTickCount(unsigned int delta);
}

class System
{
public:
    System();                                       // C2 0x83D1FA0 / C1 0x83D1FF0, byte-identical.
                                                    // IBase methods must precede the destructor to preserve virtual slot order.
    virtual void  addRef();                         // +0x00
    virtual int   getRef();                         // +0x04
    virtual int   release();                        // +0x08
    virtual void* queryInterface(unsigned int) const;  // +0x0C

    virtual ~System();                              // +0x10 / +0x14

    // Returns simulation time without narrowing its double precision.
    virtual double getExactTime();                  // +0x18
    // attested double: the one recovered caller stores it with `fstp qword ptr`.
    virtual double getWallClockTime();              // +0x1C
    virtual void   sleep(int milliseconds);         // +0x20
    virtual int    getSimulationRate();             // +0x24
    virtual float  getSimulationRatio();            // +0x28
    virtual void   storeAndTurnOffFPUExceptions();  // +0x2C  0x83D2560, 5 B - an empty stub here
    virtual void   restoreFPUExceptions();          // +0x30  0x83D2570, 5 B - an empty stub here

    // Stores the startup FPU-exception option. [INFERRED] Setter form.
    void setUseFpuExceptions(bool enabled) { m_useFpuExceptions = enabled; }

    // non-virtual
    void resetExactTime();
    void advanceExactTime();                        // once per frame, mainLoop
    void advanceExactTime(unsigned int);            // an overload, not a typo.
                                                    // The fixed-step overload accepts wall time in microseconds.
    void setSimulationRate(int);

    void increaseFpuErrorCount();
    int  getFpuErrorCount();
    void resetFpuErrorCount();

    // [INFERRED] SEH-option setter. [CHOSEN] Field placement remains unverified against the PE System layout;
    // do not transfer offsets from the differently sized ELF object.
    void setUseSeh(bool);                           // [INFERRED] name; field [CHOSEN] +0x24

private:
    // Reference offsets describe the x86 ELF. Native alignment and padding may differ.
    int                m_refCount;           // +0x04 = 1    [INFERRED] name; non-atomic
    int                m_simulationRate;     // +0x08 = 100  signed (cdq 0x83D21B8, sar31 0x83D236B)
    double             m_exactTime;          // +0x0C = 0.0  the published simulation clock, seconds
    unsigned long long m_cpuTicksPerSecond;  // +0x14 = 0    [INFERRED] name; rdtsc calibration
    bool               m_useFpuExceptions;   // +0x1C = 0    from the Win32 entry chain:
                                             // Startup FPU-exception flag. [INFERRED] Name; the corresponding exception-control virtuals are empty.
    int                m_fpuErrorCount;      // +0x20 = 0    attested by the three accessors
    int                m_unknown24;          // +0x24 = 0    [UNRESOLVED] ctor-only; seh candidate
};

// Engine-owned system instance, registered by its existing global pointer slot.
extern System* g_system;

} // namespace hfe
} // namespace dice

#endif // CORE_SYSTEM_H
