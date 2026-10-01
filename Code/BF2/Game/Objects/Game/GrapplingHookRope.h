// Simulated grappling rope with attachment, traversal and network state. [INFERRED] Source placement.
// [PLATFORM] Base and STL layouts differ between original images; use typed members.

#ifndef GAME_OBJECTS_GAME_GRAPPLINGHOOKROPE_H
#define GAME_OBJECTS_GAME_GRAPPLINGHOOKROPE_H

#include <list>
#include <map>
#include <vector>

#include "../Base/SimpleObject.h"

namespace dice {
namespace hfe {

class GrapplingHookContainerCreateEvent;
class GrapplingHookContainerUpdateEvent;
class GrapplingHookContainerDetachEvent;
class GrapplingHookCreateEvent;
class GrapplingHookUpdateEvent;
class ServerConnection;
class ServerGameLogic;

namespace world {

class GrapplingHookRopeContainer;
class GrapplingHookRopeTemplate;
class IPlayer;
class ISoldier;
class RopeLink;
class Soldier;

// 44-byte particle record. GrapplingHookUpdateEvent writes the two leading Vec3s and byte +0x29;
// the constructor (ELF 0x81A6220) zeroes dwords +0x00..+0x24 and bytes +0x28/+0x29.
struct RopeParticle
{
    RopeParticle();

    /* +0x00 */ Vec3 m_position;
    /* +0x0C */ Vec3 m_previousPosition;          // [CHOSEN name]
    /* +0x18 */ Vec3 m_force;                     // accumulated impulse; cleared by doPhysics
    /* +0x24 */ float _state24;                   // low-motion contact accumulator [CHOSEN name]
    /* +0x28 */ bool _byte28;                     // per-fixed-step endpoint visit latch [CHOSEN name]
    /* +0x29 */ bool m_eventFlag;                 // GrapplingHookUpdateEvent's one-bit field
    /* +0x2A */ char _pad2A[2];
};

class GrapplingHookRope : public SimpleObject
{
public:
    explicit GrapplingHookRope(const GrapplingHookRopeTemplate*);
    virtual ~GrapplingHookRope();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual void  init();
    virtual void  destroy(bool force);
    virtual void  handleVisualUpdate(float deltaTime, float interpolation);
    virtual void  handleFrameUpdate(float deltaTime);

    void initialize();
    void attachSoldier(Soldier* soldier, int segmentIndex, float entryAngle);
    void initContainer(GrapplingHookRopeContainer*, Soldier*, int, float);
    bool checkForEntry(IPlayer* player);
    void detachSoldier(ISoldier* soldier);
    void setAwake();
    void destroyGrapplingHook();

private:
    friend class GrapplingHookRopeTemplate;

    void applyForces();
    void doPhysics();
    void constrainParticles();
    void handleHookCollision(int);
    void handleParticleCollision(std::vector<RopeParticle>::iterator, int);
    void handleLinkCollision();
    void setRopeLinksTransformations();
    void handleTimers(float);
    void hookRopeTo(RopeParticle* particle);
    void pullBackRope();                                                  // [INFERRED return]
    void setClimbablePartsOfRope();
    int findUnclimbableIndexFromLink(std::vector<RopeLink*>::iterator, int direction);
    int findLinkIndexWithinHeightRange(std::vector<RopeLink*>::iterator, int direction,
                                       float height, float minimum, float maximum);
    float findAttachAngle(int segmentIndex);
    void throwRope();
    void dropRope();
    Vec3 getDismountPosition(const Vec3&, const Vec3&, const RopeLink&);
    bool wireBoxCheckForCollision(const Vec3&, const float&, const float&);

    static int globalGrapplingHookRopeId;            // ds:0x8AAD020; local-only id source
    static float m_frameRate;                       // ds:0x8987A14, 0.033333335f
    static float m_kineticFriction;                 // ds:0x8987A04, 0.1f
    static float m_staticFriction;                  // ds:0x8987A08, 0.0001f (squared-speed gate)
    friend class GrapplingHookRopeContainer;
    friend class PlayerControlObject;
    friend class RopeLink;
    friend class ::dice::hfe::GrapplingHookContainerCreateEvent;
    friend class ::dice::hfe::GrapplingHookContainerUpdateEvent;
    friend class ::dice::hfe::GrapplingHookContainerDetachEvent;
    friend class ::dice::hfe::GrapplingHookCreateEvent;
    friend class ::dice::hfe::GrapplingHookUpdateEvent;
    friend class ::dice::hfe::ServerGameLogic; // drop retrieval waits for _float1C0 <= 0
    // clientSendDatabase's late-join replay reads back the same four creation fields that
    // GrapplingHookCreateEvent::executeClient writes (ELF 0x808ADA1..0x808AE4B / PE 0x54C7E0 arm).
    friend class ::dice::hfe::ServerConnection;

