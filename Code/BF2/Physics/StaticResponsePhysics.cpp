// Path:     [INFERRED] - beside its header.

#include "StaticResponsePhysics.h"

#include <vector>

#include "../Core/Ids.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/ObjectLayoutAccess.h"

namespace dice {
namespace hfe {
namespace world {

typedef object_detail::LayoutAccess L;

// IResponsePhysics ctor (link triple zeroed), then the six own fields.
StaticResponsePhysics::StaticResponsePhysics()
{
    m_refCount = 1;                                  // +0x10
    m_enabled = true;                                // +0x14
    m_object = 0;                                    // +0x18
    m_collisionGroups = 0;                           // +0x1C
    m_nextToCheckIsReset = true;                     // +0x20
    m_simpleNextToCheck = 0;                         // +0x24
}

StaticResponsePhysics::~StaticResponsePhysics() {}

void StaticResponsePhysics::addRef()  { ++m_refCount; }
int  StaticResponsePhysics::getRef()  { return m_refCount; }
int  StaticResponsePhysics::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

void* StaticResponsePhysics::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)             return const_cast<StaticResponsePhysics*>(this);
    if (iid == IID_IResponsePhysics)  return const_cast<StaticResponsePhysics*>(this);
    return 0;
}

void StaticResponsePhysics::setEnabled(bool v)       { m_enabled = v; }
bool StaticResponsePhysics::getEnabled() const       { return m_enabled; }
void StaticResponsePhysics::reset()                  {}
unsigned int StaticResponsePhysics::getClassID() const { return CID_StaticResponsePhysics; }
void StaticResponsePhysics::checkObjectVsObject(IObject*, IObject*, float, bool) {}
void StaticResponsePhysics::checkVsTerrain(float)    {}
void StaticResponsePhysics::impulseOn(const Vec3&, const Vec3&, const Vec3&, float, int, int) {}
void StaticResponsePhysics::solveImpulse(IPhysicsNode*) {}
void StaticResponsePhysics::addFriction(IPhysicsNode*)  {}
void StaticResponsePhysics::setObject(IObject* o)    { m_object = o; }
IObject* StaticResponsePhysics::getObject() const    { return m_object; }
void StaticResponsePhysics::setCollisionChecked(unsigned int) {}
unsigned int StaticResponsePhysics::getCollisionChecked() const { return 0; }
void StaticResponsePhysics::addToCollisionGroup(unsigned int g)      { m_collisionGroups |= g; }
void StaticResponsePhysics::removeFromCollisionGroup(unsigned int g) { m_collisionGroups &= ~g; }
unsigned int StaticResponsePhysics::getCollisionGroups() const       { return m_collisionGroups; }
void StaticResponsePhysics::setCollisionGroups(unsigned int g)       { m_collisionGroups = g; }

// 66 B: if the chain is already in its reset state, nothing; otherwise walk it, resetting every
// link. The walk reads each element's simple next before resetting it - order matters and is
// preserved.
void StaticResponsePhysics::setDirtyNextToCheck()
{
    if (m_nextToCheckIsReset)
        return;
    for (IResponsePhysics* p = this; p; )
    {
        IResponsePhysics* n = p->getSimpleNextToCheck();
        p->resetNextToCheck();
        p = n;
    }
}

// the TU static - _Z41__static_initialization_and_destruction_0ii_392 builds, and __tcf_1_132
// destroys, a std::vector<IResponsePhysics*> at ds:0x8ABAE74 (begin / +4 end / +8 cap). Its only
// readers are the two functions below, both in this TU - a file-static scratch list [CHOSEN name].
namespace {
std::vector<IResponsePhysics*> s_tmpResponseList;
} // namespace

// Appends responses that request collision checks in self, sibling-subtree, then
// child-subtree order. A null object terminates that branch.
void StaticResponsePhysics::addToTmpResponseList(const IObject* object)
{
    if (!object)
        return;
    IResponsePhysics* const response =
        static_cast<IResponsePhysics*>(L::responsePhysics(const_cast<IObject*>(object)));   // +0x48
    if (response && response->shouldCheckCollision())
        s_tmpResponseList.push_back(response);
    addToTmpResponseList(L::sibling(const_cast<IObject*>(object)));       // +0x2C
    addToTmpResponseList(object->getChild());                             // slot 60
}

// Returns the next response, lazily rebuilding a reset chain only at its root.
// Children retain their reset flag until the root relinks them; childless roots clear their next pointer.
IResponsePhysics* StaticResponsePhysics::getNextToCheck()
{
    if (m_nextToCheckIsReset)                                             // +0x20
    {
        if (!L::parent(m_object))                                         // +0x28
        {
            IObject* const child = m_object->getChild();                  // slot 60
            if (child)
            {
                s_tmpResponseList.resize(0);
                addToTmpResponseList(child);
                IResponsePhysics* current = this;
                for (std::vector<IResponsePhysics*>::iterator it = s_tmpResponseList.begin();
                     it != s_tmpResponseList.end(); ++it)
                {
                    current->setNextToCheck(*it);                         // +0x68
                    current = *it;
                }
                current->setNextToCheck(0);
            }
            else
            {
                setNextToCheck(0);
            }
        }
    }
    return m_simpleNextToCheck;
}

IResponsePhysics* StaticResponsePhysics::getSimpleNextToCheck()  { return m_simpleNextToCheck; }

void StaticResponsePhysics::setNextToCheck(IResponsePhysics* p)
{
    m_nextToCheckIsReset = false;
    m_simpleNextToCheck = p;
}

void StaticResponsePhysics::resetNextToCheck()
{
    m_nextToCheckIsReset = true;
    m_simpleNextToCheck = 0;
}

// 107 B, branch for branch. The CID tail can never match on a Static (its own getClassID is
// CID_StaticResponsePhysics) - the shape is shared family code and is preserved, not optimised.
bool StaticResponsePhysics::shouldCheckCollision()
{
    if (!m_enabled)
        return false;

    const unsigned int f = L::flags(m_object);       // IObject +0x04
    if (!(f & 0x100) || (f & 1))                     // needs bit 8, refuses bit 0
        return false;

    if (getCollisionMesh())                          // self vtable +0x78
        return true;

    const unsigned int cid = getClassID();           // self vtable +0x24
    return cid == CID_PointResponsePhysics || cid == CID_RotationalPointResponsePhysics;
}

// -> m_object, IObject slot 51 (hasCollisionLod - non-pure there, and the object's own body reads
// its +0x40 mesh cache).
bool StaticResponsePhysics::hasCollisionLod(int lod) const
{
    return m_object->hasCollisionLod(lod);
}

// A RAW field read in the ELF - object +0x40, no dispatch.
void* StaticResponsePhysics::getCollisionMesh() const
{
    return L::collisionMesh(m_object);
}

bool StaticResponsePhysics::getIsIntersectingTerrain(Vec3&) const { return false; }
unsigned char StaticResponsePhysics::getPermanentGrip() const     { return 0; }
void StaticResponsePhysics::setPermanentGrip(unsigned char)       {}   // empty, as shipped
void StaticResponsePhysics::setSurfacePositionalSpeed(const Vec3&) {}
const Vec3& StaticResponsePhysics::getSurfacePositionalSpeed() const { return Vec3::getZero(); }
const Vec3& StaticResponsePhysics::getPositionalAdjusts() const      { return Vec3::getZero(); }

// 27 B: getRootParent(m_object)->m_responsePhysics (+0x48). Same no-null-check as the node side's
// getRootNode - preserved.
IResponsePhysics* StaticResponsePhysics::getRootResponse() const
{
    return static_cast<IResponsePhysics*>(L::responsePhysics(getRootParent(m_object)));
}

bool StaticResponsePhysics::poseChangeTest(IObject*, IObject*, float)    { return true; }
bool StaticResponsePhysics::testIfValidTransformation(const Mat4&, bool) { return true; }
void StaticResponsePhysics::updateEffect(float)                          {}

} // namespace world
} // namespace hfe
} // namespace dice
