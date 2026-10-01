// [TEMP] [CHOSEN, not recovered] collaborators for the unchanged Soldier update body.
// This tests movement/pose/callback flow, not engine layouts or virtual-slot ABI.
#pragma once
#include "../BF2/Math/Mat4.h"
#include "../BF2/Math/Vec3.h"
#include <cmath>
#include <cfloat>
#include <functional>
using dice::hfe::Mat4;
using dice::hfe::Vec3;
enum { IID_IAnimatedBundle = 1, IID_IWeaponObject = 2 };
struct IObject;
namespace anim {
struct IAnimationSystem {
    int movementCalls = 0, poseCalls = 0, updates = 0, copies = 0;
    float movementDt = -1, speed = -1, updateDt = -1;
    Vec3 movement = {}, rotation = {};
    unsigned pose = 99, messages = 0;
    bool copiedPose = false;
    IObject* updatedObject = 0;
    IAnimationSystem* copyTarget = 0;
    IAnimationSystem* syncSource = 0;
    std::function<void()> onMovement, onCopy, onUpdate;
    virtual void setTriggerMovement(float dt, const Vec3& m, float s, const Vec3& r) {
        ++movementCalls; movementDt=dt; movement=m; speed=s; rotation=r;
        if(onMovement) onMovement();
    }
    virtual void setTriggerPose(unsigned p) { ++poseCalls; pose=p; }
    virtual void copyStateToSystem(IAnimationSystem* target, bool p) {
        ++copies; copyTarget=target; copiedPose=p; if(onCopy) onCopy();
    }
    virtual void syncMovementAnimations(IAnimationSystem* s) { syncSource=s; }
    virtual void update(IObject* object, float dt) {
        ++updates; updatedObject=object; updateDt=dt; if(onUpdate) onUpdate();
    }
    virtual void setTriggerMessage(unsigned m) { messages |= m; }
    virtual void clearTriggerMessage(unsigned m) { messages &= ~m; }
};
}
struct IAnimatedBundle {
    anim::IAnimationSystem* system = 0;
    int selectedMode = -1;
    std::function<void()> onSelect;
    virtual void setActiveAnimationSystem(int mode) { selectedMode=mode; if(onSelect) onSelect(); }
    virtual void* getActiveAnimationSystem() { return system; }
};
struct IWeaponObject {
    bool zoom = false, firing = false;
    virtual int getZoom() { return zoom; }
    virtual bool isFiring() { return firing; }
};
struct IObject {
    unsigned flags = 0;
    IAnimatedBundle* bundle = 0;
    IWeaponObject* weapon = 0;
    virtual void* queryInterface(int id) {
        return id==IID_IAnimatedBundle ? static_cast<void*>(bundle) : static_cast<void*>(weapon);
    }
};
struct SoldierPhysicsNode {
    Vec3 speed = {}, tangent = {}, rotation = {};
    virtual const Vec3& getAbsolutePositionalSpeed() { return speed; }
    void getCollidingMeshTangentSpeed(Vec3& output) { output=tangent; }
    virtual const Vec3& getRotationalSpeed() { return rotation; }
};
struct Soldier : IObject {
    void* m_ragDoll = 0;
    SoldierPhysicsNode node;
    IObject* parent = 0;
    bool responseFA = true, sprint = false, canFire = true;
    bool m_byte27D = false, m_byte27C = false, m_isSkyDiving = false;
    int m_characterMode = 0, m_pose = 0, m_activeItemIndex = 0, m_grapplingHookId = 0;
    float m_facingDegrees = 0, inAirResult = 0;
    int headResets = 0;
    Mat4 frame = Mat4::getIdentity();
    anim::IAnimationSystem* m_animationSystem1P = 0;
    anim::IAnimationSystem* m_animationSystem3P = 0;
    IObject* item = 0;
    void* m_usedVehicleWeak = 0;
    void* m_refBlock1D8 = 0;
    IObject* m_ikObject = 0;
    void* m_ikTemplateComponent = 0;
    const Mat4& getMovementMatrix() { return frame; }
    virtual bool getIsSprinting() { return sprint; }
    virtual bool getCanFireWeapon() { return canFire; }
    float getSoldierSpeed(int index) { return index==4 ? 9.0f : 3.0f; }
    float soldierInAirAction(const Vec3&) { return inAirResult; }
    virtual IObject* getItem(int) { return item; }
    void resetHeadMovements() { ++headResets; }
    void updateAnimationSystems(float dt);
};
struct L {
    static SoldierPhysicsNode* physicsNode(Soldier* s) { return &s->node; }
    static IObject* parent(Soldier* s) { return s->parent; }
    static unsigned flags(IObject* object) { return object->flags; }
};
inline bool responseByteFA(Soldier* s) { return s->responseFA; }
inline void getRotationalBundleRotation(IObject*, Vec3& r) { r=Vec3{2,0,-2}; }
extern float* soldierAnim1pSpeedCrouch;
extern float* soldierAnim1pSpeedProne;
