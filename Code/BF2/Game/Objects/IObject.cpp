// Path:     [INFERRED] beside IObject.h; the original translation-unit boundary is unresolved.
// Contents: recovered shared IObject defaults and weak-reference lifecycle from both server ports.

#include "IObject.h"
#include "Object.h"
#include "../../Physics/ICollisionMesh.h"       // hasCollisionLod -> ICollisionMesh::hasLod
#include "../../Scene/Object/ObjectManager.h"
#include "../../Core/Ids.h"                        // IID_IPlayerControlObject

#include <new>

namespace dice {
namespace hfe {
namespace world {

bool IObject::useForceDestroy = false;

IObject::IObject()
    : m_interpolationTransformation(0),
      m_gridCullerObject(0),
      m_latencyCullerObject(0),
      m_objectCacheId(0),
      m_gridRefreshQueued(false),
      m_weakControl(0)
{
    // The shipped constructor deliberately leaves the remaining state to Object.
}

IObject::~IObject()
{
    if (m_weakControl)
    {
        m_weakControl->target = 0;
        if (m_weakControl->refCount-- == 1)
            ::operator delete(m_weakControl);
    }
    ::operator delete(m_interpolationTransformation);
}

WeakPtr<IObject> IObject::getWeakPtr()
{
    if (!m_weakControl)
    {
        m_weakControl = static_cast<WeakPtr<IObject>::Ctrl*>(
            ::operator new(sizeof(WeakPtr<IObject>::Ctrl)));
        m_weakControl->refCount = 1;
        m_weakControl->target = this;
    }

    WeakPtr<IObject> result;
    result.m_ctrl = m_weakControl;
    ++m_weakControl->refCount;
    return result;
}

void IObject::disableCachedObject() {}
void IObject::enableCachedObject() {}

unsigned int IObject::getObjectCacheID() const { return m_objectCacheId; }
void IObject::setObjectCacheID(unsigned int id) { m_objectCacheId = id; }

void IObject::forceDestroy()
{
    useForceDestroy = true;
    destroy(true);
    useForceDestroy = false;
}

void IObject::setGridCullerObject(CullerObject* object) { m_gridCullerObject = object; }
CullerObject* IObject::getGridCullerObject() const { return m_gridCullerObject; }
void IObject::setLatencyCullerObject(CullerObject* object) { m_latencyCullerObject = object; }
CullerObject* IObject::getLatencyCullerObject() const { return m_latencyCullerObject; }

int IObject::getGeometryPart() const { return 0; }

// Returns false without a collision mesh; otherwise tests whether the mesh contains lod.
bool IObject::hasCollisionLod(int lod) const
{
    if (!m_collisionMesh)
        return false;
    return static_cast<ICollisionMesh*>(m_collisionMesh)->hasLod(lod);
}

void IObject::SetActiveLod(ObjectLodType) {}
io::INetworkable* IObject::getNetworkable() const { return m_networkable; }
void* IObject::getDecalObject() const { return 0; }
std::string IObject::getArmorInfo() { return std::string(); }
bool IObject::isCreatedByLocalPlayer() const { return false; }

void IObject::internal_assignNewObjectID()
{
    objectManager->unregisterObject(this);
    m_objectId = objectManager->getNewID();
    objectManager->registerObject(this);

    if (m_weakControl)
    {
        m_weakControl->target = 0;
        if (m_weakControl->refCount-- == 1)
            ::operator delete(m_weakControl);
        m_weakControl = 0;
    }

    if (getChild())
        getChild()->internal_assignNewObjectID();
    if (m_sibling)
        m_sibling->internal_assignNewObjectID();
}

void IObject::destroyObject() { Object::internalDestroyObject(this); }

// . See IObject.h for the name, the placement and why no ELF symbol matches.
// It returns the queryInterface result, not the object that answered - the caller gets an
// IPlayerControlObject* it can call directly (GameClient's zipline arm calls exit() on it).
IPlayerControlObject* findOwningPlayerControlObject(IObject* object)
{
    while (object)
    {
        if (void* const pco = object->queryInterface(IID_IPlayerControlObject))
            return static_cast<IPlayerControlObject*>(pco);
        object = object->getParent();                 // the +0x28 link, read directly by the PE
    }
    return 0;
}

} // namespace world
} // namespace hfe
} // namespace dice
