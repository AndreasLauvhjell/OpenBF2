// Script-defined kit strengths and AI properties.
// [INFERRED] Source placement.
#ifndef AI_AIINTERFACE_BFKNOWLEDGE_KITTEMPLATE_H
#define AI_AIINTERFACE_BFKNOWLEDGE_KITTEMPLATE_H

#include "AIAdapterTypes.h"

namespace dice { namespace hfe { namespace ai {

class IKitTemplate
{
public:
    IKitTemplate();

    // Seven ordinary virtuals precede the destructor: ELF address point
    // In particular, getType is +0x18 on both ports.
    virtual void setStrategicStrength(int index, int strength) = 0;
    virtual const int* getStrategicStrengths() const = 0;
    virtual int getStrategicStrength(int index) const = 0;
    virtual void setBattleStrength(int index, int strength) = 0;
    virtual const int* getBattleStrengths() const = 0;
    virtual int getBattleStrength(int index) const = 0;
    virtual KitType getType() const = 0;
    virtual ~IKitTemplate();

    // The array getters return borrowed pointers; pointee constness is [INFERRED]
    // because it is not encoded in the return ABI. The indexed getters return int.
};

class KitTemplate : public IKitTemplate
{
public:
    explicit KitTemplate(KitType type);
    virtual ~KitTemplate();

    virtual void setStrategicStrength(int index, int strength);
    virtual const int* getStrategicStrengths() const;
    virtual int getStrategicStrength(int index) const;
    virtual void setBattleStrength(int index, int strength);
    virtual const int* getBattleStrengths() const;
    virtual int getBattleStrength(int index) const;
    virtual KitType getType() const;

    // Nonvirtual. Void return spelling is [INFERRED] from no return-value
    // materialization. no caller in either image.
    void initFromObject(IAIObject* object);

private:
    // Member names are [INFERRED]. Both retail x86 layouts agree on these offsets.
    /* +0x04 */ int* m_strategicStrengths; // two owned entries
    /* +0x08 */ int* m_battleStrengths;    // six owned entries
    /* +0x0C */ KitType m_type;
    /* +0x10 */ bool m_initialized;
};

static_assert(sizeof(void*) != 4 || sizeof(IKitTemplate) == 4,
              "IKitTemplate x86 interface size");
static_assert(sizeof(void*) != 4 || sizeof(KitTemplate) == 20,
              "KitTemplate x86 size");

} } }
#endif
