// Creates navigation graphs from engine geometry.
// Preserve virtual declaration order. [INFERRED] Return types require validation where implementations remain incomplete.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_BFGRAPHCREATOR_H
#define AI_AIINTERFACE_BFKNOWLEDGE_BFGRAPHCREATOR_H

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

class IBFGraphCreator
{
public:
    IBFGraphCreator();
    virtual ~IBFGraphCreator();

    virtual IBFGraph* createGraph() = 0;
    virtual void destroyGraph(IBFGraph* graph) = 0;
};

class BFGraphCreator : public IBFGraphCreator
{
public:
    BFGraphCreator();
    virtual ~BFGraphCreator();

    virtual IBFGraph* createGraph();
    virtual void destroyGraph(IBFGraph* graph);
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_BFGRAPHCREATOR_H
