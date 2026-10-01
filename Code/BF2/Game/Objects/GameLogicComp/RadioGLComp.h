// Runtime radio callouts with one-shot fail/use enable flags and alternating destroyed/repaired latches.
// Fail/use require a first-person owner or server AI; use messages containing AUTO_ bypass spam checks.
// Destroyed/repaired messages use the commander channel without that owner gate.
// init/setObject must initialize m_object before use; its owner must be a GenericFireArm.
// Preserve appended non-const hasUpdate/getter slots after the destructor and virtual flag accesses.
// [PLATFORM] Destructor slot count differs between PE and ELF. Source placement is [INFERRED].

#ifndef GAME_OBJECTS_GAMELOGICCOMP_RADIOGLCOMP_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAMELOGICCOMP_RADIOGLCOMP_H

#include "IRadioGLComp.h"

namespace dice {
namespace hfe {
namespace world {

class RadioGLCompTemplate;

class RadioGLComp : public IRadioGLComp
{
public:
    explicit RadioGLComp(RadioGLCompTemplate* objectTemplate);

    // IObjectComponent overrides
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;                    // IID_IRadioGLComp
    virtual void  init(IObject* object);                            // stores m_object
    virtual void  setObject(IObject* object);
    virtual IObject* getObject() const;
    virtual void  handleUpdate(float deltaTime);                    // empty

    // IRadioGLComp
    virtual void  playFailMessage(bool warn);
    virtual void  setFailMessageEnabled(bool enabled);              // is1P()-gated
    virtual void  playUseMessage(bool warn);                        // the AUTO_ quirk
    virtual void  setUseMessageEnabled(bool enabled);               // is1P()-gated
    virtual void  playDestroyedMessage();                           // latch pair
    virtual void  playRepairedMessage();                            // latch pair
    virtual void* getGLInfo();                                      // &template->m_failMessage
    virtual ~RadioGLComp();                                         // (D1/D2), 14 B - vptr store only

    // appended after the destructor pair
    virtual bool  hasUpdate();                                      // +0x5C false (NON-const NEW slot)
    virtual bool  getFailMessageEnabled();                          // +0x60
    virtual bool  getUseMessageEnabled();                           // +0x64

    bool is1P();                                                    // non-virtual; see .cpp

private:
    int                  m_refCount;                                // +0x04
    RadioGLCompTemplate* m_template;                                // +0x08
    IObject*             m_object;                                  // +0x0C see the header note
    bool                 m_failMessageEnabled;                      // +0x10
    bool                 m_useMessageEnabled;                       // +0x11
    bool                 m_playRepairedLatch;                       // +0x12
    bool                 m_playDestroyedLatch;                      // +0x13
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAMELOGICCOMP_RADIOGLCOMP_H
