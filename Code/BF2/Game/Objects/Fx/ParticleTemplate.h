// Path: [INFERRED] in Fx; recovered from the symbolized ELF.
// Legacy Particle configuration (CID 38011); retail ELF size 0x7F0.
// Own geometry/material behavior differs from SimpleObjectTemplate. Four
// OverTimeDistribution fields drive alpha, size, gravity modifier and drag.
// Runtime physics binding occurs at object spawn; field offsets below document the ELF.

#ifndef GAME_OBJECTS_FX_PARTICLETEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_FX_PARTICLETEMPLATE_H

#include <string>
#include <utility>
#include <vector>
#include "../ObjectTemplate.h"
#include "../../../Math/Vec3.h"
#include "../../../Core/Random.h"
#include "../../../Core/OverTimeDistribution.h" // Four effect curves.

namespace dice {
namespace hfe {

namespace world {

// The RTTI-attested +0x30 interface: identity + the geometry string, nothing else.
// getGeometry The ELF face table holds SIX entries, not five:
// the quartet, _ZThn48_...setGeometry (0x820C9C0) and _ZThn48_...getGeometry (0x820C9E0), at
// ..0x88AC57C; and BF2.exe's Particle geometry console row (0x5648C7) writes through this
// face at +0x10 and reads through it at +0x14. ParticleTemplate is the only implementor.
class IParticleTemplate
{
public:
    virtual void  addRef() = 0;                              // +0x00
    virtual int   getRef() = 0;                              // +0x04
    virtual int   release() = 0;                             // +0x08
    virtual void* queryInterface(unsigned int) const = 0;    // +0x0C
    virtual void  setGeometry(const std::string&) = 0;       // +0x10
    virtual const std::string& getGeometry() = 0;            // +0x14
};

// Legacy particle physics and appearance. The constructor's gravity sample is retained;
// changing the distribution does not resample it, while runtime particles sample separately.
class ParticleTemplate : public ObjectTemplate, public IParticleTemplate
{
public:

    ParticleTemplate();                                       // read in full

    // The per-class IBase quartet (refcount +0x34).
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                                  // delete on 0
    virtual void* queryInterface(unsigned int iid) const;     // the +0x30 interface hop
    virtual unsigned int getClassID() const;                  // reads the CID global
    virtual ~ParticleTemplate();                              // strings/vector teardown
    virtual void* queryImplementation(unsigned int classId);  // flat two-CID check
    virtual IObject* createObject() const;                    // ELF 7 B `return 0`; BF2.exe
                                                              // builds a world::Particle

    // The real physics binders - Object-spawn only: world::Particle's
    // constructor is their caller (ELF 0x820647C / 0x82064BD, BF2.exe 0x649E23 / 0x649E48).
    virtual void setPhysicsNodeComponent(IObject*) const;
    virtual void setResponsePhysicsComponent(IObject*) const;

    // The collision-mesh route - same registration pattern as SimpleObjectTemplate's, at
    // ParticleTemplate's own field.
    virtual void setCollisionMesh(const std::string& mesh);
    virtual const std::string& getCollisionMesh() const;

    virtual std::vector<std::pair<std::string, unsigned int> >* getMaterialVector();
    virtual void setMaterial(int material);
    virtual int  getMaterial() const;                         // the lazy first-named
                                                              // entry resolve
    // ELF8209E90 / dedicated PE602110: recovered conditional property writer, material loop,
    // five RandomDistributions and four sampled-curve writers. Inertia uses the shipped AND
    // test; curve output is probed before a second serialization. See the .cpp for field order.
    virtual bool makeScript(io::IStream*);                    // ELF/PE recovered

    virtual void setGeometry(const std::string& geometry);    // plain assign
    virtual const std::string& getGeometry();
    virtual void setGeometryPart(int part);
    virtual void setCollisionPart(int part);
    virtual void mapMaterial(unsigned int index, const std::string& name,
                             unsigned int material);          // resize + overwrite both

    // Non-virtual pair (0x820B9B0 / 0x820BA00): the +0x7C8 Vec4.
    void setTransparencyGraph(const Vec4& v);
    const Vec4& getTransparencyGraph() const;

    // Non-virtual, ELF 0x820C1D0 (61 B) / BF2.exe 0x64AA20: the three has*Physics console rows call
    // it after their store. The first set flag names the type - point (1), rotational point (2),
    // response (3) - and with none set the type is left alone.
    void checkPhysicsTypes();

    // Non-virtual, ELF 0x820C2D0 (77 B); BF2.exe inlines it into Particle::handleCollision
    // (0x64B5C0). The template named by collisionEffectName, resolved once and cached (a null
    // answer is simply retried next time). [INFERRED] return constness: the only caller hands it
    // straight to IGameLogic::playCollisionEffect.
    const IObjectTemplate* getCollisionEffectTemplate() const;

