// [TEMP] Exact extracted radius wrappers, real grid selector and XZ projection.
// Chosen environment/interface lifetimes omit registry and grid ownership work.
// Typed grid observers test forwarding, not the grid's spatial/filtering body.
#include "radius_wrappers_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <windows.h>
#include <crtdbg.h>

using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "MSVC x86 fixture only");
using CircleMethod = void (IAIInformationGrid::*)(std::vector<unsigned>&,const Vec2&,float,IAIInformationGridPredicate*) const;
static_assert(std::is_same<decltype(&IAIInformationGrid::getInformationWithinCircle),CircleMethod>::value,"real const grid query");
[[noreturn]] static void unexpected(const char* message)
{ std::fprintf(stderr,"UNEXPECTED %s\n",message); std::fflush(stderr); std::abort(); }
static void require(bool condition,const char* message) { if(!condition) unexpected(message); }
static unsigned bits(float value) { unsigned result; std::memcpy(&result,&value,4); return result; }
static float fromBits(unsigned value) { float result; std::memcpy(&result,&value,4); return result; }
static std::string mode;
static IAIInformationGrid* initialGrids[6];
static IAIInformationGrid* replacementGrid;
static std::vector<IAIInformationGrid*> *worldA,*worldB;
static Vec2* originalPosition;
static Vec3* original3;
struct Call { const IAIInformationGrid* receiver; int identity; std::vector<unsigned>* output; const Vec2* address; Vec2 position; unsigned radius; IAIInformationGridPredicate* predicate; };
static std::vector<Call> calls;
static int gridCounts[7];
static void observeGrid(const IAIInformationGrid* receiver,int identity,std::vector<unsigned>& out,
                        const Vec2& position,float radius,IAIInformationGridPredicate* keep)
{
    require(identity>=0 && identity<7,"known typed grid receiver");
    calls.push_back(Call{receiver,identity,&out,&position,position,bits(radius),keep});
    out.push_back(static_cast<unsigned>(identity*10+gridCounts[identity]++));
    if(calls.size()==1) {
        if(mode=="reload_grid") {
            const auto* previous=worldA->data();
            worldA->reserve(worldA->capacity()+17);
            require(worldA->data()!=previous,"genuine native vector relocation");
            (*worldA)[2]=replacementGrid;
        }
        if(mode=="borrowed_position") *originalPosition=Vec2{7.0f,8.0f};
        if(mode=="vec3_copy") *original3=Vec3{90.0f,91.0f,92.0f};
    }
}

#include "radius_wrappers_mocks.inl"

namespace dice { namespace hfe { namespace ai {
// [CHOSEN, not recovered] Real container lifetimes, no singleton/grid ownership.
IAIInformationEnvironment::IAIInformationEnvironment() {}
IAIInformationEnvironment::~IAIInformationEnvironment() {}
IAIInformationEnvironmentModifiers::IAIInformationEnvironmentModifiers() {}
IAIInformationEnvironmentModifiers::~IAIInformationEnvironmentModifiers() {}
IAIInformationGrid::~IAIInformationGrid() {}
BFInformationEnvironment::BFInformationEnvironment()
{
    m_gridsA.assign(initialGrids,initialGrids+3);
    m_gridsB.assign(initialGrids+3,initialGrids+6);
    worldA=&m_gridsA; worldB=&m_gridsB;
}
BFInformationEnvironment::~BFInformationEnvironment() {}
} } }

class Predicate : public IAIInformationGridPredicate
{ public: bool includeInformation(Information*) const override { unexpected("wrapper called predicate directly"); } };
class DelegatingEnvironment : public BFInformationEnvironment
{
public:
    using BFInformationEnvironment::getObjectsWithinRadius;
    mutable int count=0,seenSide=0;
    mutable bool seenWorld=false;
    mutable unsigned seenRadius=0;
    mutable Vec2 seenPosition={};
    mutable IAIInformationGridPredicate* seenPredicate=nullptr;
    mutable std::vector<unsigned>* seenOutput=nullptr;
    void getObjectsWithinRadius(std::vector<unsigned>& out,const Vec2& position,float radius,
                                int side,bool which,IAIInformationGridPredicate* keep) const override
    {
        ++count; seenSide=side; seenWorld=which; seenRadius=bits(radius);
        seenPosition=position; seenPredicate=keep; seenOutput=&out; out.push_back(88);
    }
};

