// Live armament information forwarded to the engine object.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIKNOWLEDGE_IPIARMAMENTREAL_H
#define AI_AIDLL_AI_AIKNOWLEDGE_IPIARMAMENTREAL_H

#include "IPIArmament.h"

namespace dice {
namespace hfe {
namespace ai {

class IPIArmamentReal : public IPIArmament
{
public:
    explicit IPIArmamentReal(Information* information);
    virtual ~IPIArmamentReal();

    virtual unsigned int getWeapon(int index);                          // +7
    virtual std::vector<unsigned int>& getWeapons();                    // +8
    virtual void updateWeapons();                                       // +12
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_IPIARMAMENTREAL_H
