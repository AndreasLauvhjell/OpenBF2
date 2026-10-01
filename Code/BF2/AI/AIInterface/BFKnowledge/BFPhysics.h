// Exposes engine physics queries to the AI.
// Preserve virtual declaration order. [INFERRED] Return types require validation where implementations remain incomplete.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_BFPHYSICS_H
#define AI_AIINTERFACE_BFKNOWLEDGE_BFPHYSICS_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "AIAdapterTypes.h"

namespace dice {
namespace hfe {

class IBase;

namespace ai {

class IAIPhysics
{
public:
    IAIPhysics();
    virtual ~IAIPhysics();

    virtual float getGravity() const = 0;
};

class BFPhysics : public IAIPhysics
{
public:
    BFPhysics();
    virtual ~BFPhysics();

    virtual float getGravity() const;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_BFPHYSICS_H