int main(int argc,char** argv)
{
    require(argc==2,"one mode"); mode=argv[1];
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    ObservedGrid a0(0),a1(1),a2(2),b0(3),b1(4),b2(5),replacement(6);
    IAIInformationGrid* setup[]={&a0,&a1,&a2,&b0,&b1,&b2};
    for(int i=0;i<6;++i) initialGrids[i]=setup[i];
    replacementGrid=&replacement;
    Predicate predicate;
    std::vector<unsigned> output{777};
    if(mode=="vec3_delegate" || mode=="vec3_nonfinite") {
        DelegatingEnvironment environment;
        const IAIInformationEnvironment& view=environment;
        const bool nonfinite=mode=="vec3_nonfinite";
        const Vec3 point=nonfinite ? Vec3{fromBits(0x80000000),fromBits(0x7fc12345),fromBits(0x7f800000)} : Vec3{3,999,4};
        const float radius=nonfinite ? fromBits(0x7fc45678) : -3.25f;
        IAIInformationGridPredicate* keep=nonfinite ? nullptr : &predicate;
        view.getObjectsWithinRadius(output,point,radius,-7,!nonfinite,keep);
        require(environment.count==1 && calls.empty(),"Vec3 calls virtual Vec2 exactly once");
        require(environment.seenSide==-7 && environment.seenWorld==!nonfinite,"side/world preserved without guards");
        require(environment.seenOutput==&output && environment.seenPredicate==keep,"output and predicate identity");
        require(environment.seenRadius==bits(radius),"radius bits preserved");
        require(bits(environment.seenPosition.x)==bits(point.x) && bits(environment.seenPosition.y)==bits(point.z),"XZ bit-pattern projection ignores height");
        require(output==std::vector<unsigned>({777,88}),"delegate append preserved");
    } else {
        require(mode=="world_a" || mode=="world_b" || mode=="side_zero" || mode=="reload_grid" || mode=="borrowed_position" || mode=="vec3_copy" || mode=="null_predicate","known mode");
        BFInformationEnvironment environment;
        const IAIInformationEnvironment& view=environment;
        Vec2 position={1,2}; originalPosition=&position;
        Vec3 position3={1,fromBits(0x7fc12345),2}; original3=&position3;
        const bool which=mode=="world_b" || mode=="side_zero" || mode=="vec3_copy";
        const int side=mode=="side_zero" ? 0 : 2;
        const float radius=mode=="null_predicate" ? fromBits(0x7fc23456) : -4.25f;
        IAIInformationGridPredicate* keep=mode=="null_predicate" ? nullptr : &predicate;
        if(mode=="vec3_copy") view.getObjectsWithinRadius(output,position3,radius,side,which,keep);
        else view.getObjectsWithinRadius(output,position,radius,side,which,keep);
        const int first=which?3:0,second=mode=="reload_grid"?6:first+side;
        require(calls.size()==2,"exactly two const grid queries");
        require(calls[0].identity==first && calls[1].identity==second,"neutral then requested grid, fresh lookup");
        require(calls[0].receiver==setup[first] && calls[1].receiver==(second==6?replacementGrid:setup[second]),"real typed grid identities");
        for(const Call& call:calls) {
            require(call.output==&output && call.radius==bits(radius) && call.predicate==keep,"same output/radius/predicate arguments");
        }
        require(calls[0].address==calls[1].address,"same borrowed Vec2 between calls");
        if(mode!="vec3_copy") require(calls[0].address==&position,"Vec2 source is borrowed");
        require(calls[0].position.x==1 && calls[0].position.y==2,"first position");
        require(calls[1].position.x==(mode=="borrowed_position"?7:1) && calls[1].position.y==(mode=="borrowed_position"?8:2),"later borrowed mutation or stable Vec3 local copy");
        const unsigned last=static_cast<unsigned>(second*10+(side==0?1:0));
        require(output==std::vector<unsigned>({777,static_cast<unsigned>(first*10),last}),"append order and side-zero duplicate");
        if(mode=="vec3_copy") require(position3.x==90 && position3.z==92,"original Vec3 changed between callbacks");
    }
    std::printf("PASS %s\n",mode.c_str()); return 0;
}
