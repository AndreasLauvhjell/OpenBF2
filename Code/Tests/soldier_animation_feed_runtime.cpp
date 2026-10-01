// [TEMP] Flow fixture. No live soldier lifecycle, rendering or playback acceptance implied.
#include "soldier_animation_feed_fixture.h"
#include <cstdio>
static_assert(sizeof(void*)==4, "x86 fixture required");
float crouchScale=2, proneScale=3;
float* soldierAnim1pSpeedCrouch=&crouchScale;
float* soldierAnim1pSpeedProne=&proneScale;
int checks=0, failures=0;
void check(bool value, const char* name) {
    ++checks; if(!value) ++failures; std::printf("%s %s\n", value?"PASS":"FAIL", name);
}
bool near(float a,float b) { return std::fabs(a-b)<0.0001f; }
struct State {
    Soldier soldier;
    anim::IAnimationSystem one,three,itemSystem,vehicleSystem;
    IObject item,vehicle,ikObject;
    IAnimatedBundle itemBundle,vehicleBundle;
    IWeaponObject weapon;
    struct { unsigned refs; IObject* object; } weak;
    State() {
        soldier.m_animationSystem1P=&one; soldier.m_animationSystem3P=&three;
        soldier.item=&item; item.bundle=&itemBundle; itemBundle.system=&itemSystem;
        vehicle.bundle=&vehicleBundle; vehicleBundle.system=&vehicleSystem;
        weak.refs=1; weak.object=&vehicle;
        ikObject.weapon=&weapon;
    }
};
int main() {
    {
        State s; s.soldier.m_characterMode=1; s.soldier.sprint=true;
        s.soldier.node.speed=Vec3{3,2,4}; s.soldier.node.rotation=Vec3{100,200,-300};
        s.soldier.updateAnimationSystems(.25f);
        check(s.three.movement.x==0 && s.three.movement.y==0 && near(s.three.movement.z,1), "3P sprint keeps forward direction and clears lateral motion");
        check(near(s.three.speed,5) && near(s.three.movementDt,.25f), "3P sprint receives horizontal speed and frame time");
        check(near(s.three.rotation.x,1) && near(s.three.rotation.y,2) && near(s.three.rotation.z,-3), "physics turn rate reaches animation input");
        check(s.three.poseCalls==1 && s.three.pose==0 && s.three.updates==1, "pose and soldier update are submitted");
        check(s.itemBundle.selectedMode==1 && s.three.copyTarget==&s.itemSystem && s.three.copiedPose, "item receives selected mode and pose-inclusive state copy");
        check(s.itemSystem.syncSource==&s.three && s.itemSystem.updatedObject==&s.item && near(s.itemSystem.updateDt,.25f), "item syncs movement and updates with original frame time");
    }
    {
        State s; s.soldier.sprint=true;
        s.soldier.node.speed=Vec3{0,0,7}; s.soldier.updateAnimationSystems(.125f);
        check(s.one.speed==7 && s.one.movement.z==1, "normal 1P sprint forwards measured speed without a cap or sprint-speed substitution");
    }
    {
        State s; s.soldier.sprint=true; s.soldier.canFire=false;
        s.soldier.node.speed=Vec3{3,2,-4}; s.soldier.updateAnimationSystems(.125f);
        check(s.one.speed==9 && s.one.movement.x==0 && s.one.movement.y==0 && s.one.movement.z==1, "1P blocked-fire cooldown substitutes speed state 4 and forward direction");
    }
    {
        State s; s.soldier.m_pose=1; s.soldier.node.speed=Vec3{0,0,2};
        s.soldier.updateAnimationSystems(.1f);
        check(s.one.speed==3 && s.one.pose==1, "1P crouch scaling is capped and crouch pose forwarded");
    }
    {
        State s; s.soldier.responseFA=false; s.soldier.m_byte27D=true;
        s.soldier.node.speed=Vec3{3,2,4}; s.soldier.updateAnimationSystems(.4f);
        check(s.one.movementDt==0 && s.one.movement.x==0 && s.one.movement.z==1, "jump/fire freezes movement clock and snaps dominant forward direction");
        check(s.itemSystem.movementCalls==1 && s.itemSystem.movementDt==0 && s.itemSystem.speed==0 && s.itemSystem.syncSource==0, "jump/fire explicitly sends zero movement to item");
        check(near(s.itemSystem.updateDt,.4f) && near(s.one.updateDt,.4f), "jump/fire preserves playback update frame time");
    }
    {
        State s; s.soldier.node.speed=Vec3{1.1f,0,0}; s.soldier.node.tangent=Vec3{1,0,0};
        s.soldier.updateAnimationSystems(.1f);
        check(s.one.speed==0 && s.one.movement.x==0 && s.one.movement.z==0, "small movement relative to moving platform becomes idle");
    }
    {
        State s; s.one.onMovement=[&] { s.soldier.m_characterMode=1; };
        s.three.onCopy=[&] { s.soldier.m_characterMode=0; };
        s.soldier.updateAnimationSystems(.1f);
        check(s.one.movementCalls==1 && s.one.poseCalls==0 && s.three.poseCalls==1, "pose uses current system after movement callback changes mode");
        check(s.itemBundle.selectedMode==1 && s.three.copies==1, "item selection and copy use current mode");
        check(s.itemSystem.syncSource==&s.one && s.one.updates==1 && s.three.updates==0, "sync and final update reselect after copy callback changes mode");
    }
    {
        State s; s.itemSystem.onUpdate=[&] { s.soldier.m_characterMode=1; };
        s.soldier.m_usedVehicleWeak=&s.weak; s.soldier.m_ikObject=&s.ikObject;
        s.soldier.m_ikTemplateComponent=&s; s.weapon.zoom=true; s.weapon.firing=true;
        s.soldier.updateAnimationSystems(.2f);
        check(s.vehicleBundle.selectedMode==1 && s.three.copyTarget==&s.vehicleSystem && !s.three.copiedPose, "vehicle gate rereads mode after item update and excludes pose from copy");
        check(s.vehicleSystem.updatedObject==&s.vehicle && near(s.vehicleSystem.updateDt,.2f), "vehicle receives original frame time");
        check(s.three.messages==5 && s.soldier.headResets==1 && s.three.updates==1, "IK weapon zoom/fire bits reach active system before final update");
    }
    {
        State s; s.soldier.m_ragDoll=&s; s.soldier.updateAnimationSystems(.1f);
        check(s.one.movementCalls==0 && s.one.updates==0 && s.itemSystem.updates==0, "ragdoll skips all animation-system feeds");
        s.soldier.m_ragDoll=0; s.item.flags=1; s.soldier.updateAnimationSystems(.1f);
        check(s.one.updates==1 && s.itemSystem.updates==0, "disabled item is skipped while soldier continues");
    }
    std::printf("%d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
