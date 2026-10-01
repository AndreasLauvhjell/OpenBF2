// Armor template settings and deferred damage-effect construction.
// Source placement is [INFERRED].

#include "ArmorGLCompTemplate.h"

#include "../../../Core/Ids.h"
#include "../../../Core/Debug.h"
#include "../../../IO/Stream/IStream.h"
#include <sstream>
#include "../../../Core/StringUtil.h"                  // compareIgnoreCase - addEffect's "none" test
#include "../IObjectTemplate.h"
#include "../ObjectTemplate.h"
#include "ArmorGLComp.h"                             // createComponent's last line                      // get/setIsPreCaching (static)

#include "../../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../../Scene/Object/ObjectUtil.h"          // turnOffPreCacheOnTemplateAndAllChildren

namespace dice {
namespace hfe {
namespace world {

// Initializes armor defaults and empty effect maps. Preserve sentinel/default values:
// material -1, shallow water -0.3, dirt threshold -1 and distinct shallow/deep damage delays.
ArmorGLCompTemplate::ArmorGLCompTemplate()
    : m_refCount(1)
    , m_needsBuild(true)                        // +12 - the one-time build latch, see the header
    , m_alignLastEffectToHitDirection(false)    // +13
    , m_showDamageAsDirt(-1.0f)                 // +156 - disabled
    , m_wreckHitPoints(10.0f)                   // +160
    , m_hitPoints(10.0f)                        // +164
    , m_maxHitPoints(10.0f)                     // +168
    , m_maxWreckHitPoints(10.0f)                // +172
    , m_attackDamage(1.0f)                      // +176
    , m_angleMod(0.0f)                          // +180
    , m_speedMod(1.0f)                          // +184
    , m_defaultMaterial(-1)                     // +188 - the no-material sentinel
    , m_explosionRadius(0.0f)                   // +192
    , m_explosionDamage(0.0f)                   // +196
    , m_explosionMaterial(0)                    // +200
    , m_explosionForce(0.0f)                    // +204
    , m_explosionForceMod(1.0f)                 // +208
    , m_explosionForceMax(10.0f)                // +212
    , m_wreckExplosionRadius(0.0f)              // +216
    , m_wreckExplosionDamage(0.0f)              // +220
    , m_wreckExplosionMaterial(0)               // +224
    , m_wreckExplosionForce(0.0f)               // +228
    , m_wreckExplosionForceMod(1.0f)            // +232
    , m_wreckExplosionForceMax(100.0f)          // +236 - ten times the intact one
    , m_hpLostWhileCriticalDamage(1.0f)         // +240
    , m_hpLostWhileUpSideDown(0.0f)             // +244
    , m_hpLostWhileInWater(0.0f)                // +248
    , m_hpLostWhileInDeepWater(0.0f)            // +252
    , m_waterDamageDelay(3.0f)                  // +256
    , m_deepWaterDamageDelay(1.0f)              // +260
    , m_waterLevel(-0.30000001f)                // +264 - negative, as shipped
    , m_deepWaterLevel(1.0f)                    // +268
    , m_canBeRepairedWhenWreck(false)           // +272
    , m_criticalDamage(0.0f)                    // +276
    , m_timeToStayAsWreck(20.0f)                // +284
    , m_timeToStayAfterDestroyed(0.0f)          // +288
    , m_wreckDelay(0.0f)                        // +292
    , m_hideChildrenOnSpectacularDeath(false)   // +296
    , m_destroyOnSpectacularDeath(false)        // +297
    , m_canBeDestroyed(true)                    // +298
    , m_useMMOnDeath(true)                      // +299
{
    // The two DamageEffects maps and the eight console queues all start empty; the ELF spells that
    // out store by store because it is hand-rolled STL. Members not listed in the initialiser list
    // are the [UNRESOLVED] slots the ctor zeroes - _dword18, _dword24, _dword12C, _dword130 and
    // _dword134 - plus _byteE/_byteF.
    // The two maps, the eight queues and the repair-point vector all start empty; the ELF spells
    // that out store by store because it is hand-rolled STL (it even allocates both map header
    // nodes by hand and self-links them). Default construction is the same observable state.
    _byteE = false;  _byteF = false;
    _dword118 = 0;   _byte111 = false;  _byte112 = false;  _byte113 = false;
}

// Deletes owned DamageEffects values, first from the intact map and then the wreck map.
// Null values are allowed; member containers tear down afterward.
ArmorGLCompTemplate::~ArmorGLCompTemplate()
{
    for (std::map<int, DamageEffects*>::iterator it = m_damageEffects.begin();
         it != m_damageEffects.end(); ++it)
    {
        if (it->second)
            delete it->second;
    }
    for (std::map<int, DamageEffects*>::iterator it = m_wreckDamageEffects.begin();
         it != m_wreckDamageEffects.end(); ++it)
    {
        if (it->second)
            delete it->second;
    }
}

// IBase - 0x8222790 (release, 57 B), 0x82227E0 (queryInterface, 53 B), and the two 11/15-byte counters.
// release() dispatches THE deleting destructor through THE vtable, slot +0x16C, which the slot
// dump confirms is D0Ev (the Itanium deleting dtor) and not D1Ev at +0x168. It also tests `this` for
// null after already having decremented through it - as shipped, and harmless.
void ArmorGLCompTemplate::addRef()  { ++m_refCount; }
int  ArmorGLCompTemplate::getRef()  { return m_refCount; }

int ArmorGLCompTemplate::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;                             // vtable +0x16C, the deleting destructor
    return remaining;
}

// Three ids answer with `this`; IConsoleSaveable answers with the second base at +4, which is what
// the RTTI says (IArmorGLCompTemplate @ +0, io::IConsoleSaveable @ +4).
void* ArmorGLCompTemplate::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent || iid == IID_IArmorGLCompTemplate)
        return const_cast<ArmorGLCompTemplate*>(this);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<ArmorGLCompTemplate*>(this));
    return 0;
}

