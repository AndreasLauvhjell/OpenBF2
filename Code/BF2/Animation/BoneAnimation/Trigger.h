// Animation trigger tree containing bundle and child-trigger references.
// [INFERRED] header placement. getType() identifies a family, not a concrete subclass.
// AddChild/addBundle resolve names through the globally active animation template;
// Callers must select that template before modifying a trigger.

#ifndef ANIMATION_BONEANIMATION_TRIGGER_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_TRIGGER_H

#include <string>
#include <vector>

#include "../../Math/Vec3.h"       // Vec2 - IdleTrigger's idle range

namespace dice {

namespace hfe { namespace io { class IStream; } }

namespace anim {

class AnimationSystem;
class Bundle;
class ValueHolder;

class Trigger
{
public:
    // the TAG getType() returns. Values attested - each override is a 7-to-10-byte `return N`.
    // The name is attested too: TriggerManager::getActiveTriggerPtr's mangling says its parameter
    // is `dice::anim::Trigger::TriggerType`, so the enum is nested here.
    enum TriggerType
    {
        TRIGGERTYPE_BASE     = 0,   // Trigger
        TRIGGERTYPE_POSE     = 1,   // PoseTrigger
        TRIGGERTYPE_IDLE     = 2,   // IdleTrigger      addChild propagates on this one
        TRIGGERTYPE_RANDOM   = 3,   // RandomTrigger
        TRIGGERTYPE_MESSAGE  = 4,   // MessageTrigger, SwitchMessageTrigger
        TRIGGERTYPE_MOVEMENT = 5    // MovementTrigger and its five subclasses
    };
    // [UNRESOLVED] enumerator names - the values are read, the spellings are this tree's.

    Trigger();
    virtual ~Trigger();
    // +0x00 / +0x04

    virtual void makeScript(hfe::io::IStream* stream);            // +0x08
    virtual bool getAlreadySaved() const;                         // +0x0C
    virtual void setAlreadySaved(bool saved);                     // +0x10
    virtual TriggerType getType() const;                          // +0x14
    virtual std::string getTypeName() const;                      // +0x18
    // five bytes each - genuinely empty, on the base and on every subclass. A trigger has no
    // binary form; it exists only as script.
    virtual void readBinary(hfe::io::IStream* stream);            // +0x1C
    virtual void writeBinary(hfe::io::IStream* stream);           // +0x20
    // called the moment A trigger becomes the console'S cursor - by createTrigger and by
    // setActiveTrigger. Empty on the base; MovementTrigger uses it to push its value holder into
    // the value-holder manager's active slot, which is how the console cursor chains.
    virtual void activate();                                      // +0x24
    virtual std::string listBundles() const;                      // +0x28

    virtual void addChild(Trigger* child);                        // +0x2C
    virtual void addChild(const std::string& name);               // +0x30
    virtual void removeChild(const std::string& name);            // +0x34
    virtual void removeChildByPtr(Trigger* child);                // +0x38
    virtual void addBundle(Bundle* bundle);                       // +0x3C
    virtual void addBundle(const std::string& name);              // +0x40
    virtual void removeBundle(const std::string& name);           // +0x44
    virtual void removeBundleByPtr(Bundle* bundle);               // +0x48

    // Updates children in order, stopping at the first false result, then applies this trigger's bundles.
    // A closed trigger gate returns true without recursing.
    virtual bool update(AnimationSystem& system);                 // +0x4C
    virtual void applyAnimations(AnimationSystem& system);        // +0x50

    // the three the priority walk uses. Inline because setPriosForTrigger stores straight into
    // +0x1C and reads +0x10/+0x14 with no call anywhere in the binary - the same evidence that put
    // NoCaseStringCompare::operator() in a header.
    bool isRoot() const                              { return !m_isChild; }
    void setPrio(int prio)                           { m_prio = prio; }
    const std::vector<Trigger*>& getChildren() const { return m_children; }

    // Public because the ELF's console executors and setPriosForTrigger reach them with no
    // accessor call - see the note on CameraSpring's fields.
    /* +0x04 */ std::vector<Bundle*>  m_bundles;     // begin +0x04, end +0x08, cap +0x0C
    /* +0x10 */ std::vector<Trigger*> m_children;    // begin +0x10, end +0x14, cap +0x18
    /* +0x1C */ int m_prio;                          // set by setPriosForTrigger, depth-first
    // by ConsoleClass225's executor, which reads and writes exactly this
    // offset for `animationTrigger.fadeInTime`. It was the last unexplained word in the class.
    /* +0x20 */ float fadeInTime;                    // ctor 0.0f
    // this was written down as `Trigger* m_parent` a sitting ago, on the
    // strength of setPriosForTriggers testing it against zero. It is a byte. addChild sets it on the
    // child and removeChildByPtr clears it, so "is somebody's child" is exactly right - but it is a
    // flag, not a pointer, and a trigger cannot name its parent.
    /* +0x24 */ bool m_isChild;                      // ctor false
    // propagated down the tree by addChild: set on a child when the parent's type is idle or the
    // parent already carries it. So it means "somewhere under an IdleTrigger", and idle-ness is
    // inherited. [CHOSEN] name; the mechanism is measured.
    /* +0x25 */ bool m_underIdle;                    // ctor false
    /* +0x26 */ bool m_alreadySaved;                 // ctor false - save deduplication, not a cycle guard
};


// The two that add nothing - same 40 bytes as the base, one overridden virtual each.

class PoseTrigger : public Trigger
{
public:
    virtual TriggerType getType() const;                          // > 1
    virtual std::string getTypeName() const;                      // > "PoseTrigger"
    virtual bool update(AnimationSystem& system);
};

class RandomTrigger : public Trigger
{
public:
    virtual TriggerType getType() const;                          // -> 3
    virtual std::string getTypeName() const;                      // -> "RandomTrigger"
    virtual void applyAnimations(AnimationSystem& system);
};


// IdleTrigger - 52 bytes. Three floats, and the console binds the first two as a Vec2:
// `animationTrigger.idleTime 5/10`. The defaults are 5 and 10 seconds.

class IdleTrigger : public Trigger
{
public:
    IdleTrigger();

