// [TEMP] Native map storage inspection and real772230/7720E0 coordinate helpers.
// No map constructor, descriptor, event face or unresolved method is called.
#include <windows.h>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>
#include "../BF2/Menu/Bf2MenuMapNode.h"
#include "../BF2/Core/SettingsRepostitory.h"
namespace dice { namespace hfe {
SettingsRepostitory* settings=nullptr;
class IGameLogic; IGameLogic* gameLogic=nullptr;
class ClassManager; ClassManager* classManager=nullptr;
class EventManager; EventManager* eventManager=nullptr;
} namespace meme {
struct Bf2MenuMapNodeFixture {
    static void layout() {
        static_assert(sizeof(void*)==4,"x86 required");
        static_assert(sizeof(TransformNode)==36,"native TransformNode scalar storage");
        static_assert(sizeof(Bf2TransformNode)==60,"native Bf2TransformNode scalar storage");
        static_assert(sizeof(Bf2MenuMapNode)==3000,"current release native STL layout");
        static_assert(sizeof(MapVehicleDrop)==24,"recovered drop record");
        static_assert(!std::is_default_constructible<Bf2MenuMapNode>::value,"no incomplete map construction");
        static_assert(offsetof(Bf2MenuMapNode,m_system)==684,"43 native vector objects");
        static_assert(offsetof(Bf2MenuMapNode,m_zoomLevel)==1576,"native bitset alignment");
        static_assert(offsetof(Bf2MenuMapNode,m_commanderPosition)==1848,"native commander coordinates");
        static_assert(offsetof(Bf2MenuMapNode,m_spawnGroup)==2300,"native signed spawn group");
        static_assert(offsetof(Bf2MenuMapNode,m_uavReloading)==2600,"native reload flag");
        static_assert(offsetof(Bf2MenuMapNode,m_selectedRequestType)==offsetof(Bf2MenuMapNode,m_selectedRequestSquad)+4,"adjacent request fields");
        static_assert(offsetof(Bf2MenuMapNode,m_selectedRequestOrderType)==offsetof(Bf2MenuMapNode,m_selectedRequestSquad)+8,"adjacent request order type");
        static_assert(std::is_same<decltype(Bf2MenuMapNode::m_icons),std::vector<MapIconRecord>[43]>::value,"native vectors, no retail overlay");
    }
};
} }
using namespace dice::hfe;
static unsigned checks,failures;
static void check(bool condition,const char* text) { ++checks; if(!condition){++failures;std::fprintf(stderr,"FAIL %s\n",text);} }
struct SettingsObserver : SettingsRepostitory {
    std::function<void(const std::string&,const int&)> callback;
    std::vector<std::string> keys;
    const int& intGet(const std::string& key,const int& fallback) override {
        keys.push_back(key); check(fallback==0,"each axis has zero fallback");
        if(callback)callback(key,fallback);
        return SettingsRepostitory::intGet(key,fallback);
    }
};
static unsigned long missingSettingsFault(Vec3* out,Vec2* pos) {
    __try { dice::meme::mapCoordinatesToWorld(*out,*pos); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
    return 0;
}
static unsigned long missingInverseSettingsFault(Vec3* pos,Vec2* out) {
    __try { dice::meme::worldCoordinatesToMap(*pos,*out); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
    return 0;
}
int main() {
    dice::meme::Bf2MenuMapNodeFixture::layout();
    SettingsObserver first,second; settings=&first;
    Vec2 point{0.25f,-0.75f}; Vec3 output{91,92,93};
    first.intSet("GLSWorldSizeX",1024);first.intSet("GLSWorldSizeZ",2048);
    dice::meme::mapCoordinatesToWorld(output,point);
    check(output.x==256&&output.y==0&&output.z==1536,"independent world sizes and inverted Z");
    check(first.keys==std::vector<std::string>({"GLSWorldSizeX","GLSWorldSizeZ"}),"X read before Z");
    first.intErase("GLSWorldSizeX");first.intErase("GLSWorldSizeZ");point={2,3};
    dice::meme::mapCoordinatesToWorld(output,point);
    check(output.x==0&&!std::signbit(output.x)&&output.y==0&&!std::signbit(output.y),"missing dimensions use positive zero");
    check(output.z==0&&std::signbit(output.z),"missing positive Z dimension preserves negative zero");
    first.intSet("GLSWorldSizeX",16777217);first.intSet("GLSWorldSizeZ",-9);point={1.5f,-2};
    dice::meme::mapCoordinatesToWorld(output,point);
    check(output.x==25165826.0f,"signed integer precision retained through x87-style product");
    check(output.z==-18,"signed negative world extent");
    first.intSet("GLSWorldSizeX",7);second.intSet("GLSWorldSizeZ",11);
    point={2,3};output={91,92,93};first.keys.clear();second.keys.clear();
    first.callback=[&](const std::string& key,const int&) {
        check(key=="GLSWorldSizeX"&&output.x==91&&output.y==92&&output.z==93,"first callback precedes all output stores");
        point.x=5;settings=&second;
    };
    second.callback=[&](const std::string& key,const int&) {
        check(key=="GLSWorldSizeZ"&&output.x==35&&output.y==0&&output.z==93,"second callback sees completed X/Y stores");point.y=-4;
    };
    dice::meme::mapCoordinatesToWorld(output,point);
    check(output.x==35&&output.z==44&&first.keys.size()==1&&second.keys.size()==1,"global and coordinate values reread after callbacks");
    first.callback=nullptr;second.callback=nullptr;settings=&first;
    point={std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()};
    dice::meme::mapCoordinatesToWorld(output,point);
    check(std::isnan(output.x)&&std::isinf(output.z)&&!std::signbit(output.z),"NaN and infinity propagate with retail signs");
    settings=nullptr;
    check(missingSettingsFault(&output,&point)==EXCEPTION_ACCESS_VIOLATION,"no invented singleton null guard");
    settings=&first;first.keys.clear();
    first.intSet("GLSWorldSizeX",1024);first.intSet("GLSWorldSizeZ",2048);
    Vec3 world{256,std::numeric_limits<float>::quiet_NaN(),-1536};Vec2 normalized{91,92};
    dice::meme::worldCoordinatesToMap(world,normalized);
    check(normalized.x==0.25f&&normalized.y==0.75f,"inverse uses X/Z and ignores height");
    check(first.keys==std::vector<std::string>({"GLSWorldSizeX","GLSWorldSizeZ"}),"inverse axis lookup order");
    dice::meme::mapCoordinatesToWorld(output,normalized);
    check(output.x==world.x&&output.y==0&&output.z==world.z,"forward/inverse roundtrip for finite extents");
    first.intSet("GLSWorldSizeX",16777217);first.intSet("GLSWorldSizeZ",16777217);
    world={16777216,3,16777216};
    dice::meme::worldCoordinatesToMap(world,normalized);
    check(normalized.x==0.9999999403953552f&&normalized.y==-0.9999999403953552f,"inverse retains integer divisor precision");
    first.intErase("GLSWorldSizeX");first.intErase("GLSWorldSizeZ");world={5,7,-8};
    dice::meme::worldCoordinatesToMap(world,normalized);
    check(std::isinf(normalized.x)&&!std::signbit(normalized.x)&&std::isinf(normalized.y)&&!std::signbit(normalized.y),"absent extents divide by zero without a guard");
    world={0,4,0};dice::meme::worldCoordinatesToMap(world,normalized);
    check(std::isnan(normalized.x)&&std::isnan(normalized.y),"zero over absent extent propagates NaN");
    first.intSet("GLSWorldSizeX",7);second.intSet("GLSWorldSizeZ",11);
    world={91,92,93};normalized={94,95};first.keys.clear();second.keys.clear();
    first.callback=[&](const std::string& key,const int&) {
        check(key=="GLSWorldSizeX"&&normalized.x==94&&normalized.y==95,"inverse first lookup precedes output stores");
        world.x=35;settings=&second;
    };
    second.callback=[&](const std::string& key,const int&) {
        check(key=="GLSWorldSizeZ"&&normalized.x==5&&normalized.y==95,"inverse second lookup sees only X store");world.z=-44;
    };
    dice::meme::worldCoordinatesToMap(world,normalized);
    check(normalized.x==5&&normalized.y==4&&first.keys.size()==1&&second.keys.size()==1,"inverse reloads singleton and reads coordinates after callbacks");
    first.callback=nullptr;second.callback=nullptr;settings=&first;
    first.intSet("GLSWorldSizeZ",7);world={-0.0f,3,0};
    dice::meme::worldCoordinatesToMap(world,normalized);
    check(normalized.x==0&&std::signbit(normalized.x)&&normalized.y==0&&std::signbit(normalized.y),"inverse preserves signed zero");
    settings=nullptr;
    check(missingInverseSettingsFault(&world,&normalized)==EXCEPTION_ACCESS_VIOLATION,"inverse retains singleton null fault");
    std::printf("HUD map coordinates: %u/%u checks passed; native map size%u; no map construction\n",checks-failures,checks,unsigned(sizeof(dice::meme::Bf2MenuMapNode)));
    return failures?1:0;
}