unsigned int ArmorGLCompTemplate::getClassID() const  { return CID_ArmorGLCompTemplate; }
unsigned int ArmorGLCompTemplate::getInterfaceID()    { return IID_IArmorGLCompTemplate; }

// "give me the implementation if you ARE this class". It asks its own getClassID
// through the vtable rather than returning the constant, so a subclass answers for itself.
void* ArmorGLCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == getClassID())
        return this;
    return 0;
}

// 0x8222??? - three five-byte bodies: `push ebp; mov ebp,esp; pop ebp; retn`. They do nothing at
// all. Every template component gets these hooks; this one declines all three.
void ArmorGLCompTemplate::objectCreated(IObject*)          {}
void ArmorGLCompTemplate::objectDestroyed(IObject*)        {}
void ArmorGLCompTemplate::setTemplate(IObjectTemplate*)    {}

// setWreckHitPoints IS NOT A plain setter - 0x8222890, 23 bytes where every sibling is 17. It
// writes TWO fields: m_wreckHitPoints AND m_maxWreckHitPoints. So a `.tweak` line that sets wreck
// hit points silently raises the ceiling too, and there is no setMaxWreckHitPoints in the class to
// undo it. Its intact-side counterpart setHitPoints does NOT do this - setMaxHitPoints exists
// separately. The asymmetry is dice's; the size difference is what gave it away.
void  ArmorGLCompTemplate::setWreckHitPoints(float v) { m_wreckHitPoints = v; m_maxWreckHitPoints = v; }
float ArmorGLCompTemplate::getWreckHitPoints()        { return m_wreckHitPoints; }

