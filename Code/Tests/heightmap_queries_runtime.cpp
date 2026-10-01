// [TEMP] [CHOSEN, not recovered] component fixture. Real Heightmap buffers, query bodies and
// MaterialManager lookup/fallback; observers record virtual calls. No listener/map/client claim.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "../BF2/Core/Hash.h"
#include "../BF2/Core/EventManager.h"
#define private public
#include "../BF2/Physics/Heightmap.h"
#include "../BF2/Physics/HeightmapCluster.h"
#include "../BF2/Physics/MaterialManager.h"
#undef private
#include "../BF2/Physics/Material.h"

using namespace dice::hfe;
using namespace dice::hfe::world;
namespace dice { namespace hfe { namespace world { extern IBase* materialManager; } } }
namespace {
int checks = 0, failures = 0;
LONG WINAPI reportFault(EXCEPTION_POINTERS* fault)
{
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, 0, TRUE);
    std::fprintf(stderr, "Fixture exception %08lx at %p\n", fault->ExceptionRecord->ExceptionCode,
                 fault->ExceptionRecord->ExceptionAddress);
    CONTEXT context = *fault->ContextRecord;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = context.Eip; frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;
    for (int i=0; i<16; ++i)
    {
        char storage[sizeof(SYMBOL_INFO)+MAX_SYM_NAME] = {};
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO); symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        if (SymFromAddr(process,frame.AddrPC.Offset,&displacement,symbol))
            std::fprintf(stderr,"  %08llx %s+%llu\n",frame.AddrPC.Offset,symbol->Name,displacement);
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386,process,GetCurrentThread(),&frame,&context,
                        0,SymFunctionTableAccess64,SymGetModuleBase64,0)) break;
    }
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
void check(bool ok, const char* text)
{
    ++checks;
    if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", text);
    std::fflush(stdout);
}
bool closeFloat(float a, float b) { return std::fabs(a-b) < 0.00002f; }
bool vectorIs(const Vec3& v, float x, float y, float z)
{
    return closeFloat(v.x,x) && closeFloat(v.y,y) && closeFloat(v.z,z);
}
using Cell = std::pair<int,int>;
struct ObservedHeightmap : Heightmap
{
    mutable std::vector<Cell> reads;
    mutable std::vector<Cell> materialReads;
    int getHeightFromGrid(int x, int z) const override
    {
        reads.emplace_back(x,z);
        return Heightmap::getHeightFromGrid(x,z);
    }
    unsigned char getMaterialFromGrid(int x, int z) const override
    {
        materialReads.emplace_back(x,z);
        return Heightmap::getMaterialFromGrid(x,z);
    }
};
struct ObservedMaterials : MaterialManager
{
    std::vector<unsigned> reads;
    Material* getMaterialPtr(unsigned id) override
    {
        reads.push_back(id);
        return MaterialManager::getMaterialPtr(id);
    }
};
// Only the signed grid sample is supplied here, to distinguish unsigned min/max comparisons.
struct SignedSamples : Heightmap
{
    int getHeightFromGrid(int x, int) const override { return x == 0 ? -1 : 4; }
};
// Only the tile selection is supplied here; the real cluster wrapper and real tile query execute.
struct SelectedTileCluster : HeightmapCluster
{
    IHeightmap* selected = 0;
    IHeightmap* getHeightmapInWorldCoords(float, float) const override { return selected; }
};
void fill(Heightmap& h, int sx=8, int sz=8)
{
    h.setSize(sx,sz);
    h.setBitResolution(16);
    h.setScale(Vec3{2.0f,0.5f,4.0f});
    h.setPos(100,-200);
    h.setMaterialScale(2);
    h.m_heightData = new unsigned char[sx*sz*2];
    h.m_materialData = new unsigned char[sx*sz*4];
    std::memset(h.m_materialData,7,sx*sz*4);
    unsigned short* samples = static_cast<unsigned short*>(h.m_heightData);
    for (int z=-sz/2; z<sz-sz/2; ++z)
        for (int x=-sx/2; x<sx-sx/2; ++x)
            samples[(z+sz/2)*sx+x+sx/2] = static_cast<unsigned short>(30+2*x+3*z+x*z);
}
bool cellsAre(const std::vector<Cell>& actual, std::initializer_list<Cell> expected)
{
    return actual == std::vector<Cell>(expected);
}
}
int main()
{
    static_assert(sizeof(void*)==4,"x86 fixture required");
    static_assert(std::is_same<decltype(std::declval<IHeightmap&>().replaceNonMappedMaterials(0)),void>::value,
                  "retail replacement is void");
    static_assert(std::is_same<decltype(std::declval<IHeightmap&>().getSquareInWorldCoords(
                  0,0,nullptr,std::declval<bool&>())),void>::value,"retail square query is void");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(reportFault);
    ObservedMaterials materials;
    Material dirt, stone;
    materials.m_materials[7] = &dirt;
    materials.m_materials[9] = &stone;
    IBase* previousMaterials = materialManager;
    materialManager = &materials;
    EventManager events;
    EventManager* previousEvents = eventManager;
    eventManager = &events;
    {
        Heightmap empty;
        check(empty.getUsingValidMaterials(),"empty dimensions validate without a material buffer");
        empty.replaceNonMappedMaterials(9);
        empty.setMaterialInGrid(INT_MIN,INT_MAX,99);
        check(!empty.m_materialData,"null material setter guard precedes index arithmetic");
    }
    {
        ObservedHeightmap h;
        fill(h);
        IHeightmap* face = &h;
        face->setMaterialInGrid(-1,0,9);
        check(h.m_materialData[134]==9 && h.m_materialData[135]==7,
              "materialScale setter updates one byte at the scaled stride");
        check(face->getMaterialInWorldCoords(99.5f,-200.5f)==7 &&
              cellsAre(h.materialReads,{{0,0}}),"world material coordinates truncate negative fractions toward zero");
        check(face->getMaterialInWorldCoords(98,-200)==9,"world material negative integral coordinate");
        check(face->getMaterialInWorldCoords(1000,-200)==0,"material getter retains its existing outside-grid result");

        Vec3 n = {}, v0 = {}, v1 = {}, v2 = {};
        unsigned char material = 0;
        h.reads.clear();
        face->getNormalInLocalCoords(.5f,1.0f,n);
        check(vectorIs(n,-.371390676f,.742781353f,-.557086015f),"lower face normal uses X scale for both slopes");
        check(cellsAre(h.reads,{{1,0},{0,1},{0,0}}),"face normal reads off-diagonal samples before lower corner");
        h.reads.clear();
        face->getNormalInLocalCoords(1,2,n);
        check(vectorIs(n,-.468521286f,.624695048f,-.624695048f),"face diagonal equality selects upper triangle");
        check(cellsAre(h.reads,{{1,0},{0,1},{1,1}}),"upper normal samples the upper corner last");
        face->getNormalInWorldCoords(100.5f,-199,n);
        check(vectorIs(n,-.371390676f,.742781353f,-.557086015f),"scalar world normal subtracts position without scaling it");
        face->getNormalInLocalCoords(Vec3{.5f,900,1},n);
        check(vectorIs(n,-.371390676f,.742781353f,-.557086015f),"local vector normal ignores Y");
        face->getNormalInWorldCoords(Vec3{100.5f,900,-199},n);
        check(vectorIs(n,-.371390676f,.742781353f,-.557086015f),"world vector normal reaches local scalar body");
        check(closeFloat(face->getWorldScaledHeightInWorldCoords(100.5f,-199),15.625f),"world-scaled scalar height");
        check(closeFloat(face->getWorldScaledHeightInWorldCoords(Vec3{100.5f,-900,-199}),15.625f),"world-scaled vector height ignores Y");

        h.reads.clear(); h.materialReads.clear();
        face->getTriangleAndNormalAndMaterialInLocalCoords(.5f,1,n,v0,v1,v2,material);
        check(vectorIs(v0,0,15,0) && vectorIs(v1,2,16,0) && vectorIs(v2,0,16.5f,4),"lower triangle vertex order 00,10,01");
        check(vectorIs(n,-.423999152f,.847998304f,-.317999364f),"triangle normal is first output and uses geometric Z scale");
        check(material==7 && cellsAre(h.materialReads,{{0,0}}),"triangle material comes from the containing cell");
        check(cellsAre(h.reads,{{0,0},{1,0},{0,1}}),"lower triangle virtual sample ordering");
        h.reads.clear();
        face->getTriangleAndNormalAndMaterialInLocalCoords(1,2,n,v0,v1,v2,material);
        check(vectorIs(v0,0,16.5f,4) && vectorIs(v1,2,16,0) && vectorIs(v2,2,18,4),"triangle diagonal equality selects 01,10,11");
        check(vectorIs(n,-.557086015f,.742781353f,-.371390676f),"upper geometric triangle normal");
        check(cellsAre(h.reads,{{0,1},{1,0},{1,1}}),"upper triangle virtual sample ordering");
        face->getTriangleAndNormalAndMaterialInWorldCoords(101,-198,n,v0,v1,v2,material);
        check(vectorIs(v0,100,16.5f,-196) && vectorIs(v1,102,16,-200) && vectorIs(v2,102,18,-196),"world triangle translates only vertices");
        check(vectorIs(n,-.557086015f,.742781353f,-.371390676f),"world triangle does not translate normal");

        Vec3 square[4] = {}; bool flipped = false;
        h.reads.clear();
        h.getSquareInLocalCoords(.5f,1,square,flipped);
        check(flipped && vectorIs(square[0],0,15,0) && vectorIs(square[3],2,18,4),"local square contains both triangle corners");
        check(cellsAre(h.reads,{{0,0},{1,0},{0,1},{1,1}}),"square reads every corner in 00,10,01,11 order");
        face->getSquareInWorldCoords(101,-198,square,flipped);
        check(!flipped,"square diagonal equality clears flipped");
        check(vectorIs(square[0],100,15,-200) && vectorIs(square[1],102,16,-200) &&
              vectorIs(square[2],100,16.5f,-196) && vectorIs(square[3],102,18,-196),"square world translation covers all four vertices");

        h.reads.clear();
        h.getPhongInterpolation(.5f,2,n);
        check(vectorIs(n,-.434531898f,.700593229f,-.565995722f),"nonlinear Phong golden: normalize X rows before Z blend");
        check(cellsAre(h.reads,{{0,0},{1,0},{0,1},{1,0},{2,0},{1,1},
                               {0,1},{1,1},{0,2},{1,1},{2,1},{1,2}}),"Phong preserves all twelve ordered virtual samples");
        h.reads.clear();
        h.getPhongInterpolation(0,0,n);
        check(h.reads.size()==12 && vectorIs(n,-.371390676f,.742781353f,-.557086015f),"Phong still reads all corners at an exact vertex");
        face->setUsePhongNormalInterpolation(true);
        const float height = face->getHeightAndNormalInLocalCoords(.5f,2,n,material);
        check(closeFloat(height,16.0f) && material==7 && vectorIs(n,-.434531898f,.700593229f,-.565995722f),"existing height/normal query executes the recovered Phong arm");
        face->getNormalInLocalCoords(.5f,1,n);
        check(vectorIs(n,-.371390676f,.742781353f,-.557086015f),"standalone normal ignores enabled Phong flag");

        float low=0, high=0;
        h.reads.clear();
        face->getMinMaxHeightInLocalCoords(0,0,2,4,low,high);
        check(closeFloat(low,15) && closeFloat(high,15) && cellsAre(h.reads,{{0,0}}),"min/max excludes integer upper bounds");
        h.reads.clear();
        face->getMinMaxHeightInWorldCoords(99.5f,-201,102.5f,-195,low,high);
        check(h.reads.size()==9 && closeFloat(low,13) && closeFloat(high,18),"world min/max floors starts and ceils ends");
        h.reads.clear();
        face->getMinMaxHeightInLocalCoords(0,0,0,0,low,high);
        check(h.reads.empty() && low==2147483648.0f && high==0,"empty rectangle preserves unsigned sentinel output");
        face->getMinMaxHeightInLocalCoords(2,4,0,0,low,high);
        check(low==2147483648.0f && high==0,"reversed rectangle is not reordered");

        // The first sizeX*sizeZ bytes are validated, not all materialScale^2 times as many.
        std::memset(h.m_materialData,7,256);
        h.m_materialData[80]=255;
        materials.reads.clear();
        check(face->getUsingValidMaterials() && materials.reads.size()==64,"material validation ignores scaled-grid tail");
        h.m_materialData[2]=255;
        materials.reads.clear();
        check(!face->getUsingValidMaterials() && materials.reads==std::vector<unsigned>({7,7,255,0}),"validation stops at first missing id after retail default lookup");
        h.m_materialData[3]=255; h.m_materialData[4]=9;
        materials.reads.clear();
        face->replaceNonMappedMaterials(0x109);
        check(h.m_materialData[2]==9 && h.m_materialData[3]==9 && h.m_materialData[4]==9 && h.m_materialData[80]==255,
              "replacement truncates argument and leaves materialScale tail unchanged");
        check(materials.reads==std::vector<unsigned>({7,255,0,9}),"replacement caches valid and missing material ids once each");
        materials.m_materials[0]=&dirt;
        h.m_materialData[1]=254;
        check(face->getUsingValidMaterials(),"registered material zero makes unknown ids valid through fallback");
        face->replaceNonMappedMaterials(9);
        check(h.m_materialData[1]==254,"replacement preserves unknown ids when material zero resolves them");
        materials.m_materials.erase(0);

        SelectedTileCluster cluster;
        check(!cluster.getTriangleAndNormalAndMaterialInWorldCoords(101,-198,n,v0,v1,v2,material),"cluster retains false when tile is absent");
        cluster.selected = &h;
        check(cluster.getTriangleAndNormalAndMaterialInWorldCoords(101,-198,n,v0,v1,v2,material) &&
              vectorIs(n,-.557086015f,.742781353f,-.371390676f) && vectorIs(v2,102,18,-196),"cluster bool wrapper forwards corrected normal-first outputs");
        check(cluster.getSquareInWorldCoords(101,-198,square,flipped) && !flipped,"cluster square bool does not depend on tile return register");

        // Exercise all allocations released by the recovered destructor, including null levels.
        h.m_quadTreeLevels=3;
        h.m_quadTree=new unsigned char*[3]{new unsigned char[4],nullptr,new unsigned char[1]};
        h.m_waterQuadTree=new unsigned char*[2]{new unsigned char[4],nullptr};
        h.m_waterData=new float[4];
    }
    check(true,"real Heightmap teardown completed with all five buffer families");
    {
        ObservedHeightmap rectangle;
        fill(rectangle,4,8);
        float low, high;
        rectangle.getMinMaxHeightInLocalCoords(-100,-100,100,100,low,high);
        check(rectangle.reads.size()==24 && rectangle.reads.front()==Cell(-2,-2) &&
              rectangle.reads.back()==Cell(1,3),"rectangular terrain retains X-size lower clamp for Z");
    }
    {
        SignedSamples signedSamples;
        signedSamples.setSize(8,8);
        signedSamples.setScale(Vec3{1,1,1});
        float low, high;
        signedSamples.getMinMaxHeightInLocalCoords(0,0,2,1,low,high);
        check(low==4 && high==4294967296.0f,"min/max compares grid samples as unsigned");
    }
    {
        Heightmap degenerate;
        fill(degenerate);
        degenerate.setScale(Vec3{0,0.5f,0});
        Vec3 n, a, b, c; unsigned char m;
        degenerate.getTriangleAndNormalAndMaterialInLocalCoords(0,0,n,a,b,c,m);
        check(vectorIs(n,0,0,0),"degenerate triangle normal is zeroed by shipped normalization");
        degenerate.internalPhongGetNormal(0,0,n);
        check(vectorIs(n,0,0,0),"degenerate Phong helper normal is zeroed");
        degenerate.setScale(Vec3{2,std::numeric_limits<float>::quiet_NaN(),4});
        degenerate.getNormalInLocalCoords(0,0,n);
        check(std::isnan(n.x) && std::isnan(n.y) && std::isnan(n.z),"unordered normal reaches normalization without a new finite guard");
    }
    materialManager = previousMaterials;
    eventManager = previousEvents;
    materials.m_materials.clear(); // stack-owned fixture materials, not production ownership.
    std::printf("%d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
