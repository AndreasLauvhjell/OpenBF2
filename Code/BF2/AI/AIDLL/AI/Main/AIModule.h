// Loadable AI module interface.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_MAIN_AIMODULE_H     // [INFERRED] guard style
#define AI_AIDLL_AI_MAIN_AIMODULE_H

#include <vector>

#include "../../../../Core/IModule.h"

namespace dice {
namespace hfe {

struct ClassRegInfo;
struct SingletonRegInfo;

namespace ai {

// sizeof == 8, proven by the `operator new(8u)` at the static initialiser: a vptr and a refcount and
// nothing else. The module carries no state of its own - everything it creates is a self-installing
// singleton that registers itself elsewhere and whose pointer it throws away.
class AIModule : public IModule
{
public:
    AIModule();

    // IBase
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    // This module publishes services through the simple-singleton registry during initialization.
    virtual void  getClassList(std::vector<ClassRegInfo>& out);          // - empty
    virtual void  getSingletonList(std::vector<SingletonRegInfo>& out);  // - empty

    // IModule
    virtual bool  init();
    virtual bool  close();

private:
    /*  0 */ // vptr
    /*  4 */ int m_refCount;        // = 1 from the constructor
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_MAIN_AIMODULE_H
