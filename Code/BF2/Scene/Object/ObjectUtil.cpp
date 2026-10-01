// Path:     [INFERRED] filename - see ObjectUtil.h for the TU evidence and what is actually known.
// Contents: recovered free object helpers. ELF addresses cited; both ports read before writing.

#include "ObjectUtil.h"

#include "../../Core/ClassManager.h"
#include "../../Core/IBase.h"
#include "../../Core/Ids.h"
#include "../../Game/Objects/IObject.h"
#include "../../Game/Objects/IObjectTemplate.h"
#include "../../Game/Objects/ObjectTemplate.h"
#include "../../Game/Objects/Base/BundleTemplate.h"           // IBundleTemplate - the child walk
#include "ObjectTypes/Base/ObjectTemplateManager.h"           // objectTemplateManager->getTemplate
#include "../../IO/Network/INetworkable.h"
#include "../../IO/Network/NetworkableInfo.h"

#include <string>

namespace dice {
namespace hfe {

extern ClassManager* classManager;      // ds:0x8AC85A0

namespace io {
extern IBase* networkableInfoManager;   // ds:0x8AC8CA0
}

namespace world {

// / No null guard surrounds the networkable result beyond
// the loop test; the parent hop is the inline +0x28 field read on both shipped ports.
IObject* findClosestParentHavingNetworkable(IObject* object)
{
    while (object)
    {
        if (object->getNetworkable())
            return object;
        object = object->getParent();
    }
    return 0;
}

// / PE 0x6D9520. Retail deliberately dereferences each candidate's template
// without a null guard and never tests `root` itself.
IObject* findClosestChildOfType(IObject* root, IObject* nearObject, unsigned int classId)
{
    while (nearObject
        && nearObject != root
        && nearObject->getTemplate()->getClassID() != classId)
    {
        nearObject = nearObject->getParent();
    }
    return nearObject;
}

// Count matching child templates in depth-first order until `wanted` is
// reached. The root itself is tested before its children and is not included in the count.
bool getChildIndexOfType(IObject* root, IObject* wanted, unsigned int classId, int& index)
{
    if (root == wanted)
        return true;

    for (IObject* child = root->getChild(); child; child = child->getSibling())
    {
        if (child->getTemplate()->getClassID() == classId)
            ++index;
        if (getChildIndexOfType(child, wanted, classId, index))
            return true;
    }
    return false;
}

// / Preserve the matching-child-only recursion,
// the root/index early return, and the absence of root/template guards.
IObject* findChildTypeIndex(IObject* root, int wantedIndex, unsigned int classId, int& index)
{
    if (wantedIndex == index)
        return root;
    for (IObject* child = root->getChild(); child; child = child->getSibling())
    {
        if (child->getTemplate()->getClassID() == classId)
        {
            ++index;
            IObject* found = findChildTypeIndex(child, wantedIndex, classId, index);
            if (found)
                return found;
        }
    }
    return 0;
}

// Attaches a replication component when requested; true does not guarantee one was attached.
// Precache, absent template and empty info name are successful no-ops. Missing manager/info fails.
// Preserve the two info-name queries across component construction. Failure releases the partial
// component; success releases the creator's reference after setComponent takes ownership.
bool setNetworkableComp(IObject* object, unsigned int networkableClassId)
{
    const IObjectTemplate* objectTemplate = object->getTemplate();   // +0x14, loaded first

    if (ObjectTemplate::getIsPreCaching())
        return true;

    if (!objectTemplate)
        return true;

    if (objectTemplate->getNetworkableInfo().compare("") == 0)       // template vtable +0x94
        return true;

    if (!io::networkableInfoManager)
        return false;

    io::INetworkableInfoManager* infoManager =
        reinterpret_cast<io::INetworkableInfoManager*>(io::networkableInfoManager);

    IBase* created = classManager->createInstance(networkableClassId, io::IID_INetworkable, 0);
    if (!created)
        return false;

    io::INetworkable* networkable = static_cast<io::INetworkable*>(created);
    networkable->init(object);                                       // vptr +0x18

    // Second call. The name is re-read, not reused.
    io::INetworkableInfo* info = infoManager->getInfo(objectTemplate->getNetworkableInfo());
    if (!info)
    {
        networkable->setNetworkableInfo(0);
        networkable->release();
        return false;
    }

    networkable->setNetworkableInfo(info);
    object->setComponent(io::IID_INetworkable, networkable);         // object vtable +0x58
    networkable->release();
    return true;
}

// Clears flag 0x40 recursively on this template and its named bundle children; null is ignored.
// The flag's precache meaning is [INFERRED] and differs from preCache's 0x20 gate.
// Re-read the virtual child count each iteration; missing child names stop at the callee's null guard.
void turnOffPreCacheOnTemplateAndAllChildren(IObjectTemplate* objectTemplate)
{
    if (!objectTemplate)
        return;
    objectTemplate->updateFlags(0u, 0x40u);                                           // +0x4C
    IBundleTemplate* bundle = static_cast<IBundleTemplate*>(
        objectTemplate->queryInterface(IID_IBundleTemplate));                         // +0x0C, 50021
    if (!bundle)
        return;
    for (int i = 0; i < bundle->getNoTemplates(); ++i)                                // +0x18
    {
        const std::string name = bundle->getTemplate(i);                              // +0x20
        turnOffPreCacheOnTemplateAndAllChildren(objectTemplateManager->getTemplate(name));   // +0x24
    }
}

// findEngine - 0x83258B0, 247 B (see the header).
IObject* findEngine(IObject* object, const std::string& name)
{
    const IObjectTemplate* t = object->getTemplate();                                // +0x14
    if (t->getClassID() == CID_EngineTemplate && t->getName() == name)               // +0x10, +0x2C
        return object;
    if (IObject* sibling = object->getSibling())                                     // +0x2C
        if (IObject* found = findEngine(sibling, name))
            return found;
    if (!object->getChild())                                                         // +0xF0
        return 0;
    return findEngine(object->getChild(), name);
}

// Applies masks to the root first, then descendants. Object::updateFlagsRecursive
// updates descendants only; projectile activation requires this root-inclusive behavior.
void updateFlagsRecursive(IObject* object, unsigned int set, unsigned int clear)
{
    object->updateFlags(set, clear);
    for (IObject* child = object->getChild(); child; child = child->getSibling())
        updateFlagsRecursive(child, set, clear);
}


// Returns the portal-mesh component and optionally writes its owning object on success only.
// On failure, preserve the additional object-interface query even though both branches return null.
IBase* getPortalMesh(IObject* object, IObject** portalObject)
{
    IBase* const portalMesh = object->queryComponent(geom::IID_IGeometry, geom::IID_IPortalMesh);
    if (portalMesh)
    {
        if (portalObject)
            *portalObject = object;

        return portalMesh;
    }

    // Result deliberately discarded - see the note above.
    object->queryInterface(IID_IObject);
    return 0;
}
} // namespace world
} // namespace hfe
} // namespace dice
