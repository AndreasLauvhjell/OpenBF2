// Profiler implementation with the dedicated-server behavior of the shipped Linux image.
// Header path [INFERRED]. Most timer operations are retail no-ops: setEnable does not change its
// flag, getIdFromName returns 1024, and reportFile returns true without writing a file.
// Keep these behaviors distinct from implementation frontiers. Preserve the 512-entry timer table
// and the IProfilerClient virtual order, with Profiler's destructor last.
// Declaration-level [UNRESOLVED]/[CHOSEN] annotations limit recovery claims.

#ifndef CORE_PROFILER_H     // [INFERRED] guard style
#define CORE_PROFILER_H

#include "IBase.h"
#include "IProfilerClient.h"

// Compatibility include for callers that obtain DebugCounterManager through this header.
#include "DebugCounterManager.h"

#include <map>
#include <string>

namespace dice {
namespace hfe {

class ReportFormatter;          // [UNRESOLVED] header. _ZTIN4dice3hfe15ReportFormatterE 0x88D25D0
class XmlReportFormatter;       // vtable 0x88D2488 - not written, see Profiler.cpp
class PlainReportFormatter;     // vtable 0x88D2428 - not written, see Profiler.cpp

// `GlobalTimer`. ds:0x898BFA4, an ELF symbol (_ZN4dice3hfe15globalTimerNameE), read only by the two
// Profiler constructors. defined in Profiler.cpp - it is this translation unit's own global.
extern const char* globalTimerName;

class Profiler : public IProfilerClient
{
public:
    // Timer name and packed state flags. The x86 reference record is eight bytes.
    struct TimerInfo
    {
        TimerInfo();

        const char* name;                               // +0x00 - a raw char*, not std::string: the
                                                        //   ctor stores globalTimerName by pointer
                                                        //   copy with no refcount touch.
        bool        enabled : 1;                        // +0x04 bit 0, initialised true
        bool        vtune   : 1;                        // +0x04 bit 1, initialised false
                                                        //   [UNRESOLVED] both bit names; the two
                                                        //   bits are read from the ctor, the roles
                                                        //   from enableTimer/enableVTuneForTimer
                                                        //   being the only pair of setters.
    };

    // Timer-tree node with one child slot per timer ID. Its 512-pointer table uses 2156 bytes on x86.
    struct Timer
    {
        Timer();
        Timer(unsigned int id, float guardTime);
        ~Timer();
        void clear();

        // [UNRESOLVED] every field name below. The bodies that would name them (startTimer,
        // stopTimer, internalReport) are the empty ones - there is no code left that reads these.
        unsigned int m_field00;                         // +0x00
        unsigned int m_field04;                         // +0x04
        unsigned int m_field08;                         // +0x08  cleared by clear()
        unsigned int m_field0C;                         // +0x0C  cleared by clear()
        int          m_field10;                         // +0x10  = -1 by ctor and by clear()
        unsigned int m_field14;                         // +0x14  cleared by clear()
        unsigned int m_field18;                         // +0x18  cleared by clear()
        unsigned int m_field1C;                         // +0x1C  cleared by clear()
        unsigned int m_field20;                         // +0x20  cleared by clear()
        unsigned int m_field24;                         // +0x24  cleared by clear()
        unsigned int m_field28;                         // +0x28  cleared by clear()
        unsigned int m_field2C;                         // +0x2C  cleared by clear()
        unsigned int m_id;                              // +0x30  <- ctor arg 1
        float        m_guardTime;                       // +0x34  <- ctor arg 2
        unsigned int m_field38;                         // +0x38  see the ctor note in the .cpp
        unsigned int m_field3C;                         // +0x3C  cleared by clear()
        unsigned int m_field40;                         // +0x40  never written by ctor or clear()
        unsigned int m_field44;                         // +0x44  "
        unsigned int m_field48;                         // +0x48  "
        unsigned int m_field4C;                         // +0x4C  "
        unsigned int m_field50;                         // +0x50  "
        unsigned int m_field54;                         // +0x54  cleared by clear() only
        unsigned int m_field58;                         // +0x58  cleared by clear() only
        unsigned int m_field5C;                         // +0x5C  cleared by clear() only
        unsigned int m_field60;                         // +0x60  cleared by clear() only
        unsigned int m_field64;                         // +0x64  never written by ctor or clear()
        float        m_field68;                         // +0x68  = 10.0f (0x41200000) by both ctors
        Timer*       m_children[512];                   // +0x6C .. +0x86B
    };