    // Particle configuration; documented offsets describe the ELF layout.
    int             m_refCount;                               // +0x34
    std::string     m_geometry;                               // +0x38 row 166
    // was m_field3C (int, "[UNRESOLVED]"). BF2.exe Particle::init (0x64A346..0x64A39A) lazily stores
    // GeometryTemplateManager::getTemplate(m_geometry) here (PE +0x84, right after the 28-byte
    // string) and builds the particle's renderable from it; the dedicated images zero it and never
    // read it (no renderer).
    IBase*          m_geometryTemplate;                       // +0x3C ctor 0 - client-only cache
    std::string     m_collisionMesh;                          // +0x40 row 169
    // was m_byte44: row hasCollisionPhysics (ELF ConsoleClass173 0x86E0C70 / BF2.exe 0x5651AB, a
    // raw byte in both arms); Particle's ctor turns OFHasCollision (0x100) on or off from it.
    bool            m_hasCollisionPhysics;                    // +0x44 ctor 0
    // +0x48 +0x58 AND THE four Vec3s below were by writing the
    // two physics binders - every one of them is a named argument to a named IPhysicsNode /
    // IResponsePhysics slot, and the ctor defaults corroborate each: a collision-group mask of
    // 56 = 0x38, a grip of 2, and an inertia modifier of (1,1,1).
    unsigned int    m_collisionGroups;                        // +0x48 ctor 56 (0x38) -> addToCollisionGroup
    // was m_byte4C/4D/4E: rows hasPointPhysics / hasRotationalPointPhysics / hasResponsePhysics
    // (ELF ConsoleClass175/176/174: store the byte, then call checkPhysicsTypes, which tests them
    // in exactly this order).
    bool            m_hasPointPhysics;                        // +0x4C ctor 1
    bool            m_hasRotationalPointPhysics;              // +0x4D ctor 0
    bool            m_hasResponsePhysics;                     // +0x4E ctor 0
    // was m_physicsNodeType [CHOSEN]: row physicsType (ELF ConsoleClass178), stored only for 0..4.
    int             m_physicsType;                            // +0x50 ctor 1 (1 point, 2 rotational, else full)
    // was m_hasPhysics [CHOSEN]: row hasMobilePhysics (ConsoleClass172, a raw byte both arms).
    bool            m_hasMobilePhysics;                       // +0x54 ctor 0 (0 -> StaticPhysicsNode)
    // Stored as an int, consumed as a byte: the binder reads `movzx eax, byte ptr [esi+58h]`.
    int             m_permanentGrip;                          // +0x58 ctor 2 -> setPermanentGrip
    int             m_geometryPart;                           // +0x5C ctor -1  row 167
    int             m_collisionPart;                          // +0x60 ctor 0   row 170
    Vec3            m_dragOffset;                             // +0x64 > setDragOffset             row 187
    float           m_mass;                                   // +0x70 ctor 0.001f, and the floor   row 188
    Vec3            m_centerOfMassOffset;                     // +0x74 > setCenterOfMassOffset     row 189
    Vec3            m_centerOfCollisionOffset;                // +0x80 > setCenterOfCollisionOffset row 190
    Vec3            m_inertiaModifier;                        // +0x8C ctor (1,1,1) -> setInertiaModifier row 191
    float           m_sampledValue;                           // +0x98 [CHOSEN name] the ctor's sample of m_gravityModifier
    // was m_byte9C. only BIT 0 IS ever : the ctor clears it (`and byte [esi+9Ch], 0FEh`),
    // row isSleeping (ConsoleClass192) rewrites it as `(b & 0FEh) | (v != 0)` and reads `b & 1`,
    // and the node binder tests `b & 1`. That is one-bit-field code; the other seven bits are never
    // written by anything recovered.
    unsigned char   m_isSleeping : 1;                         // +0x9C bit 0
    // was m_byte9D: row usedOnTransparentSurface (ConsoleClass168, a raw byte both arms); BF2.exe
    // Particle::init hands it to the geometry template's +0x48/+0x4C flag (0x64A366..0x64A38F).
    bool            m_usedOnTransparentSurface;               // +0x9D ctor 0
    unsigned char   _pad9E[2];
    // [CHOSEN] name for the rotational-physics buoyancy modifier.
    float           m_floaterMod;                             // +0xA0
    // was m_fieldA4: row otherCollisionLod (ConsoleClass194); the binder's setOtherCollisionLod.
    int             m_otherCollisionLod;                      // +0xA4 ctor -1
    // was m_fieldA8/AC/B0: rows sizeModifier (ConsoleClass201, Vec3) and sizeModifierX/Y/Z (198..200,
    // one float each); Particle::handleUpdate scales each positive axis by the sized curve.
    Vec3            m_sizeModifier;                           // +0xA8 ctor (1,1,1)
    // THE five RandomDistributions (0x14 each: distribution, value, variation, relative, Random) -
    // were twenty-five scalars. Each console row copies its block whole (five dwords, ELF
    // ConsoleClass183/184/195/185/186); world::Particle samples each once in its constructor.
    RandomDistribution m_timeToLive;                          // +0xB4 {none, 10, 0}  row 183
    RandomDistribution m_size;                                // +0xC8 {none,  1, 0}  row 184
    RandomDistribution m_transparency;                        // +0xDC {none,  0, 0}  row 195
    RandomDistribution m_gravityModifier;                     // +0xF0 {none,  1, 0}  row 185
    RandomDistribution m_drag;                                // +0x104 {none,  0, 0}  row 186
    // THE four curves (were SampledGraph m_graphs[4]), each OverTimeDistribution(1.0f).
    OverTimeDistribution m_alphaOverTime;                     // +0x118 row 205 - no reader recovered
    OverTimeDistribution m_sizeOverTime;                      // +0x2C4 row 202
    OverTimeDistribution m_gravityModifierOverTime;           // +0x470 row 203
    OverTimeDistribution m_dragOverTime;                      // +0x61C row 204
    Vec4            m_transparencyGraph;                      // +0x7C8 default (0,0,0,1)  row 196
    std::vector<std::pair<std::string, unsigned int> > m_materialVector;   // +0x7D8 row 206
    int             m_material;                               // +0x7E4 ctor -1
    // was m_string7E8: row collisionEffectName (ConsoleClass171, a whole-string assign both arms);
    // Particle::handleCollision plays it and destroys the particle.
    std::string     m_collisionEffectName;                    // +0x7E8
    // was m_field7EC: getCollisionEffectTemplate's cache (ELF 0x820C2DC reads, 0x820C313 writes).
    mutable const IObjectTemplate* m_collisionEffectTemplate; // +0x7EC ctor 0
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif
