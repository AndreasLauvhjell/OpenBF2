// Path:     [INFERRED] - see ObjectTemplate.h header block.
// Contents: recovered from bf2.i64 (ELF, symbolised). 0x8320eb0-0x8322610 plus the small accessor
// block at 0x81174a0-0x81177xx.
// Reconstructed here: constructor/destructor, component ownership and lookup, object lifecycle
// notifications, flags, binary I/O, implementation routing, physics-removal hooks, preCache and
// trivial accessors.
// Explicit placeholder: makeScript (0x8321410, the deferred .con writer).

#include "ObjectTemplate.h"
#include "ObjectLayoutAccess.h"
// before IObjectTemplateComponent.h. That header declares two of these ids `extern`, and an
// extern declaration seen first turns Ids.h's definitions into external-linkage ones that collide
// with Globals.cpp at link time.
#include "../../Core/Ids.h"
#include "IObjectTemplateComponent.h"
#include "../../Core/StringUtil.h"   // getEmptyString - the shared empty string the stubs return
#include "../../Physics/PhysicsNodeManager.h"
#include "../../Physics/ResponsePhysicsManager.h"
#include "../../Core/ClassManager.h"
#include "../../IO/Console/IConsoleSaveable.h"
#include "../../IO/Stream/IStream.h"
#include "../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"

#include <cstring>
#include <sstream>

