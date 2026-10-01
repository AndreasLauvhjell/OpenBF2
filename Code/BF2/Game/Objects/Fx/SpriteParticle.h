// Path: [INFERRED] beside the attested effects module. Dedicated PE/ELF
#ifndef GAME_OBJECTS_FX_SPRITEPARTICLE_H
#define GAME_OBJECTS_FX_SPRITEPARTICLE_H
#include "../Object.h"
#include "../ObjectTemplate.h"

namespace dice { namespace hfe { namespace world {

class SpriteParticleTemplate : public ObjectTemplate
{
public:
    SpriteParticleTemplate();
    virtual ~SpriteParticleTemplate();
    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual void* queryImplementation(unsigned int);
    virtual IObject* createObject() const;
    virtual bool makeScript(io::IStream*);
private:
    int m_refCount;                 // ELF +0x30 / PE +0x60
    void* m_particleSystem;         // ELF +0x34 / PE +0x64; ctor null, no dedicated consumer
};

class SpriteParticle : public Object
{
public:
    explicit SpriteParticle(const SpriteParticleTemplate*);
    virtual ~SpriteParticle();
    virtual void handleUpdate(float, unsigned int);
    virtual float getVisualBoundingRadius() const;
    void addParticle(const Vec3&, const Vec3&, float);
    void removeParticleSystem();
private:
    void* m_particleSystem;         // ELF +0x12C / PE +0x144
    float m_visualBoundingRadius;  // ELF +0x130 / PE +0x148
    bool m_removeParticleSystem;   // ELF +0x134 / PE +0x14C
};

} } }
#endif