// The plain accessors - 17-byte setters and 14/15-byte getters, one field each. Every offset below
// was re-read from its own disassembly this sitting and cross-checked against the header; all
// eighty-odd agreed, which is the fifth independent confirmation of this layout.
// getDefaultMaterial and getShowDamageAsDirt are the only const getters in the class (they
// mangle NK, the rest N). That asymmetry is dice's and is preserved.
void  ArmorGLCompTemplate::setHitPoints(float v) { m_hitPoints = v; }
float ArmorGLCompTemplate::getHitPoints()       { return m_hitPoints; }
void  ArmorGLCompTemplate::setMaxHitPoints(float v) { m_maxHitPoints = v; }
float ArmorGLCompTemplate::getMaxHitPoints()       { return m_maxHitPoints; }
void  ArmorGLCompTemplate::setAttackDamage(float v) { m_attackDamage = v; }
float ArmorGLCompTemplate::getAttackDamage()       { return m_attackDamage; }
void  ArmorGLCompTemplate::setAngleMod(float v) { m_angleMod = v; }
float ArmorGLCompTemplate::getAngleMod()       { return m_angleMod; }
void  ArmorGLCompTemplate::setSpeedMod(float v) { m_speedMod = v; }
float ArmorGLCompTemplate::getSpeedMod()       { return m_speedMod; }
void  ArmorGLCompTemplate::setExplosionRadius(float v) { m_explosionRadius = v; }
float ArmorGLCompTemplate::getExplosionRadius()       { return m_explosionRadius; }
void  ArmorGLCompTemplate::setExplosionDamage(float v) { m_explosionDamage = v; }
float ArmorGLCompTemplate::getExplosionDamage()       { return m_explosionDamage; }
void  ArmorGLCompTemplate::setExplosionMaterial(int v) { m_explosionMaterial = v; }
int   ArmorGLCompTemplate::getExplosionMaterial()       { return m_explosionMaterial; }
void  ArmorGLCompTemplate::setExplosionForce(float v) { m_explosionForce = v; }
float ArmorGLCompTemplate::getExplosionForce()       { return m_explosionForce; }
void  ArmorGLCompTemplate::setExplosionForceMax(float v) { m_explosionForceMax = v; }
float ArmorGLCompTemplate::getExplosionForceMax()       { return m_explosionForceMax; }
void  ArmorGLCompTemplate::setExplosionForceMod(float v) { m_explosionForceMod = v; }
float ArmorGLCompTemplate::getExplosionForceMod()       { return m_explosionForceMod; }
void  ArmorGLCompTemplate::setWreckExplosionRadius(float v) { m_wreckExplosionRadius = v; }
float ArmorGLCompTemplate::getWreckExplosionRadius()       { return m_wreckExplosionRadius; }
void  ArmorGLCompTemplate::setWreckExplosionDamage(float v) { m_wreckExplosionDamage = v; }
float ArmorGLCompTemplate::getWreckExplosionDamage()       { return m_wreckExplosionDamage; }
void  ArmorGLCompTemplate::setWreckExplosionMaterial(int v) { m_wreckExplosionMaterial = v; }
int   ArmorGLCompTemplate::getWreckExplosionMaterial()       { return m_wreckExplosionMaterial; }
void  ArmorGLCompTemplate::setWreckExplosionForce(float v) { m_wreckExplosionForce = v; }
float ArmorGLCompTemplate::getWreckExplosionForce()       { return m_wreckExplosionForce; }
void  ArmorGLCompTemplate::setWreckExplosionForceMax(float v) { m_wreckExplosionForceMax = v; }
float ArmorGLCompTemplate::getWreckExplosionForceMax()       { return m_wreckExplosionForceMax; }
void  ArmorGLCompTemplate::setWreckExplosionForceMod(float v) { m_wreckExplosionForceMod = v; }
float ArmorGLCompTemplate::getWreckExplosionForceMod()       { return m_wreckExplosionForceMod; }
void  ArmorGLCompTemplate::setHpLostWhileCriticalDamage(float v) { m_hpLostWhileCriticalDamage = v; }
float ArmorGLCompTemplate::getHpLostWhileCriticalDamage()       { return m_hpLostWhileCriticalDamage; }
void  ArmorGLCompTemplate::setHpLostWhileUpSideDown(float v) { m_hpLostWhileUpSideDown = v; }
float ArmorGLCompTemplate::getHpLostWhileUpSideDown()       { return m_hpLostWhileUpSideDown; }
void  ArmorGLCompTemplate::setHpLostWhileInWater(float v) { m_hpLostWhileInWater = v; }
float ArmorGLCompTemplate::getHpLostWhileInWater()       { return m_hpLostWhileInWater; }
void  ArmorGLCompTemplate::setHpLostWhileInDeepWater(float v) { m_hpLostWhileInDeepWater = v; }
float ArmorGLCompTemplate::getHpLostWhileInDeepWater()       { return m_hpLostWhileInDeepWater; }
void  ArmorGLCompTemplate::setWaterDamageDelay(float v) { m_waterDamageDelay = v; }
float ArmorGLCompTemplate::getWaterDamageDelay()       { return m_waterDamageDelay; }
void  ArmorGLCompTemplate::setDeepWaterDamageDelay(float v) { m_deepWaterDamageDelay = v; }
float ArmorGLCompTemplate::getDeepWaterDamageDelay()       { return m_deepWaterDamageDelay; }
void  ArmorGLCompTemplate::setWaterLevel(float v) { m_waterLevel = v; }
float ArmorGLCompTemplate::getWaterLevel()       { return m_waterLevel; }
void  ArmorGLCompTemplate::setDeepWaterLevel(float v) { m_deepWaterLevel = v; }
float ArmorGLCompTemplate::getDeepWaterLevel()       { return m_deepWaterLevel; }
void  ArmorGLCompTemplate::setCanBeRepairedWhenWreck(bool v) { m_canBeRepairedWhenWreck = v; }
bool  ArmorGLCompTemplate::getCanBeRepairedWhenWreck()       { return m_canBeRepairedWhenWreck; }
void  ArmorGLCompTemplate::setCriticalDamage(float v) { m_criticalDamage = v; }
float ArmorGLCompTemplate::getCriticalDamage()       { return m_criticalDamage; }
void  ArmorGLCompTemplate::setTimeToStayAsWreck(float v) { m_timeToStayAsWreck = v; }
float ArmorGLCompTemplate::getTimeToStayAsWreck()       { return m_timeToStayAsWreck; }
void  ArmorGLCompTemplate::setTimeToStayAfterDestroyed(float v) { m_timeToStayAfterDestroyed = v; }
float ArmorGLCompTemplate::getTimeToStayAfterDestroyed()       { return m_timeToStayAfterDestroyed; }
void  ArmorGLCompTemplate::setAlignLastEffectToHitDirection(bool v) { m_alignLastEffectToHitDirection = v; }
bool  ArmorGLCompTemplate::getAlignLastEffectToHitDirection()       { return m_alignLastEffectToHitDirection; }
void  ArmorGLCompTemplate::setWreckDelay(float v) { m_wreckDelay = v; }
float ArmorGLCompTemplate::getWreckDelay()       { return m_wreckDelay; }
void  ArmorGLCompTemplate::setHideChildrenOnSpectacularDeath(bool v) { m_hideChildrenOnSpectacularDeath = v; }
bool  ArmorGLCompTemplate::getHideChildrenOnSpectacularDeath()       { return m_hideChildrenOnSpectacularDeath; }
void  ArmorGLCompTemplate::setDestroyOnSpectacularDeath(bool v) { m_destroyOnSpectacularDeath = v; }
bool  ArmorGLCompTemplate::getDestroyOnSpectacularDeath()       { return m_destroyOnSpectacularDeath; }
void  ArmorGLCompTemplate::setCanBeDestroyed(bool v) { m_canBeDestroyed = v; }
bool  ArmorGLCompTemplate::getCanBeDestroyed()       { return m_canBeDestroyed; }
void  ArmorGLCompTemplate::setUseMMOnDeath(bool v) { m_useMMOnDeath = v; }
bool  ArmorGLCompTemplate::getUseMMOnDeath()       { return m_useMMOnDeath; }

