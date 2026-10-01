// Physics debris spawned by legacy emitters, with sampled lifetime/material properties and time-varying curves.
// Dedicated factories return null. Preserve Object then IParticle base order; IParticle exposes only IBase.
// No new virtual slots are introduced and getClassID remains Object's implementation.
// Original size is ELF 0x168 / PE 0x180. Header placement is [INFERRED].

#ifndef GAME_OBJECTS_FX_PARTICLE_H     // [INFERRED] guard style
#define GAME_OBJECTS_FX_PARTICLE_H

#include "../Object.h"

namespace dice {
namespace hfe {
namespace world {

class Emitter;
class ParticleTemplate;

// RTTI-attested (ELF _ZTIN4dice3hfe5world9IParticleE 0x88AC800, a __si_class_type_info whose base
// is dice::hfe::IBase). The +0x12C face table carries the IBase quartet and nothing more, so the
// interface declares no member of its own. GameLogicCollision's damage rule tests objects with
// queryInterface(IID_IParticle) (50389) - it is this face that answers.
class IParticle : public IBase
{
};

class Particle : public Object, public IParticle
{
public:
    // ELF C2 0x8205FC0 / C1 0x82071F0 (identical clones, 4,643 B each - five RandomDistribution
    // samplers inlined); BF2.exe 0x649CA0, 724 B (the same body with the sampler out of line at
    // ). Read in full in both.
    explicit Particle(const ParticleTemplate* objectTemplate);
    // ELF D2 0x8208C90 / D1 0x8208D20 / D0 0x8208DB0; BF2.exe deleting 0x649F80 -> body 0x649B20.
    virtual ~Particle();

    // the IBase quartet, refcount at +0x130 (PE +0x148)
    // A second refcount. Object keeps its own at +0x84; these overrides never touch it.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    // Answers IID_IBase and IID_IParticle only - not IID_IObject, not IID_IConsoleSaveable.
    virtual void* queryInterface(unsigned int iid) const;

    // the Object overrides
    virtual void init();                                 // , 586 B
    virtual void handleFrameUpdate(float deltaTime);
    virtual void handleUpdate(float deltaTime, unsigned int flags);

    virtual bool handleCollision(IObject* other, const Vec3& first, const Vec3& normal,
                                 const Vec3& contact, int firstMaterial, int secondMaterial,
                                 CollisionFaceInfo* firstFace,
                                 CollisionFaceInfo* secondFace);

    virtual void updateGeometry();                       // , 399 B
    virtual int  getMaterial() const;                    // (shared)
    virtual void handleMessage(TemplateMessage message, IPlayer* player, bool recursive);


    // NON-virtual (in no table): the client Emitter's hook. ELF 0x8209400 / PE 0x649BC0 - it keeps
    // the emitter's object ID, not the pointer, and handleCollision resolves it through the object
    // manager's registry each time.
    void setEmitter(Emitter* emitter);

    // . The live-particle count: ++ in the constructor, -- in the
    // destructor, read by nothing recovered. Zero-initialised (the TU's static-init function
    // is merely keyed on it - it stores nothing here).
    static int mNoParticlesDebug;

private:
    // fields, ELF +0x130..+0x167 / PE +0x148..+0x17F (Object ends at 0x12C / 0x144; the
    // IParticle face pointer sits in between). Declaration order is the PE constructor's store
    // order (0x649CE5..0x649D15), which is also offset order.
    int      m_refCount;          // +0x130 +0x148   ctor 1
    // geom::IScaleable* of the geometry (IID 0xEBC881FA), resolved lazily by handleUpdate's size
    // arm and cached; the interface is declared locally in Particle.cpp, as GenericProjectile.cpp
    // declares its own.
    void*    m_scaleable;         // +0x134 +0x14C   ctor 0
    float    m_age;               // +0x138 +0x150   ctor 0; advanced by handleUpdate
    // The five per-instance samples of the template's RandomDistributions, taken in the ctor body.
    float    m_timeToLive;        // +0x13C +0x154   < 0 is reported and replaced by 0.5
    float    m_size;              // +0x140 +0x158   scales the sizeOverTime curve
    float    m_transparency;      // +0x144 +0x15C   alpha is multiplied by (1 - this)
    float    m_gravityModifier;   // +0x148 +0x160   scales gravityModifierOverTime
    float    m_drag;              // +0x14C +0x164   scales dragOverTime
    // [CHOSEN name] Set to 1 by handleFrameUpdate and never cleared: the particle does not age
    // until it has had at least one frame update. A dword in both images (4-byte store/compare).
    int      m_frameUpdated;      // +0x150 +0x168   ctor 0
    // The "<template name>_lightSource" LightSource the ctor may attach (none in stock content).
    IObject* m_lightSource;       // +0x154 +0x16C   ctor 0
    // unsigned: handleCollision hands its address to std::map<unsigned int, IObject*>::find as
    // the key (ELF 0x820924A `lea eax, [edi+158h]`), so the member is the key type itself.
    unsigned int m_emitterId;     // +0x158 +0x170   ctor 0xFFFFFFFF; reset after the first sound

public:
    // written directly by the client Emitter's emission arm, right after createObject and
    // before init (BF2.exe 0x6485A2 / 0x6485B6 / 0x6485C2 - no accessor exists in either image):
    // randomizeGeomPart set  -> m_randomizeGeomPart = true
    // else useGeomPart != -1 -> m_geometryPart = m_assignedGeometryPart = useGeomPart
    int      m_geometryPart;      // +0x15C +0x174   ctor -1; init falls back to the template's
    // [UNRESOLVED role] always written together with m_geometryPart (by the Emitter above and by
    // init's randomize arm) and read by nothing recovered. [CHOSEN name].
    int      m_assignedGeometryPart;   // +0x160 +0x178   ctor -1
    bool     m_randomizeGeomPart; // +0x164 +0x17C   ctor false
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_FX_PARTICLE_H
