// Path: [INFERRED] beside attested GenericProjectile.cpp; recovered from the ELF.
// Projectile template (CID 38201) with component-defined collision and detonation behavior.
// [PLATFORM] documented offsets are retail ELF: size 0x224, IProjectileObjectTemplate +0x17C.
// MSVC x86 uses size 0x2FC and interface +0x204; do not force STL layouts to ELF offsets.

#ifndef GAME_OBJECTS_PROJECTILES_GENERICPROJECTILETEMPLATE_H   // [INFERRED] guard style
#define GAME_OBJECTS_PROJECTILES_GENERICPROJECTILETEMPLATE_H

#include <string>
#include "../Base/BundleTemplate.h"
#include "../../../Core/Random.h"

namespace dice {
namespace hfe {
namespace world {

class Material;      // Physics/Material.h - the +0x198 cache's pointee

// The projectile-template interface - name attested (_ZTIN4dice3hfe5world25IProjectileObjectTemplateE;
// IID_IProjectileObjectTemplate = 38082 selects it in queryInterface). 13 slots from the -0x17C
// thunk table. Its order differs from the class's own tail: setLocalPredictOnClient comes last
// here, and getLocalPredictOnClient is NOT part of the interface at all.
class IProjectileObjectTemplate
{
public:
    virtual void  addRef() = 0;                                          // +0x00
    virtual int   getRef() = 0;                                          // +0x04
    virtual int   release() = 0;                                         // +0x08
    virtual void* queryInterface(unsigned int) const = 0;                // +0x0C
    virtual RandomDistribution& getTimeToLive() = 0;                     // +0x10
    virtual void  setDamageMandownSoldiers(bool) = 0;                    // +0x14
    virtual bool  getDamageMandownSoldiers() const = 0;                  // +0x18
    virtual void  setTracerTemplateName(const std::string&) = 0;         // +0x1C
    virtual const std::string& getTracerTemplateName() const = 0;        // +0x20
    virtual IObjectTemplate* getTracerTemplate() const = 0;              // +0x24
    virtual void  setTracerInterval(int) = 0;                            // +0x28
    virtual int   getTracerInterval() const = 0;                         // +0x2C
    virtual void  setLocalPredictOnClient(bool) = 0;                     // +0x30
};

// Projectile tuning and cached tracer/effect templates. Preserve both interface bases.
// Arming delay stores seconds and 30 Hz ticks; minDamage == -1 uses projectileDamage.
class GenericProjectileTemplate : public BundleTemplate, public IProjectileObjectTemplate
{
    // Runtime projectiles read configuration and maintain lazy caches directly.
    friend class GenericProjectile;

public:
    GenericProjectileTemplate();                              // read in full

    // The per-class IBase quartet (refcount at +0x180; release deletes on the 1->0 edge via D0).
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;     // the five-interface hop

    virtual unsigned int getClassID() const;                  // reads the CID global
    virtual ~GenericProjectileTemplate();                     // four strings + Bundle
    virtual void templateInitialize();                        // its own explicit empty body
    virtual void* queryImplementation(unsigned int classId);  // CID hop then Bundle's chain
    virtual IObject* createObject() const;                    // new(0x238) GenericProjectile
    virtual void preCache() const;                            // complete resource/id path
                                                              // (throwaway + tracer pair + pre-store)
    virtual bool makeScript(io::IStream*);

    // An override of SimpleObjectTemplate::setGeometry (+0xF0) that does NOT run Simple's
    // resolution logic - it is a raw assign into m_geometry (+0x124). As shipped.
    virtual void setGeometry(const std::string& geometry);

    // the ten own accessors (+0x140..+0x164), all read
    virtual RandomDistribution& getTimeToLive();              // reference to the +0x184 RD
    virtual void  setDamageMandownSoldiers(bool);
    virtual bool  getDamageMandownSoldiers() const;
    virtual void  setLocalPredictOnClient(bool);
    virtual bool  getLocalPredictOnClient() const;
    virtual void  setTracerTemplateName(const std::string&);  // assigns AND nulls the cache
    virtual const std::string& getTracerTemplateName() const;
    virtual IObjectTemplate* getTracerTemplate() const;       // ELF 14 B raw read; BF2.exe 0x6126D0
                                                              // (42 B) fills the cache first
                                                              // (client-only lazy lookup)
    virtual void  setTracerInterval(int);                     // INT (extender said float:
    virtual int   getTracerInterval() const;

    // non-virtual, both read
    IObjectTemplate* getWaterTracerTemplate() const;          // ELF 14 B raw read; BF2.exe inlines
                                                              // the same lazy lookup into init
    void  setArmingDelay(float seconds);                      // stores seconds AND
                                                              // int(seconds*30) ticks
    float getMinDamage() const;                               // the -1.0f sentinel rule

