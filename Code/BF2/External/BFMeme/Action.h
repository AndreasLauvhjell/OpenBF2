// Name descriptor-attested83C320; path/method names and const spelling inferred.
#ifndef BFMEME_ACTION_H
#define BFMEME_ACTION_H
#include "Object.h"
#if !DEDICATED
namespace dice { namespace meme {
struct Rect;
struct NodeState;
struct NodeInputEvent;
class Action : public Object
{
public:
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    // ActionNode830BB0 forwards these unchanged from Node's event dispatcher.
    virtual bool execute(System*, const Rect*, const NodeState*, const NodeInputEvent*);
    static TypeDescriptor* getClassType();
};
} }
#endif
#endif
