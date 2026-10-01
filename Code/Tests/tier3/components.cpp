// [TEMP] Component regression fixture, not production startup or a client test.
// Links without /FORCE. Supplies the unused GameLogic pointer and the two attested
// ID constants normally defined alongside all singleton storage in Globals.cpp.
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include "../../BF2/Core/ClassFactory.h"
#include "../../BF2/Core/Ids.h"
#include "../../BF2/Game/Objects/HudGLCompTemplates.h"
#include "../../BF2/Game/Objects/StrategicObjectCompTemplate.h"
#include "../../BF2/Game/Objects/StrategicObjectComp.h"
#include "../../BF2/IO/Stream/IStream.h"
namespace dice { namespace hfe { class IGameLogic; IGameLogic* gameLogic = nullptr; } }
namespace dice { namespace hfe { namespace world {
extern const unsigned int IID_IObjectTemplateComponent = 50379;
extern const unsigned int IID_IObjectComponent = 50043;
} } }
using namespace dice::hfe;
using namespace dice::hfe::world;
namespace {
int checks, failures;
void check(bool ok, const char* text)
{
    ++checks; if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", text);
}
bool close(const Vec3& a, const Vec3& b)
{
    return std::fabs(a.x-b.x)<0.0001f && std::fabs(a.y-b.y)<0.0001f && std::fabs(a.z-b.z)<0.0001f;
}
struct Output : io::IStream
{
    std::string text;
    int writes = 0;
    bool shortWrite = false;
    void addRef() override { std::abort(); }
    int getRef() override { std::abort(); }
    int release() override { std::abort(); }
    void* queryInterface(unsigned int) const override { std::abort(); }
    unsigned int read(void*, unsigned int) override { std::abort(); }
    unsigned int write(const void* buffer, unsigned int size) override
    { ++writes; text.assign(static_cast<const char*>(buffer),size); return shortWrite && size ? size-1 : size; }
    int skip(unsigned int) override { std::abort(); }
    int getAvailableBytes() override { std::abort(); }
    io::IStream* clone() override { std::abort(); }
    bool canRead() override { std::abort(); }
    bool canWrite() override { std::abort(); }
};
struct RepeatedRotation : IkTemplateComponent
{
    mutable int reads = 0;
    Vec3 getRotationLeft() const override { ++reads; return Vec3{float(reads),0,0}; }
};
}
int main()
{
    Output out;
    IkTemplateComponent ik;
    auto face = static_cast<IIkTemplateComponent*>(ik.queryInterface(IID_IIkTemplateComponent));
    auto save = static_cast<io::IConsoleSaveable*>(ik.queryInterface(io::IID_IConsoleSaveable));
    check(face == &ik && save == static_cast<io::IConsoleSaveable*>(&ik), "IK interface adjustment");
    check(ik.getRef()==1 && !ik.queryInterface(0) && !ik.queryImplementation(0), "IK identity/default ownership");
    check(ik.queryImplementation(CID_IkTemplateComponent)==&ik && !face->createComponent(), "retail template-only IK factory");
    check(save->makeScript(&out) && out.text.empty() && out.writes==1, "IK defaults emit one empty write");
    bool identity = true;
    for (int i=0;i<16;++i)
        identity &= face->getRotationLeftMat().m[i] == (i%5==0 ? 1.0f : 0.0f)
            && face->getRotationRightMat().m[i] == (i%5==0 ? 1.0f : 0.0f);
    check(identity, "both IK matrices start at identity");
    face->setPositionLeft(Vec3{1,2,3}); face->setPoleVectorLeft(Vec3{4,5,6});
    face->setPositionRight(Vec3{-1,-2,-3}); face->setPoleVectorRight(Vec3{-4,-5,-6});
    check(close(face->getPositionLeft(),Vec3{1,2,3}) && close(face->getPositionRight(),Vec3{-1,-2,-3}), "IK positions through interface");
    check(close(face->getPoleVectorLeft(),Vec3{4,5,6}) && close(face->getPoleVectorRight(),Vec3{-4,-5,-6}), "IK poles through interface");
    face->setRotationLeft(Vec3{90,0,0}); face->setRotationRight(Vec3{0,0,90});
    const Mat4& left=face->getRotationLeftMat(); const Mat4& right=face->getRotationRightMat();
    check(std::fabs(left.m[2]+1)<0.0001f && std::fabs(left.m[8]-1)<0.0001f
        && std::fabs(right.m[1]-1)<0.0001f && std::fabs(right.m[4]+1)<0.0001f, "IK inverse yaw/pitch/roll axis mapping");
    check(close(face->getRotationLeft(),Vec3{90,0,0}) && close(face->getRotationRight(),Vec3{0,0,90}), "IK rotation inverse through interface");
    face->setRotationLeft(Vec3{}); face->setRotationRight(Vec3{});
    check(save->makeScript(&out) && out.text ==
        "ObjectTemplate.ik.positionLeft 1/2/3\nObjectTemplate.ik.poleVectorLeft 4/5/6\n"
        "ObjectTemplate.ik.positionRight -1/-2/-3\nObjectTemplate.ik.poleVectorRight -4/-5/-6\n", "IK script ordering and vector syntax");
    out.shortWrite=true; check(!save->makeScript(&out), "IK rejects short writes"); out.shortWrite=false;
    IkTemplateComponent epsilon;
    epsilon.setPositionLeft(Vec3{FLT_EPSILON,-FLT_EPSILON,0}); epsilon.makeScript(&out);
    check(out.text.empty(), "IK inclusive zero threshold");
    epsilon.setPositionLeft(Vec3{2*FLT_EPSILON,0,0}); epsilon.makeScript(&out);
    check(!out.text.empty(), "IK values above zero threshold are retained");
    RepeatedRotation repeated; repeated.makeScript(&out);
    check(repeated.reads==2 && out.text=="ObjectTemplate.ik.rotationLeft 2/0/0\n", "IK nonzero rotation is read twice");

    StrategicObjectCompTemplate strategic;
    auto script=static_cast<io::IConsoleSaveable*>(strategic.queryInterface(io::IID_IConsoleSaveable));
    check(strategic.getRef()==1 && !strategic.queryInterface(0) && !strategic.queryImplementation(0)
        && script==static_cast<io::IConsoleSaveable*>(&strategic), "strategic interfaces and ownership");
    check(script->makeScript(&out) && out.text.empty() && strategic.getPart()==0
        && strategic.get3dMapIcon()==-1 && !strategic.has3dMapIcon() && !strategic.getIsOnePiece(), "strategic defaults");
    strategic.setStrategicObjectName("bridge"); strategic.setDestroyedIcon("destroyed.dds");
    strategic.setIntactIcon("intact.dds"); strategic.setPart(2); strategic.set3dMapIcon(4); strategic.setIsOnePiece(true);
    check(script->makeScript(&out) && out.text==
        "ObjectTemplate.StrategicObject.strategicObjectName bridge\n"
        "ObjectTemplate.StrategicObject.destroyedIcon destroyed.dds\n"
        "ObjectTemplate.StrategicObject.intactIcon intact.dds\n"
        "ObjectTemplate.StrategicObject.part 2\nObjectTemplate.StrategicObject.3dMapIcon 4\n"
        "ObjectTemplate.StrategicObject.isOnePiece 1\n", "strategic script ordering and vocabulary");
    out.shortWrite=true; check(!script->makeScript(&out), "strategic rejects short writes"); out.shortWrite=false;
    auto base = strategic.createComponent();
    auto comp = static_cast<IStrategicObjectComp*>(base->queryInterface(IID_IStrategicObjectComp));
    check(comp && comp->getRef()==1 && comp->getObject()==nullptr && comp->getIndex()==0
        && !comp->hasUpdate() && !comp->hasFrameUpdate(), "strategic runtime factory/defaults");
    check(&comp->getStrategicObjectName()==&strategic.getStrategicObjectName()
        && &comp->getIntactIconName()==&strategic.getIntactIcon()
        && &comp->getDestroyedIconName()==&strategic.getDestroyedIcon(), "strategic name/icon reference forwarding");
    check(comp->getPart()==2 && comp->has3dMapIcon() && comp->get3dMapIcon()==4 && comp->isOnePiece(), "strategic scalar forwarding");
    check(!comp->getIntactIconPtr() && !comp->getDestroyedIconPtr(), "dedicated icon handles remain retail null");
    check(comp->getHudInfo()==&strategic.getStrategicObjectName()
        && comp->getHudIcons()==&strategic.getDestroyedIcon(), "strategic opaque HUD payload addresses");
    check(!comp->isPartOfDestroyedHolder() && &comp->getHolderPosition()==&Vec3::getZero(), "unattached holder uses shared zero reference");
    comp->setIndex(19); check(comp->getIndex()==19, "strategic index store");
    comp->addRef(); check(comp->release()==1 && comp->release()==0 && strategic.getRef()==1, "runtime template pointer is not owned");
    auto factory = world::ClassFactory<StrategicObjectCompTemplate>::create(io::IID_IConsoleSaveable,nullptr);
    check(factory && factory->release()==0, "factory secondary interface releases through correct subobject");
    check(!world::ClassFactory<IkTemplateComponent>::create(0,nullptr)
        && !world::ClassFactory<StrategicObjectCompTemplate>::create(0,nullptr), "factory mismatch cleans up both templates");
    check(!world::ClassFactory<IkTemplateComponent>::create(IID_IIkTemplateComponent,face), "factory rejects aggregation");
    std::printf("[tier3-components] checks=%d failures=%d\n",checks,failures);
    return failures ? 1 : 0;
}
