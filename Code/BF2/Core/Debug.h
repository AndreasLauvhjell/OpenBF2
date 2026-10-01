// raii diagnostic stream; destruction filters and emits the accumulated message.
// Header path [INFERRED]. Emitted messages abort after the callback returns, except Info and Log.
// Filtered messages return normally. Types 7 and 4 use the Error and Assert policies respectively.
// The callback may terminate earlier; do not assume that a warning or error permits continuation.

#ifndef CORE_DEBUG_H     // [INFERRED] guard style
#define CORE_DEBUG_H

#include <sstream>
#include <string>

namespace dice {
namespace hfe {

// Values are frozen by the binary. Enumerator names come from the emitters' switch literals; 4 and
// 7 never reach a switch (folded away in ~Debug), so their names are [UNRESOLVED] - the "2" suffix
// records only "alias of the base type".
enum DebugType
{
    DebugTypeDebug    = 0,   // "Debug"     - aborts if emitted
    DebugTypeInfo     = 1,   // "Info"      - continues
    DebugTypeWarning  = 2,   // "Warning"   - aborts if emitted (unused in this binary)
    DebugTypeAssert   = 3,   // "Assert"    - aborts if emitted; also triggers the
                             //               "Debug assertion failed!" block in the default emitter
    DebugTypeAssert2  = 4,   // [UNRESOLVED name] folded to 3 by ~Debug before anything else
    DebugTypeDxAssert = 5,   // "DxAssert"  - aborts if emitted (unused in this binary)
    DebugTypeError    = 6,   // "Error"     - aborts if emitted; also triggers the assert block
    DebugTypeError2   = 7,   // [UNRESOLVED name] folded to 6 by ~Debug before anything else
    DebugTypeLog      = 8,   // "Log"       - continues
    DebugTypeCount    = 9,   // attested: the static-init loop builds exactly 9 debugModules
                             // Numeric compatibility aliases. Prefer the named types in new code; Error terminates the process.
    DEBUGTYPE_1 = DebugTypeInfo,     // = 1
    DEBUGTYPE_6 = DebugTypeError,    // = 6
    DEBUGTYPE_7 = DebugTypeError2,   // = 7
    DEBUGTYPE_8 = DebugTypeLog       // = 8
};

// Receives the parent-directory module name, basename, source location, expression and message.
// fullFileName retains the complete path; data is the caller-provided callback context.
typedef void (*DebugCallback)(DebugType type, const std::string& module,
                              const std::string& file, int line, const std::string& func,
                              const std::string& text, const std::string& fullFileName,
                              void* data);   // typedef name [INFERRED]

class Debug
{
public:
    // Records a diagnostic. func carries a function name or failed assertion expression.
    // [CHOSEN] The optional fourth argument supports callers that omit this detail.
    Debug(DebugType type, const std::string& file, int line, const std::string& func = "");

    // the emitter. Emitted iff debugOn[type] XOR debugModules[type] lists the module (the parent
    // directory of file) - debugModules is an exception list either way. If emitted:
    // bfGlobal_callback[type] when set, else internal_debugCallback; then abort() unless the type
    // is Info(1) or Log(8). See the correction block above - this is where 570 of the 593 call
    // sites end the process.
    ~Debug();

    // [INFERRED] Stream insertion wrapper for the owned message buffer.
    template <typename T>
    Debug& operator<<(const T& v) { m_stream << v; return *this; }

private:
    // Layout (was [UNRESOLVED]) - every ctor store accounted for. Declaration
    // order is attested: the ctor constructs m_func (+8) before storing m_line (+12).
    DebugType          m_type;    // [this+0]   mutated by ~Debug itself (7->6, 4->3)
    std::string        m_file;    // [this+4]
    std::string        m_func;    // [this+8]
    int                m_line;    // [this+12]
    std::ostringstream m_stream;  // [this+16]  (the binary shows GCC 3.3-era ostringstream innards)
};

// Free functions of the same TU (names/signatures frozen from mangled symbols)

void setDebugCallback(DebugType type, DebugCallback callback, void* data);

// The built-in emitter, used when no callback is registered for the type. Formats
//     <fullFileName>(<line>): <TypeName>: <module>: [(func) ]<text>\n
// (the "(func) " part only for Assert) into an ostringstream that is never output on this build,
// then for Assert/Error composes the "Debug assertion failed!" block and abort()s.
void internal_debugCallback(DebugType type, const std::string& module,
                            const std::string& file, int line, const std::string& func,
                            const std::string& text, const std::string& fullFileName,
                            void* data);

// these four take plain `int`, not DebugType - attested by the mangled names ('i'). All four
// have zero callers in the server binary: dead but exported API, recovered as part of the TU.
void turnOnAllDebug(int type);
void turnOffAllDebug(int type);
void turnOnDebug(int type, const std::string& module);
void turnOffDebug(int type, const std::string& module);

// Global state (pointer + storage pairs; see Debug.cpp for why they are pointers)

template <typename K, typename V, typename HashFunc> class Hash;   // Core/Hash.h
struct StringHashFunc;                                             // Core/Hash.h

extern bool*          debugOn;                // 0x8990158 -> _debugOn 0x8990144, bool[9] all true
extern DebugCallback* bfGlobal_callback;      // 0x8990150 -> _bfGlobal_callback 0x8AC8600 (.bss)
extern void**         bfGlobal_callbackData;  // 0x8990154 -> _bfGlobal_callbackData 0x8AC8640 (.bss)
extern Hash<std::string, std::string, StringHashFunc>* debugModules;
                                              // 0x899015C -> _debugModules 0x8AC8680, 9 hashes

} // namespace hfe
} // namespace dice

#endif // CORE_DEBUG_H