namespace dice {
namespace hfe {

extern ClassManager* classManager;      // ds:0x8AC85A0, defined in Core/Globals.cpp

namespace world {

extern const unsigned int CID_ObjectTemplate;

// Physics managers and interface ids used when detaching components.
extern PhysicsNodeManager*     physicsNodeManager;          // ds:0x8B27844
                                                            // _ZN4dice3hfe5world18physicsNodeManagerE
extern ResponsePhysicsManager* responsePhysicsManager;      // ds:0x8B27848
                                                            // _ZN4dice3hfe5world22responsePhysicsManagerE

extern const unsigned int IID_IPhysicsNode;
extern const unsigned int IID_IResponsePhysics;

// Initializes empty template state with a cull-radius scale of 1.
ObjectTemplate::ObjectTemplate()
    : m_templateId(0)
    , m_flags(0)
    , m_cullRadiusScale(1.0f)
    , m_specialMaterials(0)
{
    // m_templateName and m_networkableInfo default-construct to empty.
    // m_componentMap is allocated here (operator new(0x18) - a 24-byte _Rb_tree header).
}

// Attaches the queried template-component interface; a null component removes the entry.
// Calls setTemplate before replacing the old entry, releases the previous component,
// and adds a reference through the original incoming pointer.
void ObjectTemplate::setComponent(unsigned int classId, IBase* component)
{
    if (!component)
    {
        m_componentMap.erase(classId);                 // NULL means remove
        return;
    }

    IObjectTemplateComponent* comp = static_cast<IObjectTemplateComponent*>(
        component->queryInterface(IID_IObjectTemplateComponent));

    comp->setTemplate(this);                            // +0x24 before insertion

    ComponentMap::iterator slot = m_componentMap.lower_bound(classId);
    if (slot == m_componentMap.end() || classId < slot->first)
        slot = m_componentMap.insert(slot,
                   std::make_pair(classId, SmartPtr<IObjectTemplateComponent>()));

    slot->second.assignQueriedInterface(comp, component);
                                                        // retain through the original, not `comp`
}

// Returns the held template component for classId, or null when absent.
IObjectTemplateComponent* ObjectTemplate::queryComponent(unsigned int classId)
{
    ComponentMap::iterator it = m_componentMap.find(classId);
    return (it == m_componentMap.end()) ? 0 : it->second.get();
}

// Notifies each template component, creates its runtime component and attaches it under the runtime class id.
// Releases the temporary reference after attachment. A null factory result is allowed.
void ObjectTemplate::objectCreated(IObject* obj)
{
    for (ComponentMap::iterator it = m_componentMap.begin(); it != m_componentMap.end(); ++it)
    {
        IObjectTemplateComponent* c = static_cast<IObjectTemplateComponent*>(
            it->second->queryInterface(IID_IObjectTemplateComponent));
        if (!c)
            continue;

        c->objectCreated(obj);                          // +0x1C

        IObjectComponent* inst = c->createComponent();  // +0x18
        if (inst)
        {
            obj->setComponent(inst->getInterfaceID(), inst);
            inst->release();                            // the object now holds it
        }
    }
}

// Notifies template components and detaches using their IObjectComponent interface IDs.
// The detach ID may differ from the created runtime component's attach ID.
void ObjectTemplate::objectDestroyed(IObject* obj)
{
    for (ComponentMap::iterator it = m_componentMap.begin(); it != m_componentMap.end(); ++it)
    {
        IBase* held = it->second.get();

        IObjectTemplateComponent* c = static_cast<IObjectTemplateComponent*>(
            held->queryInterface(IID_IObjectTemplateComponent));
        if (!c)
            continue;

        c->objectDestroyed(obj);                        // +0x20

        IObjectComponent* oc = static_cast<IObjectComponent*>(
            held->queryInterface(IID_IObjectComponent));
        if (oc)
            obj->setComponent(oc->getInterfaceID(), 0); // detach by the runtime id
    }
}

// Applies set before clear and returns the resulting flags; clear wins when masks overlap.
unsigned int ObjectTemplate::updateFlags(unsigned int set, unsigned int clear)
{
    m_flags = ~clear & (set | m_flags);
    return m_flags;
}

// Reads only networkable info and flags; other state is authored through .con scripts.
bool ObjectTemplate::readBinary(io::IStream* stream)
{
    io::read(stream, m_networkableInfo);
    stream->read(&m_flags, 4);                          // IStream vtable +0x10
    return true;
}

// Returns this for ObjectTemplate's class ID, otherwise null.
void* ObjectTemplate::queryImplementation(unsigned int classId)
{
    return (classId == CID_ObjectTemplate) ? this : 0;
}

// Scans components by class id and returns the matching queried interface, or null.
// The component map uses a different key, so this is a linear lookup.
IObjectTemplateComponent* ObjectTemplate::getComponentFromClassID(unsigned int classId) const
{
    for (ComponentMap::const_iterator it = m_componentMap.begin(); it != m_componentMap.end(); ++it)
    {
        IObjectTemplateComponent* comp = static_cast<IObjectTemplateComponent*>(
            it->second->queryInterface(IID_IObjectTemplateComponent));   // +0x0C
        if (comp && comp->getClassID() == classId)                       // +0x10
            return comp;
    }
    return 0;
}

// Writes the networkable-info string and flags through their virtual getters, then returns true.
bool ObjectTemplate::writeBinary(io::IStream* stream) const
{
    const std::string& info = getNetworkableInfo();     // vtable +0x94
    unsigned int length = (unsigned int)info.size();    // from _Rep, at data[-0x0C]

    stream->write(&length, 4);                          // IStream vtable +0x14
    stream->write(info.data(), length);

    unsigned int flags = getFlags();                    // vtable +0x48
    stream->write(&flags, 4);
    return true;
}

// Returns the component to its manager before detaching it, even when the manager is absent.
// PhysicsNodeManager receives the node; ResponsePhysicsManager receives the object.
// Use shared field access so native builds do not depend on retail x86 offsets.

void ObjectTemplate::removePhysicsNodeComponent(IObject* obj) const
{
    IPhysicsNode* node = static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(obj));
    if (!node)
        return;

    if (physicsNodeManager)                                              // ds:0x8B27844
        physicsNodeManager->removePhysicsNode(node);                     // ELF +0x20 / PE +0x1C

