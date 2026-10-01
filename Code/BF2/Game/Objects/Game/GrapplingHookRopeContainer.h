// Path:     [INFERRED] beside GrapplingHookRope.{h,cpp}; no class-local __FILE__ was found.
// Contents: typed container surface/layout recovered from bf2.i64 and bf2_w32ded.exe.

#ifndef GAME_OBJECTS_GAME_GRAPPLINGHOOKROPECONTAINER_H
#define GAME_OBJECTS_GAME_GRAPPLINGHOOKROPECONTAINER_H

#include "PlayerControlObject.h"

#include <vector>

namespace dice {
namespace hfe {

class GrapplingHookContainerCreateEvent;
class GrapplingHookContainerUpdateEvent;
class GrapplingHookContainerDetachEvent;
class ServerConnection;

namespace world {

class GrapplingHookRope;
class GrapplingHookRopeContainerTemplate;
class RopeLink;
class Soldier;

// ELF sizeof 0x420 (factory 0x8182350); PE uses its wider PlayerControlObject base. Only the
// fields reached by recovered event/animation code receive semantic names here.
class GrapplingHookRopeContainer : public PlayerControlObject
{
public:
    enum AnimationState
    {
        AnimationState_1 = 1,     // [UNRESOLVED] semantic name
        AnimationState_2 = 2,     // [UNRESOLVED] semantic name
        AnimationState_4 = 4      // ctor/default state
    };

    void initialize(Soldier*, GrapplingHookRope*, unsigned int);
    void setCurrentState(unsigned int segmentIndex, float t);
    void setState(AnimationState);
    IObject* getCamera();

    explicit GrapplingHookRopeContainer(const GrapplingHookRopeContainerTemplate*);
    virtual ~GrapplingHookRopeContainer();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual void  destroy(bool force);
    virtual io::INetworkable* getNetworkable() const;
    virtual void  handleFrameUpdate(float deltaTime);
    virtual void  handlePlayerInput(IPlayer*, const io::PlayerInput&, float, unsigned int);
    virtual bool  enter(IPlayer*);
    virtual void  exit();
    virtual bool  isAllowedToExit();

    // GrapplingHookRopeContainer's four appended PCO slots, in shipped order.
    virtual bool _enter(IPlayer*);
    virtual void _exit();
    virtual bool applyCameraAnimation(const Mat4&, Mat4&);
    virtual bool allowEnterOtherVehicle();

private:
    void updateClimbAnimation();
    void handleClimber();
    void handleDismount(std::vector<RopeLink*>::iterator, int);

    friend class GrapplingHookRope;
    friend class GrapplingHookRopeContainerTemplate;
    friend class PlayerControlObject;
    friend class ::dice::hfe::GrapplingHookContainerCreateEvent;
    friend class ::dice::hfe::GrapplingHookContainerUpdateEvent;
    friend class ::dice::hfe::GrapplingHookContainerDetachEvent;
    // clientSendDatabase's active-container association reads m_rope, m_segmentIndex and
    // m_entryAngle (ELF 0x808A778..0x808A7F3 / PE 0x54C7E0 reads +0x414/+0x420/+0x424).
    friend class ::dice::hfe::ServerConnection;

    /* ELF +0x3BC */ int m_refCount;
    /* ELF +0x3C0 */ float _float3C0;                   // ctor 0.029999999 [UNRESOLVED]
    /* ELF +0x3C4 */ const GrapplingHookRopeContainerTemplate* m_containerTemplate;
    /* ELF +0x3C8 */ GrapplingHookRope* m_rope;
    /* ELF +0x3CC */ Soldier* m_soldier;
    /* ELF +0x3D0 */ RopeLink* m_segment;
    /* ELF +0x3D4 */ unsigned int m_segmentIndex;
    /* ELF +0x3D8 */ float m_entryAngle;
    /* ELF +0x3DC */ float _float3DC;                   // initialized from rope/template [UNRESOLVED]
    /* ELF +0x3E0 */ float m_segmentProgress;
    /* ELF +0x3E4 */ Vec3 m_initialSoldierPosition;
    /* ELF +0x3F0 */ Vec3 m_detachPosition;
    /* ELF +0x3FC */ float _float3FC;
    /* ELF +0x400 */ float _float400;
    /* ELF +0x404 */ float _float404;                   // PIAction sample; compared as a float
    /* ELF +0x408 */ float m_eventField18;
    /* ELF +0x40C */ float _float40C;
    /* ELF +0x410 */ float m_animationTime;              // current playing-animation time
    /* ELF +0x414 */ float _float414;
    /* ELF +0x418 */ AnimationState m_animationState;
    /* ELF +0x41C */ AnimationState m_previousAnimationState;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_GRAPPLINGHOOKROPECONTAINER_H
