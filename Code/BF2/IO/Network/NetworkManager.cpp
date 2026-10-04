// Network object registration, state collection and update dispatch.

#include "NetworkManager.h"

#include <cstring>                                // std::memset

#include "INetworkable.h"
#include "INetworkableObject.h"
#include "NetworkableInfo.h"                      // INetworkableInfo::getForceNetworkableId
#include "../../Core/Debug.h"
#include "../../Core/EventManager.h"
#include "../../Core/Ids.h"
#include "../../Core/SettingsRepostitory.h"
#include "../../Core/StringUtil.h"                // formatString
#include "../../Game/IGame.h"                     // game - the IGameServer ghost hooks
#include "../../Game/IGameServer.h"
#include "../../Game/Objects/IObject.h"
#include "../../Game/Objects/IObjectTemplate.h"
#include "../../Game/Objects/ObjectLayoutAccess.h" // addDescriptor's raw template read (+0x14)
#include "../../Game/Objects/RotationalBundleTemplate.h"     // compareProperties' field reads
#include "../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>

namespace dice {
namespace hfe {
namespace io {

// Runtime precision settings used by replication codecs.
int StatePrecisionFull   = 3;                    // ds:0x88E9AA0
int StatePrecisionGhost  = 2;                    // ds:0x88E9AA4
int StatePrecisionLow    = 1;                    // the header's recorded family - all four were
int StatePrecisionLowest = 0;                    //   extern-only, all four defined together now

namespace {
// Returns the replicated object stored by the networkable-object interface, or null for a null interface.
world::IObject* objectOfNetworkableObject(INetworkableObject* networkableObject)
{
    return networkableObject ? networkableObject->m_object : 0;
}

// Returns whether the object's replication policy forces a network ID. Both networkable and policy must exist.
bool networkableInfoForcesId(world::IObject* object)
{
    INetworkable* networkable = object->getNetworkable();
    return networkable->m_networkableInfo->getForceNetworkableId();
}
} // namespace

// Initializes empty descriptor stores and network-ID allocation, then registers for core events.
NetworkManager::NetworkManager()
    : m_descriptorHash(4096)
{
    m_idCounter = 642;                           // +0x10 (0x282)
    m_word12 = 1;                                // +0x12
    m_interpolationTime = 100;                   // +0x50
    m_extrapolationTime = 1200;                  // +0x54
    m_globalPredictionMode = 1;                  // +0x58
    m_byte5C = false;                            // +0x5C
    m_dword6C = 0;                               // +0x6C
    m_ghostNetworkableUpdatesMissing = 0;        // +0x70
    eventManager->registerEventHandler(static_cast<EventCategory>(5), this, 0);   // +0x2C
}

// Clears replication state and unregisters core-event handling before member destruction.
NetworkManager::~NetworkManager()
{
    clear();
    eventManager->unregisterEventHandler(static_cast<EventCategory>(5), this);   // +0x30
}


void NetworkManager::handleEvent(EventCategory category, unsigned int id, EventNode* node, void* /*userData*/)
{
    if (category != static_cast<EventCategory>(5) || id != 1)
        return;
    const int which = *reinterpret_cast<int*>(node);
    if (which == 1005)
    {
        unsigned int value = 100;
        m_interpolationTime = static_cast<int>(settings->U32Get("GSInterpolationTime", value));
    }
    else if (which == 1006)
    {
        unsigned int value = 1200;
        m_extrapolationTime = static_cast<int>(settings->U32Get("GSExtrapolationTime", value));
    }
}

// Frees descriptors and resets registration state. Aborts if any networkable object remains registered.
void NetworkManager::clear()
{
    if (!m_networkableObjects.empty())
        std::abort();
    m_descriptorHash.clear();
    for (std::map<unsigned short, NetworkableDescriptor*>::iterator it = m_descriptors.begin();
         it != m_descriptors.end(); ++it)
    {
        if (it->second)
            delete it->second;
    }
    m_descriptors.clear();
    m_activeDescriptors.clear();
    m_scopableDescriptors.clear();
    m_staticDescriptors.clear();
    m_networkedTemplates.clear();
    m_scopableRootDescriptors.clear();
    m_networkableStateMasks.clear();
    m_word12 = 1;
    m_idCounter = 642;
    m_byte5C = false;
    m_globalPredictionMode = 1;
}

// Allocates a dynamic ID when requested is zero, otherwise reserves the requested ID and advances the counter.
// Duplicate descriptors trigger a diagnostic and return zero if execution continues.
unsigned short NetworkManager::assignId(unsigned short requested)
{
    unsigned short newId = requested;
    if (requested == 0)
    {
        newId = m_idCounter;
        m_idCounter = static_cast<unsigned short>(m_idCounter + 1);
    }
    else if (m_idCounter <= requested)
    {
        m_idCounter = static_cast<unsigned short>(requested + 1);
    }
    if (m_descriptorHash.exists(newId, newId % 4096u))
    {
        Debug(DEBUGTYPE_7, "IO/Network/NetworkManager.cpp", 121, __FUNCTION__)
            << "Duplicate descriptor detected error:" << newId;
        return 0;
    }
    return newId;
}

// `ret`.
void NetworkManager::releaseId() {}

// 37 / 39 B: both forward - the object path is analyseObject.
unsigned short NetworkManager::addObject(world::IObject* object)
{
    return addObject(object, 0);
}
unsigned short NetworkManager::addObject(world::IObject* object, unsigned short netId)
{
    return analyseObject(object, netId);
}

// Typed conversion between player-control interfaces and their owning objects.
namespace {
world::IObject* pcoInterfaceToObject(world::IPlayerControlObject* pcoInterface)
{
    return static_cast<world::IObject*>(pcoInterface->queryInterface(world::IID_IObject));   // ipco +0x0C
}
world::IPlayerControlObject* pcoInterfaceOf(world::IObject* object)
{
    return static_cast<world::IPlayerControlObject*>(object->queryInterface(world::IID_IPlayerControlObject));
}
} // namespace

// getAllPlayerControlObjects - 0x846C870, 163 B: the object itself if it answers IID_IPlayer
// ControlObject, then every child subtree (getChild / getSibling), depth-first, in tree order.
void NetworkManager::getAllPlayerControlObjects(world::IObject* object,
                                                std::vector<world::IPlayerControlObject*>& out)
{
    world::IPlayerControlObject* pco = pcoInterfaceOf(object);
    if (pco)
        out.push_back(pco);
    for (world::IObject* child = const_cast<world::IObject*>(object->getChild()); child;
         child = const_cast<world::IObject*>(child->getSibling()))
        getAllPlayerControlObjects(child, out);
}

// Appends networkable objects and descends through non-PCO children. Child PCOs belong to separate descriptors.
void NetworkManager::_analyseObject(world::IObject* object, std::vector<OBJECTINFO>& out)
{
    const world::IObjectTemplate* objTemplate = world::object_detail::LayoutAccess::objectTemplate(object);
    if (object->getNetworkable())
    {
        OBJECTINFO info;
        info.classId = objTemplate->getClassID();
        info.object  = object;
        out.push_back(info);
    }
    for (world::IObject* child = const_cast<world::IObject*>(object->getChild()); child;
         child = const_cast<world::IObject*>(child->getSibling()))
    {
        if (!child->queryInterface(world::IID_IPlayerControlObject))
            _analyseObject(child, out);
    }
}

// Returns whether the two object records may share a network property.
// Rotational bundles compare reset policy, input maps and movement limits within FLT_EPSILON.
bool NetworkManager::compareProperties(OBJECTINFO& a, OBJECTINFO& b)
{
    typedef world::object_detail::LayoutAccess L;
    const world::IObjectTemplate* ta = L::objectTemplate(a.object);
    const world::IObjectTemplate* tb = L::objectTemplate(b.object);
    if (ta == tb)
        return true;
    if (a.classId != b.classId)
    {
        const bool rotThenCamera = a.classId == world::CID_RotationalBundleTemplate && b.classId == world::CID_CameraTemplate;
        const bool cameraThenRot = a.classId == world::CID_CameraTemplate && b.classId == world::CID_RotationalBundleTemplate;
        if (!rotThenCamera && !cameraThenRot)
            return false;
    }
    if (!const_cast<world::IObjectTemplate*>(ta)->queryImplementation(world::CID_RotationalBundleTemplate))   // vptr +0x50
        return false;
    const world::RotationalBundleTemplate* ra = static_cast<const world::RotationalBundleTemplate*>(ta);
    const world::RotationalBundleTemplate* rb = static_cast<const world::RotationalBundleTemplate*>(tb);
    if (rb->m_automaticReset != ra->m_automaticReset)     return false;
    if (ra->getInputToYaw()   != rb->getInputToYaw())     return false;
    if (ra->getInputToPitch() != rb->getInputToPitch())   return false;
    if (ra->getInputToRoll()  != rb->getInputToRoll())    return false;
    const float eps = 1.1920929e-07f;                       // FLT_EPSILON, flt_8888D4C
    if (std::fabs(ra->m_maxRotation.x  - rb->m_maxRotation.x)  > eps) return false;
    if (std::fabs(ra->m_maxRotation.y  - rb->m_maxRotation.y)  > eps) return false;
    if (std::fabs(ra->m_maxRotation.z  - rb->m_maxRotation.z)  > eps) return false;
    if (std::fabs(ra->m_minRotation.x  - rb->m_minRotation.x)  > eps) return false;
    if (std::fabs(ra->m_minRotation.y  - rb->m_minRotation.y)  > eps) return false;
    if (std::fabs(ra->m_minRotation.z  - rb->m_minRotation.z)  > eps) return false;
    if (std::fabs(ra->m_maxSpeed.x     - rb->m_maxSpeed.x)     > eps) return false;
    if (std::fabs(ra->m_maxSpeed.y     - rb->m_maxSpeed.y)     > eps) return false;
    if (std::fabs(ra->m_maxSpeed.z     - rb->m_maxSpeed.z)     > eps) return false;
    if (std::fabs(ra->m_acceleration.x - rb->m_acceleration.x) > eps) return false;
    if (std::fabs(ra->m_acceleration.y - rb->m_acceleration.y) > eps) return false;
    if (std::fabs(ra->m_acceleration.z - rb->m_acceleration.z) > eps) return false;
    return true;
}

// Builds or reuses a template grouping plan, then registers this object's network descriptors.
// Returns the object networkable's id, or zero when it has no networkable. Property groups must fit twelve state masks.
unsigned short NetworkManager::analyseObject(world::IObject* object, unsigned short netId)
{
    typedef world::object_detail::LayoutAccess L;
    std::vector<world::IPlayerControlObject*> controlObjs;
    std::vector<OBJECTINFO> objInfos;

    if (!object->getNetworkable())
        return 0;

    const int templateId = static_cast<int>(L::objectTemplate(object)->getId());
    std::map<int, ObjectDescriptor>::iterator planIt = m_networkedTemplates.find(templateId);
    if (planIt == m_networkedTemplates.end())
    {
        // Phase 1: build the template's plan
        std::vector<DescDescriptor> plan;
        controlObjs.clear();
        objInfos.clear();
        getAllPlayerControlObjects(object, controlObjs);
        if (controlObjs.empty())
            controlObjs.push_back(0);                                   // "the object itself"

        for (unsigned int p = 0; p < controlObjs.size(); ++p)
        {
            world::IObject* pcoObject = controlObjs[p]
                ? pcoInterfaceToObject(controlObjs[p])                  // ipco -> queryInterface(IID_IObject)
                : static_cast<world::IObject*>(object->queryInterface(world::IID_IObject));
            objInfos.clear();
            if (pcoObject->getNetworkable())
            {
                _analyseObject(pcoObject, objInfos);
                std::vector<std::vector<int> > groups;
                for (unsigned int i = 0; i < objInfos.size(); ++i)
                {
                    world::IObject* o = objInfos[i].object;
                    // a projectile riding on a parent is not part of the plan
                    if (o && o->queryInterface(world::IID_IProjectileObject) && o->getParent())
                        continue;
                    INetworkable* net = o->getNetworkable();
                    if (net->m_networkableInfo->getForceNetworkableId())          // vptr +0x3C
                    {
                        if (o != object && o->getParent())
                            continue;                                    // a forced-id child: skipped
                    }
                    bool joined = false;
                    if (!net->m_networkableInfo->getIsUnique())                   // vptr +0x34
                    {
                        for (unsigned int g = 0; g < groups.size(); ++g)
                        {
                            if (compareProperties(objInfos[i], objInfos[groups[g][0]]))
                            {
                                groups[g].push_back(static_cast<int>(i));
                                joined = true;
                                break;
                            }
                        }
                    }
                    if (!joined)
                    {
                        std::vector<int> single;
                        single.push_back(static_cast<int>(i));
                        groups.push_back(single);
                    }
                }
                DescDescriptor desc;
                desc.dword0 = 0;                                         // never read (see the header)
                desc.groups = groups;
                plan.push_back(desc);
            }
            else
            {
                Debug(DEBUGTYPE_6, "IO/Network/NetworkManager.cpp", 706, __FUNCTION__)
                    << "PCO of template " << L::objectTemplate(pcoObject)->getName() << " is not networkable.";
            }
        }
        ObjectDescriptor objectDescriptor;
        objectDescriptor.pcoDescriptors = plan;
        m_networkedTemplates.insert(std::make_pair(templateId, objectDescriptor));
        planIt = m_networkedTemplates.find(templateId);
    }

    // Phase 2: this object's descriptors
    controlObjs.clear();
    objInfos.clear();
    getAllPlayerControlObjects(object, controlObjs);
    if (controlObjs.empty())
        controlObjs.push_back(0);
    unsigned short requestedId = netId;
    for (unsigned int p = 0; p < controlObjs.size(); ++p)
    {
        world::IObject* pcoObject = controlObjs[p]
            ? pcoInterfaceToObject(controlObjs[p])
            : static_cast<world::IObject*>(object->queryInterface(world::IID_IObject));
        objInfos.clear();
        _analyseObject(pcoObject, objInfos);
        const DescDescriptor& desc = planIt->second.pcoDescriptors[p];   // index-aligned, unchecked as shipped

        NetworkableDescriptor* descriptor = new NetworkableDescriptor;
        descriptor->controlObjId = 0;                                    // the ELF leaves +0/+4/+8/+0x18..
        descriptor->descId = 0;                                          //   +0x1F unwritten here;
        descriptor->dword8 = 0;                                          //   addDescriptor stamps them
        descriptor->addedToNetwork = false;
        descriptor->byte19 = descriptor->byte1A = descriptor->byte1B = false;
        descriptor->minimumBitSize = 0;
        for (unsigned int g = 0; g < desc.groups.size(); ++g)
        {
            UniqueNetProperty property;
            const std::vector<int>& members = desc.groups[g];
            for (unsigned int m = 0; m < members.size(); ++m)
                property.networkables.push_back(objInfos[members[m]].object->getNetworkable());
            descriptor->properties.push_back(property);
        }
        const unsigned int propertyCount = static_cast<unsigned int>(descriptor->properties.size());
        if (propertyCount != 0)
        {
            if (propertyCount > 12)
            {
                std::ostringstream text;
                text << "Object " << L::objectTemplate(pcoObject)->getName()
                     << " has to many statemasks (" << propertyCount
                     << ") and will crash the network. Please fix!";
                for (unsigned int g = 0; g < desc.groups.size(); ++g)
                {
                    text << std::endl;
                    world::IObject* first = objInfos[desc.groups[g][0]].object;
                    text << g << " unique networkableObject:" << L::objectTemplate(first)->getName();
                }
                text << std::endl;
                Debug(DEBUGTYPE_7, "IO/Network/NetworkManager.cpp", 842, __FUNCTION__) << text.str();
            }
            const unsigned short assigned = assignId(requestedId);
            if (assigned == 0)
            {
                Debug(DEBUGTYPE_7, "IO/Network/NetworkManager.cpp", 847, __FUNCTION__)
                    << "Failed to assign networkID:" << requestedId;
            }
            addDescriptor(descriptor, assigned);
            if (pcoObject == object)
                descriptor->addedToNetwork = true;
            requestedId = static_cast<unsigned short>(assigned + 1);
        }
        // A PCO with no properties leaves its newly allocated descriptor unreleased.
    }
    return object->getNetworkable()->m_networkableId;
}

// Removes descriptors for the object's PCOs, or its own networkable when no PCO exists.
void NetworkManager::removeDescriptors(world::IObject* object)
{
    std::vector<world::IPlayerControlObject*> controlObjs;
    getAllPlayerControlObjects(object, controlObjs);
    if (controlObjs.empty())
    {
        INetworkable* networkable = object->getNetworkable();
        if (networkable)
            deleteDescriptorForId(networkable->m_networkableId);
    }
    for (unsigned int p = 0; p < controlObjs.size(); ++p)
    {
        if (!controlObjs[p])
            continue;
        world::IObject* pcoObject = pcoInterfaceToObject(controlObjs[p]);
        if (!pcoObject)
            continue;
        INetworkable* networkable = pcoObject->getNetworkable();
        if (networkable)
            deleteDescriptorForId(networkable->m_networkableId);
    }
}

// Registers an unassigned networkable and returns its allocated ID. Existing registration triggers a diagnostic.
unsigned short NetworkManager::addNetworkable(INetworkable* networkable)
{
    const unsigned short netId = networkable->m_networkableId;
    if (netId == 0 && !getObject(0))
        return addNetworkable(networkable, 0);
    Debug(DEBUGTYPE_6, "IO/Network/NetworkManager.cpp", 174, __FUNCTION__)
        << "Networkable already added to network manager, netId=" << netId;
    return netId;
}

// Registers a networkable or its owning object and returns its id.
// Already registered objects and occupied ids emit a diagnostic and return the current id.

unsigned short NetworkManager::addNetworkable(INetworkable* networkable, unsigned short netId)
{
    if (networkable->m_networkableId != 0 || getObject(netId))
    {

        Debug(DEBUGTYPE_6, "IO/Network/NetworkManager.cpp", 233, __FUNCTION__)
            << "Networkable already added to network manager, netId=" << networkable->m_networkableId;
        return networkable->m_networkableId;
    }
    INetworkableObject* networkableObject =
        static_cast<INetworkableObject*>(networkable->queryInterface(IID_INetworkableObject));
    if (networkableObject)
        return addObject(objectOfNetworkableObject(networkableObject), netId);

    NetworkableDescriptor* descriptor = new NetworkableDescriptor;
    descriptor->controlObjId = 0;                // not written on this path (heap garbage in the ELF)
    descriptor->descId = 0;
    descriptor->dword8 = 0;
    descriptor->addedToNetwork = false;
    descriptor->byte19 = descriptor->byte1A = descriptor->byte1B = false;
    descriptor->minimumBitSize = 0;
    UniqueNetProperty property;
    property.networkables.push_back(networkable);
    descriptor->properties.push_back(property);

    const unsigned short assigned = assignId(netId);
    if (assigned == 0)
    {
        Debug(DEBUGTYPE_6, "IO/Network/NetworkManager.cpp", 262, __FUNCTION__)
            << formatString("Failed to assign networkID");
    }
    addDescriptor(descriptor, assigned);
    descriptor->addedToNetwork = true;
    return networkable->m_networkableId;
}

// Registers a descriptor, classifies its ghost visibility and notifies its networkables.
// Requires a nonempty property list. [UNRESOLVED] Name of the non-PCO policy check.
void NetworkManager::addDescriptor(NetworkableDescriptor* descriptor, unsigned short netId)
{
    INetworkable* first = descriptor->properties[0].networkables[0];
    first->m_networkableId = netId;
    descriptor->descId = netId;
    descriptor->addedToNetwork = false;
    descriptor->minimumBitSize = 0;

    INetworkableObject* networkableObject = first->m_networkableObject;
    if (networkableObject)
    {
        world::IObject* object = objectOfNetworkableObject(networkableObject);
        m_networkableObjects.insert(std::make_pair(netId, object));
        void* pco = object->queryInterface(world::IID_IPlayerControlObject);
        if (pco)
        {
            descriptor->controlObjId = static_cast<int>(object->getObjectCacheID());
        }
        else if (networkableInfoForcesId(object) || world::getRootParent(object) == object)
        {
            descriptor->controlObjId = -1;
        }
        else
        {
            const std::string& objectName =
                world::object_detail::LayoutAccess::objectTemplate(object)->getName();
            world::IObject* root = world::getRootParent(object);
            const std::string& rootName =
                world::object_detail::LayoutAccess::objectTemplate(root)->getName();
            Debug(DEBUGTYPE_6, "IO/Network/NetworkManager.cpp", 300, __FUNCTION__)
                << "Descriptors can only be assigned to PCO:s or RootObjects. RootObject = "
                << rootName << " Object = " << objectName;
        }
    }

    const unsigned int bucket = netId % 4096u;
    if (m_descriptorHash.exists(netId, bucket))
    {
        Debug(DEBUGTYPE_7, "IO/Network/NetworkManager.cpp", 319, __FUNCTION__)
            << "Duplicate descriptor detected:" << netId;
    }
    else
    {
        m_descriptorHash.insert(netId, descriptor, bucket);
    }
    m_descriptors.insert(std::make_pair(netId, descriptor));

    if (!networkableObject || first->m_basePriority < 0.0f)
    {
        m_staticDescriptors.insert(std::make_pair(netId, descriptor));
        if (game)
        {
            IGameServer* gameServer = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
            if (gameServer)
                gameServer->addStaticGhost(descriptor);
        }
    }
    else
    {
        m_scopableDescriptors.insert(std::make_pair(netId, descriptor));
        if (first->m_basePriority > 0.0f && game)
        {
            IGameServer* gameServer = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
            if (gameServer)
                gameServer->addScopableGhost(descriptor);
        }
    }

    descriptor->minimumBitSize = 0;
    for (std::vector<UniqueNetProperty>::iterator p = descriptor->properties.begin();
         p != descriptor->properties.end(); ++p)
    {
        // the ELF sums only each property's first networkable's getMinimumBitSize
        descriptor->minimumBitSize += static_cast<int>(p->networkables[0]->getMinimumBitSize());
    }
    callAddedToNetworkManager(descriptor);
}

// an object networkable's descriptor goes through removeDescriptors (the whole
// object tree); a plain one through deleteDescriptorForId.
void NetworkManager::removeNetworkable(unsigned short netId)
{
    NetworkableDescriptor* descriptor = getDescriptor(netId);
    if (!descriptor)
        return;
    INetworkable* first = descriptor->properties[0].networkables[0];
    INetworkableObject* networkableObject =
        static_cast<INetworkableObject*>(first->queryInterface(IID_INetworkableObject));
    if (networkableObject)
        removeDescriptors(objectOfNetworkableObject(networkableObject));   // [FRONTIER]
    else
        deleteDescriptorForId(netId);
}

// the counter restarts at max(642, highest id + 1).
unsigned short NetworkManager::regressNetworkableIds()
{
    int highest = -1;
    for (std::map<unsigned short, NetworkableDescriptor*>::iterator it = m_descriptors.begin();
         it != m_descriptors.end(); ++it)
    {
        if (static_cast<int>(it->first) >= highest)
            highest = it->first;
    }
    unsigned short next = 642;
    if (highest + 1 > 0x281)
        next = static_cast<unsigned short>(highest + 1);
    m_idCounter = next;
    return next;
}

// Returns the highest descriptor ID not excluded by its replicated object's cache flag 0x20.
// Descriptors with no networkable-object or object pointer still count. Test the object's flags,
// not NetworkableInfo::m_isUsed: this watermark controls client ID regression during reset.
int NetworkManager::getHighestUsedNetID()
{
    int highest = -1;
    for (std::map<unsigned short, NetworkableDescriptor*>::iterator it = m_descriptors.begin();
         it != m_descriptors.end(); ++it)
    {
        INetworkable* const first = it->second->properties[0].networkables[0];
        INetworkableObject* const networkableObject = first->m_networkableObject;   // +0x18
        bool excluded = false;
        if (networkableObject)
        {
            world::IObject* const object = networkableObject->m_object;             // +0x08
            if (object)
                excluded = (object->getFlags() & 0x20u) != 0;                       // +0x04 bit 0x20
        }
        if (!excluded && static_cast<int>(it->first) >= highest)
            highest = it->first;
    }

    return highest;
}

// Removes the descriptor from lookup and ghost lists, notifies its networkables and frees owned properties.
void NetworkManager::deleteDescriptorForId(unsigned short netId)
{
    NetworkableDescriptor* descriptor = getDescriptor(netId);
    m_networkableObjects.erase(netId);
    m_descriptors.erase(netId);
    m_descriptorHash.remove(netId);
    m_staticDescriptors.erase(netId);
    m_scopableDescriptors.erase(netId);
    if (game)
    {
        IGameServer* gameServer = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
        if (gameServer)
            gameServer->removeGhost(descriptor);
    }
    m_activeDescriptors.erase(netId);
    if (descriptor)
    {
        callRemovedFromNetworkManager(descriptor);
        delete descriptor;                       // frees the property vectors with it
    }
}

// insert-if-absent into the active map; a null draws Debug(6, 490).
void NetworkManager::addActiveDescriptor(NetworkableDescriptor* descriptor)
{
    if (!descriptor)
    {
        Debug(DEBUGTYPE_6, "IO/Network/NetworkManager.cpp", 490, __FUNCTION__)
            << "Trying to activate nonexisting desc";
        return;
    }
    if (m_activeDescriptors.find(descriptor->descId) == m_activeDescriptors.end())
        m_activeDescriptors.insert(std::make_pair(descriptor->descId, descriptor));
}

// / 0x846A840, 274 B (Debug(6, 512) on a null).
void NetworkManager::removeActiveDescriptor(unsigned short netId)
{
    m_activeDescriptors.erase(netId);
}
void NetworkManager::removeActiveDescriptor(NetworkableDescriptor* descriptor)
{
    if (!descriptor)
    {
        Debug(DEBUGTYPE_6, "IO/Network/NetworkManager.cpp", 512, __FUNCTION__)
            << "Trying to deactivate nonexisting desc";
        return;
    }
    removeActiveDescriptor(descriptor->descId);
}

// id 0 -> null; else the descriptor's networkable's INetworkableObject's object.
world::IObject* NetworkManager::getObject(unsigned short netId)
{
    if (netId == 0)
        return 0;
    NetworkableDescriptor* descriptor = getDescriptor(netId);
    if (!descriptor)
        return 0;
    INetworkableObject* networkableObject =
        descriptor->properties[0].networkables[0]->m_networkableObject;
    return networkableObject ? objectOfNetworkableObject(networkableObject) : 0;
}

std::map<unsigned short, world::IObject*>& NetworkManager::getObjectMap()               { return m_networkableObjects; }
std::map<unsigned short, NetworkableDescriptor*>& NetworkManager::getDescriptorMap()    { return m_descriptors; }
std::list<std::list<std::pair<INetworkableObject*, NetworkableDescriptor*> > >&
                                                  NetworkManager::getScopableRootDescriptors() { return m_scopableRootDescriptors; }
std::map<unsigned short, NetworkableDescriptor*>& NetworkManager::getStaticDescriptors() { return m_staticDescriptors; }

// the hash chain walk (Core/Hash.h's get).
NetworkableDescriptor* NetworkManager::getDescriptor(unsigned short netId)
{
    NetworkableDescriptor* descriptor = 0;
    if (m_descriptorHash.get(netId, descriptor))
        return descriptor;
    return 0;
}

void NetworkManager::setGlobalPredictionMode(int mode) { m_globalPredictionMode = mode; }
int  NetworkManager::getGlobalPredictionMode() const   { return m_globalPredictionMode; }

// each: every networkable of every property.
void NetworkManager::callAddedToNetworkManager(NetworkableDescriptor* descriptor)
{
    if (!descriptor)
        return;
    for (std::vector<UniqueNetProperty>::iterator p = descriptor->properties.begin();
         p != descriptor->properties.end(); ++p)
    {
        for (std::vector<INetworkable*>::iterator n = p->networkables.begin(); n != p->networkables.end(); ++n)
            (*n)->addedToManager();              // vptr +0x20
    }
}
void NetworkManager::callRemovedFromNetworkManager(NetworkableDescriptor* descriptor)
{
    if (!descriptor)
        return;
    for (std::vector<UniqueNetProperty>::iterator p = descriptor->properties.begin();
         p != descriptor->properties.end(); ++p)
    {
        for (std::vector<INetworkable*>::iterator n = p->networkables.begin(); n != p->networkables.end(); ++n)
            (*n)->removedFromManager();          // vptr +0x24
    }
}


// Rebuilds changed-descriptor masks and scopable root groups for this frame's replication work.
// Each property writes one dword into a twelve-dword mask; callers must not exceed twelve properties.
// Scoping requires an active ghost and positive priority. Excludes child control objects that are
// unarmored and unoccupied, removes kit-template descriptors from the scopable set, and groups
// survivors by root object ID.
namespace {

// The twelve-way or the ELF emits at 0x846CC00 and again in the scopable pass.
inline bool stateMaskIsNonZero(const NetworkableStateMask& mask)
{
    return (mask.m_opaque[0]  | mask.m_opaque[1]  | mask.m_opaque[2]  | mask.m_opaque[3]
          | mask.m_opaque[4]  | mask.m_opaque[5]  | mask.m_opaque[6]  | mask.m_opaque[7]
          | mask.m_opaque[8]  | mask.m_opaque[9]  | mask.m_opaque[10] | mask.m_opaque[11]) != 0;
}

// The per-descriptor pass both halves share: zero a mask, then let every property write its own
// dword into it. The slot pointer is unbounded - see the note above.
void buildStateMask(NetworkableDescriptor* descriptor, NetworkableStateMask& mask)
{
    std::memset(&mask, 0, sizeof(mask));

    int* slot = mask.m_opaque;
    for (std::vector<UniqueNetProperty>::iterator p = descriptor->properties.begin();
         p != descriptor->properties.end(); ++p)
    {
        INetworkable* const networkable = *p->networkables.begin();
        networkable->updateStateMask(reinterpret_cast<unsigned int*>(slot));
        ++slot;
    }
}

} // namespace

std::vector<std::pair<const NetworkableDescriptor*, NetworkableStateMask> >*
NetworkManager::updateStateMasks()
{
    typedef std::map<unsigned short, NetworkableDescriptor*>::iterator DescIt;
    typedef std::list<std::pair<INetworkableObject*, NetworkableDescriptor*> > RootList;

    // 1. both outputs are rebuilt from scratch every frame
    m_networkableStateMasks.clear();
    m_scopableRootDescriptors.clear();

    // 2. the static descriptors (+0x2C)
    for (DescIt it = m_staticDescriptors.begin(); it != m_staticDescriptors.end(); ++it)
    {
        NetworkableDescriptor* const descriptor = it->second;

        NetworkableStateMask mask;
        buildStateMask(descriptor, mask);

        if (stateMaskIsNonZero(mask))
            m_networkableStateMasks.push_back(
                std::make_pair(static_cast<const NetworkableDescriptor*>(descriptor), mask));
    }

    // 3. the scopable descriptors (+0x38)
    // The root map is local to this call - it only exists to find the list a root already owns.
    std::map<unsigned int, RootList*> rootLists;

    DescIt it = m_scopableDescriptors.begin();
    while (it != m_scopableDescriptors.end())
    {
        NetworkableDescriptor* const descriptor = it->second;

        INetworkable* const networkable = *descriptor->properties.begin()->networkables.begin();
        INetworkableObject* const netObject = networkable->m_networkableObject;   // +0x18
        world::IObject* const object = netObject->m_object;                       // +0x08

        if (object->getFlags() & 1)              // bit 0 of +0x04 skips the whole body
        {
            ++it;
            continue;
        }

        if (object->getTemplate()->getClassID() == world::CID_KitTemplate)
        {
            m_scopableDescriptors.erase(it++);   // erased, not skipped - see the note above
            continue;
        }

        NetworkableStateMask mask;
        buildStateMask(descriptor, mask);

        if (stateMaskIsNonZero(mask))
            m_networkableStateMasks.push_back(
                std::make_pair(static_cast<const NetworkableDescriptor*>(descriptor), mask));

        // the include rule - see the note above
        world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(object->queryInterface(world::IID_IPlayerControlObject));
        const bool include = (object->getParent() == 0)
                          || (pco == 0)
                          || (object->getArmorComponent() != 0)
                          || pco->isOccupied();

        if (include && networkable->m_ghostActive && networkable->m_basePriority > 0.0f)
        {
            const unsigned int rootId = world::getRootParent(object)->getObjectId();

            RootList* list = 0;
            const std::map<unsigned int, RootList*>::iterator found = rootLists.find(rootId);
            if (found == rootLists.end())
            {
                m_scopableRootDescriptors.push_front(RootList());   // inserted at begin
                list = &m_scopableRootDescriptors.front();
                rootLists.insert(std::make_pair(rootId, list));
            }
            else
            {
                list = found->second;
            }

            list->push_back(std::make_pair(netObject, descriptor));
        }

        ++it;
    }

    return &m_networkableStateMasks;
}

} // namespace io
} // namespace hfe
} // namespace dice
