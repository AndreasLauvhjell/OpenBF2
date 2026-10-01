// Destructible bundle with cache, collision and replication behavior.
// Armor state belongs to its component. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_DESTROYABLEOBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_DESTROYABLEOBJECT_H

#include "../Base/Bundle.h"

namespace dice {
namespace hfe {
namespace world {

class DestroyableObjectTemplate;

// DestroyableObject : Bundle - the ctor's first call is Bundle::Bundle(template). sizeof = 0x140
// (320) from createObject's new; Bundle occupies 0x000..0x13B, own state is ONE dword.
// Vtable off_8897BC8 (ZTV 0x8897BC0): 96 slots = Bundle's 96, NO new slots. Overrides: the quartet
// (0..3), dtor (5/6), disableCachedObject (9), enableCachedObject (10), handleCollision (45),
// initNetworkableComp (94). Secondary table off_8897D50 at +0x80 = io::IConsoleSaveable.
// queryInterface 0x8174170 (45 B): IBase / IObject -> this; IConsoleSaveable -> this + 0x80; else 0.
class DestroyableObject : public Bundle
{
public:
    explicit DestroyableObject(const DestroyableObjectTemplate* objectTemplate);   // read in full
    virtual ~DestroyableObject();                    // (D1) / 51 B (D0) - vptr restores only

    // the quartet, refcount at +0x13C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;

    virtual void  disableCachedObject();             // drop the AI component, then Simple's
    virtual void  enableCachedObject();              // , the cache-revive arm
    virtual bool  handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                  int, int, CollisionFaceInfo*, CollisionFaceInfo*);
                                                     // [FRONTIER] banked in the .cpp
    virtual void  initNetworkableComp();             // (one guarded store, see the .cpp)

protected:
    /* 0x13C */ int m_refCount;                      // ctor 1 - the only own field
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_DESTROYABLEOBJECT_H
