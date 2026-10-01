// Path: [INFERRED] header beside attested ArmorGLCompTemplate.cpp (__FILE__, lines 358/401).
// Armor configuration recovered from ELF fields, accessors and console bindings.
// Retail ELF size: 0x138; IArmorGLCompTemplate first, IConsoleSaveable second.
// Intact, wreck and environmental damage have separate fields and timers.
// Effect methods append to damage-state lists; their index-to-list evidence is archived.

#ifndef GAME_OBJECTS_ARMORGLCOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_ARMORGLCOMPTEMPLATE_H

#include <map>
#include <string>
#include <vector>

#include "DamageEffects.h"

#include "../CompTemplateInterfaces.h"
#include "../../../IO/Console/IConsoleSaveable.h"
#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;

// The element of addRepairPoint's vector - 16 bytes, from the stride the ELF indexes that vector by
// (`>> 4`, and a 16-byte back-up to reach the last element). A Vec3 and a float account for it
// exactly, and addRepairPoint writes the four words in that order.
struct RepairPoint
{
    Vec3  position;
    float radius;
};

// Armor settings and serialization. Effect methods take a damage-state index before name/position data.
// Preserve virtual declaration order and the two const getters.
class ArmorGLCompTemplate : public IArmorGLCompTemplate, public io::IConsoleSaveable
{
public:
    // with the first bodies. The ELF has C2Ev/C1Ev at 0x821DAF0/0x821DE70 (893 B),
    // a default constructor taking nothing - the header had never declared one.
    ArmorGLCompTemplate();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();
    virtual void  objectCreated(IObject*);
    virtual void  objectDestroyed(IObject*);
    virtual void  setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual void  setWreckHitPoints(float);            virtual float getWreckHitPoints();
    virtual void  setHitPoints(float);                 virtual float getHitPoints();
    virtual void  setMaxHitPoints(float);              virtual float getMaxHitPoints();
    virtual void  setAttackDamage(float);              virtual float getAttackDamage();
    virtual void  setAngleMod(float);                  virtual float getAngleMod();
    virtual void  setSpeedMod(float);                  virtual float getSpeedMod();
    virtual void  setExplosionRadius(float);           virtual float getExplosionRadius();
    virtual void  setExplosionDamage(float);           virtual float getExplosionDamage();
    virtual void  setExplosionMaterial(int);           virtual int   getExplosionMaterial();
    virtual void  setExplosionForce(float);            virtual float getExplosionForce();
    virtual void  setExplosionForceMax(float);         virtual float getExplosionForceMax();
    virtual void  setExplosionForceMod(float);         virtual float getExplosionForceMod();
    virtual void  setWreckExplosionRadius(float);      virtual float getWreckExplosionRadius();
    virtual void  setWreckExplosionDamage(float);      virtual float getWreckExplosionDamage();
    virtual void  setWreckExplosionMaterial(int);      virtual int   getWreckExplosionMaterial();
    virtual void  setWreckExplosionForce(float);       virtual float getWreckExplosionForce();
    virtual void  setWreckExplosionForceMax(float);    virtual float getWreckExplosionForceMax();
    virtual void  setWreckExplosionForceMod(float);    virtual float getWreckExplosionForceMod();
    virtual void  setHpLostWhileCriticalDamage(float); virtual float getHpLostWhileCriticalDamage();
    virtual void  setHpLostWhileUpSideDown(float);     virtual float getHpLostWhileUpSideDown();
    virtual void  setHpLostWhileInWater(float);        virtual float getHpLostWhileInWater();
    virtual void  setHpLostWhileInDeepWater(float);    virtual float getHpLostWhileInDeepWater();
    virtual void  setWaterDamageDelay(float);          virtual float getWaterDamageDelay();
    virtual void  setDeepWaterDamageDelay(float);      virtual float getDeepWaterDamageDelay();
    virtual void  setWaterLevel(float);                virtual float getWaterLevel();
    virtual void  setDeepWaterLevel(float);            virtual float getDeepWaterLevel();
    virtual void  setCanBeRepairedWhenWreck(bool);     virtual bool  getCanBeRepairedWhenWreck();
    virtual void  setCriticalDamage(float);            virtual float getCriticalDamage();
    virtual void  setTimeToStayAsWreck(float);         virtual float getTimeToStayAsWreck();
    virtual void  setTimeToStayAfterDestroyed(float);  virtual float getTimeToStayAfterDestroyed();
    virtual void  setAlignLastEffectToHitDirection(bool);
    virtual bool  getAlignLastEffectToHitDirection();
    virtual void  addArmorEffect(int, const std::string&, const Vec3&, const Vec3&);
    virtual void  addArmorEffectSpectacular(int, const std::string&, const Vec3&, const Vec3&);
    virtual void  addWreckArmorEffect(int, const std::string&, const Vec3&, const Vec3&);
    virtual void  setDefaultMaterial(int);             virtual int   getDefaultMaterial() const;
    virtual void  setWreckDelay(float);                virtual float getWreckDelay();
    virtual void  setHideChildrenOnSpectacularDeath(bool);
    virtual bool  getHideChildrenOnSpectacularDeath();
    virtual void  setDestroyOnSpectacularDeath(bool);  virtual bool  getDestroyOnSpectacularDeath();
    virtual void  setCanBeDestroyed(bool);             virtual bool  getCanBeDestroyed();
    virtual void  setShowDamageAsDirt(float);          virtual float getShowDamageAsDirt() const;
    virtual void  setUseMMOnDeath(bool);               virtual bool  getUseMMOnDeath();
    virtual ~ArmorGLCompTemplate();
    virtual bool  makeScript(io::IStream*);

    // Non-virtual precache and repair-point helpers.
    void preCache();
    void addRepairPoint(const Vec3& position, float radius);