void ArmorGLCompTemplate::setDefaultMaterial(int v)     { m_defaultMaterial = v; }
int  ArmorGLCompTemplate::getDefaultMaterial() const    { return m_defaultMaterial; }
void ArmorGLCompTemplate::setShowDamageAsDirt(float v)  { m_showDamageAsDirt = v; }
float ArmorGLCompTemplate::getShowDamageAsDirt() const  { return m_showDamageAsDirt; }

// Console effect commands queue raw arguments until the first component is created,
// when all referenced templates can be resolved and both effect maps precached.

// Adds an intact damage-state effect after resolving its template; unknown names are silently ignored.
// Case-insensitive "none" inserts a null value only for a new state, which preCache later dereferences.
// Preserve this behavior and the existing-state no-op.
void ArmorGLCompTemplate::addEffect(std::string name, Vec3 position, Vec3 direction,
                                    int damageState, bool spectacular)
{
    if (compareIgnoreCase("none", name.c_str()) == 0)
    {
        m_damageEffects.insert(std::make_pair(damageState, static_cast<DamageEffects*>(0)));
        return;
    }

    const IObjectTemplate* effectTemplate = objectTemplateManager->getTemplate(name);
    if (!effectTemplate)
        return;

    DamageEffects* effects = 0;
    std::map<int, DamageEffects*>::iterator it = m_damageEffects.find(damageState);
    if (it == m_damageEffects.end())
    {
        effects = new DamageEffects();
        if (!effects)
            Debug(DEBUGTYPE_6, __FILE__, 358, "") << "Unable to allocate memory!";
        m_damageEffects.insert(std::make_pair(damageState, effects));
    }
    else
    {
        effects = it->second;
    }

    if (spectacular)
    {
        effects->m_spectacularEffects.push_back(effectTemplate);
        effects->m_spectacularPositions.push_back(position);
        effects->m_spectacularDirections.push_back(direction);
    }
    else
    {
        effects->m_effects.push_back(effectTemplate);
        effects->m_positions.push_back(position);
        effects->m_directions.push_back(direction);
    }
}

