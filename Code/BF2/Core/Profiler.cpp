// Profiler implementation for the shipped image, whose disabled instrumentation methods are no-ops.
// Source path [INFERRED]. Preserve constructor initialization order, the destructor's retained
// timer/formatter allocations and createNewTimerId's unchecked 512-entry table access.

#include "Profiler.h"

#include "Ids.h"            // IID_IBase, IID_IProfilerClient
#include "StringUtil.h"     // compareIgnoreCase(const char*, const char*)   0x83D0510
#include "System.h"         // g_system (ds:0x8AC0D20), System::resetExactTime  0x83D2080

namespace dice {
namespace hfe {

// Borrowed name used for the root profiling timer.
const char* globalTimerName = "GlobalTimer";        // string data at 0x88D12C1

// Creates a disabled, one-reference profiler and returns the requested interface.
// Rejects aggregation; an IID mismatch releases and destroys the new object.
IBase* Profiler::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;

    Profiler* self = new Profiler(false);

    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();

    return result;
}

// Creates timers and profiling state. m_field1120 intentionally reads m_enable before it is initialized.
// Report formatters remain absent as marked in the initializer list.
Profiler::Profiler(bool enable)
    : m_refCount(1)                     // +0x0008  0x83C6451: mov dword ptr [edi+8], 1
                                        // m_timerInfo[512] default-constructs here - the
                                        //   `add ebx, 8` loop at 0x83C6460, 512 iterations.
    , m_currentThread(0)                // +0x1110
    , m_field1114(0)                    // +0x1114
    , m_field1118(0)                    // +0x1118
    , m_field111C(0)                    // +0x111C
    , m_field1120(m_enable)             // +0x1120  0x83C6471/0x83C648C  defect 1, see above
    , m_field1124(0)                    // +0x1124
    , m_field1128(0)                    // +0x1128
    , m_calibrationResult(0)            // +0x112C  0x83C64A4 / 0x83C64AC - the 64-bit pair
    , m_calibrationIterations(10)       // +0x1134  0x83C64C7 (= 10) / 0x83C64D5 (high half = 0)
    , m_enableVTune(false)              // +0x113C
    , m_numTimers(0)                    // +0x1140
    , m_field1144(0)                    // +0x1144
    , m_field1148(0)                    // +0x1148
    , m_enablePageFaults(false)         // +0x114C
    , m_field114D(false)                // +0x114D
                                        // m_field1150 and m_hole1158 retain their original uninitialized state.
    , m_field11A4(0)                    // +0x11A4
    // [UNRESOLVED] Report formatter construction is missing. Replace these nulls with owned
    // XmlReportFormatter and PlainReportFormatter instances when their implementations are available.
    , m_xmlFormatter(0)                 // +0x11A8  [UNRESOLVED] should be new XmlReportFormatter()
    , m_plainFormatter(0)               // +0x11AC  [UNRESOLVED] should be new PlainReportFormatter()
{
    m_rootTimer = new Timer();                          // 0x83C6805 operator new(0x86Cu), 0x83C6811

    m_enable = enable;                                  // after m_field1120 read it

    // 0x83C681D..0x83C6836. 1024 is the "no timer" sentinel; getIdFromName returns the same value.
    for (int i = 0; i <= 31; ++i)
    {
        m_currentTimer[i]   = 0;
        m_currentTimerId[i] = 1024;
    }

    // 0x83C6838..0x83C684D. m_currentThread is 0 here, so this re-writes slot 0 of both arrays -
    // the second store to m_currentTimerId[0] is redundant in the emitted code too.
    m_currentTimerId[m_currentThread] = 1024;
    m_currentTimer[m_currentThread]   = m_rootTimer;

    // Register the root timer name.
    createNewTimerId(globalTimerName);

    // 0x83C6862..0x83C686A. No null check on g_system, and there is none in the binary either -
    // main() constructs it long before ClassManager::registerSingleton runs.
    g_system->resetExactTime();
}

// Preserves the empty destructor: raw timer/formatter allocations are not deleted.
// The destructor slots remain last after reportToFile; ELF and MSVC emit different destructor-slot counts.
Profiler::~Profiler()
{
}

// the IBase quartet - vtable +0x00..+0x0C.

// No return value materialised - void, matching IBase.h.
void Profiler::addRef()
{
    ++m_refCount;
}


int Profiler::getRef()
{
    return m_refCount;
}


// Releases one reference, deleting this profiler at zero. Returns the remaining count.
int Profiler::release()
{
    int refs = --m_refCount;
    if (refs == 0)
        delete this;
    return refs;
}

// Both ids are loaded from memory (ds:_ZN4dice3hfe9IID_IBaseE and
// ds:_ZN4dice3hfe19IID_IProfilerClientE), not immediates - they are real globals, which is why
// Core/Ids.h models them as constants rather than an enum.

//     cmp edx, IID_IBase / mov eax, ecx / jz done
//     xor eax, eax / cmp edx, IID_IProfilerClient / cmovz eax, ecx

// Note what is absent: no CID case, no queryImplementation. This class answers two interfaces only.
void* Profiler::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<Profiler*>(this);

