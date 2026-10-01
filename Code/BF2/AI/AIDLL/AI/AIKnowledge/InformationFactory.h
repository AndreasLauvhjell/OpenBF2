// Creates information records and plug-ins. Unsupported concrete types stop by name.
// The concrete factory introduces registerInformation and registerWeapon after the base interface.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIKNOWLEDGE_INFORMATIONFACTORY_H
#define AI_AIDLL_AI_AIKNOWLEDGE_INFORMATIONFACTORY_H

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>

#include "../Main/AIDllTypes.h"
#include "../../../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace ai {

// ELF `_ZN4dice3hfe2ai23IID_IInformationFactoryE` / - both read 9990007.
// A libaidll symbol, not one of the engine's (Core/Ids.h is bf2's census), so it lives here.
const unsigned int IID_IInformationFactory = 9990007u;

// Reference-counted information factory interface. Preserve virtual declaration order.
class IInformationFactory : public IBase
{
public:
    IInformationFactory();
    virtual ~IInformationFactory();           // slot 4, see the note above
    virtual unsigned int requestInformationReal(unsigned int handle, IAIObject* object,
                                                unsigned int types, float basicTemp, double time,
                                                float allowedTimeDiff) const = 0;          // +0x18
    virtual unsigned int requestInformationKnown(unsigned int handle, IAIObject* object,
                                                 unsigned int types, double time, float degeneration,
                                                 float basicTemp, float allowedTimeDiff,
                                                 int side) const = 0;                       // +0x1C
    virtual bool requestInformationPlugin(unsigned int handle, int type, bool real) const = 0;   // +0x20
    virtual bool requestInformationPlugin(unsigned int sourceHandle, unsigned int targetHandle,
                                          int type, bool real) const = 0;                  // +0x24
    virtual bool updateInformationPlugin(unsigned int handle, int type) const = 0;          // +0x28
    virtual unsigned int requestWeaponInfo(unsigned int handle, IWeapon* weapon,
                                           bool real) const = 0;                            // +0x2C
    virtual bool checkIfInfoExists(unsigned int handle) const = 0;                          // +0x30
    virtual bool checkIfWeaponInfoExists(unsigned int handle) const = 0;                    // +0x34
};

// InformationFactory - sizeof == 8, from BF2AI::init step 8. The constructor is inlined in both
// images; dtor / (the vptr, then a tail jump into the base's).
class InformationFactory : public IInformationFactory
{
public:
    InformationFactory();
    virtual ~InformationFactory();

    // IBase
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    // IInformationFactory
    virtual unsigned int requestInformationReal(unsigned int handle, IAIObject* object,
                                                unsigned int types, float basicTemp, double time,
                                                float allowedTimeDiff) const;
    virtual unsigned int requestInformationKnown(unsigned int handle, IAIObject* object,
                                                 unsigned int types, double time, float degeneration,
                                                 float basicTemp, float allowedTimeDiff,
                                                 int side) const;
    virtual bool requestInformationPlugin(unsigned int handle, int type, bool real) const;
    virtual bool requestInformationPlugin(unsigned int sourceHandle, unsigned int targetHandle,
                                          int type, bool real) const;
    virtual bool updateInformationPlugin(unsigned int handle, int type) const;
    virtual unsigned int requestWeaponInfo(unsigned int handle, IWeapon* weapon,
                                           bool real) const;
    virtual bool checkIfInfoExists(unsigned int handle) const;
    virtual bool checkIfWeaponInfoExists(unsigned int handle) const;

    // introduced here (see the header note)
    virtual unsigned int registerInformation(unsigned int handle, Information* information) const;
    virtual unsigned int registerWeapon(unsigned int handle, WeaponInfo* weaponInfo) const;

private:
    /* +0x04 */ int m_refCount;                // never written by the ctor [UNRESOLVED]
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_INFORMATIONFACTORY_H
