// [INFERRED] path: adjacent to the attested AIKnowledge sources; filename follows the class.
// Full body: , . The ELF mangling attests const.
#ifndef AI_AIDLL_AI_AIKNOWLEDGE_UNITPREDICATE_H
#define AI_AIDLL_AI_AIKNOWLEDGE_UNITPREDICATE_H

#include "AIInformationGrid.h"
#include "Information.h"

namespace dice { namespace hfe { namespace ai {

class UnitPredicate : public IAIInformationGridPredicate
{
public:
    bool includeInformation(Information* information) const override
    {
        return information->getUnit() != nullptr;
    }
};

} } }
#endif