// addWreckEffect - 0x8221140, 890 B, read in full. The wreck-side twin, on the second map. Same
// "none"-inserts-null behaviour, same silent drop of an unresolvable name, its own Debug at line 401.
// IT HAS ONE Vec3 AND uses IT twice. The single `vec` argument is pushed into both m_positions
// and m_directions, so a wreck effect's direction always equals its position. Read from the bytes:
// two consecutive list inserts, both sourced from a3. There is no spectacular arm at all - the wreck
// map only ever uses members 1-3 of DamageEffects.
void ArmorGLCompTemplate::addWreckEffect(std::string name, Vec3 vec, int damageState)
{
    if (compareIgnoreCase("none", name.c_str()) == 0)
    {
        m_wreckDamageEffects.insert(std::make_pair(damageState, static_cast<DamageEffects*>(0)));
        return;
    }

    const IObjectTemplate* effectTemplate = objectTemplateManager->getTemplate(name);
    if (!effectTemplate)
        return;

    DamageEffects* effects = 0;
    std::map<int, DamageEffects*>::iterator it = m_wreckDamageEffects.find(damageState);
    if (it == m_wreckDamageEffects.end())
    {
        effects = new DamageEffects();
        if (!effects)
            Debug(DEBUGTYPE_6, __FILE__, 401, "") << "Unable to allocate memory!";
        m_wreckDamageEffects.insert(std::make_pair(damageState, effects));
    }
    else
    {
        effects = it->second;
    }

    effects->m_effects.push_back(effectTemplate);
    effects->m_positions.push_back(vec);
    effects->m_directions.push_back(vec);          // the same vector, twice - as shipped
}