    if (iid == IID_IProfilerClient)
        return const_cast<Profiler*>(this);

    return 0;
}

// the rest of the vtable - +0x10..+0x38. Nine of these eleven are empty in the shipped build.

void Profiler::startGlobalTimer()                       // +0x10  0x83C7080,  5 B - empty
{
}

void Profiler::stopGlobalTimer()                        // +0x14  0x83C7090,  5 B - empty
{
}

// Accepts timer-start requests without recording them; profiling is disabled in this implementation.
void Profiler::startTimer(unsigned int& id, const char* name, float guardTime)
{
    (void)id; (void)name; (void)guardTime;
}

void Profiler::stopTimer(unsigned int id)               // +0x1C  0x83C7120,  5 B - empty
{
    (void)id;
}

// +0x20  0x83C7100, 5 B - empty. It does not write through `id`, so a caller's variable keeps
// whatever it held. [UNRESOLVED] return type: the frame materialises nothing, so void.
void Profiler::getTimerIdByName(const char* name, unsigned int& id)
{
    (void)name; (void)id;
}

// Accepts enable requests without changing state, matching the disabled profiler implementation.
void Profiler::setEnable(bool enable)
{
    (void)enable;
}

// +0x28  0x83CA130, 12 B. `movzx eax, byte ptr [eax+4]`.
// the slot BF2Engine::initEngine calls at 0x813929B, and the result is discarded there.
bool Profiler::getEnable()
{
    return m_enable;
}

// +0x2C  0x83C9E20, 5 B - empty. The slot Demo::beginMainLoop (0x8141CF0) and Demo::endMainLoop
// (0x8142EB3) call with `false`; Game/Main/demo.cpp records both as UNRESOLVED_slot0x2C.
void Profiler::pauseVTune(bool pause)
{
    (void)pause;
}

bool Profiler::getEnablePageFaults() const              // +0x30
{
    return m_enablePageFaults;
}

void Profiler::setEnablePageFaults(bool enable)         // +0x34
{
    m_enablePageFaults = enable;
}

// Forwards a full-depth report request. The current reportFile implementation returns true without writing a file.
bool Profiler::reportToFile(const std::string& fileName)
{
    return reportFile(fileName, 0.0f, 32, true);
}

// Returns the existing case-insensitive timer ID, or appends a new name.
// The caller must keep distinct names within the 512-entry capacity; no bounds check is performed.
unsigned int Profiler::createNewTimerId(const char* name)
{
    for (unsigned int i = 0; i < m_numTimers; ++i)
    {
        if (compareIgnoreCase(m_timerInfo[i].name, name) == 0)
            return i;
    }

    unsigned int id = m_numTimers;
    m_numTimers = id + 1;
    m_timerInfo[id].name = name;
    return id;
}


void Profiler::setEnableVTune(bool enable)
{
    m_enableVTune = enable;
}


bool Profiler::getEnableVTune()
{
    return m_enableVTune;
}

