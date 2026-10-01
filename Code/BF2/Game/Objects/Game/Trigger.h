// Spherical trigger volume activating matching effectors on events or timed proximity checks.
// Applies probability and minimum-delay limits. Downstream effectors retain their own implementation boundaries.

#ifndef GAME_OBJECTS_GAME_TRIGGER_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_TRIGGER_H

#include <vector>

#include "../Base/Bundle.h"
#include "../Base/BundleTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class IPlayer;
class Trigger;

// The event kinds updateOnEvent dispatches on (the ELF's `dice::hfe::world::TriggerType` enum;
// values read from updateOnEvent's switch - only these three are handled, everything else falls to
// the chance roll with a null source and a zero position). Names [CHOSEN] from the source object
// each arm reads: 2 asks GenericFireArm's vcall +0x1C0 for its player, 8 takes the
// source's position, 16 takes the position AND keeps the source as the effector's cause.
enum TriggerType
{
    TriggerType_Weapon       = 2,
    TriggerType_Object       = 8,
    TriggerType_ControlPoint = 16
};

// Configures trigger id, event mask, probability, timing and per-player activation.
// [UNRESOLVED] Radius has no identified console binding in this property group.
class TriggerTemplate : public BundleTemplate
{
public:
    TriggerTemplate();                                       // read in full
    virtual unsigned int getClassID() const;
    virtual ~TriggerTemplate();                              // (D1) / 65 B (D0)
    virtual void* queryImplementation(unsigned int classId); // one-cid hop into Bundle's
    virtual IObject* createObject() const;                   // new(0x16C) Trigger(this)
    virtual bool  makeScript(io::IStream*);

    // The quartet, refcount at +0x17C.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;

    // the ONE accessor - clamps to [0, 1] before storing. `chance`'s SET arm.
    void setChance(float chance);

    // Read by the object half (Trigger) and the console GET arms.
    int   getEvents() const                          { return m_events; }
    float getRadius() const                          { return m_radius; }
    int   getTriggerId() const                       { return m_triggerId; }
    float getChance() const                          { return m_chance; }
    float getAverageTimeBetweenTests() const         { return m_averageTimeBetweenTests; }
    float getMinimumTimeBetweenTriggering() const    { return m_minimumTimeBetweenTriggering; }
    bool  getTriggerForEachActivatingPlayer() const  { return m_triggerForEachActivatingPlayer; }

    // The console field bindings write these directly (BF2_CONSOLE_FIELD's shape is the ELF's:
    // the executor stores into the template with no accessor between).
    /* 0x17C */ int   m_refCount;                    // ctor 1
    /* 0x180 */ int   m_events;                      // ctor 0  - `events`: the TriggerType mask
                                                     // updateOnTime tests (bit 2 = "only when the
                                                     // PCO is moving")
    /* 0x184 */ float m_radius;                      // ctor 0.0f - the sphere; see the above
    /* 0x188 */ int   m_triggerId;                   // ctor -1 - links effectors by id
    /* 0x18C */ float m_chance;                      // ctor 0.0f - via setChance (clamped)
    /* 0x190 */ float m_averageTimeBetweenTests;     // ctor 0.0f - handleTrigger's exponent base
                                                     // (max(1.0, value))
    /* 0x194 */ float m_minimumTimeBetweenTriggering;// ctor 1.0f - seconds between executions
    /* 0x198 */ bool  m_triggerForEachActivatingPlayer;   // ctor false - updateOnTime keeps
                                                     // walking after the first hit when set
    /* 0x199 */ char  _pad199[3];
};

// Trigger instance managed by TriggerManager; its nonvirtual updates select and execute effectors.
class Trigger : public Bundle
{
public:
    explicit Trigger(const TriggerTemplate* objectTemplate);   // read in full
    virtual ~Trigger();                              // (D1) / 146 B (D0)

    // the quartet, refcount at +0x13C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;

    virtual void  init();                            // Bundle::init + manager->addTrigger
    virtual void  destroy(bool force);               // SimpleObject::destroy
    virtual void  handleMessage(TemplateMessage, IPlayer*, bool);   // Bundle's

    // non-virtual queries the manager asks
    int   getTypes() const;                          // template->m_events
    float getRadius() const;                         // template->m_radius
    bool  isInArea(const Vec3& point) const;         // |pos - point| < radius (strict)
    bool  isValid();                                 // has effectors && (server || !flag)
    bool  tryRandomTrigger(float chance);            // the tick-seeded Lehmer roll

    // AmbientEffectArea::trigger remains a separate frontier.
    void  findEffectors();                           // ELF81C1C60
    bool  checkTriggerArea();                        // ELF81C2010
    void  updateOnEvent(TriggerType type, IBase* source);
    void  updateOnTime();
    // Corrected from bool: no defined return or result-consuming caller in either port.
    void  handleTrigger(IPlayer* player, const Vec3& position);
    void  executeTrigger(IBase* source, const Vec3& position, int effectorIndex);

protected:
    // layout +0x13C..+0x16B - every ctor value below is from the 167-byte constructor, read in full.
    /* 0x13C */ int   m_refCount;                    // ctor 1
    /* 0x140 */ const TriggerTemplate* m_template;   // ctor = the argument
    /* 0x144 */ int   m_lastTestTick;                // [CHOSEN] ctor 0. updateOnTime seeds it on the
                                                     // first call and re-stamps it after each
                                                     // test pass; handleTrigger's elapsed base
    /* 0x148 */ int   m_lastTriggerTick;             // [CHOSEN] ctor 0. executeTrigger stamps it;
                                                     // both updates gate on (now - it) * tickTime
                                                     // >= minimumTimeBetweenTriggering
    /* 0x14C */ std::vector<IObject*> m_effectors;   // ctor empty (+0x14C/+0x150/+0x154). Filled
                                                     // by findEffectors from the two managers by
                                                     // triggerId; isValid = !empty
    /* 0x158 */ bool  _bool158;                      // [UNRESOLVED] name; ctor false. findEffectors
                                                     // writes it false at entry AND false again
                                                     // when it finds a non-networked AmbientEffect
                                                     // Area (both stores are `mov byte, 0` - read
                                                     // at the instruction level); isValid tests
                                                     // `!it` on the client arm. Preserved as
                                                     // shipped: it can never become true.
    /* 0x159 */ char  _pad159[3];
    /* 0x15C */ std::vector<IObject*> m_objectsInArea;   // ctor empty. checkTriggerArea's grid
                                                     // result; updateOnTime walks it
    /* 0x168 */ int   m_lastRandom;                  // [CHOSEN] ctor 0x759DC150 (the tree's
                                                     // recurring "never" bit pattern - as an int).
                                                     // tryRandomTrigger stores each roll here and
                                                     // nothing reads it back - the roll is
                                                     // re-seeded from the tick count every call.
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_TRIGGER_H
