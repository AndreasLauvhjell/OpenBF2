// Live information about an owned or neutral object. Queries read the engine object directly.
// Owns its attached plug-ins. Position and transformation expose the underlying references.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIKNOWLEDGE_INFORMATIONREAL_H
#define AI_AIDLL_AI_AIKNOWLEDGE_INFORMATIONREAL_H

#include "Information.h"

namespace dice {
namespace hfe {
namespace ai {

class InformationReal : public Information
{
public:
    // The base gets `time - 2 * allowedTimeDiff`.
    InformationReal(IAIObject* object, unsigned int types, float basicTemp, double time,
                    float allowedTimeDiff);
    virtual ~InformationReal();

    virtual int    getInformationSort();                                // +2 - 0
    virtual bool   setTime(double time);                                // +3
    virtual bool   setTime(double time, float f);                       // +4 - forwards
    virtual double getTimeStamp() const;                                // +5 - the game time
    virtual void   addContact();                                        // +6 - empty
    virtual void   removeContact();                                     // +7 - empty
    virtual void   removeAllContacts();                                 // +8 - empty
    virtual bool   hasContact() const;                                  // +9 - true
    virtual float  getSecurity(double time);                            // +10 - 1
    virtual float  getSecurity();                                       // +11 - 1
    virtual Vec3&  getPosition();                                       // +12
    virtual float  getAltitude();                                       // +13
    virtual float  getAltitude(const Vec3& direction);                  // +14
    virtual Mat4&  getTransformation();                                 // +15
    virtual Vec3   getFaceing();                                        // +16
    virtual int    getSide() const;                                     // +17
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_INFORMATIONREAL_H
