// Path:     [INFERRED] beside the attested weapon-component families.
// Contents: the complete WeaponBasedRecoilComp runtime/template family recovered from the
// symbolised Linux ELF and cross-checked against the Win32 dedicated-server image.
// IWeaponRecoilComp is a real secondary face at +0x10 in both shipped x86 images.  Its destructor
// sits after IWeaponComp::preEnable, so Itanium's two destructor entries shift every recoil-only
// slot four bytes relative to MSVC.  Keep callers typed; raw slot dispatch is not portable here.

#ifndef GAME_OBJECTS_WEAPONS_RECOILCOMPONENTS_WEAPONBASEDRECOILCOMP_H
#define GAME_OBJECTS_WEAPONS_RECOILCOMPONENTS_WEAPONBASEDRECOILCOMP_H

#include "../FireComponents/FireComp.h"
#include "../../CompTemplateBases.h"
#include "../../../../Core/Random.h"
#include "../../../../Math/Vec3.h"

#include <vector>

namespace dice {
namespace hfe {

namespace io { class IStream; }

namespace world {

class IObject;
class IObjectComponent;
class IObjectTemplate;

class IWeaponRecoilComp : public IWeaponComp
{
public:
    virtual ~IWeaponRecoilComp();                                      // ELF +0x84/+0x88
    virtual void setZoom(int zoom) = 0;                                // ELF +0x8C, PE +0x88
    virtual float getRecoilForceModifier() const = 0;                  // ELF +0x90, PE +0x8C
    virtual float getPitchRecoil() = 0;                                // ELF +0x94, PE +0x90
    virtual float getYawRecoil() = 0;                                  // ELF +0x98, PE +0x94
    virtual const RandomDistribution& getRandomRecoilForceLeftRight() const = 0;
                                                                        // ELF +0x9C, PE +0x98
    virtual const RandomDistribution& getRandomRecoilForceUp() const = 0;
                                                                        // ELF +0xA0, PE +0x9C
    virtual bool getGoBackOnRecoil() const = 0;                         // ELF +0xA4, PE +0xA0
    virtual bool getHasRandomRecoilForce() const = 0;                   // ELF +0xA8, PE +0xA4
    virtual const std::vector<float>& getRecoilVector() const = 0;      // ELF +0xAC, PE +0xA8
};

class RecoilComp : public WeaponComp, public IWeaponRecoilComp
{
public:
    explicit RecoilComp(RecoilCompTemplate* objectTemplate);
    virtual ~RecoilComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual void initComponent(WeaponInitEvent&);
    virtual bool preUpdate();
    virtual void updateComponent(float, unsigned int);
    virtual bool isSelectable() const;
    virtual void fireStop(bool);
    virtual bool requestFire(bool, bool);
    virtual void postFire(WeaponFireEvent&);
    virtual void fireComplete(unsigned int);
    virtual void postSwitchFireRate(FireRate);
    virtual bool requestReload(bool);
    virtual void reload(ReloadState);
    virtual void postReload();
    virtual void outOfAmmo(bool);
    virtual void stoppedTracking();
    virtual void postChangeTarget(IObject*);
    virtual void projectileWasReset(IObject*);
    virtual void enable();
    virtual void disable();
    virtual void stop();
    virtual bool busy();
    virtual void exit();
    virtual void destroy();
    virtual void pickup();
    virtual void reset();

    // Primary RecoilComp tail order, read from the shipped vtable.  The secondary
    // IWeaponRecoilComp face above has its own inherited slot order.
    virtual void setZoom(int);
    virtual float getPitchRecoil();
    virtual float getYawRecoil();
    virtual bool getGoBackOnRecoil() const;
    virtual const RandomDistribution& getRandomRecoilForceUp() const;
    virtual const RandomDistribution& getRandomRecoilForceLeftRight() const;
    virtual bool getHasRandomRecoilForce() const;
    virtual float getRecoilForceModifier() const;
    virtual const std::vector<float>& getRecoilVector() const;

protected:
    /* +0x14 */ int m_recoilRefCount;
    /* +0x18 */ RandomDistribution m_defaultDistribution;
    // The shipped base is 0x5C bytes.  No recovered method names or touches this 44-byte block;
    // retain it so the proven derived offsets and x86 ABI remain exact without inventing fields.
    /* +0x2C */ unsigned char m_unresolved2C[0x2C];
    /* +0x58 */ RecoilCompTemplate* m_recoilTemplate;
};

class WeaponBasedRecoilCompTemplate;

class WeaponBasedRecoilComp : public RecoilComp
{
public:
    explicit WeaponBasedRecoilComp(WeaponBasedRecoilCompTemplate* objectTemplate);
    virtual ~WeaponBasedRecoilComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;
    virtual void initComponent(WeaponInitEvent&);
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void postFire(WeaponFireEvent& event);

private:
    /* +0x5C */ int m_weaponRefCount;
    /* +0x60 */ Vec3 m_savedRelativePosition;
    /* +0x6C */ unsigned int m_recoilEndTick;
    /* +0x70 */ unsigned int m_returnEndTick;
    /* +0x74 */ WeaponBasedRecoilCompTemplate* m_weaponTemplate;
};

class WeaponBasedRecoilCompTemplate : public RecoilCompTemplate
{
public:
    WeaponBasedRecoilCompTemplate();
    virtual ~WeaponBasedRecoilCompTemplate();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int classId);
    virtual bool makeScript(io::IStream* stream);

    // Only the two speed setters are emitted retail methods.  The other four tiny accessors are
    // [CHOSEN] typed homes for generated console executors which directly load/store the fields.
    void setRecoilForce(float value) { m_recoilForce = value; }
    float getRecoilForce() const { return m_recoilForce; }
    void setRecoilSpeed(float value);
    float getRecoilSpeed() const { return m_recoilSpeed; }
    void setRecoilSpeedGB(float value);
    float getRecoilSpeedGB() const { return m_recoilSpeedGB; }
    void setRecoilSize(float value) { m_recoilSize = value; }
    float getRecoilSize() const { return m_recoilSize; }
    void setCameraRecoilSpeed(float value) { m_cameraRecoilSpeed = value; }
    float getCameraRecoilSpeed() const { return m_cameraRecoilSpeed; }
    void setCameraRecoilSize(float value) { m_cameraRecoilSize = value; }
    float getCameraRecoilSize() const { return m_cameraRecoilSize; }

    float getRecoilSpeedPeriod() const { return m_recoilSpeedPeriod; }
    float getRecoilSpeedGBPeriod() const { return m_recoilSpeedGBPeriod; }

private:
    /* +0x08 */ int m_refCount;
    /* +0x0C */ float m_recoilForce;
    /* +0x10 */ float m_recoilSpeed;
    /* +0x14 */ float m_recoilSpeedGB;
    /* +0x18 */ float m_recoilSize;
    /* +0x1C */ float m_recoilSpeedPeriod;
    /* +0x20 */ float m_recoilSpeedGBPeriod;
    /* +0x24 */ float m_cameraRecoilSpeed;
    /* +0x28 */ float m_cameraRecoilSize;
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(RecoilComp) == 0x5C,
              "RecoilComp must match the shipped x86 layout");
static_assert(sizeof(WeaponBasedRecoilComp) == 0x78,
              "WeaponBasedRecoilComp must match the shipped x86 layout");
static_assert(sizeof(WeaponBasedRecoilCompTemplate) == 0x2C,
              "WeaponBasedRecoilCompTemplate must match the shipped x86 layout");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_RECOILCOMPONENTS_WEAPONBASEDRECOILCOMP_H