// Returns the existing disabled-feature message.
std::string Profiler::reportGuards()
{
    return std::string("Currently disabled. Will fix on demand. -JonasK");
}

// `xor eax, eax` only.
// [UNRESOLVED] the return type - a zeroed eax is equally bool, int or a pointer. bool chosen because
// the name reads as an operation that reports whether it did anything.
bool Profiler::applyCalibrationOnTimers(Timer* timer)
{
    (void)timer;
    return false;
}

// `xor eax, eax` and `xor edx, edx` - the return value is 64 bits wide, which is the
// only thing the body tells us about it. [UNRESOLVED] what it means.
unsigned long long Profiler::internalReport(Timer* timer, float totalTime, std::string indent,
                                            bool showAll, unsigned int depth, bool leaf)
{
    (void)timer; (void)totalTime; (void)indent; (void)showAll; (void)depth; (void)leaf;
    return 0;
}

void Profiler::getTimers(Timer* timer, std::map<unsigned long long, unsigned int>& out)
{                                                       // empty
    (void)timer; (void)out;
}

// `mov eax, 1` and return. It writes no file. Every caller - reportToFile at the
// vtable's last non-destructor slot included - is told the report succeeded.
bool Profiler::reportFile(const std::string& fileName, float totalTime, unsigned int depth,
                          bool showAll)
{
    (void)fileName; (void)totalTime; (void)depth; (void)showAll;
    return true;
}

void Profiler::clearTimers()                            // empty
{
}

void Profiler::enableTimer(unsigned int id)             // empty
{
    (void)id;
}

void Profiler::disableTimer(unsigned int id)            // empty
{
    (void)id;
}

void Profiler::enableVTuneForTimer(unsigned int id)     // empty
{
    (void)id;
}

void Profiler::disableVTuneForTimer(unsigned int id)    // empty
{
    (void)id;
}

// `xor eax, eax`.
Profiler::Timer* Profiler::findTimer(Timer* root, unsigned int id)
{
    (void)root; (void)id;
    return 0;
}

// `fldz` - the return value is left in ST0, which is what makes this float and not
// int.
float Profiler::getGuardTimeForTimer(unsigned int id)
{
    (void)id;
    return 0.0f;
}

void Profiler::setGuardTimeForTimer(unsigned int id, float guardTime)
{                                                       // empty
    (void)id; (void)guardTime;
}

void Profiler::enableGraphForTimer(unsigned int id, float a, float b, float c, float d)
{                                                       // empty
    (void)id; (void)a; (void)b; (void)c; (void)d;
}

void Profiler::disableGraphForTimer(unsigned int id)    // empty
{
    (void)id;
}

// `mov eax, 400h`. It never looks at the string.

// 1024 is the same sentinel the constructor loads into all 32 entries of m_currentTimerId, so the
// constant means "no such timer" - and every ...FromName wrapper below therefore forwards 1024 into
// a function that ignores it. Two layers of nothing, faithfully preserved.
unsigned int Profiler::getIdFromName(const std::string& name)
{
    (void)name;
    return 1024;
}

// The four ...FromName wrappers, 41 B each, all the same shape: call getIdFromName, then tail-jump
// (`jmp`, not `call`) into the id-taking function. The tail jump is what fixes the return type as
// void - the callee's void return becomes theirs.
void Profiler::enableTimerFromName(const std::string& name)
{
    enableTimer(getIdFromName(name));
}

void Profiler::disableTimerFromName(const std::string& name)
{
    disableTimer(getIdFromName(name));
}

void Profiler::enableVTuneForTimerFromName(const std::string& name)
{
    enableVTuneForTimer(getIdFromName(name));
}

void Profiler::disableVTuneForTimerFromName(const std::string& name)
{
    disableVTuneForTimer(getIdFromName(name));
}

// Returns empty report text because timer reporting is disabled.
std::string Profiler::listTimers()
{
    return std::string("");
}

std::string Profiler::reportPerTimer()
{
    return std::string("");
}

void Profiler::enableAllTimers()                        // empty
{
}

void Profiler::disableAllTimers()                       // empty
{
}

