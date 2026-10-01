// [TEMP] [CHOSEN, not recovered] component fixture, never linked into production.
// Real template constructors, typed secondary interfaces, setters/getters and matrix routines.
// No replacement implementation of a recovered method; no map, listener or client acceptance.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef near
#include <DbgHelp.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>
#include "../BF2/Game/Objects/PlayerControlObjectTemplate.h"
#include "../BF2/Game/Objects/TemplateLeaves.h"

using namespace dice::hfe;
using namespace dice::hfe::world;
namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", label);
    std::fflush(stdout);
}
bool near(float a, float b) { return std::fabs(a-b) < 0.0001f; }
bool vectorIs(const Vec3& v, float x, float y, float z)
{
    return near(v.x,x) && near(v.y,y) && near(v.z,z);
}
bool matrixIs(const Mat4& a, const Mat4& b)
{
    for (int i=0; i<16; ++i) if (!near(a.m[i],b.m[i])) return false;
    return true;
}
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
struct InspectTemplate : PlayerControlObjectTemplate
{
    using PlayerControlObjectTemplate::m_seatParent;
    using PlayerControlObjectTemplate::m_pcoPosIds;
};
// This observer is used only to distinguish direct field writes from virtual setter dispatch.
struct SetterObserver : InspectTemplate
{
    int calls = 0;
    void setSeatParent(const std::string&) override { ++calls; }
    void setSeatPosition(const Vec3&) override { ++calls; }
    void setSeatRotation(const Vec3&) override { ++calls; }
};
}
int main()
{
    static_assert(sizeof(void*) == 4, "x86 fixture required");
    static_assert(std::is_same<decltype(std::declval<IPlayerControlObjectTemplate&>().getArtPos()),
                               bool>::value, "retail template getter returns bool");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(reportFault);
    InspectTemplate t;
    IPlayerControlObjectTemplate* face = &t;
    check(static_cast<void*>(face) != static_cast<void*>(&t), "real adjusted secondary interface");
    check(face->getPcoId()==-1 && face->getPcoFlags()==0, "retail identity defaults");
    face->setPcoId(-239); check(face->getPcoId()==-239, "signed PCO id preserved");
    face->setPcoFlags(0x12); face->setPcoFlags(0x44); face->setPcoFlags(0);
    check(face->getPcoFlags()==0x56, "flag writes accumulate and zero does not clear");
    face->setPcoFlags(std::numeric_limits<int>::min());
    check(face->getPcoFlags()==0x80000056u, "flag high bit preserved");
    face->setPcoFlags(-1); check(face->getPcoFlags()==0xffffffffu, "negative flag mask preserves all bits");
    face->setVehicleCategory(static_cast<VehicleCategory>(-77));
    face->setControlsCategory(static_cast<VehicleCategory>(0x12345678));
    face->setVehicleType(static_cast<VehicleType>(-901));
    face->setToolTipType(static_cast<ToolTipType>(888));
    check(face->getVehicleCategory()==-77 && face->getControlsCategory()==0x12345678,
          "categories remain separate and unclamped");
    check(face->getVehicleType()==-901 && face->getToolTipType()==888, "type and tooltip retain arbitrary enum values");
    face->setArtPos(true); check(face->getArtPos(), "art position true through bool interface");
    face->setArtPos(false); check(!face->getArtPos(), "art position false through bool interface");

    face->setDirBarXScale(-11); face->setDirBarYScaleAbove(23); face->setDirBarYScaleBelow(-37);
    face->setDirBarRotate(721); face->setDirBarYScaleMin(55); face->setDirBarYScaleMax(-89);
    check(face->getDirBarXScale()==-11, "direction bar X scale");
    check(face->getDirBarYScaleAbove()==23 && face->getDirBarYScaleBelow()==-37, "direction bar above and below stay separate");
    check(face->getDirBarRotate()==721, "direction bar rotation is not normalized");
    check(face->getDirBarYScaleMin()==55 && face->getDirBarYScaleMax()==-89, "direction bar inverted min/max are not reordered");
    t.addPcoPosId(-1); t.addPcoPosId(42); t.addPcoPosId(42);
    for (int i=0;i<2048;++i) t.addPcoPosId(i);
    bool ordered = t.m_pcoPosIds.size()==2051 && t.m_pcoPosIds[0]==-1 &&
                   t.m_pcoPosIds[1]==42 && t.m_pcoPosIds[2]==42;
    for (int i=0;i<2048;++i) ordered = ordered && t.m_pcoPosIds[i+3]==i;
    check(ordered, "position IDs preserve duplicates, signed values and append order across growth");

    Vec3 input = {1.25f,-2.5f,3.75f};
    face->setNameTagOffset(input); face->setRidingWithOffset(Vec3{-4,5,-6}); input.x=99;
    check(vectorIs(face->getNameTagOffset(),1.25f,-2.5f,3.75f), "nametag setter copies by-value input");
    check(vectorIs(face->getRidingWithOffset(),-4,5,-6), "riding vector uses independent storage");
    const Vec3* nameRef = &face->getNameTagOffset();
    const Vec3* ridingRef = &face->getRidingWithOffset();
    face->setNameTagOffset(Vec3{7,8,9}); face->setRidingWithOffset(Vec3{10,11,12});
    check(nameRef==&face->getNameTagOffset() && vectorIs(*nameRef,7,8,9), "nametag getter returns live member reference");
    check(ridingRef==&face->getRidingWithOffset() && vectorIs(*ridingRef,10,11,12), "riding getter returns live member reference");
    face->setRidingWithDistMod(-4.5f); check(face->getRidingWithDistMod()==-4.5f, "riding distance modifier is unclamped");
    face->setVehicleCameraShake(static_cast<CameraShakeTriggers>(0x76543210));
    face->setVehicleFov(-275);
    check(face->getVehicleCameraShake()==0x76543210 && face->getVehicleFov()==-275, "camera values retain full width and sign");

    face->setSeatLeftRotationLimit(-800); face->setSeatRightRotationLimit(900);
    check(face->getSeatLeftRotationLimit()==-800 && face->getSeatRightRotationLimit()==900,
          "seat traverse limits have no conversion or clamp");
    face->seatInformation("seat_root",Vec3{4,5,6},Vec3{0,0,90});
    const Mat4& seat = face->getSeatTransformation();
    check(near(seat.m[0],0) && near(seat.m[1],1) && near(seat.m[4],-1) && near(seat.m[5],0),
          "seat rotation reverses and negates input into ZXY degrees");
    check(seat.m[3]==0 && seat.m[7]==0 && seat.m[11]==0 && seat.m[15]==1,
          "combined seat setter resets affine matrix entries");
    check(vectorIs(face->getSeatPosition(),4,5,6) && face->getSeatParent()=="seat_root", "combined setter applies parent and position");
    check(face->getSeatLeftRotationLimit()==-800 && face->getSeatRightRotationLimit()==900,
          "combined setter preserves traverse limits");
    const std::string* parentRef = &face->getSeatParent();
    const Mat4 beforeParent = seat;
    face->setSeatParent(std::string("parent\0suffix",13));
    check(parentRef==&face->getSeatParent() && parentRef->size()==13 && (*parentRef)[6]=='\0',
          "seat parent preserves embedded NUL and reference identity");
    check(matrixIs(beforeParent,seat), "parent-only setter preserves matrix");
    const Vec3* positionRef = &face->getSeatPosition();
    face->setSeatPosition(Vec3{-7,8,-9});
    check(positionRef==&face->getSeatPosition() && vectorIs(*positionRef,-7,8,-9), "position getter aliases live matrix translation");
    bool rotationUnchanged = true;
    for (int i=0;i<12;++i) rotationUnchanged = rotationUnchanged && seat.m[i]==beforeParent.m[i];
    check(rotationUnchanged && seat.m[15]==beforeParent.m[15], "position-only setter preserves other matrix elements");
    t.m_seatParent.seatTransformation.m[3]=12;
    t.m_seatParent.seatTransformation.m[7]=13;
    t.m_seatParent.seatTransformation.m[11]=14;
    t.m_seatParent.seatTransformation.m[15]=15;
    face->setSeatRotation(Vec3{90,0,0});
    check(vectorIs(*positionRef,-7,8,-9), "rotation-only setter restores saved translation");
    check(near(seat.m[0],0) && near(seat.m[2],-1) && near(seat.m[8],1) && near(seat.m[10],0),
          "first input component selects negated Y rotation");
    check(seat.m[3]==0 && seat.m[7]==0 && seat.m[11]==0 && seat.m[15]==1,
          "rotation-only setter also resets affine matrix entries");
    face->setSeatRotation(Vec3{0,90,0});
    check(near(seat.m[5],0) && near(seat.m[6],1) && near(seat.m[9],-1) && near(seat.m[10],0),
          "middle input component selects negated X rotation");
    face->setSeatRotation(Vec3{30,20,10});
    const Vec3* cached = &face->getSeatRotation();
    check(vectorIs(*cached,30,20,10), "first rotation getter decomposes current matrix");
    face->setSeatRotation(Vec3{0,0,0});
    check(cached==&face->getSeatRotation() && vectorIs(*cached,30,20,10), "rotation getter intentionally stays stale after mutation");
    InspectTemplate another;
    another.setSeatRotation(Vec3{-15,25,35});
    check(cached==&another.getSeatRotation() && vectorIs(another.getSeatRotation(),30,20,10),
          "rotation getter shares first template's cached value across objects");
    SetterObserver observer;
    observer.seatInformation("direct",Vec3{1,2,3},Vec3{0,0,0});
    check(observer.calls==0 && observer.getSeatParent()=="direct" && vectorIs(observer.getSeatPosition(),1,2,3),
          "combined setter does not dispatch virtual setters");
    another.setSeatPosition(Vec3{4,5,6});
    another.seatInformation("alias",another.getSeatPosition(),Vec3{0,0,0});
    check(vectorIs(another.getSeatPosition(),0,0,0), "combined setter reads aliased position only after identity reset");
    another.setSeatPosition(Vec3{4,5,6});
    another.setSeatRotation(another.getSeatPosition());
    check(another.getSeatTransformation().m[0]==1 && vectorIs(another.getSeatPosition(),4,5,6),
          "rotation setter reads aliased angle after reset and restores saved translation");

    face->setSoundFilter(std::string("filter\0tail",11));
    const std::string* sound = &face->getSoundFilter();
    check(sound->size()==11 && (*sound)[6]=='\0', "sound filter preserves complete string");
    face->setSoundFilter(""); check(sound==&face->getSoundFilter() && sound->empty(), "sound filter reference stays live after clear");
    face->setListenerObstruction(-8); check(face->getListenerObstruction()==0, "retail obstruction lower clamp");
    face->setListenerObstruction(8); check(face->getListenerObstruction()==1, "retail obstruction upper clamp");
    face->setListenerObstruction(0.375f); check(face->getListenerObstruction()==0.375f, "retail obstruction in-range value");
    face->setListenerObstruction(-0.0f); check(std::signbit(face->getListenerObstruction()), "obstruction retains negative zero");
    face->setListenerObstruction(std::numeric_limits<float>::quiet_NaN());
    check(face->getListenerObstruction()==0, "PE unordered clamp selects lower bound");
    face->setListenerObstruction(std::numeric_limits<float>::infinity());
    check(face->getListenerObstruction()==1, "obstruction positive infinity clamp");
    face->setListenerObstruction(-std::numeric_limits<float>::infinity());
    check(face->getListenerObstruction()==0, "obstruction negative infinity clamp");
    face->setMaintainCameraOnExit(true); face->setMaintainCameraOnEnter(false); face->setNeverDrawAs1p(true);
    check(face->getMaintainCameraOnExit() && !face->getMaintainCameraOnEnter() && face->getNeverDrawAs1p(), "camera flags remain independent");
    face->setMaintainCameraOnExit(false); face->setMaintainCameraOnEnter(true); face->setNeverDrawAs1p(false);
    check(!face->getMaintainCameraOnExit() && face->getMaintainCameraOnEnter() && !face->getNeverDrawAs1p(), "camera flags can each be cleared");
    LadderContainerTemplate ladder;
    ParachuteTemplate parachute;
    ZiplineContainerTemplate zipline;
    GrapplingHookRopeContainerTemplate grapple;
    IPlayerControlObjectTemplate* leaves[] = {&ladder,&parachute,&zipline,&grapple};
    bool inherited = true;
    for (int i=0;i<4;++i)
    {
        leaves[i]->setPcoId(100+i);
        leaves[i]->setPcoFlags(1<<i);
        leaves[i]->seatInformation("leaf",Vec3{1,2,3},Vec3{0,0,0});
        inherited = inherited && leaves[i]->getPcoId()==100+i && leaves[i]->getPcoFlags()==(1u<<i)
                    && vectorIs(leaves[i]->getSeatPosition(),1,2,3);
    }
    check(inherited, "all four existing leaf templates inherit recovered secondary methods");
    std::printf("PlayerControlObjectTemplate runtime: %d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