    obj->setComponent(IID_IPhysicsNode, 0);                              // IObject vtable +0x58
}

void ObjectTemplate::removeResponsePhysicsComponent(IObject* obj) const
{
    if (!object_detail::LayoutAccess::responsePhysics(obj))
        return;

    // Handed the object, not the component - see above. This is the one place the two functions
    // genuinely differ, and it is because ResponsePhysicsManager indexes by object.
    if (responsePhysicsManager)                                          // ds:0x8B27848
        responsePhysicsManager->removeObject(obj);                       // ELF +0x1C / PE +0x18

    obj->setComponent(IID_IResponsePhysics, 0);                          // IObject vtable +0x58
}

// Precaching is a process-wide mode shared by all templates.
namespace {
bool g_isPreCaching = false;                            // ds:0x8AB0AB4
}

void ObjectTemplate::setIsPreCaching(bool on) { g_isPreCaching = on; }
bool ObjectTemplate::getIsPreCaching()        { return g_isPreCaching; }

// THE plain field accessors. Offsets read from the instruction, not inferred.
unsigned int ObjectTemplate::getClassID() const   { return CID_ObjectTemplate; }

void ObjectTemplate::setName(const std::string& v) { m_templateName.assign(v); }   // +0x0C
const std::string& ObjectTemplate::getName() const { return m_templateName; }

void ObjectTemplate::setNetworkableInfo(const std::string& v) { m_networkableInfo.assign(v); }  // +0x10
const std::string& ObjectTemplate::getNetworkableInfo() const { return m_networkableInfo; }

void ObjectTemplate::setId(unsigned int v)   { m_templateId = v; }                 // +0x14
unsigned int ObjectTemplate::getId() const   { return m_templateId; }
unsigned int ObjectTemplate::getFlags() const { return m_flags; }                  // +0x18

ObjectTemplate::ComponentMap* ObjectTemplate::getComponentMap() { return &m_componentMap; }  // +0x1C

void  ObjectTemplate::setCullRadiusScale(float v) { m_cullRadiusScale = v; }       // +0x28
float ObjectTemplate::getCullRadiusScale() const  { return m_cullRadiusScale; }

void ObjectTemplate::setSpecialMaterials(unsigned int v) { m_specialMaterials = v; }  // +0x2C
unsigned int ObjectTemplate::getSpecialMaterials() const { return m_specialMaterials; }

// Default template hooks intentionally do nothing or return empty values.
// Returned empty-string and zero-vector references point to shared storage.
void ObjectTemplate::templateInitialize()                     {}
void ObjectTemplate::setCreator(const std::string&)           {}
void ObjectTemplate::setModifiedByUser(const std::string&)    {}
// IObjectTemplate's inherited set/getCollisionMesh defaults now live in IObjectTemplate.cpp.
void ObjectTemplate::setAnchor(const Vec3&)                   {}
void ObjectTemplate::setAnchorOffset(const Vec3&)             {}
void ObjectTemplate::setIsOvergrowth(bool)                    {}
void ObjectTemplate::setMaterial(int)                         {}

void ObjectTemplate::setPhysicsComponent(IObject*) const         {}
void ObjectTemplate::removePhysicsComponent(IObject*) const      {}
void ObjectTemplate::setPhysicsNodeComponent(IObject*) const     {}
void ObjectTemplate::setResponsePhysicsComponent(IObject*) const {}
void ObjectTemplate::setGameLogicComponents(IObject*)            {}
void ObjectTemplate::removeGameLogicComponents(IObject*) const   {}
void ObjectTemplate::setHudComponents(IObject*)                  {}
void ObjectTemplate::removeHudComponents(IObject*) const         {}

const std::string& ObjectTemplate::getCreator() const         { return getEmptyString(); }
const std::string& ObjectTemplate::getModifiedByUser() const  { return getEmptyString(); }

const Vec3& ObjectTemplate::getAnchor() const        { return Vec3::getZero(); }
const Vec3& ObjectTemplate::getAnchorOffset() const  { return Vec3::getZero(); }
// BY value, unlike the two above - 0x8117650 copies the three floats into the caller's sret
// buffer and ends `retn 4`. Returning a reference here would be a different ABI.
Vec3 ObjectTemplate::getAnchorPos() const            { return Vec3::getZero(); }

bool ObjectTemplate::getIsOvergrowth() const         { return false; }
int  ObjectTemplate::getMaterial() const             { return 0; }
std::vector<std::pair<std::string, unsigned int> >* ObjectTemplate::getMaterialVector()
{
    return 0;
}

// Member destructors release the component map and strings; no additional cleanup is needed.
ObjectTemplate::~ObjectTemplate()
{
}

// Precaches by creating, initializing and force-destroying a temporary object.
// Returns early when the template flag is set; Object::init sets that flag after successful creation.
void ObjectTemplate::preCache() const
{
    if (getFlags() & 0x20)                  // vptr+0x48, virtual getFlags - 0x8322631
        return;

    IObject* obj = createObject();          // +0x54
    if (!obj)
        return;

    // Cross-port vtable recovery identifies these as init() and forceDestroy().  Object creation
    // itself performs the template notification; the throwaway object is initialized normally and
    // then enters the same forced deferred-destruction path as other runtime objects.
    obj->init();
    obj->forceDestroy();
}


// Writes template settings and component scripts. Most templates emit activeSafe; eligible editor-created templates emit create.
// Returns false for a short primary write or failed component serialization. Later write counts are ignored.
// A failed component leaves its BeginComp section unclosed; networkable-info presence is tested on the field before calling its getter.
bool ObjectTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;