// Disables precaching recursively on any currently resolved effect template, then queues raw arguments.
// The template is resolved again during component creation; the three variants differ in their target queues.
void ArmorGLCompTemplate::addArmorEffect(int damageState, const std::string& name,
                                         const Vec3& position, const Vec3& direction)
{
    IObjectTemplate* effectTemplate = objectTemplateManager->getTemplate(name);
    if (effectTemplate)
        turnOffPreCacheOnTemplateAndAllChildren(effectTemplate);

    m_queuedEffectNames.push_back(name);
    m_queuedEffectPos.push_back(position);
    m_queuedEffectDir.push_back(direction);
    m_queuedEffectStates.push_back(damageState);
    m_queuedEffectFlags.push_back(false);          // not spectacular
}

void ArmorGLCompTemplate::addArmorEffectSpectacular(int damageState, const std::string& name,
                                                    const Vec3& position, const Vec3& direction)
{
    IObjectTemplate* effectTemplate = objectTemplateManager->getTemplate(name);
    if (effectTemplate)
        turnOffPreCacheOnTemplateAndAllChildren(effectTemplate);

    m_queuedEffectNames.push_back(name);
    m_queuedEffectPos.push_back(position);
    m_queuedEffectDir.push_back(direction);
    m_queuedEffectStates.push_back(damageState);
    m_queuedEffectFlags.push_back(true);           // spectacular
}

// The fourth argument goes onto a queue nobody reads - see m_queuedWreckDirs in the header.
void ArmorGLCompTemplate::addWreckArmorEffect(int damageState, const std::string& name,
                                              const Vec3& vec, const Vec3& unused)
{
    IObjectTemplate* effectTemplate = objectTemplateManager->getTemplate(name);
    if (effectTemplate)
        turnOffPreCacheOnTemplateAndAllChildren(effectTemplate);

    m_queuedWreckNames.push_back(name);
    m_queuedWreckVecs.push_back(vec);
    m_queuedWreckDirs.push_back(unused);           // stored, never consumed
    m_queuedWreckValues.push_back(damageState);
}

// addRepairPoint - 0x8221CF0, 209 B. A plain push_back written the long way round: the ELF computes
// the new size, calls _M_fill_insert (or erase, on the impossible shrink arm) to reach it, then
// writes the Vec3 and the float into the last element. resolving IT answered three [UNRESOLVED]
// slots AT once - +300/+304/+308 are this vector's begin/end/capacity.
void ArmorGLCompTemplate::addRepairPoint(const Vec3& position, float radius)
{
    RepairPoint point;
    point.position = position;
    point.radius   = radius;
    m_repairPoints.push_back(point);
}

// preCache - 0x821F3B0, 178 B. Warms every effect template in both maps.
// IT raises THE global precache flag AND restores THE previous value, not `false`. It saves
// ObjectTemplate::getIsPreCaching(), forces it true, walks, and puts the saved value back - so a
// nested precache does not clear the flag out from under its caller.
void ArmorGLCompTemplate::preCache()
{
    const bool wasPreCaching = ObjectTemplate::getIsPreCaching();
    ObjectTemplate::setIsPreCaching(true);

    for (std::map<int, DamageEffects*>::const_iterator it = m_damageEffects.begin();
         it != m_damageEffects.end(); ++it)
        it->second->preCache();

    for (std::map<int, DamageEffects*>::const_iterator it = m_wreckDamageEffects.begin();
         it != m_wreckDamageEffects.end(); ++it)
        it->second->preCache();

    ObjectTemplate::setIsPreCaching(wasPreCaching);
}

// Creates an ArmorGLComp. On first use, clamps hit-point pairs back into the template,
// builds/precache effect maps and clears the build latch. Wreck directions are queued but not replayed.
IObjectComponent* ArmorGLCompTemplate::createComponent()
{
    if (m_needsBuild)
    {
        if (m_hitPoints > m_maxHitPoints)
            m_hitPoints = m_maxHitPoints;
        if (m_wreckHitPoints > m_maxWreckHitPoints)
            m_wreckHitPoints = m_maxWreckHitPoints;

        for (unsigned int i = 0; i < m_queuedEffectNames.size(); ++i)
            addEffect(m_queuedEffectNames[i], m_queuedEffectPos[i], m_queuedEffectDir[i],
                      m_queuedEffectStates[i], m_queuedEffectFlags[i]);

        for (unsigned int i = 0; i < m_queuedWreckNames.size(); ++i)
            addWreckEffect(m_queuedWreckNames[i], m_queuedWreckVecs[i], m_queuedWreckValues[i]);

        preCache();
        m_needsBuild = false;
    }

    return new ArmorGLComp(this);                  // operator new(0x264) - sizeof is 612 bytes
}