    // The two lower-level effect sinks the three console methods above are replayed into. They are
    // NOT console names themselves - createComponent calls them, once, off the queues.
    void addEffect(std::string name, Vec3 position, Vec3 direction,
                   int damageState, bool spectacular);
    void addWreckEffect(std::string name, Vec3 vec, int damageState);


    friend class ArmorGLComp;   // the component reads its template's fields directly, as the ELF does

protected:
    /*   8 */ int   m_refCount;

    // The first createComponent call clamps hitpoints, builds queued effects, and precaches.
    // Later calls only allocate the component.
    /*  12 */ bool  m_needsBuild;                    // ctor true
    /*  13 */ bool  m_alignLastEffectToHitDirection;
    /*  14 */ bool  _byteE;
    /*  15 */ bool  _byteF;

    // Owns damage-state effect maps and parallel queues of raw console arguments.
    // createComponent resolves each queued effect once; every writer must keep the parallel queues aligned.
    // Offsets describe the original ELF container layout, not the current compiler's STL.
    /*  16 */ std::map<int, DamageEffects*> m_damageEffects;        // +16 node ptr / +20 count
    /*  24 */ // [UNRESOLVED] 4 bytes the ctor never writes
    /*  28 */ std::map<int, DamageEffects*> m_wreckDamageEffects;   // +28 node ptr / +32 count
    /*  36 */ // [UNRESOLVED] 4 bytes the ctor never writes

    // The armor-effect queue: five parallel vectors, one element per console line.
    /*  40 */ std::vector<std::string> m_queuedEffectNames;   // -> addEffect arg 1
    /*  52 */ std::vector<bool>        m_queuedEffectFlags;   // Packed spectacular-effect flags.
    // Effect positions, directions, and states are consumed in lockstep with their names.
    /*  72 */ std::vector<Vec3> m_queuedEffectPos;       // -> addEffect arg 2
    /*  84 */ std::vector<Vec3> m_queuedEffectDir;       // -> addEffect arg 3
    /*  96 */ std::vector<int>  m_queuedEffectStates;    // -> addEffect arg 4, the damage state

    /* 108 */ std::vector<std::string> m_queuedWreckNames;   // -> addWreckEffect arg 1
    /* 120 */ std::vector<Vec3>        m_queuedWreckVecs;    // -> addWreckEffect arg 2
    // Retains the authoring direction for serialization; runtime construction never reads this queue.
    // addWreckEffect uses the queued position as both position and direction.
    /* 132 */ std::vector<Vec3> m_queuedWreckDirs;
    /* 144 */ std::vector<int>  m_queuedWreckValues;    // -> addWreckEffect arg 3

    /* 156 */ float m_showDamageAsDirt;

    // Hit points precede two adjacent 24-byte explosion blocks with matching field order.
    /* 160 */ float m_wreckHitPoints;
    /* 164 */ float m_hitPoints;
    /* 168 */ float m_maxHitPoints;
    /* 172 */ float m_maxWreckHitPoints;

    /* 176 */ float m_attackDamage;
    /* 180 */ float m_angleMod;
    /* 184 */ float m_speedMod;
    /* 188 */ int   m_defaultMaterial;

    // explosion, intact
    /* 192 */ float m_explosionRadius;
    /* 196 */ float m_explosionDamage;
    /* 200 */ int   m_explosionMaterial;
    /* 204 */ float m_explosionForce;
    /* 208 */ float m_explosionForceMod;
    /* 212 */ float m_explosionForceMax;

    // explosion, wreck
    /* 216 */ float m_wreckExplosionRadius;
    /* 220 */ float m_wreckExplosionDamage;
    /* 224 */ int   m_wreckExplosionMaterial;
    /* 228 */ float m_wreckExplosionForce;
    /* 232 */ float m_wreckExplosionForceMod;
    /* 236 */ float m_wreckExplosionForceMax;

    // environmental damage
    /* 240 */ float m_hpLostWhileCriticalDamage;
    /* 244 */ float m_hpLostWhileUpSideDown;
    /* 248 */ float m_hpLostWhileInWater;
    /* 252 */ float m_hpLostWhileInDeepWater;
    /* 256 */ float m_waterDamageDelay;
    /* 260 */ float m_deepWaterDamageDelay;
    /* 264 */ float m_waterLevel;
    /* 268 */ float m_deepWaterLevel;

    /* 272 */ bool  m_canBeRepairedWhenWreck;
    /* 273 */ bool  _byte111;                        // [UNRESOLVED]
    /* 274 */ bool  _byte112;
    /* 275 */ bool  _byte113;

    /* 276 */ float m_criticalDamage;
    /* 280 */ int   _dword118;                       // [UNRESOLVED]

    // death timing
    /* 284 */ float m_timeToStayAsWreck;
    /* 288 */ float m_timeToStayAfterDestroyed;
    /* 292 */ float m_wreckDelay;

    /* 296 */ bool  m_hideChildrenOnSpectacularDeath;
    /* 297 */ bool  m_destroyOnSpectacularDeath;
    /* 298 */ bool  m_canBeDestroyed;
    /* 299 */ bool  m_useMMOnDeath;

    // ALL three AT once, by addRepairPoint. They are the begin / end /
    // capacity of a std::vector<RepairPoint>: it indexes the span by `>> 4` (16-byte elements),
    // grows it by one and writes the last element's Vec3 and float. That also explains why a fourth
    // dword had to exist at +308 - the ctor zeroes all three - and closes THE proven size:
    // 308 + 4 = 312 = 0x138, the operator new argument.
    /* 300 */ std::vector<RepairPoint> m_repairPoints;   // +300 begin / +304 end / +308 capacity
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ARMORGLCOMPTEMPLATE_H