    /* ELF +0x138 */ int m_refCount;
    /* ELF +0x13C */ std::list<ISoldier*> m_soldiers;
    /* ELF +0x140 */ std::list<ISoldier*>::iterator m_soldierIterator;
    /* ELF +0x144 */ const GrapplingHookRopeTemplate* m_ropeTemplate;
    /* ELF +0x148 */ int m_networkableId;
    /* ELF +0x14C */ std::vector<RopeParticle> m_particles;
    /* ELF +0x158 */ std::vector<RopeLink*> m_ropeLinks;

    /* ELF +0x164 */ float _float164;                          // initialize <- template +0x174
    /* ELF +0x168 */ int _dword168;                            // initialize <- template +0x178
    /* ELF +0x16C */ int _dword16C;                            // initialize = _dword168 + 1
    /* ELF +0x170 */ int _dword170;                            // initialize = trunc(_dword168 * .8)
    /* ELF +0x174 */ float _float174;                          // _float164 / _dword168
    /* ELF +0x178 */ float _float178;                          // square of _float174
    /* ELF +0x17C */ float m_unusedLifetime;          // server lifetime while no riders [CHOSEN]
    /* ELF +0x180 */ int m_riderCount;                         // ctor 0
    /* ELF +0x184 */ Vec3 m_globalForce;              // added to every free particle, then cleared
    /* ELF +0x190 */ float m_deltaTime;               // last frame delta [CHOSEN]
    /* ELF +0x194 */ float m_frameAccumulator;                 // [CHOSEN name], ctor 0
    /* ELF +0x198 */ int m_constraintIterations;               // [CHOSEN name], ctor 6
    /* ELF +0x19C */ float _float19C;                          // ctor 0.2, initialize overwritten
    /* ELF +0x1A0 */ int   m_ownerPlayerId;
    /* ELF +0x1A4 */ float m_creationScalar;
    /* ELF +0x1A8 */ Vec3  m_creationPosition;
    /* ELF +0x1B4 */ Vec3  m_creationDirection;
    /* ELF +0x1C0 */ float _float1C0;                          // ctor 0.5
    /* ELF +0x1C4 */ RopeParticle* _particle1C4;               // [UNRESOLVED], ctor uninitialized
    /* ELF +0x1C8 */ bool _byte1C8;                            // ctor false; setAwake false
    /* ELF +0x1C9 */ unsigned char _pad1C9[3];
    /* ELF +0x1CC */ float _float1CC;                          // ctor 0.5
    /* ELF +0x1D0 */ RopeParticle* m_hookParticle;             // set once by hookRopeTo
    /* ELF +0x1D4 */ Vec3 m_hookPosition;                      // attachment position
    /* ELF +0x1E0 */ int _dword1E0;                            // ctor deliberately uninitialized
    /* ELF +0x1E4 */ float _float1E4;                          // ctor 0.3
    /* ELF +0x1E8 */ float _float1E8;                          // ctor 1.0
    /* ELF +0x1EC */ float m_timeAwake;               // reset by setAwake [CHOSEN]
    /* ELF +0x1F0 */ bool m_awake;                             // [CHOSEN name], ctor true
    /* ELF +0x1F1 */ unsigned char _pad1F1[3];
    /* ELF +0x1F4 */ float m_hookedAwakeTime;         // reset by setAwake [CHOSEN]
    /* ELF +0x1F8 */ float _float1F8;                          // ctor 6; initialize <- template +0x1D0
    /* ELF +0x1FC */ float m_updateEventAccumulator;  // local owner's wire cadence [CHOSEN]
    /* ELF +0x200 */ float m_updateEventInterval;     // ctor 4.0f [CHOSEN]
    /* ELF +0x204 */ bool _byte204;                            // ctor false [UNRESOLVED]
    /* ELF +0x205 */ bool m_initialized;                       // create-container event gate
    /* ELF +0x206 */ unsigned char _pad206[2];
    /* ELF +0x208 */ float _float208;                          // ctor 0.5 [UNRESOLVED]
    /* ELF +0x20C */ bool _byte20C;                            // ctor true [UNRESOLVED]
    /* ELF +0x20D */ bool _byte20D;                            // ctor true [UNRESOLVED]
    /* ELF +0x20E */ unsigned char _pad20E[2];
};

// ds:0x8AAD024. initialize inserts; destroy erases. Empty on non-SF maps.
extern std::map<int, GrapplingHookRope*> grapplingHookRopeMap;

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_GRAPPLINGHOOKROPE_H
