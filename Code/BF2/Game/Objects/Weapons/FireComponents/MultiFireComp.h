// Path:     attested - Game/Objects/Weapons/FireComponents/MultiFireComp.cpp occurs in both retail
// source-path maps. This header is [INFERRED] beside it.
// Contents: MultiFireComp runtime/template contracts recovered from the BF2 1.50 PE and ELF.

#ifndef GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_MULTIFIRECOMP_H
#define GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_MULTIFIRECOMP_H

#include <list>
#include <string>
#include <vector>

#include "FireComp.h"

namespace dice {
namespace hfe {
namespace world {

class MultiFireCompTemplate;

class MultiFireComp : public FireComp
{
public:
    explicit MultiFireComp(MultiFireCompTemplate* objectTemplate);
    virtual ~MultiFireComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual void initComponent(WeaponInitEvent& event);
    virtual bool preUpdate();
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual bool requestFire(bool released, bool alternate);
    virtual void outOfAmmo(bool alternate);
    virtual int fire(bool alternate, int index);
    virtual void enable();
    virtual void fireComplete(unsigned int tick);
    virtual void postReload();
    virtual bool busy();
    virtual const Mat4& getProjectileOrigin() const;
    virtual bool launchesProjectiles(int index) const;

    const Mat4& getMediumProjectileOrigin();

private:
    void createAllBarrels();
    void fireCompleteUpdateStates();
    void dummyLaunchProjectile(int index);
    bool launchProjectile(int index);
    void getProjectileOriginForBarrel(int index, Mat4& out) const;
    const Mat4& getProjectileOriginFromCamera() const;

    // ELF layout: FireComp +0x000..+0x127, sizeof MultiFireComp 0x1BC. Container representation
    // shifts offsets in the PE, but its ctor/vtable and key bodies confirm this source order.
    /* +0x128 */ int m_refCount;
    /* +0x12C */ std::vector<unsigned int> m_unusedValues; // constructed/destroyed only [UNRESOLVED]
    /* +0x138 */ std::vector<IObject*> m_barrels;
    /* +0x144 */ unsigned int m_fireCompleteAtTick;
    /* +0x148 */ int m_pendingBarrel;
    /* +0x14C */ std::vector<bool> m_barrelWaiting;
    /* +0x160 */ std::vector<unsigned int> m_barrelFireAtTicks;
    /* +0x16C */ bool m_requestingFire;
                 // +0x16D..+0x16F padding
    /* +0x170 */ int m_currentBarrel;
    /* +0x174 */ int m_unknown174; // not initialized/read [UNRESOLVED]
    /* +0x178 */ MultiFireCompTemplate* m_multiTemplate;
    /* +0x17C */ mutable Mat4 m_mediumProjectileOrigin;
};

class MultiFireCompTemplate : public FireCompTemplate
{
public:
    MultiFireCompTemplate();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int classId);
    virtual ~MultiFireCompTemplate();
    virtual bool makeScript(io::IStream* stream);

    void addBarrelName(const std::string& name);
    void removeBarrelName(const std::string& name);
    bool isBarrel(const std::string& name);

    void setUseDummyProjectiles(bool value) { m_useDummyProjectiles = value; }
    bool getUseDummyProjectiles() const { return m_useDummyProjectiles; }
    const std::list<std::string>& getBarrelNames() const { return m_barrelNames; }
    void setUseSequentialBarrels(bool value) { m_useSequentialBarrels = value; }
    bool getUseSequentialBarrels() const { return m_useSequentialBarrels; }

private:
    // ELF: base 0x74, leaf 0x84. PE offsets differ only through STL representation.
    int m_refCount;
    bool m_useDummyProjectiles;
    std::list<std::string> m_barrelNames;
    bool m_useSequentialBarrels;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_MULTIFIRECOMP_H