    // retail ELF fields (+0x180..+0x223; documentary offsets, ctor values above)
    int             m_refCount;                               // +0x180 ctor 1
    RandomDistribution m_timeToLive;                          // +0x184 uniform 3.0f at ctor tail
    // Lazily resolved material cache, populated by projectile construction.
    Material*       m_field198;                               // +0x198 ctor 0 - the material cache
    // Tracer scaling parameters and the launch-relative team-kill expiry window.
    // A timeToRemoveTK value of -1 disables expiry.
    float           m_maxTracerScaler;                        // +0x19C ctor 100.0f  console maxTracerScaler
    float           m_minTracerScaler;                        // +0x1A0 ctor   5.0f  console minTracerScaler
    float           m_tracerScaler;                           // +0x1A4 ctor   0.0f  console tracerScaler - also the
                                                              // `> 0` gate that enables tracers at all
    float           m_tracerSizeModifier;                     // +0x1A8 ctor   1.0f  console tracerSizeModifier
    float           m_timeToRemoveTK;                         // +0x1AC ctor  -1.0f  console timeToRemoveTK
                                                              // (-1 disables; seconds otherwise)
    float           m_field1B0;                               // +0x1B0 ctor -1.0f
    // The two caches are filled on the client by the getters themselves (BF2.exe 0x6126D0 and
    // the copy inlined into GenericProjectile::init). Both getters are const in both images'
    // mangled names, so the caches are mutable; neither the layout nor any signature changes.
    // Each holds an erased geom::IGeometryTemplate* from GeometryTemplateManager::getTemplate.
    std::string     m_tracerTemplateName;                     // +0x1B4
    mutable IObjectTemplate* m_tracerTemplate;                // +0x1B8 cache, ctor 0 (client: lazy)
    std::string     m_waterTracerTemplateName;                // +0x1BC [INFERRED name]
    mutable IObjectTemplate* m_waterTracerTemplate;           // +0x1C0 cache, ctor 0 (client: lazy)
    int             m_tracerInterval;                         // +0x1C4 ctor 1
    // Distance at which tracer convergence begins.
    float           m_tracerConvergeDistance;                 // +0x1C8 ctor 100.0f
    float           m_armingDelay;                            // +0x1CC via setArmingDelay(0)
    int             m_armingDelayTicks;                       // +0x1D0 = int(delay * 30)
    float           m_field1D4;                               // +0x1D4 ctor 0.0f - start-effect delay
    int             m_field1D8;                               // +0x1D8 NOT initialised (as
                                                              // shipped - the ctor skips it)
    float           m_projectileDamage;                       // +0x1DC ctor 10.0f - named by
                                                              // getProjectileDamage; also
                                                              // fallback)
    float           m_minDamage;                              // +0x1E0 ctor -1.0f (sentinel)
    // Damage-distance thresholds, acceleration, and speed limit.
    float           m_distToStartLoseDamage;                  // +0x1E4 ctor 0.0f
    float           m_distToMinDamage;                        // +0x1E8 ctor 0.0f
    float           m_field1EC, m_field1F0;                   // ctor 0.0f
    // Four separately addressed byte flags; preserve their widths and constructor defaults.
    unsigned char   m_hasOnTimeEffect;                        // +0x1F4 ctor 1
    unsigned char   m_invisible;                              // +0x1F5 ctor 0
    unsigned char   m_addToProjectileList;                    // +0x1F6 ctor 0
    unsigned char   m_useProjectileCamera;                    // +0x1F7 ctor 0
    unsigned char   m_byte1F8;                                // +0x1F8 ctor 0 - damageOwner
    unsigned char   m_damageMandownSoldiers;                  // +0x1F9 ctor 0
    unsigned char   m_byte1FA;                                // +0x1FA ctor 0 - visibleOn3dMap
    int             m_field1FC;                               // +0x1FC ctor -1 - 3dMapIcon
    unsigned char   m_localPredictOnClient;                   // +0x200 ctor 0
    float           m_field204;                               // +0x204 ctor 0.5f - water velocity mod
    // Trail names and cached templates selected by the activation depth test.
    // Creation samples the selected effect distribution and suppresses only the -1 result.
    std::string     m_string208;                              // +0x208 underwater trail effect name
    std::string     m_string20C;                              // +0x20C above-water trail effect name
    IObjectTemplate* m_field210;                              // +0x210 ctor 0 - +0x208's cache
    IObjectTemplate* m_field214;                              // +0x214 ctor 0 - +0x20C's cache
    // Stopped-projectile detonation and pickup settings.
    unsigned char   m_detonateAfterProjectileStopped;         // +0x218 ctor 0
    float           m_detonateAfterProjectileStoppedMinSpeed; // +0x21C ctor 0.125f
    unsigned char   m_canPickup;                              // +0x220 ctor 0
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif
