// Path: [INFERRED] from the namespace map - dice::hfe::world -> Game/Objects/.
// PE BF2.exe 1.50 and the symbolized Linux image agree on this complete ABI.  Proven
// retail allocations are PE 0xA4 and ELF 0x2C; the difference is the two historical
// std::string layouts.  The modern rebuild STL intentionally has neither retail size.

#ifndef GAME_OBJECTS_WARNINGHUDCOMPTEMPLATE_H
#define GAME_OBJECTS_WARNINGHUDCOMPTEMPLATE_H

#include <string>

#include "CompTemplateInterfaces.h"
#include "IObjectComponent.h"
#include "../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {
namespace world {

enum WarningState : int;

// Returned by IWarningHudComp::getHudInfo().  PE callers read warningType at +0x38.
struct WarningHudInfo
{
    std::string firstWarningSound;
    std::string secondWarningSound;
    int warningType;
};

class IWarningHudComp : public IObjectComponent
{
public:
    virtual void setWarningState(WarningState state) = 0;
    virtual WarningState getWarningState() = 0;
    virtual WarningHudInfo* getHudInfo() = 0;
    // [CHOSEN source spelling] Both images prove an address return; pointer versus
    // reference is not encoded by the ABI.  A pointer states that machine contract directly.
    virtual std::string* getHudIcons() = 0;

protected:
    ~IWarningHudComp() {}
};

class WarningHudCompTemplate;

class WarningHudComp : public IWarningHudComp
{
public:
    explicit WarningHudComp(WarningHudCompTemplate* componentTemplate);

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int id) const;
    virtual unsigned int getInterfaceID() const;
    virtual void init(IObject* object);
    virtual void setObject(IObject* object);
    virtual IObject* getObject() const;
    virtual bool hasUpdate() const;
    virtual void handleUpdate(float deltaTime);
    virtual void setWarningState(WarningState state);
    virtual WarningState getWarningState();
    virtual WarningHudInfo* getHudInfo();
    virtual std::string* getHudIcons();
    virtual ~WarningHudComp();

private:
    int m_refCount;
    WarningHudCompTemplate* m_componentTemplate;
    IObject* m_object;
    WarningState m_warningState;
};

// Primary vtable: PE 20 slots / ELF 22 slots (Itanium has two destructor entries and
// exposes the primary-adjusting makeScript thunk).  The secondary IConsoleSaveable
// vtable carries makeScript in both images.
class WarningHudCompTemplate : public IWarningHudCompTemplate, public io::IConsoleSaveable
{
public:
    WarningHudCompTemplate();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int id) const;
    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();
    virtual void objectCreated(IObject* object);
    virtual void objectDestroyed(IObject* object);
    virtual void setTemplate(IObjectTemplate* objectTemplate);
    virtual void* queryImplementation(unsigned int id);
    virtual void setWarningType(int warningType);
    virtual int getWarningType();
    virtual void setFirstWarningSound(std::string sound);
    virtual std::string getFirstWarningSound();
    virtual void setSecondWarningSound(std::string sound);
    virtual std::string getSecondWarningSound();
    virtual void setWarningIcon(std::string icon);
    virtual std::string getWarningIcon();
    virtual ~WarningHudCompTemplate();
    virtual bool makeScript(io::IStream* stream);

private:
    friend class WarningHudComp;

    int m_refCount;
    bool m_firstComponentPending;

    // Both are real std::strings: ctor/dtor coverage proves their type.  No recovered
    // WarningHud method reads or writes them, so their semantic names remain unresolved.
    std::string m_unresolvedString10;
    std::string m_unresolvedString14;
    WarningHudInfo m_hudInfo;
    std::string m_warningIcon;

    // Allocation is four bytes larger than the final string in both images.  The ctor
    // deliberately leaves this dword untouched and no recovered method references it.
    int m_unresolvedTail;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WARNINGHUDCOMPTEMPLATE_H
