// Grappling-rope settings and seat animation. Unnamed tuning values retain neutral indices.
// The stored Player pointer is live runtime state. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GRAPPLINGHOOKROPETEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_GRAPPLINGHOOKROPETEMPLATE_H

#include "Base/SimpleObjectTemplate.h"

#include <string>

namespace dice {
namespace anim { class IAnimationSystemTemplate; }
namespace hfe {
namespace world {

class GrapplingHookRope;
class GrapplingHookRopeContainer;
class IPlayer;
class PlayerControlObject;

// The trailing template word is intentionally left uninitialized and copied into the live rope during initialization.
class GrapplingHookRopeTemplate : public SimpleObjectTemplate
{
public:
    GrapplingHookRopeTemplate();

    // Inherits makeScript from SimpleObjectTemplate; do not add an override here.
    virtual void  addRef();                          // `++[this+0x164]`, read directly
    virtual int   getRef();                          // `return [this+0x164]`
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;   // byte-identical to the base's 56 B
    virtual unsigned int getClassID() const;
    virtual ~GrapplingHookRopeTemplate();            // (D1) - see the note at the dtor body
    virtual void* queryImplementation(unsigned int); // flat 3-cid, no base call
    virtual IObject* createObject() const;           // new(0x210) GrapplingHookRope

    // Resolves or renames the seat-animation template through its manager. The destructor releases it.
    // Replacing a nonnull template with an empty name releases it without clearing the stored pointer.
    void setSeatAnimationSystem(const std::string& name);
    const std::string& getSeatAnimationSystem();

    // (528 B), PE 0x578F90 (372 B).  FireComp reaches this only for the
    // GrapplingHookRope projectile class-id arm.
    GrapplingHookRope* createGrapplingHookObject(IPlayer*, Mat4&, float);

protected:
    friend class GrapplingHookRope;
    friend class GrapplingHookRopeContainer;
    friend class PlayerControlObject;

    // layout, offsets 356..467. SimpleObjectTemplate occupies 0..355.
    // Every default below is from the full ctor read.
    /* 356 */ int   m_refCount2;                     // ctor 1 - and addRef/getRef both read it
    /* 360 */ anim::IAnimationSystemTemplate* m_seatAnimationSystem; // ctor 0; manager-owned ref
    /* 364 */ std::string m_animationName;           // retyped int -> std::string on the ctor's
                                                     // _S_empty_rep construction AND the dtor's
                                                     // refcount decrement. Ctor: empty.
                                                     // [UNRESOLVED] which name it holds - no
                                                     // accessor reads it, and the seat animation
                                                     // system uses +0x168 instead.

    // Runtime state in a template - see the header block.
    /* 368 */ IPlayer* m_player;                     // createGrapplingHookObject's player; ctor 0

    /* 372 */ float _float174;                       // [UNRESOLVED] name; ctor 14.0f
    /* 376 */ int   _dword178;                       // [UNRESOLVED] name; ctor 28 (0x1C - stored as
                                                     // a plain integer immediate, and 0x1C read
                                                     // as a float would be a denormal, so int)

    // 380..463: 21 consecutive unnamed floats - the rope simulation constants. [UNRESOLVED]
    // individually, exactly as the header block above explains: no accessors, no console bindings,
    // and their recovered consumers still do not attest the original property names. Naming them
    // would mint 21 invented names. The defaults are what a rebuild actually needs - content that
    // omits a property inherits them - and those are recovered in full.
    /* 380 */ float _ropeParams[21];                 // 380..463
    /* 464 */ float _float1D0;                       // [UNRESOLVED]; ctor deliberately leaves it
                                                     // uninitialized; initialize reads it
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GRAPPLINGHOOKROPETEMPLATE_H
