// Path: [INFERRED] beside the other world runtime products; no __FILE__ string found.
#ifndef GAME_OBJECTS_GAME_DYNAMICBUNDLE_H
#define GAME_OBJECTS_GAME_DYNAMICBUNDLE_H

#include "../Base/Bundle.h"

namespace dice { namespace hfe { namespace world {
class DynamicBundleTemplate;

// Both dedicated vtables differ from Bundle only in their destructor slots.
class DynamicBundle : public Bundle
{
public:
    explicit DynamicBundle(const DynamicBundleTemplate*);
    virtual ~DynamicBundle();
};
} } }
#endif
