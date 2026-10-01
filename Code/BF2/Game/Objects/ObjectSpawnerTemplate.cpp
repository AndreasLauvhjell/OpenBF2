// Object-spawner defaults, construction, and script serialization.

#include "ObjectSpawnerTemplate.h"
#include "../../IO/Stream/IStream.h"
#include <sstream>

#include "../../Core/Ids.h"
#include "Game/ObjectSpawner.h"                  // createObject's product
#include "../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"   // setObjectTemplate's resolve
#include "../IGameLogic.h"                       // gameLogic - addPreStoredTemplate, noVehicles pair
#include "../GameServer/ServerSettings.h"        // serverSettings->getNoVehicles

namespace dice {
namespace hfe {
namespace world {

// The constructor - 0x8190EC0 (C2) / 0x8191000 (C1), 316 bytes, read end to end.
ObjectSpawnerTemplate::ObjectSpawnerTemplate()
{
    m_refCount = 1;                              // +0x168
    // m_effectOnSpawn (+0x16C): default-constructed empty (_S_empty_rep).
    // m_objectTemplates[0..2] (+0x170..+0x187): the ctor's three-iteration loop - each slot's
    // resolved pointer zeroed, each name empty.
    for (int i = 0; i < 3; ++i)
        m_objectTemplates[i].resolved = 0;

    m_minSpawnDelay = 30.0f;                     // +0x188
    m_maxSpawnDelay = 60.0f;                     // +0x18C
    m_timeToLive = 60.0f;                        // +0x190
    m_distance = 50.0f;                          // +0x194
    m_team = 0;                                  // +0x198
    m_nrOfObjectToSpawn = -1;                    // +0x19C "spawn every filled slot"
    m_maxNrOfObjectSpawned = 1;                  // +0x1A0
    m_radius = 5.0f;                             // +0x1A4
    m_useButtonRadius = 0.0f;                    // +0x1A8
    m_spawnOffset.x = 0.0f;                      // +0x1AC
    m_spawnOffset.y = 0.0f;
    m_spawnOffset.z = 0.0f;
    m_damageWhenLost = 50.0f;                    // +0x1B8
    m_controlPointId = -1;                       // +0x1BC the console's raw `controlPointId`
    m_holdObject = false;                        // +0x1C0
    m_teamOnVehicle = false;                     // +0x1C1
    m_byte1C2 = false;                           // +0x1C2 [UNRESOLVED role]
    m_spawnDelayAtStart = false;                 // +0x1C3
    m_activeOnStart = true;                      // +0x1C4

    updateFlags(0x50, 0);                        // bits 4+6 - the SpawnPointTemplate pair again
}

ObjectSpawnerTemplate::~ObjectSpawnerTemplate() {}

void ObjectSpawnerTemplate::addRef()  { ++m_refCount; }
int  ObjectSpawnerTemplate::getRef()  { return m_refCount; }
int  ObjectSpawnerTemplate::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

// , five cases - written out, not forwarded: the base's version has no
// IID_IObjectSpawnerTemplate -> +0x164 arm, and "same size as some base" proves nothing (the
// ControlPointTemplate lesson).
void* ObjectSpawnerTemplate::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)            return const_cast<ObjectSpawnerTemplate*>(this);
    if (iid == IID_IObjectTemplate)  return const_cast<ObjectSpawnerTemplate*>(this);
    if (iid == IID_ISimpleObjectTemplate)
        return static_cast<ISimpleObjectTemplate*>(const_cast<ObjectSpawnerTemplate*>(this));
    if (iid == IID_IObjectSpawnerTemplate)
        return const_cast<void**>(&pVMT_IObjectSpawnerTemplate);         // this + 0x164
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<ObjectSpawnerTemplate*>(this));
    return 0;
}

unsigned int ObjectSpawnerTemplate::getClassID() const { return CID_ObjectSpawnerTemplate; }

// the flat three-cid shape, no base call, exactly GrapplingHookRopeTemplate's.
void* ObjectSpawnerTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_ObjectSpawnerTemplate ||
        classId == CID_SimpleObjectTemplate ||
        classId == CID_ObjectTemplate)
        return this;
    return 0;
}

// Creates an object spawner using this template.
IObject* ObjectSpawnerTemplate::createObject() const
{
    return new ObjectSpawner(this);
}

// . Omitted fields and the mixed-case keys are retail behavior.
bool ObjectSpawnerTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream out;
    if (!SimpleObjectTemplate::makeScript(stream)) return false;
    for (unsigned int i = 0; i < 3; ++i)
        if (!m_objectTemplates[i].name.empty()) out << "ObjectTemplate.setObjectTemplate " << i << " " << m_objectTemplates[i].name << std::endl;
    if (m_minSpawnDelay != 30.0f) out << "ObjectTemplate.minSpawnDelay " << m_minSpawnDelay << std::endl;
    if (m_maxSpawnDelay != 60.0f) out << "ObjectTemplate.maxSpawnDelay " << m_maxSpawnDelay << std::endl;
    if (m_spawnDelayAtStart) out << "ObjectTemplate.spawnDelayAtStart " << m_spawnDelayAtStart << std::endl;
    if (m_timeToLive != 60.0f) out << "ObjectTemplate.TimeToLive " << m_timeToLive << std::endl;
    if (m_distance != 50.0f) out << "ObjectTemplate.Distance " << m_distance << std::endl;
    if (m_nrOfObjectToSpawn != -1) out << "ObjectTemplate.nrOfObjectToSpawn " << m_nrOfObjectToSpawn << std::endl;
    if (m_maxNrOfObjectSpawned != 1) out << "ObjectTemplate.maxNrOfObjectSpawned " << m_maxNrOfObjectSpawned << std::endl;
    if (m_radius != 5.0f) out << "ObjectTemplate.radius " << m_radius << std::endl;
    if (m_useButtonRadius != 0.0f) out << "ObjectTemplate.useButtonRadius " << m_useButtonRadius << std::endl;
    if (m_spawnOffset.x != 0.0f || m_spawnOffset.y != 0.0f || m_spawnOffset.z != 0.0f)
        out << "ObjectTemplate.spawnOffset " << m_spawnOffset.x << '/' << m_spawnOffset.y << '/' << m_spawnOffset.z << std::endl;
    if (m_team) out << "ObjectTemplate.team " << m_team << std::endl;
    if (m_holdObject) out << "ObjectTemplate.holdObject " << m_holdObject << std::endl;
    if (m_teamOnVehicle) out << "ObjectTemplate.teamOnVehicle " << m_teamOnVehicle << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

// concatenate the three nonempty names, each with a newline.
std::string ObjectSpawnerTemplate::getObjectTemplate() const
{
    std::stringstream out;
    for (unsigned int i = 0; i < 3; ++i)
        if (!m_objectTemplates[i].name.empty()) out << m_objectTemplates[i].name << std::endl;
    return out.str();
}

// no index guard and no newline in the indexed overload.
std::string ObjectSpawnerTemplate::getObjectTemplate(unsigned int index) const
{
    std::stringstream out;
    if (!m_objectTemplates[index].name.empty()) out << m_objectTemplates[index].name;
    return out.str();
}

// Assigns and resolves the named spawn-slot template.
// When either no-vehicles policy applies, omitted names are replaced with dummy before storage.
void ObjectSpawnerTemplate::setObjectTemplate(unsigned int index, std::string name)
{
    if (serverSettings->getNoVehicles() || gameLogic->getNoVehiclesMode())
    {
        if (gameLogic->isOmitted(name))
            name.assign("dummy");
    }

    SpawnSlot& slot = m_objectTemplates[index];
    slot.name.assign(name);
    if (!slot.name.empty())
        slot.resolved = objectTemplateManager->getTemplate(slot.name);
}

// a pure forwarder into slot 0, through the vtable as shipped.
void ObjectSpawnerTemplate::setObjectTemplate(std::string name)
{
    setObjectTemplate(0u, name);
}

void  ObjectSpawnerTemplate::setSpawnDelay(float v)      { m_minSpawnDelay = v; m_maxSpawnDelay = v; }
// byte-identical to setSpawnDelay in the ELF - a shipped alias, not a getter (the old
// declaration's "no get prefix" guess). Both write both delays.
void  ObjectSpawnerTemplate::spawnDelay(float v)         { m_minSpawnDelay = v; m_maxSpawnDelay = v; }
void  ObjectSpawnerTemplate::setMaxSpawnDelay(float v)   { m_maxSpawnDelay = v; }
float ObjectSpawnerTemplate::getMaxSpawnDelay()          { return m_maxSpawnDelay; }
void  ObjectSpawnerTemplate::setMinSpawnDelay(float v)   { m_minSpawnDelay = v; }
float ObjectSpawnerTemplate::getMinSpawnDelay()          { return m_minSpawnDelay; }
void  ObjectSpawnerTemplate::setSpawnDelayAtStart(bool v){ m_spawnDelayAtStart = v; }
bool  ObjectSpawnerTemplate::getSpawnDelayAtStart()      { return m_spawnDelayAtStart; }
void  ObjectSpawnerTemplate::setTimeToLive(float v)      { m_timeToLive = v; }
float ObjectSpawnerTemplate::getTimeToLive()             { return m_timeToLive; }
void  ObjectSpawnerTemplate::setDistance(float v)        { m_distance = v; }
float ObjectSpawnerTemplate::getDistance()               { return m_distance; }
void  ObjectSpawnerTemplate::setTeam(unsigned int v)     { m_team = v; }
unsigned int ObjectSpawnerTemplate::getTeam()            { return m_team; }
void  ObjectSpawnerTemplate::setActiveOnStart(bool v)    { m_activeOnStart = v; }
bool  ObjectSpawnerTemplate::getActiveOnStart()          { return m_activeOnStart; }
void  ObjectSpawnerTemplate::setNrOfObjectToSpawn(int v) { m_nrOfObjectToSpawn = v; }
int   ObjectSpawnerTemplate::getNrOfObjectToSpawn()      { return m_nrOfObjectToSpawn; }
void  ObjectSpawnerTemplate::setRadius(float v)          { m_radius = v; }
float ObjectSpawnerTemplate::getRadius()                 { return m_radius; }
void  ObjectSpawnerTemplate::setMaxNrOfObjectSpawned(unsigned int v) { m_maxNrOfObjectSpawned = v; }
unsigned int ObjectSpawnerTemplate::getMaxNrOfObjectSpawned()        { return m_maxNrOfObjectSpawned; }
void  ObjectSpawnerTemplate::setUseButtonRadius(float v) { m_useButtonRadius = v; }
float ObjectSpawnerTemplate::getUseButtonRadius()        { return m_useButtonRadius; }
void  ObjectSpawnerTemplate::setEffectOnSpawn(std::string v)   { m_effectOnSpawn.assign(v); }
const std::string& ObjectSpawnerTemplate::getEffectOnSpawn()   { return m_effectOnSpawn; }
void  ObjectSpawnerTemplate::setSpawnOffset(Vec3 v)      { m_spawnOffset = v; }
Vec3  ObjectSpawnerTemplate::getSpawnOffset()            { return m_spawnOffset; }
void  ObjectSpawnerTemplate::setDamageWhenLost(float v)  { m_damageWhenLost = v; }
float ObjectSpawnerTemplate::getDamageWhenLost()         { return m_damageWhenLost; }
void  ObjectSpawnerTemplate::setHoldObject(bool v)       { m_holdObject = v; }
bool  ObjectSpawnerTemplate::getHoldObject()             { return m_holdObject; }
void  ObjectSpawnerTemplate::setTeamOnVehicle(bool v)    { m_teamOnVehicle = v; }
bool  ObjectSpawnerTemplate::getTeamOnVehicle()          { return m_teamOnVehicle; }

// Marks this spawner precached, registers each filled slot for pre-storage, and precaches its template.
// Does not create a temporary spawner object.
void ObjectSpawnerTemplate::preCache() const
{
    const_cast<ObjectSpawnerTemplate*>(this)->updateFlags(0x20, 0);

    for (int i = 0; i <= 2; ++i)
    {
        IObjectTemplate* t = m_objectTemplates[i].resolved;
        if (t)
        {
            gameLogic->addPreStoredTemplate(t->getId(), 1, 0);
            if (!(t->getFlags() & 0x20))
                t->preCache();
        }
    }
}

} // namespace world
} // namespace hfe
} // namespace dice