    // ThreadInfo - 72 bytes. derived, not read: the Profiler ctor allocates the map's header node
    // with operator new(0x5C) = 92 bytes, and for this libstdc++ a node is
    // 16 (_Rb_tree_node_base) + sizeof(pair<const unsigned, ThreadInfo>), so the pair is 76 and
    // ThreadInfo is 72. Its contents are unrecoverable here: registerThread - the only function that
    // would fill one in - is one of the empty bodies.
    struct ThreadInfo
    {
        char m_storage[72];                             // [UNRESOLVED] all 72 bytes
    };

    // the factory. 0x84FA380, 96 B. This is what ClassManager::createInstance calls through
    // ClassRegInfo::create, and it is the whole reason this class blocks the boot.
    static IBase* create(unsigned int iid, IBase* outer);

    explicit Profiler(bool enable);

    // IProfilerClient overrides in ABI order, followed by the concrete destructor.
    virtual void  addRef();                                     // +0x00
    virtual int   getRef();                                     // +0x04
    virtual int   release();                                    // +0x08
    virtual void* queryInterface(unsigned int iid) const;       // +0x0C
    virtual void  startGlobalTimer();                           // +0x10  0x83C7080    5 B  empty
    virtual void  stopGlobalTimer();                            // +0x14  0x83C7090    5 B  empty
    virtual void  startTimer(unsigned int& id, const char* name, float guardTime);
                                                                // +0x18  0x83C7110    5 B  empty
    virtual void  stopTimer(unsigned int id);                   // +0x1C  0x83C7120    5 B  empty
    virtual void  getTimerIdByName(const char* name, unsigned int& id);
                                                                // +0x20  0x83C7100    5 B  empty
    virtual void  setEnable(bool enable);                       // +0x24  0x83C7130    5 B  empty
    virtual bool  getEnable();                                  // +0x28
    virtual void  pauseVTune(bool pause);                       // +0x2C  0x83C9E20    5 B  empty
    virtual bool  getEnablePageFaults() const;                  // +0x30
    virtual void  setEnablePageFaults(bool enable);             // +0x34
    virtual bool  reportToFile(const std::string& fileName);    // +0x38

    // Keep the destructor after all interface methods. Moving it changes MSVC virtual slot order.
    virtual ~Profiler();                                        // +0x3C / +0x40

    // NON-virtual surface. Every one of these is a real ELF symbol; the addresses are exact and the
    // sizes are end-start. `EMPTY` means the five-byte do-nothing frame described at the top.
    unsigned int createNewTimerId(const char* name);            // real
    void         setEnableVTune(bool enable);                   // real
    bool         getEnableVTune();                              // real
    void         doCalibration();                               // [UNRESOLVED body]
                                                                //   Spine read: early-out on
                                                                //   `if (m_calibrationResult) return;`
                                                                //   then System::<+0x1C>() and an
                                                                //   rdtsc loop bounded by
                                                                //   m_calibrationIterations + 1.
    void         testCalibration(int iterations);               // [UNRESOLVED body]

    void         getTimers(Timer* timer, std::map<unsigned long long, unsigned int>& out);
                                                                // empty
    unsigned long long internalReport(Timer* timer, float totalTime, std::string indent,
                                      bool showAll, unsigned int depth, bool leaf);
                                                                // returns 0
                                                                //   [UNRESOLVED] return type: the
                                                                //   body zeroes both eax and edx, so
                                                                //   it is 64 bits wide, not int.
    std::string  reportGuards();                                // real
    void         reportFileGuards();                            // [UNRESOLVED body]
    bool         applyCalibrationOnTimers(Timer* timer);        // returns 0
                                                                //   [UNRESOLVED] return type
    void         reportSummary();                               // [UNRESOLVED body]
    void         reportProcessAndThreads();                     // [UNRESOLVED body]
    void         reportCounters(float totalTime, unsigned int depth, bool showAll);
                                                                // [UNRESOLVED body]
    void         internalReportPerTimer();                      // [UNRESOLVED body]
    void         generateReport(float totalTime, unsigned int depth, bool showAll);
                                                                // [UNRESOLVED body]
    void         reportFile(float totalTime, unsigned int depth, bool showAll);
                                                                // [UNRESOLVED body]
    bool         reportFile(const std::string& fileName, float totalTime, unsigned int depth,
                            bool showAll);                      // real
    void         clearTimers();                                 // empty
    void         enableTimer(unsigned int id);                  // empty
    void         disableTimer(unsigned int id);                 // empty
    void         enableVTuneForTimer(unsigned int id);          // empty
    void         disableVTuneForTimer(unsigned int id);         // empty
    Timer*       findTimer(Timer* root, unsigned int id);       // returns 0
    float        getGuardTimeForTimer(unsigned int id);         // returns 0.0f
    void         setGuardTimeForTimer(unsigned int id, float guardTime);
                                                                // empty
    void         enableGraphForTimer(unsigned int id, float a, float b, float c, float d);
                                                                // empty
    void         disableGraphForTimer(unsigned int id);         // empty
    unsigned int getIdFromName(const std::string& name);        // returns 1024
    void         enableTimerFromName(const std::string& name);  // real
    void         disableTimerFromName(const std::string& name); // real
    void         enableVTuneForTimerFromName(const std::string& name);
                                                                // real
    void         disableVTuneForTimerFromName(const std::string& name);
                                                                // real
    std::string  listTimers();                                  // real
    void         enableAllTimers();                             // empty
    void         disableAllTimers();                            // empty
    void         registerThread(unsigned int a, unsigned int b, const std::string& name);
                                                                // empty
    void         unregisterThread(unsigned int id);             // empty
    void         sumTimer(Timer** dst, Timer** src, int count); // empty
    std::string  reportPerTimer();                              // real
    void         drawGraph();                                   // empty

private:
    // Member order follows the reference layout. Native STL sizes differ from the original x86 ELF offsets.
    bool               m_enable;                    // +0x0004  getEnable() reads it. not set by the
                                                    //   init list - assigned last, in the ctor body.
    int                m_refCount;                  // +0x0008  = 1. the one-reference rule holds.
    Timer*             m_rootTimer;                 // +0x000C  new Timer(), assigned in the body
    TimerInfo          m_timerInfo[512];            // +0x0010 .. +0x100F
    Timer*             m_currentTimer[32];          // +0x1010 .. +0x108F  all 0, then [cur] = root
    unsigned int       m_currentTimerId[32];        // +0x1090 .. +0x110F  all 1024 (the "no timer"
                                                    //   sentinel getIdFromName also returns)
    unsigned int       m_currentThread;             // +0x1110  = 0, indexes the two arrays above
    unsigned int       m_field1114;                 // +0x1114  = 0   [UNRESOLVED]
    unsigned int       m_field1118;                 // +0x1118  = 0   [UNRESOLVED]
    unsigned int       m_field111C;                 // +0x111C  = 0   [UNRESOLVED]
    bool               m_field1120;                 // +0x1120  shipped bug - see the ctor
    unsigned int       m_field1124;                 // +0x1124  = 0   [UNRESOLVED]
    unsigned int       m_field1128;                 // +0x1128  = 0   [UNRESOLVED]
    unsigned long long m_calibrationResult;         // +0x112C  = 0. 64-bit: doCalibration ORs the
                                                    //   two halves together for its early-out.
                                                    //   [UNRESOLVED] name.
    unsigned long long m_calibrationIterations;     // +0x1134  = 10. 64-bit: `add esi,1 / adc edi,0`
                                                    //   in doCalibration's loop bound.
                                                    //   [UNRESOLVED] name.
    bool               m_enableVTune;               // +0x113C  getEnableVTune/setEnableVTune
    unsigned int       m_numTimers;                 // +0x1140  = 0, then ++ for GlobalTimer.
                                                    //   createNewTimerId's counter - attested by use
    unsigned int       m_field1144;                 // +0x1144  = 0   [UNRESOLVED]
    unsigned int       m_field1148;                 // +0x1148  = 0   [UNRESOLVED]
    bool               m_enablePageFaults;          // +0x114C  get/setEnablePageFaults
    bool               m_field114D;                 // +0x114D  = false  [UNRESOLVED]
    unsigned int       m_field1150;                 // +0x1150  never written by the ctor [UNRESOLVED]
    std::string        m_field1154;                 // +0x1154  empty. Destroyed by ~Profiler, which
                                                    //   is how we know it is a std::string and where
                                                    //   it sits.  [UNRESOLVED] name and purpose.
    char               m_hole1158[64];              // +0x1158 .. +0x1197  64 bytes the ctor never
                                                    // [UNRESOLVED] Untouched storage retained between adjacent fields.
    std::map<unsigned int, ThreadInfo> m_threads;   // +0x1198  registerThread's table - and it is
                                                    //   always empty, because registerThread is one
                                                    //   of the do-nothing bodies.
    unsigned int       m_field11A4;                 // +0x11A4  = 0   [UNRESOLVED]
    XmlReportFormatter*   m_xmlFormatter;           // +0x11A8  new, 0xEC bytes, vtable 0x88D2488
    PlainReportFormatter* m_plainFormatter;         // +0x11AC  new, 0xF0 bytes, vtable 0x88D2428

private:
    // [CHOSEN, not recovered] Disable copying of owned timer and report storage.
    Profiler(const Profiler&);
    Profiler& operator=(const Profiler&);
};

} // namespace hfe
} // namespace dice

#endif // CORE_PROFILER_H
