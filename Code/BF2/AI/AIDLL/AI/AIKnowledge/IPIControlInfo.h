// Control information plug-in; adds no virtual methods to InformationPlugIn.
// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding IPIControlInfo operation.
#ifndef AI_AIDLL_AI_AIKNOWLEDGE_IPICONTROLINFO_H
#define AI_AIDLL_AI_AIKNOWLEDGE_IPICONTROLINFO_H

#include "InformationPlugIn.h"

namespace dice { namespace hfe { namespace ai {

class IPIControlInfo : public InformationPlugIn
{
public:
    explicit IPIControlInfo(Information* information) { m_information = information; }

    // only the base's vptr store - nothing of its own.
    virtual ~IPIControlInfo() {}
    // 5 - Information::removePlugIn's case for the control-info plug-in.
    virtual int getType() { return 5; }
    // empty - the control values are read
    // through to the engine object on every call, so there is nothing to refresh.
    virtual void setTime() {}
    virtual void setTime(double /*time*/) {}
};

} } }
#endif