    // Bit 7 marks templates created in the editor.
    const bool activeSafe = (getFlags() & 0x80) == 0 ||
                            (getFlags() & 1) != 0 ||
                            (getFlags() & 0x400) != 0;

    const std::string& templateName = getName();
    const std::string& className =
        objectTemplateManager->getClassNameFromTemplateClassID(getClassID());

    script << (activeSafe ? "ObjectTemplate.activeSafe " : "ObjectTemplate.create ")
           << className << " " << templateName << std::endl;

    if (!m_networkableInfo.empty())
        script << "ObjectTemplate.setNetworkableInfo " << getNetworkableInfo() << std::endl;

    if (getFlags() & 0x0001) script << "ObjectTemplate.saveInSeparateFile 1" << std::endl;
    if (getFlags() & 0x0002) script << "ObjectTemplate.createNotInGrid 1" << std::endl;
    if (getFlags() & 0x0004) script << "ObjectTemplate.createInvisible 1" << std::endl;
    if (getFlags() & 0x0008) script << "ObjectTemplate.createVisibleInEditor 1" << std::endl;
    if (getFlags() & 0x0010) script << "ObjectTemplate.isNotSaveable 1" << std::endl;
    if ((getFlags() & 0x80) != 0)          // `test al,al` again: ELF 0x832169E (js), PE 0x6DA1DC (jns)
        script << "ObjectTemplate.createdInEditor 1" << std::endl;
    if (getFlags() & 0x0200) script << "ObjectTemplate.isDestructable 1" << std::endl;
    if (getFlags() & 0x0800) script << "ObjectTemplate.castsDynamicShadow 1" << std::endl;
    if (getFlags() & 0x1000) script << "ObjectTemplate.hasRelativeOffset 1" << std::endl;

    const std::string text = script.str();
    if (stream->write(text.c_str(), static_cast<unsigned int>(text.length())) != text.length())
        return false;

    for (ComponentMap::iterator it = m_componentMap.begin(); it != m_componentMap.end(); ++it)
    {
        IBase* component = reinterpret_cast<IBase*>(it->second.get());

        void* templateComponent = component->queryInterface(IID_IObjectTemplateComponent);
        if (!templateComponent)
            continue;

        void* saveable = component->queryInterface(io::IID_IConsoleSaveable);
        if (!saveable)
            continue;

        std::string qualifiedName;
        if (!classManager->getClassName(
                static_cast<IObjectTemplateComponent*>(templateComponent)->getClassID(),
                qualifiedName))
            continue;

        const std::string name = qualifiedName.substr(qualifiedName.rfind(".") + 1);

        std::ostringstream header;
        header << "rem ---BeginComp:" << name << " ---" << std::endl;
        header << "ObjectTemplate.createComponent " << name << std::endl;

        const std::string headerText = header.str();
        stream->write(headerText.c_str(), static_cast<unsigned int>(headerText.length()));

        if (!static_cast<io::IConsoleSaveable*>(saveable)->makeScript(stream))
            return false;

        static const char endMarker[] = "rem ---EndComp ---\n";
        stream->write(endMarker, static_cast<unsigned int>(std::strlen(endMarker)));
    }

    return true;
}

} // namespace world
} // namespace hfe
} // namespace dice