    virtual void makeScript(hfe::io::IStream* stream);
    virtual TriggerType getType() const;                          // -> 2
    virtual std::string getTypeName() const;                      // -> "IdleTrigger"
    virtual bool update(AnimationSystem& system);

    /* +0x28 */ float m_idleTimeMin;    // ctor  5.0f  ┐ the console's Vec2 `idleTime`
    /* +0x2C */ float m_idleTimeMax;    // ctor 10.0f  ┘
    /* +0x30 */ float m_nextIdleTime;   // ctor  5.0f - [CHOSEN]; update() is unwritten
};


// Message-gated trigger; addBundle preserves the base append behavior.

class MessageTrigger : public Trigger
{
public:
    MessageTrigger();

    virtual void makeScript(hfe::io::IStream* stream);
    virtual TriggerType getType() const;                          // -> 4
    virtual std::string getTypeName() const;                      // -> "MessageTrigger"
    virtual void addBundle(Bundle* bundle);                       // base tail-call
    virtual bool update(AnimationSystem& system);

    /* +0x28 */ int m_message;          // ctor 0
};

class SwitchMessageTrigger : public MessageTrigger
{
public:
    virtual std::string getTypeName() const;                      // -> "SwitchMessageTrigger"
    virtual bool update(AnimationSystem& system);
    // no getType() override - it answers 4, the same as its base.
};


// MovementTrigger - 48 bytes, and the only subclass that extends the vtable (nine more slots).
// Its activate() is what chains the console cursor: selecting a movement trigger makes its value
// holder the value-holder manager's active one, so `AnimationValueHolder.min` next edits that.

class MovementTrigger : public Trigger
{
public:
    MovementTrigger();

    virtual void makeScript(hfe::io::IStream* stream);
    virtual TriggerType getType() const;                          // -> 5
    virtual std::string getTypeName() const;                      // -> "MovementTrigger"
    virtual void activate();
    virtual bool update(AnimationSystem& system);
    virtual void applyAnimations(AnimationSystem& system);

    virtual void setValueHolder(const std::string& name);         // +0x54
    virtual const std::string& getValueHolder() const;            // +0x58
    virtual void setTriggerOnAcceleration(bool on);               // +0x5C
    virtual bool getTriggerOnAcceleration() const;                // +0x60
    virtual void setUseDirection(bool on);                        // +0x64
    virtual bool getUseDirection() const;                         // +0x68
    virtual void clearValueHolder();                              // +0x6C
    virtual bool isWithinRange(float value) const;                // +0x70

    /* +0x28 */ ValueHolder* m_valueHolder;      // ctor 0 - not owned; the manager owns it
    /* +0x2C */ bool m_triggerOnAcceleration;    // ctor false
    /* +0x2D */ bool m_useDirection;             // ctor false
};

// The five that add nothing to MovementTrigger but a different applyAnimations/update pair. All 48
// bytes, all type 5. Their `useDirection`/`triggerOnAcceleration` console lines are what
// distinguishes them in the shipped content, not their class.
class ForwardTrigger : public MovementTrigger
{ public: virtual std::string getTypeName() const;
          virtual bool update(AnimationSystem&);
          virtual void applyAnimations(AnimationSystem&); };
class SideTrigger : public MovementTrigger
{ public: virtual std::string getTypeName() const;
          virtual bool update(AnimationSystem&);
          virtual void applyAnimations(AnimationSystem&); };
class UpTrigger : public MovementTrigger
{ public: virtual std::string getTypeName() const;
          virtual bool update(AnimationSystem&);
          virtual void applyAnimations(AnimationSystem&); };
class TurnTrigger : public MovementTrigger
{ public: virtual std::string getTypeName() const;
          virtual bool update(AnimationSystem&);
          virtual void applyAnimations(AnimationSystem&); };
class LookAroundTrigger : public MovementTrigger
{ public: virtual std::string getTypeName() const;
          virtual bool update(AnimationSystem&);
          virtual void applyAnimations(AnimationSystem&); };

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_TRIGGER_H
