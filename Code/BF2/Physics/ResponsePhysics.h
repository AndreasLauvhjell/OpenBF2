// Mesh collision response paired with a rigid physics node. Header path [INFERRED].
// Accumulates contacts, position corrections and two effect records; uses its own reference counter.
// reset clears contact accumulators without clearing grip or effects. Destruction releases the
// contact storage and intrusive auxiliary record. Collision effects are disabled in the dedicated image.

#ifndef PHYSICS_RESPONSEPHYSICS_H     // [INFERRED] guard style
#define PHYSICS_RESPONSEPHYSICS_H

#include <vector>

#include "PointResponsePhysics.h"

namespace dice {
namespace hfe {
namespace world {

class EngineTemplate;
class IObjectTemplate;

class ResponsePhysics : public PointResponsePhysics
{
    friend class PhysicsNode;                        // updatePhysics reads the +0xF4 resting-on block
public:
    // Forty-eight-byte contact record. [CHOSEN] Field names describe values stored by impulseOn.
    struct Contact
    {
        /* +0x00 */ Vec3  position;          // where the contact is (the caller's point)
        /* +0x0C */ Vec3  normal;            // the surface normal
        /* +0x18 */ Vec3  negativeSpeed;     // -(the caller's speed at the point)
        /* +0x24 */ float normalSpeed;       // -(speed . normal): the closing speed
        /* +0x28 */ float elasticity;        // the two materials' mean
        /* +0x2C */ float friction;          // the two materials' mean
    };

    // The 60-byte effect record - two of them, +0x124 and +0x160. Names from the ctor's defaults,
    // updateEffect, and addEffect / solveImpulse, which book and fire them: record 0
    // is the pair's effect template, record 1 its sound. active: 0 free, 1 booked, 2 fired.
    struct EffectRecord
    {
        /* +0x00 */ int   active;                    // 0; cleared by updateEffect at timeout
        /* +0x04 */ int   id0;                       // -1; the own material
        /* +0x08 */ int   id1;                       // -1; the other material
        /* +0x0C */ void* sound;                     // 0; record 1: MaterialManager::getSound's object
        /* +0x10 */ float speedSq;                   // 0; the booking contact's |speed|^2; cleared at timeout
        /* +0x14 */ float timer;                     // 0; updateEffect's countdown, 0.5..1.5 s
        /* +0x18 */ Vec3  position;                  // 0; the contact point
        /* +0x24 */ Vec3  normal;                    // (0, 1, 0)
        /* +0x30 */ Vec3  direction;                 // (0, -1, 0); the contact speed, normalised
    };

    ResponsePhysics();
    virtual ~ResponsePhysics();                      // 143 B - see the header

    virtual void  addRef();                          // the +0x90 counter
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;

    virtual void  reset();
    virtual unsigned int getClassID() const;         // CID_ResponsePhysics
    virtual void  checkObjectVsObject(IObject*, IObject*, float, bool);
    virtual void  checkVsTerrain(float);
    virtual void  impulseOn(const Vec3&, const Vec3&, const Vec3&, float, int, int);
    virtual void  solveImpulse(IPhysicsNode*);
    virtual void  addFriction(IPhysicsNode*);
    virtual void  setCollisionChecked(unsigned int); // +0x2C
    virtual bool  getIsIntersectingTerrain(Vec3&) const;   // +0xF8 / +0x104
    virtual unsigned char getPermanentGrip() const;  // +0xED
    virtual void  setPermanentGrip(unsigned char);
    virtual const Vec3& getPositionalAdjusts() const;      // +0x94
    virtual void  updateEffect(float dt);

    // Non-virtual.
    void computeFriction(Vec3*, float*, float*, const EngineTemplate*, const Vec3&, float,
                         const Vec3&, const Vec3&, bool);
    void addFrictionNew(IPhysicsNode*);
    void addEffect(int, int, Vec3, float, const Vec3&, const Vec3&);
    void playCollisionEffect(const Vec3&, const IObjectTemplate*, const Vec3&, const Vec3&);   // ELF 13 B empty / BF2.exe 0x6E5EF0
    bool getShouldTestDeathMaterial();                             // the
                                                                   //   +0x108 cache; the Engine child's
                                                                   //   slot +0x1AC == 33 test

protected:
    // Layout 0x90..0x19B, every default from the 499-byte ctor.
    /* +0x090 */ int     m_refCount;                 // 1
    /* +0x094 */ Vec3    m_positionalAdjusts;        // 0 - setAdjustVector of -depth * normal per contact
    /* +0x0A0 */ Vec3    m_rootPositionalAdjusts;    // 0 - the root response's adjusts, copied by impulseOn
    /* +0x0AC */ Vec3    m_speedAdjusts;             // 0 - setAdjustVector of the closing speed along the normal
    /* +0x0B8 */ Vec3    m_averageNormal;            // 0 - addAdjustVector over the frame's contacts
    /* +0x0C4 */ Vec3    m_averageSpeed;             // 0 - addAdjustVector
    /* +0x0D0 */ Vec3    m_averagePosition;          // 0 - addAdjustVector
    /* +0x0DC */ int     m_contactCount;             // 0, zeroed by reset - impulseOn's counter
    /* +0x0E0 */ float   m_averageFriction;          // 1.0f - the last contact's material mean
    /* +0x0E4 */ float   m_averageElasticity;        // 0
    /* +0x0E8 */ float   m_averageResistance;        // 0
    /* +0x0EC */ unsigned char m_grip;               // 0 - getPermanentGrip() | (old & 0x40), impulseOn
    /* +0x0ED */ unsigned char m_permanentGrip;      // 1
    /* +0x0F0 */ int     m_dwordF0;                  // 0
    /* +0x0F4 */ int*    m_sharedF4;                 // 0 - WeakPtr<IObject>::Ctrl*, stored when a
                                                     //   heavier dynamic other object is hit; kept
                                                     //   raw here to preserve the already-frozen layout
    /* +0x0F8 */ Vec3    m_intersectingTerrainPoint; // 0
    /* +0x104 */ bool    m_isIntersectingTerrain;    // false
    /* +0x108 */ int     m_shouldTestDeathMaterial;  // -1 = not yet computed
    /* +0x10C */ float   m_peakAdjustSq;             // 0 - solveImpulse: the largest |adjust|^2 seen (server)
    /* +0x110 */ int     m_pushLevel;                // -1 - solveImpulse: -1 per frame, +2 per hard push, clamped [-1, 3]
    /* +0x114 */ float   m_hardPush;                 // 0 - solveImpulse writes it; checkObjectVsObject
                                                     //   multiplies both callback speeds by (1 + value)
    /* +0x118 */ std::vector<Contact> m_contacts;    // empty (12 B: begin/end/cap)
    /* +0x124 */ EffectRecord m_effect0;             // see the header
    /* +0x160 */ EffectRecord m_effect1;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_RESPONSEPHYSICS_H
