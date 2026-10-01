// LP firing component and template.

#ifndef GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_LPFIRECOMP_H
#define GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_LPFIRECOMP_H

#include "FireComp.h"

namespace dice {
namespace hfe {
namespace world {

class LPFireCompTemplate;

class LPFireComp : public FireComp
{
public:
    explicit LPFireComp(LPFireCompTemplate* objectTemplate);
    virtual ~LPFireComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual void updateComponent(float seconds, unsigned int flags);
    virtual bool requestFire(bool released, bool alternate);
    virtual int fire(bool alternate, int index);
    virtual void fireStop(bool alternate);
    virtual bool busy();
    virtual Vec3 getStartPos() const;
    virtual Vec3 getEndPos() const;
    virtual bool launchesProjectiles(int index) const;
    virtual std::vector<IObject*>* getFiredProjectiles();
    virtual void postReload();

private:
    // No recovered caller observes a return value, and both ports leave the final tick in eax.
    // [INFERRED] source return spelling: void.
    void launchProjectile(int index);

    // ELF layout: FireComp is 0x128 bytes; LPFireComp is 0x15C bytes. The PE has the same member
    // order, with the expected STL-driven displacement inherited from FireComp.
    /* +0x128 */ int m_refCount;
    /* +0x12C */ unsigned int m_radioMessageStartedAtTick;
    /* +0x130 */ bool m_radioMessagePending;
                 // +0x131..+0x133 padding
    /* +0x134 */ Vec3 m_unusedPosition;       // ctor zero; no recovered reader [UNRESOLVED]
    /* +0x140 */ Vec3 m_startPosition;
    /* +0x14C */ Vec3 m_endPosition;
    /* +0x158 */ LPFireCompTemplate* m_lpTemplate;
};

class LPFireCompTemplate : public FireCompTemplate
{
public:
    LPFireCompTemplate();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int classId);
    virtual ~LPFireCompTemplate();
    virtual bool makeScript(io::IStream* stream);

    void setRadioMessageDelay(float seconds);
    float getRadioMessageDelay() const { return m_radioMessageDelay; }
    int getRadioMessageDelayInTicks() const { return m_radioMessageDelayTimes30; }

private:
    // ELF: FireCompTemplate 0x74; this leaf 0x80. PE preserves the source order after its larger
    // std::vector-bearing base.
    int m_refCount;
    float m_radioMessageDelay;
    int m_radioMessageDelayTimes30;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_LPFIRECOMP_H
