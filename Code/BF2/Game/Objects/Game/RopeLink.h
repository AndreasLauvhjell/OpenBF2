// Path:     [INFERRED] Game/Objects/Game/RopeLink.h - beside GrapplingHookRopeTemplate's family.
// Contents: the complete small virtual/refcount surface and endpoint layout recovered from the
// symbolized ELF and checked against the PE, including its transformation builder.

#ifndef GAME_OBJECTS_GAME_ROPELINK_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_ROPELINK_H

#include "../Base/SimpleObject.h"

namespace dice {
namespace hfe {
class GrapplingHookContainerUpdateEvent;
class ServerGameLogic;
namespace world {

class GrapplingHookRope;
class GrapplingHookRopeContainer;
class HookLinkTemplate;
class RopeLinkTemplate;
struct RopeParticle;

class RopeLink : public SimpleObject          // RTTI: single base SimpleObject
{
public:
    Vec3 getMidpoint();                       // / PE counterpart
    void setTransformation();
    void setAwake();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual void  initNetworkableComp();

private:
    explicit RopeLink(const RopeLinkTemplate*);
    virtual ~RopeLink();

    friend class GrapplingHookRope;
    friend class GrapplingHookRopeContainer;
    friend class HookLinkTemplate;
    friend class RopeLinkTemplate;
    friend class ::dice::hfe::GrapplingHookContainerUpdateEvent;
    friend class ::dice::hfe::ServerGameLogic; // drop search reads rope and deploying kit type

    // ELF ctor 0x81A5010: refcount 1 followed by four null pointers. PE confirms the same tail
    // after its larger SimpleObject base. +0x140/+0x144 point at RopeParticle records, whose first
    // Vec3 is the endpoint consumed by GrapplingHookContainerUpdateEvent.
    /* ELF +0x138 */ int m_refCount;
    /* ELF +0x13C */ GrapplingHookRope* m_rope;
    /* ELF +0x140 */ RopeParticle* m_startParticle;
    /* ELF +0x144 */ RopeParticle* m_endParticle;
    /* ELF +0x148 */ int m_climbDirection;      // -1 / 0 / 1 from link's normalized vertical axis
    /* ELF +0x14C */ int _dword14C;             // server initialize stores ISoldier::getKitType;
                                                // deliberately uninitialized by the ctor
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_ROPELINK_H
