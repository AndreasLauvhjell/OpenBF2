// Path: [INFERRED] from dice::hfe::world -> Game/Objects/.
// Complete template/runtime storage recovered from BF2.exe 1.50 and the symbolized Linux image.
// Retail sizes are 0x224 (VC7.1) and 136 (old libstdc++); the 412-byte difference is exactly
// seventeen std::string deltas plus the single vector-layout delta.
#ifndef GAME_OBJECTS_WEAPONHUDCOMPTEMPLATE_H
#define GAME_OBJECTS_WEAPONHUDCOMPTEMPLATE_H

#include <string>
#include <vector>

#include "CompTemplateInterfaces.h"
#include "../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;
class WeaponHudComp;

// ELF +0x18 / PE +0x48. PE reads displaySelectOnActivation at info+0xBD.
struct WeaponHudInfo
{
    std::string hudName;
    std::string hudSid;
    std::string firstLockSound;
    std::string secondLockSound;
    std::string overheatSound;
    int guiIndex;
    int altGuiIndex;
    int deviationFactor;
    int maxVisualDeviation;
    int minVisualDeviation;
    int crosshairIconSize;
    std::vector<int> showOnCamModes;
    bool showInfo;
    bool enableMouse;
    bool hasFireRate;
    bool hasMag;
    bool hasRangeFinder;
    bool switchGuiOnCamera;
    bool showAmmo;
    bool showClips;
    bool disableOnSprint;
    bool displaySelectOnActivation;
    bool unknownBool0; // [UNRESOLVED] ctor intentionally leaves these two bytes untouched.
    bool unknownBool1;
    int enablePostProcessingOnGuiIndex;
};

struct WeaponHudIcons
{
    std::string weaponIcon;
    std::string altWeaponIcon;
    std::string specialAbilityIcon;
    std::string selectIcon;
    std::string crosshairIcon;
    std::string altCrosshairIcon;
    std::string ammoBarFrontIcon;
    std::string ammoBarBackIcon;
    std::string clipIcon;
    std::string ammoIcon;
};

// Full 81-slot ELF table (79 callable MSVC slots; one destructor there rather than two).
class WeaponHudCompTemplate : public IWeaponHudCompTemplate, public io::IConsoleSaveable
{
public:
    WeaponHudCompTemplate();

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
    virtual void  setGuiIndex(int);                  virtual int  getGuiIndex();
    virtual void  setAltGuiIndex(int);               virtual int  getAltGuiIndex();
    virtual void  setHudName(std::string);
    virtual void  setHudSid(std::string);
    virtual std::string getHudName();
    virtual std::string getHudSid();
    virtual void  setEnableMouse(bool);              virtual bool getEnableMouse();
    virtual void  setWeaponIcon(std::string);        virtual std::string getWeaponIcon();
    virtual void  setAltWeaponIcon(std::string);     virtual std::string getAltWeaponIcon();
    virtual void  setSpecialAbilityIcon(std::string);virtual std::string getSpecialAbilityIcon();
    virtual void  setSelectIcon(std::string);        virtual std::string getSelectIcon();
    virtual void  setCrosshairIcon(std::string);     virtual std::string getCrosshairIcon();
    virtual void  setCrosshairIconSize(int);         virtual int  getCrosshairIconSize();
    virtual void  setAltCrosshairIcon(std::string);  virtual std::string getAltCrosshairIcon();
    virtual void  setAmmoBarFrontIcon(std::string);  virtual std::string getAmmoBarFrontIcon();
    virtual void  setAmmoBarBackIcon(std::string);   virtual std::string getAmmoBarBackIcon();
    virtual void  setClipIcon(std::string);          virtual std::string getClipIcon();
    virtual void  setAmmoIcon(std::string);          virtual std::string getAmmoIcon();
    virtual void  setHasFireRate(bool);              virtual bool getHasFireRate();
    virtual void  setHasMag(bool);                   virtual bool getHasMag();
    virtual void  setHasRangeFinder(bool);           virtual bool getHasRangeFinder();
    virtual void  setSwitchGuiOnCamera(bool);        virtual bool getSwitchGuiOnCamera();
    virtual void  setShowOnCamMode(int);
    virtual void  addShowOnCamMode(int);
    virtual bool  getShowOnCamMode(int);
    virtual void  setShowAmmo(bool);                 virtual bool getShowAmmo();
    virtual void  setShowClips(bool);                virtual bool getShowClips();
    virtual void  setDeviationFactor(int);           virtual int  getDeviationFactor();
    virtual void  setMaxVisualDeviation(int);        virtual int  getMaxVisualDeviation();
    virtual void  setMinVisualDeviation(int);        virtual int  getMinVisualDeviation();
    virtual void  setEnablePostProcessingOnGuiIndex(int);
    virtual int   getEnablePostProcessingOnGuiIndex();
    virtual void  setFirstLockSound(std::string);    virtual std::string getFirstLockSound();
    virtual void  setSecondLockSound(std::string);   virtual std::string getSecondLockSound();
    virtual void  setOverheatSound(std::string);     virtual std::string getOverheatSound();
    virtual void  setShowInfo(bool);                 virtual bool getShowInfo();
    virtual void  setDisableOnSprint(bool);          virtual bool getDisableOnSprint();
    virtual void  setDisplaySelectOnActivation(bool);virtual bool getDisplaySelectOnActivation();
    virtual ~WeaponHudCompTemplate();
    virtual bool  makeScript(io::IStream*);

protected:
    friend class WeaponHudComp;

    int m_refCount;
    bool m_needsBuild;
    std::string m_unknownString10; // [UNRESOLVED name], ctor/dtor-owned
    std::string m_unknownString14; // [UNRESOLVED name], ctor/dtor-owned
    WeaponHudInfo m_hudInfo;
    WeaponHudIcons m_hudIcons;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONHUDCOMPTEMPLATE_H
