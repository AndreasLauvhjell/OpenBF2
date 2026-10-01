// [TEMP] Actual HUD target body, native Frustum and Vars. External ray/cull
// services are typed observers. Actual constructors run; wider lifetimes are
// retained to process exit while their unrelated virtual bodies remain open.
// [RETIRE-WHEN: HUD-SPOTTED-TARGET-FIXTURE] Replace these observers/retained
// owners when the complete world, HUD and renderer lifetimes are executable.
#define HUD_PROJECTION_FIXTURE_NO_MAIN
#include "hud_projection_runtime.cpp"
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Game/Utility/Vars.h"
#include "../BF2/Game/Objects/Object.h"
#include "../BF2/Scene/Object/ObjectManager.h"
#include "../BF2/Scene/Object/ObjectPredicator.h"
#include "../BF2/Scene/Object/BinTreeCuller.h"
#include "../BF2/Scene/Object/QuadTreeCuller.h"
#include "../BF2/Physics/HeightmapCluster.h"
#include "../BF2/Math/Frustum.h"
#include "../BF2/IO/Network/INetworkable.h"
namespace dice { namespace hfe {
IBase* g_hudInformationLayer=nullptr;
namespace world { ObjectManager* objectManager=nullptr; }
} }
struct TargetNet : io::INetworkable {
    void addRef() override {actionUnsupported("net addref");}
    int getRef() override {actionUnsupported("net getref");}
    int release() override {actionUnsupported("net release");}
    void* queryInterface(unsigned) const override {actionUnsupported("net query");}
    bool init(void*) override {actionUnsupported("net init");}
    unsigned getGhostStateMask() const override {actionUnsupported("net mask");}
    void updateStateMask(unsigned*) override {actionUnsupported("net mask update");}
    bool getNetUpdate(io::BitStream&,io::BaseLineData*,unsigned,int) override {actionUnsupported("net update");}
    void setNetUpdate(io::BitStream&,float,bool,io::BaseLineData*,int) override {actionUnsupported("net set");}
    bool getBaseLine(io::BaseLineData&) override {actionUnsupported("net baseline");}
};
struct TargetObject : world::Object {
    Vec3 position={0,0,0};
    Mat4 transform=Mat4::getIdentity();
    io::INetworkable* network=nullptr;
    bool pco=false;
    std::string label;
    TargetObject(const char* value):Object(nullptr),label(value) {m_flags|=0x01000100;}
    void parent(world::IObject* p) {m_parent=p;m_cachedRootParent=nullptr;m_flags&=~0x01000000u;}
    void flags(unsigned value) {m_flags=value;m_cachedRootParent=nullptr;}
    const Vec3& getAbsolutePosition() const override {trace.push_back(label+" position");return position;}
    const Mat4& getAbsoluteTransformation() const override {trace.push_back(label+" transform");return transform;}
    io::INetworkable* getNetworkable() const override {trace.push_back(label+" net");return network;}
    void* queryInterface(unsigned iid) const override {
        check(iid==world::IID_IPlayerControlObject,"cull predicate asks IPCO");
        // This fixture's returned IBase identity is only tested for nonnull;
        // no IPCO method is dispatched through it.
        return pco?const_cast<TargetObject*>(this):nullptr;
    }
};
struct TargetPlayer : ProjectionPlayer {
    world::IObject* vehicle=nullptr;
    mutable int vehicleReads=0;
    world::IObject* getVehicle() const override {++vehicleReads;trace.push_back("vehicle");return vehicle;}
};
struct TargetView : ProjectionView {
    Frustum frustum;
    Frustum* getFrustumPtr() override {trace.push_back("frustum");return &frustum;}
};
struct TargetTerrain : world::HeightmapCluster {
    bool hit=false;
    Vec3 point={0,0,0},lastFrom,lastTo;
    float fraction=0;
    mutable unsigned calls=0;
    bool intersectRayInWorldCoords(const Vec3& from,const Vec3& to,Vec3& out,
        float& f,Vec3& normal,unsigned char& material,bool flag) const override {
        ++calls;trace.push_back("terrain");
        check(flag&&f==0&&material==0,"terrain input flags and outputs initially zero");
        const_cast<TargetTerrain*>(this)->lastFrom=from;
        const_cast<TargetTerrain*>(this)->lastTo=to;
        if(hit) {out=point;f=fraction;normal={0,1,0};material=7;}
        return hit;
    }
};
struct TargetClasses : ClassManager {
    TargetTerrain* terrain=nullptr;
    IBase* getSingleton(const std::string& name) override {
        check(name=="HeightmapCluster","target looks up named terrain singleton");
        trace.push_back("heightmap");return terrain;
    }
    IBase* createInstance(unsigned cid,unsigned iid,IBase* outer) override {
        if(cid==world::CID_BinTreeCuller) return world::BinTreeCuller::create(iid,outer);
        if(cid==world::CID_QuadTreeCuller) return world::QuadTreeCuller::create(iid,outer);
        actionUnsupported("target class factory");
    }
};
struct TargetWorld : world::ObjectManager {
    world::IObject* rayObject=nullptr;
    Vec3 rayPoint={0,0,0},lastDirection;
    float cullFar=0;
    unsigned rays=0,culls=0;
    std::vector<world::IObject*> candidates;
    TargetObject* excluded=nullptr;
    TargetObject* predCandidate=nullptr;
    bool intersectLine(world::IObject*& object,Vec3& hit,Vec3&,int&,const Vec3&,
        const Vec3& direction,world::ObjectPredicator* pred,bool a,bool b,bool c,bool d,unsigned e) override {
        ++rays;trace.push_back("ray");lastDirection=direction;
        check(a&&!b&&!c&&d&&e==0,"target object ray flags");
        check(pred->getFlags()==0x100 && pred->performCulling(nullptr) && pred->predicateSurface(7),"complete typed ray predicate suffix");
        if(excluded)check(!pred->predicate(excluded),"ray predicate excludes local root");
        if(predCandidate)check(pred->predicate(predCandidate),"ray predicate accepts another flagged root");
        if(rayObject){object=rayObject;hit=rayPoint;}
        return false; // target deliberately ignores the boolean; it checks object.
    }
    void getObjectsFromGridNoCullConst(const Frustum& f,std::vector<world::IObject*>& out,
        world::ObjectPredicator* pred,bool b,float x,float y) override {
        ++culls;trace.push_back("cull");cullFar=f.m_farDist;
        check(!b&&x==0&&y==0&&out.empty(),"cull arguments and native empty vector");
        check(pred->getFlags()==0x100 && pred->performCulling(nullptr) && pred->predicateSurface(0),"complete typed PCO predicate suffix");
        if(excluded)check(!pred->predicate(excluded),"cull excludes local root");
        if(predCandidate) {
            predCandidate->pco=false;check(!pred->predicate(predCandidate),"cull requires PCO");
            predCandidate->pco=true;check(pred->predicate(predCandidate),"cull accepts root PCO");
        }
        out=candidates;
    }
};
struct TargetInfo : HudInformationLayer {
    std::deque<bool> replies;
    std::vector<float> ranges;
    bool isWithinRadius(Vec3,Vec3,float range) override {
        trace.push_back("radius");ranges.push_back(range);
        if(replies.empty())actionUnsupported("target radius reply");
        bool result=replies.front();replies.pop_front();return result;
    }
};
struct TargetHud : Hud {
    std::deque<bool> aims;
    unsigned aimCalls=0;
    bool isWithinAimThreshold(world::IObject*,float threshold) override {
        check(threshold==0.3f,"target uses authored aim threshold");
        trace.push_back("aim");++aimCalls;
        if(aims.empty())actionUnsupported("target aim reply");
        bool result=aims.front();aims.pop_front();return result;
    }
};
int main() {
    setvbuf(stdout,nullptr,_IONBF,0);
    std::puts("[TEMP] constructing native target fixture dependencies");
    // Heightmap/HUD listeners are retained with their owners; retaining the
    // actual EventManager keeps this bounded test outside their shutdown path.
    EventManager& events=*new EventManager;eventManager=&events;
    auto* own=new TargetObject("own");
    auto* target=new TargetObject("target");
    auto* alternate=new TargetObject("alternate");
    auto* child=new TargetObject("child");child->parent(target);
    TargetNet net;
    TargetPlayer player;player.vehicle=own;player.camera.transformation.m[12]=10;
    Players players;world::playerManager=&players;players.local=&player;
    auto* terrain=new TargetTerrain;
    TargetClasses classes;classes.terrain=terrain;classManager=&classes;
    auto* objects=new TargetWorld;world::objectManager=objects;objects->excluded=own;objects->predCandidate=target;
    auto* info=new TargetInfo;g_hudInformationLayer=info;
    auto* hud=new TargetHud;hudManager=hud;hud->m_field1f4=0.3f;
    menu::Ref2System root(800,600);hud->m_field0f4=&root;
    TargetView view;Mat4 identity=Mat4::getIdentity();view.frustum.setPerspective(1,0.75f,1,100,identity);
    root.m_previousRenderView=nullptr;
    world::IObject* result=nullptr;Vec3 position;
    std::puts("[TEMP] running recovered target selection");
    *Vars::getFloat("coll-name-dist",100)=20;
    trace.clear();
    check(!hud->findSpottedTarget(result,position,100,false)&&!result&&same(position,{-4000,-4000,-4000}),"no hits preserves sentinel");
    check(same(terrain->lastFrom,{10,0,0})&&same(terrain->lastTo,{10,0,20})&&same(objects->lastDirection,{0,0,100}),"Vars range applies to terrain endpoint, object fallback uses requested distance");
    check(trace==std::vector<std::string>{"player","vehicle","own transform","camera","transform","heightmap","terrain","ray"},"target setup and ray callback order");
    terrain->hit=true;terrain->point={10,0,20};terrain->fraction=0.25f;
    check(hud->findSpottedTarget(result,position,100,false)&&!result,"terrain-only hit succeeds independently of object");
    check(same(objects->lastDirection,{0,0,26}),"terrain hit extends object test by shipped 1.3 multiplier");
    check(hud->m_field264==10&&hud->m_field268==0&&hud->m_field26c==20,"cache stores raw point before bias");
    check(position.x<10&&position.x>9.9f&&position.z<20&&position.z>19.8f,"bias points toward world origin, not camera origin");
    terrain->fraction=std::numeric_limits<float>::quiet_NaN();
    hud->findSpottedTarget(result,position,100,false);
    check(objects->lastDirection.z==26,"unordered terrain fraction follows accepted arm");
    terrain->fraction=-101;
    check(!hud->findSpottedTarget(result,position,100,false)&&position.x==-4000,"over-range terrain fraction falls back to maximum and rejects final terrain hit");
    terrain->hit=false;
    objects->rayObject=child;objects->rayPoint={10,0,8};
    check(hud->findSpottedTarget(result,position,100,false)&&result==child,"ray object output wins despite ray service false return");
    target->network=&net;target->position={30,40,50};trace.clear();
    check(hud->findSpottedTarget(result,position,100,true)&&result==target&&same(position,{30,40,50}),"networkable root overrides hit position on request");
    check(trace.back()=="target position","root position query is last");
    trace.clear();hud->findSpottedTarget(result,position,100,false);
    check(trace.back()=="target net","networkability queried before false vehicle-position flag");
    target->network=nullptr;

    root.m_previousRenderView=&view;
    objects->rayObject=nullptr;objects->candidates={nullptr,own,target,alternate};
    target->position={10,0,20};alternate->position={10,0,5};
    info->replies={false,true};hud->aims={true};
    unsigned before=player.vehicleReads;
    check(!hud->findSpottedTarget(result,position,100,false)&&result==alternate&&position.x==-4000,"cull selects object while no ray point preserves false/sentinel");
    check(player.vehicleReads==before+4,"vehicle queried once for camera and once per nonnull candidate");
    check(info->ranges.size()==2&&info->ranges[0]==100&&info->ranges[1]==100,"failed radius candidate does not narrow range");
    check(objects->cullFar==100&&view.frustum.m_farDist==100,"native cull frustum is a copy");
    info->ranges.clear();info->replies={true,true};hud->aims={true,true};
    objects->candidates={target,alternate};
    hud->findSpottedTarget(result,position,100,false);
    check(result==alternate&&info->ranges[0]==100&&info->ranges[1]==20,"each accepted candidate narrows next radius and last accepted wins");
    check(hud->aims.empty()&&info->replies.empty(),"candidate callback sequence consumed exactly");
    alternate->network=&net;
    info->replies={true,true};hud->aims={true,true};
    check(hud->findSpottedTarget(result,position,100,true)&&same(position,alternate->position),"networkable cull target becomes successful final position");
    objects->candidates.clear();objects->rayObject=target;objects->rayPoint={10,0,8};
    hud->findSpottedTarget(result,position,100,false);
    check(objects->cullFar==8&&view.frustum.m_farDist==100,"object ray distance narrows copied Frustum only");
    world::playerManager=nullptr;root.m_previousRenderView=nullptr;objects->excluded=nullptr;objects->predCandidate=nullptr;objects->rayObject=nullptr;
    check(!hud->findSpottedTarget(result,position,100,false)&&same(terrain->lastFrom,{0,0,0}),"missing player manager still runs zero-origin terrain/object queries");
    std::printf("HUD spotted target: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
