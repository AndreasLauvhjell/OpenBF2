// Tracks phase-weighted loading progress and invokes the registered UI callback on counter changes.
// GameLogic brackets loading statistics. Source placement is [INFERRED].

#ifndef CORE_LOADSTAT_H     // [INFERRED] guard style
#define CORE_LOADSTAT_H

#include <string>

namespace dice {
namespace hfe {

// Load phase payload: step count, weight and display name; field names are [CHOSEN].
// Weight contributes 0.15 times its value to progress, with total progress capped at 1.
struct NextLoadPart
{
    int         steps;      // +0x00 - the increments this phase will report; 0 means "ignore me"
    float       weight;     // +0x04 - scaled by 0.15 into the bar's range
    const char* name;       // +0x08 - may be null, which clears both strings
};

class LoadStat
{
public:
    LoadStat();

    void resetLoadStats();
    // Starts the phase described by the namespace-level NextLoadPart payload.
    void setNextLoadPart(NextLoadPart*);
    void setCurrentFile(const std::string&);

    // Increments progress and invokes executeLoadCallback whenever m_loadCallback exists.
    // Callback throttling occurs inside executeLoadCallback, independently of counting.
    void incLoadCounter(int n);

    // Models FileManager's inline counter increment without exposing the layout to callers.
    void incExistsCounter() { ++m_existsCounter; }

    void   setSecondsBetweenCallbacks(double);
    double getSecondsBetweenCallbacks() const;
    void   executeLoadCallback();

    void registerAtIncLoadCallback(void (*fn)());
    void registerLoadCallback(void (*fn)());
    void unregisterLoadCallback();
    void report();

private:
    // Load-progress state shared with loadCallback and GameLogic.
    // The main callback is throttled; the at-increment callback runs on every increment.
    // The suppression byte and callback re-entry guard are distinct from the load counter.
    // Keep unknown fields in place and use the byte offsets shown below; reset also clears a separate
    // global immediately after loadStat. Public members permit the existing progress callbacks' access.
public:
    void (*m_loadCallback)();       // +0x00
    void (*m_atIncCallback)();      // +0x04
    // RendDX9 suppresses loading-screen callbacks during device loss and shader compilation.
    bool  m_unknown08;              // +0x08 Callback suppression flag.
    bool  m_unknown09;              // +0x09  padding/unused; neither body touches it
    bool  m_inCallback;             // +0x0A
    bool  m_unknown0B;              // +0x0B  [UNRESOLVED]
    // A new phase initializes both strings; setCurrentFile subsequently changes only the current-file string.
    std::string m_string0C;         // +0x0C  setCurrentFile's target; = m_string10 on a new phase
    std::string m_string10;         // +0x10  the phase name from NextLoadPart::name
    int   m_unknown14;              // +0x14  [UNRESOLVED]
    int   m_unknown18;              // +0x18  [UNRESOLVED]
    float m_unknown1C;              // +0x1C  [UNRESOLVED], ctor -1.0f
    int   m_loadCounter;            // +0x20
    int   m_unknown24;              // +0x24 Expected phase steps, used as loadCallback's divisor.
    int   m_unknown28;              // +0x28  [UNRESOLVED]
    int   m_existsCounter;          // +0x2C
    int   m_unknown30;              // +0x30  [UNRESOLVED]
    bool  m_unknown34;              // +0x34  [UNRESOLVED]
    // Current loading phase's start and end fractions.
    float m_unknown38;              // +0x38  the phase range's start fraction
    float m_unknown3C;              // +0x3C  its end fraction, clamped to 1.0
};

extern LoadStat* loadStat;

} // namespace hfe
} // namespace dice

#endif // CORE_LOADSTAT_H
