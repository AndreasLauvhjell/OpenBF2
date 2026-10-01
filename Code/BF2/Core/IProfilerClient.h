// Timing and profiler-control interface implemented by Profiler. Header path [INFERRED].
// The destructor is nonvirtual; Profiler introduces its own destructor after reportToFile.
// Keep virtual declaration order. [UNRESOLVED] whether the page-fault and report methods were
// declared by this interface or Profiler; their positions in the combined surface are fixed.

#ifndef CORE_IPROFILERCLIENT_H      // [INFERRED] guard style
#define CORE_IPROFILERCLIENT_H

#include "IBase.h"

#include <string>

namespace dice {
namespace hfe {

class IProfilerClient : public IBase
{
public:
    // IBase supplies +0x00..+0x0C: addRef, getRef, release, queryInterface const.

    virtual void  startGlobalTimer()                                            = 0;  // +0x10
    virtual void  stopGlobalTimer()                                             = 0;  // +0x14
    virtual void  startTimer(unsigned int& id, const char* name, float guardTime) = 0;// +0x18
    virtual void  stopTimer(unsigned int id)                                    = 0;  // +0x1C
    virtual void  getTimerIdByName(const char* name, unsigned int& id)          = 0;  // +0x20
    virtual void  setEnable(bool enable)                                        = 0;  // +0x24
    virtual bool  getEnable()                                                   = 0;  // +0x28
    virtual void  pauseVTune(bool pause)                                        = 0;  // +0x2C
    virtual bool  getEnablePageFaults() const                                   = 0;  // +0x30
    virtual void  setEnablePageFaults(bool enable)                              = 0;  // +0x34
    virtual bool  reportToFile(const std::string& fileName)                     = 0;  // +0x38


protected:
    // Non-virtual and protected, unchanged: the destructor pair sits at +0x3C/+0x40, after every
    // interface slot, so it belongs to the concrete class. Same reasoning as IBase.h's.
    ~IProfilerClient() {}
};

extern IProfilerClient* profilerClient;     // ds:0x8ABBCA8, name attested

} // namespace hfe
} // namespace dice

#endif // CORE_IPROFILERCLIENT_H
