// Collision and damage properties for one material ID. Header path [INFERRED].
// IMaterial owns no IBase reference-counting slots; its virtual destructor precedes property accessors.
// Preserve accessor order and native string layout. MaterialManager owns and deletes material records.

#ifndef PHYSICS_MATERIAL_H     // [INFERRED] guard style
#define PHYSICS_MATERIAL_H

#include <string>

namespace dice {
namespace hfe {
namespace world {

class IMaterial
{
public:
    virtual ~IMaterial() {}                                            // slots 0/1 (D1/D0)
    virtual int   getType() const = 0;                                 // 2
    virtual void  setType(int type) = 0;                               // 3
    virtual void  setName(const std::string& name) = 0;                // 4
    virtual std::string getName() = 0;                                 // 5  (0x810D8C0, 41 B: a copy)
    virtual void  setFriction(float v) = 0;                            // 6
    virtual float getFriction() const = 0;                             // 7
    virtual void  setElasticity(float v) = 0;                          // 8
    virtual float getElasticity() const = 0;                           // 9
    virtual void  setResistance(float v) = 0;                          // 10
    virtual float getResistance() const = 0;                           // 11
    virtual void  setProjectileCollisionHardness(float v) = 0;         // 12
    virtual float getProjectileCollisionHardness() const = 0;          // 13
    virtual void  setDamageLoss(float v) = 0;                          // 14
    virtual float getDamageLoss() const = 0;                           // 15
    virtual void  setMinDamageLoss(float v) = 0;                       // 16
    virtual float getMinDamageLoss() const = 0;                        // 17
    virtual void  setMaxDamageLoss(float v) = 0;                       // 18
    virtual float getMaxDamageLoss() const = 0;                        // 19
    virtual void  setPenetrationDeviation(float v) = 0;                // 20
    virtual float getPenetrationDeviation() const = 0;                 // 21
    virtual void  setOverrideNeverPenetrate(bool b) = 0;               // 22
    virtual bool  getOverrideNeverPenetrate() const = 0;               // 23
    virtual void  setHasWaterPhysics(bool b) = 0;                      // 24
    virtual bool  getHasWaterPhysics() const = 0;                      // 25  +0x64
    virtual void  setIsBarbwire(bool b) = 0;                           // 26
    virtual bool  getIsBarbwire() const = 0;                           // 27
    virtual void  setIsOneSided(bool b) = 0;                           // 28
    virtual bool  getIsOneSided() const = 0;                           // 29
    virtual void  setIsSeeThrough(bool b) = 0;                         // 30
    virtual bool  getIsSeeThrough() const = 0;                         // 31
};

class Material : public IMaterial
{
public:
    Material()
        : m_type(1), m_friction(1.0f), m_elasticity(0.0f), m_resistance(0.01f),
          m_projectileCollisionHardness(0.0f), m_damageLoss(-1.0f), m_minDamageLoss(0.0f),
          m_maxDamageLoss(100000.0f), m_penetrationDeviation(0.1f),
          m_overrideNeverPenetrate(false), m_hasWaterPhysics(false), m_isBarbwire(false),
          m_isOneSided(false), m_isSeeThrough(false)
    {}
    virtual ~Material() {}

    virtual int   getType() const                       { return m_type; }
    virtual void  setType(int type)                     { m_type = type; }
    virtual void  setName(const std::string& name)      { m_name = name; }
    virtual std::string getName()                       { return m_name; }
    virtual void  setFriction(float v)                  { m_friction = v; }
    virtual float getFriction() const                   { return m_friction; }
    virtual void  setElasticity(float v)                { m_elasticity = v; }
    virtual float getElasticity() const                 { return m_elasticity; }
    virtual void  setResistance(float v)                { m_resistance = v; }
    virtual float getResistance() const                 { return m_resistance; }
    virtual void  setProjectileCollisionHardness(float v) { m_projectileCollisionHardness = v; }
    virtual float getProjectileCollisionHardness() const  { return m_projectileCollisionHardness; }
    virtual void  setDamageLoss(float v)                { m_damageLoss = v; }
    virtual float getDamageLoss() const                 { return m_damageLoss; }
    virtual void  setMinDamageLoss(float v)             { m_minDamageLoss = v; }
    virtual float getMinDamageLoss() const              { return m_minDamageLoss; }
    virtual void  setMaxDamageLoss(float v)             { m_maxDamageLoss = v; }
    virtual float getMaxDamageLoss() const              { return m_maxDamageLoss; }
    virtual void  setPenetrationDeviation(float v)      { m_penetrationDeviation = v; }
    virtual float getPenetrationDeviation() const       { return m_penetrationDeviation; }
    virtual void  setOverrideNeverPenetrate(bool b)     { m_overrideNeverPenetrate = b; }
    virtual bool  getOverrideNeverPenetrate() const     { return m_overrideNeverPenetrate; }
    virtual void  setHasWaterPhysics(bool b)            { m_hasWaterPhysics = b; }
    virtual bool  getHasWaterPhysics() const            { return m_hasWaterPhysics; }
    virtual void  setIsBarbwire(bool b)                 { m_isBarbwire = b; }
    virtual bool  getIsBarbwire() const                 { return m_isBarbwire; }
    virtual void  setIsOneSided(bool b)                 { m_isOneSided = b; }
    virtual bool  getIsOneSided() const                 { return m_isOneSided; }
    virtual void  setIsSeeThrough(bool b)               { m_isSeeThrough = b; }
    virtual bool  getIsSeeThrough() const               { return m_isSeeThrough; }

private:
    std::string m_name;                          // +0x04
    int   m_type;                                // +0x08
    float m_friction;                            // +0x0C
    float m_elasticity;                          // +0x10
    float m_resistance;                          // +0x14
    float m_projectileCollisionHardness;         // +0x18
    float m_damageLoss;                          // +0x1C
    float m_minDamageLoss;                       // +0x20
    float m_maxDamageLoss;                       // +0x24
    float m_penetrationDeviation;                // +0x28
    bool  m_overrideNeverPenetrate;              // +0x2C
    bool  m_hasWaterPhysics;                     // +0x2D
    bool  m_isBarbwire;                          // +0x2E
    bool  m_isOneSided;                          // +0x2F
    bool  m_isSeeThrough;                        // +0x30
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_MATERIAL_H
