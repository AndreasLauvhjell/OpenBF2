// SimpleObject emitter controlled by its effect bundle's start/stop messages.
// Client updates spawn template/alternate objects with sampled transforms and physics; particles retain the emitter
// for collision sounds. Dedicated creation returns null and excludes emission/camera code at compile time.
// Adds no virtual slots beyond SimpleObject. Header placement is [INFERRED] beside the attested implementation.

#ifndef GAME_OBJECTS_FX_EMITTER_H     // [INFERRED] guard style
#define GAME_OBJECTS_FX_EMITTER_H

#include "../Base/SimpleObject.h"

namespace dice {
namespace hfe {
namespace world {

class EmitterTemplate;
class EffectBundle;

class Emitter : public SimpleObject
{
public:
    // ELF C2 0x81FE310 / C1 0x81FE3D0 (186 B each) / BF2.exe 0x647D20 (143 B). Object size ELF 0x16C,
    // BF2.exe 0x184 (the operator new in EmitterTemplate::createObject, 0x648C4C).
    explicit Emitter(const EmitterTemplate* objectTemplate);
    // ELF D2 0x81FE490 / D1 0x81FE4C0 / D0 0x81FE4F0; BF2.exe scalar-deleting 0x647DB0. No own work.
    virtual ~Emitter();

    // the six overrides, in vtable order
    virtual void init();                                              // +0x1C
    virtual void handleVisualUpdate(float deltaTime, float distance); // +0xA4
    virtual void handleFrameUpdate(float deltaTime);                  // +0xA8
    virtual void handleUpdate(float deltaTime, unsigned int flags);   // ELF slot 44 / PE slot 43 (+0xAC): ELF 0x81FE9E0 (empty) / PE 0x647DF0
    virtual void handleMessage(TemplateMessage message, IPlayer* player,
                               bool recursive);                      // +0x100

    // [sic] "Itensity" - dice's spelling, attested by the mangled name
    // _ZNK4dice3hfe5world7Emitter15calcInvItensityEPKNS1_15EmitterTemplateEf (ELF 0x81FE590, 1,011 B /
    // BF2.exe 0x6479E0, 243 B). The seconds until the next particle. Return type float: both bodies
    // leave the result in st0 and every caller stores or adds it as a float.
    float calcInvItensity(const EmitterTemplate* objectTemplate, float speed) const;

    // _ZN4dice3hfe5world7Emitter26playParticleCollisionSoundERKNS0_4Vec3Eii (ELF 0x81FF360, 106 B /
    // BF2.exe 0x647B50, 60 B). world::Particle::handleCollision's call. Return type [INFERRED] void:
    // neither body materialises a value (eax is whatever the last call left). Parameter names
    // [INFERRED]: they are MaterialManager::getSound's first two (the cell's material pair).
    void playParticleCollisionSound(const Vec3& position, int material, int otherMaterial);

    // Inverse of Particles.enableOldParticles; suppresses legacy client emission when set.
    // The shared all-effects switch is g_effectsDisabled; do not create a second independent copy here.
    static bool mDebugNoOldParticles;

protected:
    // EffectBundle::init writes m_lodDistance RAW (ELF 0x81F9C00 / BF2.exe 0x6413B0): a bundle caps the
    // LOD of every emitter child whose template's lodDistance exceeds its own.
    friend class EffectBundle;

    // own fields, ELF +0x138.. / BF2.exe +0x150.. (SimpleObject is 0x138 / 0x150). Every offset is
    // read from the two constructors and the bodies below; the names are [CHOSEN] from use unless noted.
    // The members are all 4-byte (x86) and declared in retail order, so on the x86 client this class
    // keeps retail's relative layout: m_lodDistance sits at sizeof(SimpleObject) + 0x28 (asserted in
    // Emitter.cpp).
    IObjectTemplate* m_particleTemplate; // ELF +0x138 / PE +0x150  ctor 0. The main `template`, looked up
                                         // by name on the first start; handleUpdate returns early while
                                         // it is null (so a missing template never emits).
    float m_unknown13C;                  // ELF +0x13C / PE +0x154  ctor -1.0f. [UNRESOLVED] - no reader in
                                         // either image's Emitter bodies, nor in world::Particle.
    float m_delay;                       // ELF +0x140 / PE +0x158  NOT written by either constructor.
                                         // Sampled from the template's `delay` on start and on each loop;
                                         // handleUpdate counts it down before any emission.
    float m_timeToLive;                  // ELF +0x144 / PE +0x15C  NOT written by either constructor.
                                         // Sampled from `timeToLive` on start/loop (a loop floors it at
                                         // 0.01); the emission window, measured against m_age.
    float m_age;                         // ELF +0x148 / PE +0x160  ctor 0. Seconds of emission so far;
                                         // advances only after the first handleFrameUpdate. start's
                                         // "fresh start" block runs only while it is exactly 0.
    float m_nextEmitTime;                // ELF +0x14C / PE +0x164  ctor 0. The age at which the next
                                         // particle is due; grows by calcInvItensity per particle.
    Vec3  m_lastPosition;                // ELF +0x150 / PE +0x168  ctor 0. The emitter position at the end of
                                         // the last emitting update; spawns are interpolated from it.
    unsigned int m_visibleFrame;         // ELF +0x15C / PE +0x174  ctor 0. The renderer frame ordinal + 100
                                         // (world::getCurrentFrame) of the last visual update within
                                         // LOD range; 0 = out of range. handleUpdate spawns only while
                                         // the current ordinal has not passed it. Always 0 on a server.
    float m_lodDistance;                 // ELF +0x160 / PE +0x178  ctor 300.0f; init copies the template's
                                         // `lodDistance`; EffectBundle::init may lower it.
    bool  m_hasFrameUpdate;              // ELF +0x164 / PE +0x17C  ctor false. Latched true by
                                         // handleFrameUpdate, cleared by stop; m_age only advances while set.
    float m_startTime;                   // ELF +0x168 / PE +0x180  ctor -1.0f. (float)g_system->getExactTime()
                                         // at start; a non-looping emitter stops itself once more than
                                         // m_timeToLive seconds of wall time have passed since.
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_FX_EMITTER_H