// empty. Which is why m_threads is always an empty map: this is its only writer.
// The whole ThreadInfo type, and the 92-byte map header the constructor allocates for it, exist to
// hold data nothing ever puts there.
void Profiler::registerThread(unsigned int a, unsigned int b, const std::string& name)
{
    (void)a; (void)b; (void)name;
}

void Profiler::unregisterThread(unsigned int id)        // empty
{
    (void)id;
}

void Profiler::sumTimer(Timer** dst, Timer** src, int count)
{                                                       // empty
    (void)dst; (void)src; (void)count;
}

void Profiler::drawGraph()                              // empty
{
}

// Profiler::TimerInfo

// Starts each timer slot enabled with VTune off; other bits remain untouched by bitfield writes.
Profiler::TimerInfo::TimerInfo()
    : name(0)
    , enabled(true)
    , vtune(false)
{
}

// Initializes the timer and child table. Several unused fields remain uninitialized;
// callers must not read them before a writer supplies their values.
Profiler::Timer::Timer()
    : m_field00(0)
    , m_field04(0)
    , m_field08(0)
    , m_field0C(0)
    , m_field10(-1)
    , m_field14(0)
    , m_field18(0)
    , m_field1C(0)
    , m_field20(0)
    , m_field24(0)
    , m_field28(0)
    , m_field2C(0)
    , m_id(0)
    , m_guardTime(0.0f)
    , m_field68(10.0f)
{
    for (int i = 0; i <= 511; ++i)                      // 0x83C61F0 loop, `cmp eax, 1FFh / jbe`
        m_children[i] = 0;

    clear();                                            // tail jump at 0x83C6206
}

// Initializes a timer with the requested ID and guard time.
Profiler::Timer::Timer(unsigned int id, float guardTime)
    : m_field00(0)
    , m_field04(0)
    , m_field08(0)
    , m_field0C(0)
    , m_field10(-1)
    , m_field14(0)
    , m_field18(0)
    , m_field1C(0)
    , m_field20(0)
    , m_field24(0)
    , m_field28(0)
    , m_field2C(0)
    , m_id(id)                                          // 0x30 <- arg 1
    , m_guardTime(guardTime)                            // 0x34 <- arg 2
    , m_field38(0)
    , m_field3C(0)
    , m_field68(10.0f)
{
    for (int i = 0; i <= 511; ++i)
        m_children[i] = 0;

    clear();
}

// 0x83C62B0 (D2) / 0x83C6300 (D1), 80 B. Recursive delete of the child table, and nothing else - the
// slots are not nulled on the way out, which is fine only because the node is being destroyed.
Profiler::Timer::~Timer()
{
    for (int i = 0; i <= 511; ++i)
    {
        if (m_children[i])
            delete m_children[i];
    }
}

// Resets the accumulators and recurses. Note precisely which fields it leaves
// alone: m_field00, m_field04, m_id, m_guardTime, m_field38, m_field40..m_field50, m_field64 and
// m_field68 all survive a clear() - so those are identity/configuration, and the ones it zeroes are
// per-run measurements. That split is the strongest evidence available for what the unnamed fields
// are for, and it is the reason clear() is worth reproducing statement by statement.

// The store order below is the emitted order.
void Profiler::Timer::clear()
{
    m_field3C = 0;
    m_field08 = 0;
    m_field0C = 0;
    m_field2C = 0;
    m_field10 = -1;
    m_field14 = 0;
    m_field18 = 0;
    m_field1C = 0;
    m_field20 = 0;
    m_field24 = 0;
    m_field28 = 0;
    m_field54 = 0;
    m_field58 = 0;
    m_field5C = 0;
    m_field60 = 0;

    for (int i = 0; i <= 511; ++i)                      // 0x83C63D0 loop
    {
        if (m_children[i])
            m_children[i]->clear();
    }
}

// Calibration and detailed report-writing methods remain declaration-only where marked in Profiler.h.
// Their absence is not necessarily visible to a link check when no implemented caller reaches them.

} // namespace hfe
} // namespace dice
