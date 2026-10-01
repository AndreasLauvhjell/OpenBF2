// Test scaffolding. Diagnostic scope for private physics source copies. No repaired values.
#pragma once
#include "../../BF2/Physics/IPhysicsNode.h"
extern void floatPhysics(dice::hfe::world::IPhysicsNode*,const char*,const char*);
struct FloatPhysicsScope {
    dice::hfe::world::IPhysicsNode* node;const char* method;
    FloatPhysicsScope(dice::hfe::world::IPhysicsNode* n,const char* m):node(n),method(m){floatPhysics(node,method,"before");}
    ~FloatPhysicsScope(){floatPhysics(node,method,"after");}
};