// . Serialize the authoring queues, including the wreck
// direction that createComponent does not replay. Repair points are not emitted by retail.
bool ArmorGLCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream out;
    if (m_maxHitPoints != 10.0f) out << "ObjectTemplate.armor.maxHitPoints " << m_maxHitPoints << std::endl;
    if (m_hitPoints != 10.0f) out << "ObjectTemplate.armor.hitPoints " << m_hitPoints << std::endl;
    if (m_defaultMaterial != -1) out << "ObjectTemplate.armor.defaultMaterial " << m_defaultMaterial << std::endl;
    if (m_hpLostWhileUpSideDown != 0.0f) out << "ObjectTemplate.armor.hpLostWhileUpSideDown " << m_hpLostWhileUpSideDown << std::endl;
    if (m_hpLostWhileInWater != 0.0f) out << "ObjectTemplate.armor.hpLostWhileInWater " << m_hpLostWhileInWater << std::endl;
    if (m_hpLostWhileInDeepWater != 0.0f) out << "ObjectTemplate.armor.hpLostWhileInDeepWater " << m_hpLostWhileInDeepWater << std::endl;
    if (m_hpLostWhileCriticalDamage != 1.0f) out << "ObjectTemplate.armor.hpLostWhileCriticalDamage " << m_hpLostWhileCriticalDamage << std::endl;
    if (m_waterDamageDelay != 3.0f) out << "ObjectTemplate.armor.waterDamageDelay " << m_waterDamageDelay << std::endl;
    if (m_deepWaterDamageDelay != 1.0f) out << "ObjectTemplate.armor.deepWaterDamageDelay " << m_deepWaterDamageDelay << std::endl;
    if (m_waterLevel != 1.0f) out << "ObjectTemplate.armor.waterLevel " << m_waterLevel << std::endl;
    if (m_deepWaterLevel != 1.0f) out << "ObjectTemplate.armor.deepWaterLevel " << m_deepWaterLevel << std::endl;
    if (m_explosionForce != 0.0f) out << "ObjectTemplate.armor.explosionForce " << m_explosionForce << std::endl;
    if (m_explosionForceMod != 1.0f) out << "ObjectTemplate.armor.explosionForceMod " << m_explosionForceMod << std::endl;
    if (m_explosionForceMax != 10.0f) out << "ObjectTemplate.armor.explosionForceMax " << m_explosionForceMax << std::endl;
    if (m_explosionDamage != 0.0f) out << "ObjectTemplate.armor.explosionDamage " << m_explosionDamage << std::endl;
    if (m_explosionRadius != 0.0f) out << "ObjectTemplate.armor.explosionRadius " << m_explosionRadius << std::endl;
    if (m_explosionMaterial) out << "ObjectTemplate.armor.explosionMaterial " << m_explosionMaterial << std::endl;
    if (m_wreckExplosionForce != 0.0f) out << "ObjectTemplate.armor.wreckExplosionForce " << m_wreckExplosionForce << std::endl;
    if (m_wreckExplosionForceMod != 1.0f) out << "ObjectTemplate.armor.wreckExplosionForceMod " << m_wreckExplosionForceMod << std::endl;
    if (m_wreckExplosionForceMax != 100.0f) out << "ObjectTemplate.armor.wreckExplosionForceMax " << m_wreckExplosionForceMax << std::endl;
    if (m_wreckExplosionDamage != 0.0f) out << "ObjectTemplate.armor.wreckExplosionDamage " << m_wreckExplosionDamage << std::endl;
    if (m_wreckExplosionRadius != 0.0f) out << "ObjectTemplate.armor.wreckExplosionRadius " << m_wreckExplosionRadius << std::endl;
    if (m_wreckExplosionMaterial) out << "ObjectTemplate.armor.wreckExplosionMaterial " << m_wreckExplosionMaterial << std::endl;
    if (m_wreckHitPoints != 10.0f) out << "ObjectTemplate.armor.wreckHitPoints " << m_wreckHitPoints << std::endl;
    if (m_timeToStayAsWreck != 20.0f) out << "ObjectTemplate.armor.timeToStayAsWreck " << m_timeToStayAsWreck << std::endl;
    if (m_timeToStayAfterDestroyed != 0.0f) out << "ObjectTemplate.armor.timeToStayAfterDestroyed " << m_timeToStayAfterDestroyed << std::endl;
    if (m_criticalDamage != 0.0f) out << "ObjectTemplate.armor.criticalDamage " << m_criticalDamage << std::endl;
    if (m_angleMod != 0.0f) out << "ObjectTemplate.armor.angleMod " << m_angleMod << std::endl;
    if (m_attackDamage != 1.0f) out << "ObjectTemplate.armor.attackDamage " << m_attackDamage << std::endl;
    if (m_speedMod != 1.0f) out << "ObjectTemplate.armor.speedMod " << m_speedMod << std::endl;
    if (m_wreckDelay != 0.0f) out << "ObjectTemplate.armor.wreckDelay " << m_wreckDelay << std::endl;
    if (m_canBeRepairedWhenWreck) out << "ObjectTemplate.armor.canBeRepairedWhenWreck 1" << std::endl;
    if (m_alignLastEffectToHitDirection) out << "ObjectTemplate.armor.alignLastEffectToHitDirection " << m_alignLastEffectToHitDirection << std::endl;
    if (m_hideChildrenOnSpectacularDeath) out << "ObjectTemplate.armor.hideChildrenOnSpectacularDeath 1" << std::endl;
    if (m_destroyOnSpectacularDeath) out << "ObjectTemplate.armor.destroyOnSpectacularDeath 1" << std::endl;
    if (!m_canBeDestroyed) out << "ObjectTemplate.armor.canBeDestroyed 0" << std::endl;
    if (m_showDamageAsDirt > -1.0f) out << "ObjectTemplate.armor.showDamageAsDirt " << m_showDamageAsDirt << std::endl;
    if (!m_useMMOnDeath) out << "ObjectTemplate.armor.useMMOnDeath 0" << std::endl;
    for (std::size_t i = 0; i < m_queuedEffectNames.size(); ++i)
    {
        out << (m_queuedEffectFlags[i] ? "ObjectTemplate.armor.addArmorEffectSpectacular " : "ObjectTemplate.armor.addArmorEffect ");
        out << m_queuedEffectStates[i] << ' ' << m_queuedEffectNames[i] << ' '
            << m_queuedEffectPos[i].x << '/' << m_queuedEffectPos[i].y << '/' << m_queuedEffectPos[i].z << ' '
            << m_queuedEffectDir[i].x << '/' << m_queuedEffectDir[i].y << '/' << m_queuedEffectDir[i].z << std::endl;
    }
    for (std::size_t i = 0; i < m_queuedWreckNames.size(); ++i)
        out << "ObjectTemplate.armor.addWreckArmorEffect " << m_queuedWreckValues[i] << ' ' << m_queuedWreckNames[i] << ' '
            << m_queuedWreckVecs[i].x << '/' << m_queuedWreckVecs[i].y << '/' << m_queuedWreckVecs[i].z << ' '
            << m_queuedWreckDirs[i].x << '/' << m_queuedWreckDirs[i].y << '/' << m_queuedWreckDirs[i].z << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace world
} // namespace hfe
} // namespace dice
