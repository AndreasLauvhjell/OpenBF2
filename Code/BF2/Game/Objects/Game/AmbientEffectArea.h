// Trigger-driven ambient effect area with weak references to spawned effects.
// The manager indexes areas by id and trigger id. [INFERRED] Header placement.

#ifndef GAME_OBJECTS_GAME_AMBIENTEFFECTAREA_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_AMBIENTEFFECTAREA_H

#include <list>
#include <string>

#include "../../../Core/WeakPtr.h"
#include "../Base/Bundle.h"
#include "../Base/BundleTemplate.h"

namespace dice {
namespace hfe {
class AmbientEffectAreaEvent;
namespace world {

class IPlayer;

// Ambient effect selection, placement and networking settings.
// playAtPlayerPosition and playAtEventPosition are console aliases for the same byte.
class AmbientEffectAreaTemplate : public BundleTemplate
{
public:
    AmbientEffectAreaTemplate();                             // read in full
    virtual unsigned int getClassID() const;
    virtual ~AmbientEffectAreaTemplate();                    // (D1) / 112 B (D0)
    virtual void* queryImplementation(unsigned int classId); // one-cid hop into Bundle's
    virtual IObject* createObject() const;                   // new(0x158) AmbientEffectArea(this)
    virtual bool  makeScript(io::IStream*);                  // ELF/PE recovered

    // The quartet, refcount at +0x17C.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;

    float getRadius() const                          { return m_radius; }
    int   getTriggerId() const                       { return m_triggerId; }
    int   getNumNetworkedEffects() const             { return m_numNetworkedEffects; }

    // The console field bindings write these directly.
    /* 0x17C */ int         m_refCount;              // ctor 1
    /* 0x180 */ float       m_radius;                // ctor 0.0f
    /* 0x184 */ int         m_triggerId;             // ctor -1
    /* 0x188 */ bool        m_playAtEventPosition;   // ctor false - [CHOSEN] name: both
                                                     // playAtEventPosition AND playAtPlayerPosition
                                                     // write this byte (see the header block)
    /* 0x189 */ char        _pad189[3];
    /* 0x18C */ int         m_numNetworkedEffects;   // ctor 0 - isNetworked() = (it > 0)
    /* 0x190 */ std::string m_linkedEffectContainer; // ctor "" - the effect template played
    /* 0x194 */ bool        m_projectOnHeightmap;    // ctor true
    /* 0x195 */ bool        m_attachToEventObject;   // ctor true
    /* 0x196 */ char        _pad196[2];
};

// Ambient-effect trigger area. trigger is virtual; its message and effect helpers are non-virtual.
class AmbientEffectArea : public Bundle
{
    friend class AmbientEffectAreaManager; // manager reads the attested per-area id directly
    friend class dice::hfe::AmbientEffectAreaEvent; // same id: ELF +14C / retail PE +16C
public:
    explicit AmbientEffectArea(const AmbientEffectAreaTemplate* objectTemplate);   // read in full
    virtual ~AmbientEffectArea();                    // (D1) / 210 B (D0)

    // the quartet, refcount at +0x13C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;

    virtual void  init();                            // Bundle::init + manager->add
    virtual void  destroy(bool force);               // SimpleObject::destroy

    // Places and optionally attaches/projects the effect, sending network events or playing locally.
    virtual void  trigger(IPlayer* player, const Vec3& position);

    // non-virtual
    // message 0 (the trigger message) -> the virtual trigger(player, position); every
    // other TemplateMessage is ignored. Trigger::executeTrigger's call.
    void  handleAmbientMessage(TemplateMessage message, IPlayer* player, const Vec3& position);
    // ELF81653D0 / dedicated PE589260 / client 5E8710: selects, creates, prunes/counts,
    // initializes, samples lifetime, attaches and starts.
    // The integer is a child index, not a team.
    void  playEffect(Vec3& position, int effectIndex, IObject* eventObject);
    int   getTriggerId() const;                      // template->m_triggerId
    bool  isNetworked() const;                       // template->m_numNetworkedEffects > 0
    float getRadius() const;                         // template->m_radius

protected:
    // layout +0x13C..+0x157 - every ctor value below is from the 153-byte constructor, read in full.
    /* 0x13C */ int   m_refCount;                    // ctor 1
    /* 0x140 */ const AmbientEffectAreaTemplate* m_template;   // ctor = the argument
    /* 0x144 */ std::list<WeakPtr<IObject> > m_effects;   // ctor empty (the SGI head node is
                                                     // allocated here). playEffect appends; the
                                                     // dtor destroy(true)s every live target
    /* 0x148 */ int   m_lastRandom;                  // Random's persistent scalar seed, ctor 0x759DC150
    /* 0x14C */ int   m_id;                          // ctor = ambientEffectAreaNextId++ (the TU's
                                                     // counter, ds:0x8AACDE8; the manager's
                                                     // map<id, area> key and removeAll resets it)
    /* 0x150 */ IObject* m_effectContainer;           // [INFERRED name] client cache; dedicated ctor 0
    /* 0x154 */ IObjectTemplate* m_effectTemplate;    // [INFERRED name] linked template; dedicated ctor 0
};

// The id counter above - a TU-static in dice's file with no ELF symbol; exposed at namespace scope
// [CHOSEN name] because AmbientEffectAreaManager::removeAll resets it.
extern int ambientEffectAreaNextId;                  // ds:0x8AACDE8

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_AMBIENTEFFECTAREA_H
