// Common and soldier-specific deviation settings. Grouped setters take the maximum first but
// store it at the highest offset of each block; serialization must follow argument order, not declaration order.
// Fire deviation stores add/subtract/maximum. Source placement is [INFERRED].

#ifndef GAME_OBJECTS_DEVIATIONCOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_DEVIATIONCOMPTEMPLATE_H

#include "../../CompTemplateInterfaces.h"
#include "../../../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;

// Abstract weapon-component template with a separate IConsoleSaveable base at x86 +4.
// Preserve declaration order: destruction precedes makeScript; preCache is inherited.
class DeviationCompTemplate : public IWeaponCompTemplate, public io::IConsoleSaveable
{
public:
    DeviationCompTemplate();

    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();      // real body in the ELF, not pure
    virtual void  objectCreated(IObject*);
    virtual void  objectDestroyed(IObject*);
    virtual void  setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual ~DeviationCompTemplate();                 // +0x30 +0x34
    virtual bool  makeScript(io::IStream*);           // +0x38

    void setFireDev(float max, float add, float sub);

    // Accessors without an address comment are [CHOSEN] typed homes for the recovered direct
    // console stores and runtime field loads; they are not claimed as separately emitted symbols.
    void setFireDevMax(float value) { m_fireDevMax = value; }
    float getFireDevMax() const { return m_fireDevMax; }
    void setFireDevAdd(float value) { m_fireDevAdd = value; }
    float getFireDevAdd() const { return m_fireDevAdd; }
    void setFireDevSub(float value) { m_fireDevSub = value; }
    float getFireDevSub() const { return m_fireDevSub; }
    void setMinDev(float value) { m_minDev = value; }
    float getMinDev() const { return m_minDev; }
    void setSubProjectileDev(float value) { m_subProjectileDev = value; }
    float getSubProjectileDev() const { return m_subProjectileDev; }

protected:
    /* +0x08 */ float m_fireDevAdd;
    /* +0x0C */ float m_fireDevSub;
    /* +0x10 */ float m_fireDevMax;
    /* +0x14 */ float m_minDev;            // console `deviation.minDev`; matches the two-hop
                                           // harvester's DeviationComp result at template +0x14
    /* +0x18 */ float m_subProjectileDev;  // by elimination from makeScript — see the caveat in
                                           // the accessor instructions
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(DeviationCompTemplate) == 0x1C,
              "DeviationCompTemplate x86 ABI drift");
#endif

// Adds soldier turn, speed and miscellaneous deviation blocks to the common template base.
// Preserve the inherited interface methods and this class's virtual declaration order.
class SoldierDeviationCompTemplate : public DeviationCompTemplate
{
public:
    SoldierDeviationCompTemplate();

    // Overrides all eleven leading slots, including setTemplate; only preCache is inherited.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();
    virtual void  objectCreated(IObject*);
    virtual void  objectDestroyed(IObject*);
    virtual void  setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual ~SoldierDeviationCompTemplate();
    virtual bool  makeScript(io::IStream*);

    void setTurnDev(float max, float addPitch, float addYaw, float sub);
    void setSpeedDev(float max, float addForward, float addStrafe, float sub);
    void setMiscDev(float max, float addJump, float sub);

    // As above, these inline accessors name recovered field traffic without claiming standalone
    // retail functions.  The three grouped setters above are the emitted symbols.
    void setTurnDevAddPitch(float value) { m_turnDevAddPitch = value; }
    float getTurnDevAddPitch() const { return m_turnDevAddPitch; }
    void setTurnDevAddYaw(float value) { m_turnDevAddYaw = value; }
    float getTurnDevAddYaw() const { return m_turnDevAddYaw; }
    void setTurnDevSub(float value) { m_turnDevSub = value; }
    float getTurnDevSub() const { return m_turnDevSub; }
    void setTurnDevMax(float value) { m_turnDevMax = value; }
    float getTurnDevMax() const { return m_turnDevMax; }

    void setSpeedDevAddForward(float value) { m_speedDevAddForward = value; }
    float getSpeedDevAddForward() const { return m_speedDevAddForward; }
    void setSpeedDevAddStrafe(float value) { m_speedDevAddStrafe = value; }
    float getSpeedDevAddStrafe() const { return m_speedDevAddStrafe; }
    void setSpeedDevSub(float value) { m_speedDevSub = value; }
    float getSpeedDevSub() const { return m_speedDevSub; }
    void setSpeedDevMax(float value) { m_speedDevMax = value; }
    float getSpeedDevMax() const { return m_speedDevMax; }

    void setMiscDevAddJump(float value) { m_miscDevAddJump = value; }
    float getMiscDevAddJump() const { return m_miscDevAddJump; }
    void setMiscDevSub(float value) { m_miscDevSub = value; }
    float getMiscDevSub() const { return m_miscDevSub; }
    void setMiscDevMax(float value) { m_miscDevMax = value; }
    float getMiscDevMax() const { return m_miscDevMax; }

    void setDevModStand(float value) { m_devModStand = value; }
    float getDevModStand() const { return m_devModStand; }
    void setDevModCrouch(float value) { m_devModCrouch = value; }
    float getDevModCrouch() const { return m_devModCrouch; }
    void setDevModLie(float value) { m_devModLie = value; }
    float getDevModLie() const { return m_devModLie; }
    void setDevModZoom(float value) { m_devModZoom = value; }
    float getDevModZoom() const { return m_devModZoom; }

protected:
    /* +0x1C */ int m_soldierRefCount;

    /* +0x20 */ float m_turnDevAddPitch;
    /* +0x24 */ float m_turnDevAddYaw;
    /* +0x28 */ float m_turnDevSub;
    /* +0x2C */ float m_turnDevMax;

    /* +0x30 */ float m_speedDevAddForward;
    /* +0x34 */ float m_speedDevAddStrafe;
    /* +0x38 */ float m_speedDevSub;
    /* +0x3C */ float m_speedDevMax;

    /* +0x40 */ float m_miscDevAddJump;
    /* +0x44 */ float m_miscDevSub;
    /* +0x48 */ float m_miscDevMax;

    /* +0x4C */ float m_devModStand;
    /* +0x50 */ float m_devModCrouch;
    /* +0x54 */ float m_devModLie;
    /* +0x58 */ float m_devModZoom;

    // ClassFactory allocates 0x60 bytes in the ELF, but no recovered constructor, setter,
    // serializer or runtime read touches this final dword.  Keep the observed tail without
    // inventing its role or initializing it.
    /* +0x5C */ unsigned int m_unresolved5C; // [UNRESOLVED]
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(SoldierDeviationCompTemplate) == 0x60,
              "SoldierDeviationCompTemplate must match the shipped x86 layout");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif
