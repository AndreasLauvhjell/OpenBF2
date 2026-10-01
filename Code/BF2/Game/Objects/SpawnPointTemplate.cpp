// Spawn-point template defaults and script serialization.

#include "SpawnPointTemplate.h"
#include "../../IO/Stream/IStream.h"
#include <cmath>
#include <sstream>
#include "SpawnPoint.h"                 // what createObject news

#include "../../Core/Ids.h"

namespace dice {
namespace hfe {

// ctor - 0x8116A30, 156 bytes, read in full. world::ObjectTemplate base, the three vptr stores
// (primary, the Script interface at +0x08, the SpawnPointTemplateInterface at +0x30), then the
// defaults. Tail: updateFlags(0x50, 0) - bits 4 and 6 together, which no other template family
// recovered so far sets as a pair.
// The two non-obvious defaults: the spawn offset's Y is 1.02f (not zero - it lifts the spawn off
// the ground), and minSpawnHeight is -1.0f, a disabled-sentinel rather than a height.
SpawnPointTemplate::SpawnPointTemplate()
{
    m_refCount = 1;                              // +0x34

    m_spawnPositionOffset.x = 0.0f;
    m_spawnPositionOffset.y = 1.02f;
    m_spawnPositionOffset.z = 0.0f;

    m_spawnPreventionDelay = 0.0f;
    m_group = 0;
    m_controlPointId = 0xFFFFFFFFu;              // -1
    m_active = true;                             // true by default
    m_enterOnSpawn = false;
    m_aiEnterOnSpawn = false;
    m_spawnAsParaTroper = false;
    m_minSpawnHeight = -1.0f;                    // a sentinel, not a height
    m_scatterSpawnPositions = false;
    m_onlyForHuman = false;
    m_onlyForAI = false;
    m_allowSpawnCloseToVehicle = false;

    updateFlags(0x50, 0);
}

// (D1): vptr resets + ~ObjectTemplate.
SpawnPointTemplate::~SpawnPointTemplate()
{
}

// the BF2_FACTORY shape, identical in behaviour to ClassFactory<T>::create: refuse
// aggregation, build, query, and release on a failed query.
IBase* SpawnPointTemplate::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;

    SpawnPointTemplate* object = new SpawnPointTemplate;
    IBase* result = static_cast<IBase*>(object->queryInterface(iid));
    if (!result)
        object->release();
    return result;
}

// The per-class quartet - refcount +0x34, delete on the 1->0 edge through the D0 slot.
void SpawnPointTemplate::addRef()  { ++m_refCount; }
int  SpawnPointTemplate::getRef()  { return m_refCount; }

int SpawnPointTemplate::release()
{
    const int r = --m_refCount;
    if (r == 0)
        delete this;
    return r;
}

// the standard four-way hop. It does NOT chain to ObjectTemplate: that class does not
// override queryInterface at all (its own header records the quartet as "not overridden"), so the
// walk is written out here. The subobject offsets are the ctor's own vptr stores: +0x08 is the
// Script/IConsoleSaveable table and +0x30 is SpawnPointTemplateInterface, which RTTI names
// ISpawnPointTemplate.
void* SpawnPointTemplate::queryInterface(unsigned int iid) const
{
    SpawnPointTemplate* self = const_cast<SpawnPointTemplate*>(this);
    if (iid == IID_IBase || iid == world::IID_IObjectTemplate)
        return static_cast<world::IObjectTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    if (iid == IID_ISpawnPointTemplate)          // 50301 - the +0x30 subobject (a real base now)
        return static_cast<ISpawnPointTemplate*>(self);
    return 0;
}

unsigned int SpawnPointTemplate::getClassID() const { return CID_SpawnPointTemplate; }

// its own CID or world::CID_ObjectTemplate; flat, no chain. The two constants come from
// different namespaces in the ELF (hfe:: and hfe::world::) and sit side by side in this one body -
// which is the clearest single proof that this class is not in world::.
void* SpawnPointTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_SpawnPointTemplate || classId == world::CID_ObjectTemplate)
        return this;
    return 0;
}

// createObject - 47 B. the day the SpawnPoint object class was recovered
// (the precache drive faulted here the moment the control points finished their lifecycles).
world::IObject* SpawnPointTemplate::createObject() const
{
    return new SpawnPoint(this);                     // new(0x158)
}

// the eighteen console accessors: every one a plain store or load
void  SpawnPointTemplate::setSpawnPositionOffset(Vec3& v)   { m_spawnPositionOffset = v; }
Vec3& SpawnPointTemplate::getSpawnPositionOffset()          { return m_spawnPositionOffset; }
void  SpawnPointTemplate::setSpawnPreventionDelay(float v)  { m_spawnPreventionDelay = v; }
float SpawnPointTemplate::getSpawnPreventionDelay()         { return m_spawnPreventionDelay; }
void  SpawnPointTemplate::setGroup(unsigned int v)          { m_group = v; }
unsigned int SpawnPointTemplate::getGroup()                 { return m_group; }
void  SpawnPointTemplate::setControlPointId(unsigned int v) { m_controlPointId = v; }
unsigned int SpawnPointTemplate::getControlPointId() const  { return m_controlPointId; }
void  SpawnPointTemplate::setActive(bool v)                 { m_active = v; }
bool  SpawnPointTemplate::getActive()                       { return m_active; }
void  SpawnPointTemplate::setEnterOnSpawn(bool v)           { m_enterOnSpawn = v; }
bool  SpawnPointTemplate::getEnterOnSpawn()                 { return m_enterOnSpawn; }
void  SpawnPointTemplate::setAIEnterOnSpawn(bool v)         { m_aiEnterOnSpawn = v; }
bool  SpawnPointTemplate::getAIEnterOnSpawn()               { return m_aiEnterOnSpawn; }
void  SpawnPointTemplate::setSpawnAsParaTroper(bool v)      { m_spawnAsParaTroper = v; }
bool  SpawnPointTemplate::getSpawnAsParaTroper()            { return m_spawnAsParaTroper; }
void  SpawnPointTemplate::setMinSpawnHeight(float v)        { m_minSpawnHeight = v; }
float SpawnPointTemplate::getMinSpawnHeight() const         { return m_minSpawnHeight; }
void  SpawnPointTemplate::setScatterSpawnPositions(bool v)  { m_scatterSpawnPositions = v; }
bool  SpawnPointTemplate::getScatterSpawnPositions()        { return m_scatterSpawnPositions; }
void  SpawnPointTemplate::setOnlyForAI(bool v)              { m_onlyForAI = v; }
bool  SpawnPointTemplate::getOnlyForAI() const              { return m_onlyForAI; }
void  SpawnPointTemplate::setOnlyForHuman(bool v)           { m_onlyForHuman = v; }
bool  SpawnPointTemplate::getOnlyForHuman() const           { return m_onlyForHuman; }
void  SpawnPointTemplate::setAllowSpawnCloseToVehicle(bool v){ m_allowSpawnCloseToVehicle = v; }
bool  SpawnPointTemplate::getAllowSpawnCloseToVehicle() const{ return m_allowSpawnCloseToVehicle; }

// ; vector comparison PE 0x539360 uses <= FLT_EPSILON.
// Active/minSpawnHeight are deliberately not emitted. The two unsigned-stored IDs print signed.
bool SpawnPointTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream out;
    if (!world::ObjectTemplate::makeScript(stream)) return false;
    const float eps = 1.19209290e-7f;
    if (!(std::fabs(m_spawnPositionOffset.x) <= eps &&
          std::fabs(m_spawnPositionOffset.y - 1.02f) <= eps &&
          std::fabs(m_spawnPositionOffset.z) <= eps))
        out << "ObjectTemplate.setSpawnPositionOffset " << m_spawnPositionOffset.x << '/' << m_spawnPositionOffset.y << '/' << m_spawnPositionOffset.z << std::endl;
    if (m_spawnPreventionDelay != 0.0f) out << "ObjectTemplate.setSpawnPreventionDelay " << m_spawnPreventionDelay << std::endl;
    if (m_group) out << "ObjectTemplate.setGroup " << static_cast<int>(m_group) << std::endl;
    if (m_controlPointId != 0xFFFFFFFFu) out << "ObjectTemplate.setControlPointId " << static_cast<int>(m_controlPointId) << std::endl;
    if (m_enterOnSpawn) out << "ObjectTemplate.setEnterOnSpawn " << m_enterOnSpawn << std::endl;
    if (m_aiEnterOnSpawn) out << "ObjectTemplate.setAIEnterOnSpawn " << m_aiEnterOnSpawn << std::endl;
    if (m_spawnAsParaTroper) out << "ObjectTemplate.setSpawnAsParaTroper " << m_spawnAsParaTroper << std::endl;
    if (m_scatterSpawnPositions) out << "ObjectTemplate.setScatterSpawnPositions " << m_scatterSpawnPositions << std::endl;
    if (m_onlyForHuman) out << "ObjectTemplate.setOnlyForHuman " << m_onlyForHuman << std::endl;
    if (m_onlyForAI) out << "ObjectTemplate.setOnlyForAI " << m_onlyForAI << std::endl;
    if (m_allowSpawnCloseToVehicle) out << "ObjectTemplate.setAllowSpawnCloseToVehicle " << m_allowSpawnCloseToVehicle << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace hfe
} // namespace dice
