// Path:     attested by the retail __FILE__ string
// "Game/Objects/Weapons/TargetComponents/LPTargetComp.cpp".

#ifndef GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_LPTARGETCOMP_H
#define GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_LPTARGETCOMP_H

#include <string>

#include "TargetComp.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectTemplate;
class TargetObject;

class LPTargetCompTemplate : public TargetCompTemplate
{
public:
    LPTargetCompTemplate();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();
    virtual void objectCreated(IObject* object);
    virtual void objectDestroyed(IObject* object);
    virtual void setTemplate(IObjectTemplate* objectTemplate);
    virtual void* queryImplementation(unsigned int classId);
    virtual void preCache() const;
    virtual bool makeScript(io::IStream* stream);

    void setTargetObjectTemplate(const std::string& name)
    {
        m_targetObjectTemplateName = name;
        m_targetObjectTemplate = 0;
    }
    const std::string& getTargetObjectTemplateName() const { return m_targetObjectTemplateName; }
    IObjectTemplate* getTargetObjectTemplate() const;
    void setUpdateInterval(float seconds);
    float getUpdateInterval() const { return m_updateInterval; }
    int getUpdateIntervalTicks() const { return m_updateIntervalTicks; }
    void setDisableFireWhenActiveMissile(bool value) { m_disableFireWhenActiveMissile = value; }
    bool getDisableFireWhenActiveMissile() const { return m_disableFireWhenActiveMissile; }
    void setDisableGuidanceOnReload(bool value) { m_disableGuidanceOnReload = value; }
    bool getDisableGuidanceOnReload() const { return m_disableGuidanceOnReload; }

private:
    /* ELF +0x3C / PE +0x84 */ int m_leafRefCount;
    /* ELF +0x40 / PE +0x88 */ std::string m_targetObjectTemplateName;
    /* ELF +0x44 / PE +0xA4 */ mutable IObjectTemplate* m_targetObjectTemplate;
    /* ELF +0x48 / PE +0xA8 */ float m_updateInterval;
    /* ELF +0x4C / PE +0xAC */ int m_updateIntervalTicks;
    /* ELF +0x50 / PE +0xB0 */ bool m_disableFireWhenActiveMissile;
    /* ELF +0x51 / PE +0xB1 */ bool m_disableGuidanceOnReload;
};

class LPTargetComp : public TargetComp
{
public:
    explicit LPTargetComp(LPTargetCompTemplate* objectTemplate);
    virtual ~LPTargetComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual void initComponent(WeaponInitEvent& event);
    virtual void projectileWasReset(IObject* projectile);
    virtual void destroy();
    virtual void fireStop(bool primary);
    virtual void enable();
    virtual void disable();
    virtual void reload(ReloadState state);
    virtual void updateComponent(float dt, unsigned int tick);
    virtual void postFire(WeaponFireEvent& event);
    virtual void setCurrentProjectile(IObject* projectile);
    virtual IObject* getProjectile() const;
    virtual bool requestFire(bool released, bool alternate);
    virtual void setTargetObject(IObject* target, bool locked, IObject* lockVehicle);
    virtual void setTargetDirection(Vec3 start, Vec3 end);
    virtual bool getLaunched();
    virtual bool busy();
    virtual io::PlayerInputMap getTargetInput() const;

private:
    void enableTargetObject();
    void disableTargetObject();
    void updateTargetObject(Vec3 start, Vec3 end);

    /* +0x4C */ int m_leafRefCount;
    /* +0x50 */ unsigned int m_lastUpdateTick;
    /* +0x54 */ WeakPtr<IObject> m_currentProjectile;
    /* +0x58 */ TargetObject* m_ownedTarget;
    /* +0x5C */ LPTargetCompTemplate* m_lpTemplate;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(LPTargetComp) == 0x60, "LPTargetComp x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_LPTARGETCOMP_H
