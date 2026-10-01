// Destroyable-object lifecycle, replication, and collision behavior.

#include "DestroyableObject.h"


#include "../../../AI/AIInterface/BFKnowledge/IAIObject.h"   // enableCachedObject's init()
#include "../../../Core/Ids.h"
#include "../DestroyableObjectTemplate.h"
#include "../ObjectTemplate.h"                    // getNetworkableInfo / queryInterface on the template
#include "../../../IO/Network/INetworkable.h"    // m_basePriority - the raw store
#include "../../../Scene/Object/ObjectUtil.h"    // setNetworkableComp
#include "../../IGameLogic.h"                     // gameLogic->isServer - handleCollision
#include "../../../Physics/ICollisionMesh.h"      // hasWreckGeom - handleCollision
#include "../GameLogicComp/IArmorGLComp.h"        // isDestroyed - handleCollision
#include "../ObjectLayoutAccess.h"                // the +0x3C armor walk, +0x40 mesh

namespace dice {
namespace hfe {
namespace world {

// (C2) / 0x8173640 (C1), 87 B: Bundle, the two vptrs, refcount 1 - and then a loop that
// walks this->parent chain looking for the first object with an armor component (+0x3C) and
// discards the result: no store, no call, no branch on it. Compiler residue of an inlined lookup
// whose value the source never used (or a debug-only use compiled out). Nothing to reproduce.
DestroyableObject::DestroyableObject(const DestroyableObjectTemplate* objectTemplate)
    : Bundle(objectTemplate)
{
    m_refCount = 1;                                  // +0x13C
}

// (D1) / 51 B (D0): vptr restores, Bundle's dtor (D0 adds operator delete).
DestroyableObject::~DestroyableObject() {}

// The quartet - refcount at +0x13C, the ControlPoint arrangement.
void DestroyableObject::addRef()  { ++m_refCount; }
int  DestroyableObject::getRef()  { return m_refCount; }
int  DestroyableObject::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

// , three cases: IBase / IObject -> this; IConsoleSaveable -> the +0x80 sub-object; else 0.
void* DestroyableObject::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)   return const_cast<DestroyableObject*>(this);
    if (iid == IID_IObject) return const_cast<DestroyableObject*>(this);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<DestroyableObject*>(this));   // this + 0x80
    return 0;
}

// `if (m_aiObject) setComponent(ai::IID_IAIObject, 0)` (slot 22, +0x58 - clears
// the AI object component before the pooled object goes dormant), then SimpleObject's own.
void DestroyableObject::disableCachedObject()
{
    if (m_aiObject)
        setComponent(ai::IID_IAIObject, 0);
    SimpleObject::disableCachedObject();
}

// Revives the object through Bundle, then initializes the AI interface captured before that call.
void DestroyableObject::enableCachedObject()
{
    aiObjectCreate();                                // bots-gated inside; a no-op without AI
    hfe::ai::IAIObject* const aiObject =
        static_cast<hfe::ai::IAIObject*>(m_aiObject); // +0x34 read before Bundle's enable
    Bundle::enableCachedObject();
    if (aiObject)
        aiObject->init();                            // ELF +0x48 / PE +0x44
    enableObject(this, false);                       // clears the disabled bit
}

// Uses simple-object replication for mobile physics and destroyable replication otherwise.
// Requires successful attachment: the final base-priority assignment dereferences the networkable unconditionally.
void DestroyableObject::initNetworkableComp()
{
    if (m_template && m_template->getNetworkableInfo().compare("") != 0)
    {
        ISimpleObjectTemplate* simple = static_cast<ISimpleObjectTemplate*>(
            m_template->queryInterface(IID_ISimpleObjectTemplate));
        const unsigned int cid = (simple && simple->getHasMobilePhysics())
                                     ? CID_SimpleObjectNetworkable
                                     : CID_DestroyableObjectNetworkable;
        setNetworkableComp(this, cid);
    }
    io::INetworkable* networkable = getNetworkable();
    networkable->m_basePriority = -1.0f;
}

// Uses bundle collision behavior unless destroyed armor and wreck geometry select the wreck path.
// Clients without wreck geometry report false.
bool DestroyableObject::handleCollision(IObject* other, const Vec3& position, const Vec3& normal,
                                        const Vec3& velocity, int material, int otherMaterial,
                                        CollisionFaceInfo* faceA, CollisionFaceInfo* faceB)
{
    const bool r = Bundle::handleCollision(other, position, normal, velocity, material, otherMaterial,
                                           faceA, faceB);
    IArmorGLComp* armor = 0;
    for (IObject* o = this; o; o = object_detail::LayoutAccess::parent(o))
    {
        if (IObjectComponent* comp = object_detail::LayoutAccess::armorComponent(o))   // +0x3C
        {
            armor = static_cast<IArmorGLComp*>(comp);
            break;
        }
    }
    if (!armor)
        return r;
    ICollisionMesh* const mesh =
        static_cast<ICollisionMesh*>(object_detail::LayoutAccess::collisionMesh(this));   // +0x40
    if (!mesh)
        return r;
    if (gameLogic->isServer() || mesh->hasWreckGeom())                  // slot 12
    {
        if (armor->isDestroyed() == true)                               // slot 21
            return mesh->hasWreckGeom();
        return r;
    }
    return false;
}


} // namespace world
} // namespace hfe
} // namespace dice
