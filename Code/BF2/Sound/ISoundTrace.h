// [INFERRED] Source placement.

// Sound DLL logging callbacks. Preserve virtual order; there is no virtual destructor.
// Callbacks receive message, file and line. [INFERRED] assertFailed uses the same argument triple.
// [UNRESOLVED] Client assertion behavior; the default implementation terminates.


#ifndef SOUND_ISOUNDTRACE_H     // [INFERRED] guard style
#define SOUND_ISOUNDTRACE_H

namespace dice {
namespace hfe {

class ISoundTrace
{
public:
    virtual void error(const char* msg, const char* file, int line) = 0;         // +0x00
    virtual void warning(const char* msg, const char* file, int line) = 0;       // +0x04
    virtual void info(const char* msg, const char* file, int line) = 0;          // +0x08
    virtual void assertFailed(const char* msg, const char* file, int line) = 0;  // +0x0C [INFERRED arity]
    virtual void debug(const char* msg, const char* file, int line) = 0;         // +0x10

protected:
    ~ISoundTrace() {}
};

} // namespace hfe
} // namespace dice

#endif // SOUND_ISOUNDTRACE_H
