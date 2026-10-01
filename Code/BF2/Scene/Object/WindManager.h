// Global wind and local affectors, traversed in pointer-set order.
// [INFERRED] placement from ELF link order beside ObjectManager.
// Preserve the shipped quirks: getWindDirection ignores position, and
// getWindDirectionAndSpeed multiplies its direction by its magnitude.

#ifndef SCENE_OBJECT_WINDMANAGER_H     // [INFERRED] guard style
#define SCENE_OBJECT_WINDMANAGER_H

#include <set>

#include "../../Core/IBase.h"
#include "../../Game/Objects/IWindAffector.h"
#include "../../Math/Vec3.h"   // hfe::Vec3 (a typedef of Math::Vec3 - never forward-declare it)

namespace dice {
namespace hfe {
namespace world {

// Known sole base interface. Do not infer additional methods solely from its current implementor.
class IWindManager : public IBase
{
protected:
    ~IWindManager() {}
};

// The vtable, in slot order, 20 slots / 80 bytes.
class WindManager : public IWindManager
{
public:
    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();                                                 // +0x00
    virtual int   getRef();                                                 // +0x04
    virtual int   release();                                                // +0x08
    virtual void* queryInterface(unsigned int iid) const;                   // +0x0C

    // The per-frame tick
    virtual void  update(float deltaTime);                                  // +0x10

    // The affector registry
    virtual const std::set<IWindAffector*>& getAllWindAffectors() const;    // +0x14
    virtual const std::set<IWindAffector*>& getWindAffectors(
                      const Vec3& position, float radius) const;            // +0x18

    // The global wind
    // speed and direction are stored separately from their product; both setters recompute
    // m_globalWind eagerly so that getGlobalWind stays an 11-byte read. Keep that eager update - a
    // rebuild that computed the product lazily would still be correct, but every caller of
    // getGlobalWind would then be paying for it.
    virtual float        getGlobalWindSpeed() const;                        // +0x1C
    virtual void         setGlobalWindSpeed(float speed);                   // +0x20
    virtual const Vec3&  getGlobalWindDirection() const;                    // +0x24
    virtual void         setGlobalWindDirection(const Vec3& direction);     // +0x28
    virtual const Vec3&  getGlobalWind() const;                             // +0x2C

    // Sampling at a point
    virtual float        getWindSpeed(const Vec3& position) const;          // +0x30
    virtual const Vec3&  getWindDirection(const Vec3& position) const;      // +0x34
                                                                            //        ignores position
    virtual void         getWindDirectionAndSpeed(const Vec3& position,
                                                  Vec3& direction,
                                                  float& speed) const;      // +0x38
                                                                            //        shipped defect
    virtual Vec3         getWind(const Vec3& position) const;               // +0x3C

    // Registration
    virtual void  addWindAffector(IWindAffector* affector);                 // +0x40
    virtual void  removeWindAffector(IWindAffector* affector);              // +0x44


    virtual ~WindManager();                                                 // +0x48 / +0x4C

public:
    // C2 0x831FDD0 (416 B) / C1 0x831FF70 (248 B). recovered - see the .cpp.
    WindManager();

    // The factory ClassManager::createInstance calls. Signature is fixed by ClassRegInfo::create in
    // Core/ClassManager.h and by the mangled name
    // _ZN4dice3hfe5world11WindManager6createEjPNS0_5IBaseE. Registered by world::ObjectModule as
    // "dice.hfe.world.WindManager.Standard" (CID_WindManager); the singleton row that makes
    // ClassManager actually call it is world::Module's, flags 1.
    static IBase* create(unsigned int iid, IBase* outer);

private:
    /* +0x04 */ int m_refCount;

    /* +0x08 */ std::set<IWindAffector*> m_windAffectors;

    // [UNRESOLVED] +0x14. Constructed and destroyed, never read or written by any of the 20 vtable
    // slots. Deliberately unnamed - see the header block above.
    /* +0x14 */ std::set<IWindAffector*> m_unknown14;

    // [INFERRED name] +0x20. getWindAffectors clears this, refills it with whatever is in range and
    // returns a reference to it - so it is a reused scratch buffer, not state, and the reference the
    // caller gets is invalidated by the next getWindAffectors call on the same manager.
    /* +0x20 */ mutable std::set<IWindAffector*> m_affectorsInRange;

    /* +0x2C */ float m_globalWindSpeed;
    /* +0x30 */ Vec3  m_globalWindDirection;
    /* +0x3C */ Vec3  m_globalWind;          // == m_globalWindDirection * m_globalWindSpeed
};

// ds:0x8B1B92C, the fourth of the four adjacent world-manager pointers (0x8B1B920 objectManager /
// 24 objectTemplateManager / 28 playerManager / 2C windManager). defined in Core/Globals.cpp - do
// not define it here.
extern IBase* windManager;   // SEE the integrator note: Core/Globals.cpp currently gives this the
                             // IBase* type, which is why Core/ModuleRegistry.cpp's row passes
                             // `&windManager` with no cast while its three neighbours are cast.

} // namespace world
} // namespace hfe
} // namespace dice

#endif // SCENE_OBJECT_WINDMANAGER_H
