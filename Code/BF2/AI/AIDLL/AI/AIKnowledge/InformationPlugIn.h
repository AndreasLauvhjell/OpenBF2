// Base for information plug-ins; each instance borrows its owning information record.
// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding InformationPlugIn operation.
#ifndef AI_AIDLL_AI_AIKNOWLEDGE_INFORMATIONPLUGIN_H
#define AI_AIDLL_AI_AIKNOWLEDGE_INFORMATIONPLUGIN_H

namespace dice { namespace hfe { namespace ai {

class Information;

class InformationPlugIn
{
public:
    virtual ~InformationPlugIn() {}           // +0 / +1 ( / )
    virtual int getType() = 0;                // +2
    virtual void setTime() = 0;               // +3
    virtual void setTime(double time) = 0;    // +4 (PE: the overload pair is reversed, +4 / +8)

protected:
    // [INFERRED] member name. Every concrete plug-in's constructor stores its Information here
    // (IPIUnit, IPIPhysicalReal: ELF and PE +4) and every forwarding body reads it back.
    Information* m_information;
};

} } }
#endif
