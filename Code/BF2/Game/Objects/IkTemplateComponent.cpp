// Path: [INFERRED], beside the existing HudGLCompTemplates.h declaration.
// Complete template: ELF 81E6B90..81E7CD0 / dedicated PE 5F8690..5F8E50.
#include "HudGLCompTemplates.h"
#include "../../Core/Ids.h"
#include "../../IO/Stream/IStream.h"
#include <cfloat>
#include <cmath>
#include <sstream>

namespace dice { namespace hfe { namespace world {

IkTemplateComponent::IkTemplateComponent()
    : m_refCount(1), m_positionLeft{}, m_rotationLeft{}, m_poleVectorLeft{},
      m_positionRight{}, m_rotationRight{}, m_poleVectorRight{}
{
    m_rotationLeft.m[0] = m_rotationLeft.m[5] = m_rotationLeft.m[10] = m_rotationLeft.m[15] = 1.0f;
    m_rotationRight.m[0] = m_rotationRight.m[5] = m_rotationRight.m[10] = m_rotationRight.m[15] = 1.0f;
}
IkTemplateComponent::~IkTemplateComponent() {}
void IkTemplateComponent::addRef() { ++m_refCount; }
int IkTemplateComponent::getRef() { return m_refCount; }
int IkTemplateComponent::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        delete this;
    return remaining;
}
void* IkTemplateComponent::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent || iid == IID_IIkTemplateComponent)
        return const_cast<IkTemplateComponent*>(this);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<IkTemplateComponent*>(this));
    return 0;
}
unsigned int IkTemplateComponent::getClassID() const { return CID_IkTemplateComponent; }
unsigned int IkTemplateComponent::getInterfaceID() { return IID_IIkTemplateComponent; }
void* IkTemplateComponent::queryImplementation(unsigned int cid)
{
    return cid == getClassID() ? this : 0;
}
// Retail is template-only: ELF 81E7B30 and dedicated PE vtable +18 -> 723A40
// both explicitly return null. Soldier reads IIkTemplateComponent on its seat.
IObjectComponent* IkTemplateComponent::createComponent() { return 0; }
void IkTemplateComponent::objectCreated(IObject*) {}   // ELF 81E7980 / PE 5F8700
void IkTemplateComponent::objectDestroyed(IObject*) {} // ELF 81E7990 / PE 5F8710
void IkTemplateComponent::setTemplate(IObjectTemplate*) {} // ELF 81E7B40 / PE 70BF80

void IkTemplateComponent::setPositionLeft(const Vec3& v) { m_positionLeft = v; }
const Vec3& IkTemplateComponent::getPositionLeft() const { return m_positionLeft; }
void IkTemplateComponent::setPoleVectorLeft(const Vec3& v) { m_poleVectorLeft = v; }
const Vec3& IkTemplateComponent::getPoleVectorLeft() const { return m_poleVectorLeft; }
void IkTemplateComponent::setPositionRight(const Vec3& v) { m_positionRight = v; }
const Vec3& IkTemplateComponent::getPositionRight() const { return m_positionRight; }
void IkTemplateComponent::setPoleVectorRight(const Vec3& v) { m_poleVectorRight = v; }
const Vec3& IkTemplateComponent::getPoleVectorRight() const { return m_poleVectorRight; }
void IkTemplateComponent::setRotationLeft(const Vec3& v)
{
    setRotationZXY(m_rotationLeft, -v.z, -v.y, -v.x);
}
void IkTemplateComponent::setRotationRight(const Vec3& v)
{
    setRotationZXY(m_rotationRight, -v.z, -v.y, -v.x);
}
Vec3 IkTemplateComponent::getRotationLeft() const
{
    float z, y, x;
    getRotationZXY(m_rotationLeft, z, y, x);
    return Vec3{-x, -y, -z};
}
Vec3 IkTemplateComponent::getRotationRight() const
{
    float z, y, x;
    getRotationZXY(m_rotationRight, z, y, x);
    return Vec3{-x, -y, -z};
}
const Mat4& IkTemplateComponent::getRotationLeftMat() const { return m_rotationLeft; }
const Mat4& IkTemplateComponent::getRotationRightMat() const { return m_rotationRight; }

namespace {
bool isZero(const Vec3& v)
{
    // Inclusive FLT_EPSILON on every axis. Unordered values are not zero.
    return std::fabs(v.x) <= FLT_EPSILON && std::fabs(v.y) <= FLT_EPSILON &&
           std::fabs(v.z) <= FLT_EPSILON;
}
}
// ELF 81E6DF0 / PE 5F8720: omit zero values, retain two virtual rotation reads.
bool IkTemplateComponent::makeScript(io::IStream* stream)
{
    std::ostringstream out;
    if (!isZero(m_positionLeft))
        out << "ObjectTemplate.ik.positionLeft " << m_positionLeft << std::endl;
    if (!isZero(getRotationLeft()))
    {
#if defined(_WIN32)
        const Vec3 rotation = getRotationLeft();
        out << "ObjectTemplate.ik.rotationLeft " << rotation << std::endl;
#else
        out << "ObjectTemplate.ik.rotationLeft ";
        out << getRotationLeft() << std::endl;
#endif
    }
    if (!isZero(m_poleVectorLeft))
        out << "ObjectTemplate.ik.poleVectorLeft " << m_poleVectorLeft << std::endl;
    if (!isZero(m_positionRight))
        out << "ObjectTemplate.ik.positionRight " << m_positionRight << std::endl;
    if (!isZero(getRotationRight()))
    {
#if defined(_WIN32)
        const Vec3 rotation = getRotationRight();
        out << "ObjectTemplate.ik.rotationRight " << rotation << std::endl;
#else
        out << "ObjectTemplate.ik.rotationRight ";
        out << getRotationRight() << std::endl;
#endif
    }
    if (!isZero(m_poleVectorRight))
        out << "ObjectTemplate.ik.poleVectorRight " << m_poleVectorRight << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<int>(text.size())) == text.size();
}

#if defined(_WIN64) || defined(__LP64__)
// [CHOSEN, not recovered] Two native interface vptrs; refcount, vectors and matrices stay 32-bit.
// Accessors address members by name, and makeScript writes text rather than this object image.
static_assert(sizeof(IkTemplateComponent) == 200, "native x64 IK layout");
#else
static_assert(sizeof(IkTemplateComponent) == 188, "retail x86 IK layout");
#endif
} } }
