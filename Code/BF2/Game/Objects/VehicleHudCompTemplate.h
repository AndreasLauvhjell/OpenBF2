// Path: [INFERRED] from dice::hfe::world -> Game/Objects/.
// Complete template/runtime storage recovered from BF2.exe 1.50 and the symbolized Linux image.
// Retail sizes are 0x1B4 (VC7.1) and 176 (old libstdc++); the 260-byte difference is exactly
// ten std::string deltas plus five vector-layout deltas. Native STL layout controls rebuilt size.
#ifndef GAME_OBJECTS_VEHICLEHUDCOMPTEMPLATE_H
#define GAME_OBJECTS_VEHICLEHUDCOMPTEMPLATE_H

#include <string>
#include <vector>

#include "VehicleHudIcons.h"
#include "CompTemplateInterfaces.h"
#include "../../IO/Console/IConsoleSaveable.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;
class VehicleHudComp;

// ELF +0x18 / PE +0x48. The two native layouts differ only through std::string/std::vector.
struct VehicleHudInfo
{
    int miniMapIconLeaderSizeValue; // 0 small, 1 medium, 2 large
    std::string miniMapIconLeaderSize;
    std::string hudName;
    std::string hudSid;
    std::string pantingSound;
    std::string injurySound;

    bool hasTurretIcon;
    bool showCrossHair;
    bool showInfo;
    bool isCoDriver;
    bool usePlayerIcon;
    bool useChildHud;
    bool standardHelpEnabled;
    bool useSelectionIcons;
    bool displaySelectionOnEnter;
    bool useVehicleCommRose;
    bool disableOnSprint;
    bool unknownBool; // [UNRESOLVED] ctor intentionally leaves this byte untouched.

    std::vector<int> showOnCamModes;
    int enablePostProcessingOnGuiIndex;
    int miniMapIconSize;
    int spottedIconSize;
    int guiIndex;
    int maxClimbSpeed;
    int vehicleType;
    Vec2 vehicleIconPos;
};

// Full 77-slot ELF table (75 callable MSVC slots; one destructor there rather than two).
class VehicleHudCompTemplate : public IVehicleHudCompTemplate, public io::IConsoleSaveable
{
public:
    VehicleHudCompTemplate();

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
    virtual void  setHudName(std::string);
    virtual void  setHudSid(std::string);
    virtual std::string getHudName();
    virtual std::string getHudSid();
    virtual void  setVehicleIcon(const std::string&);
    // Both shipped bodies return the selected vector element's address, falling back to begin().
    // Pointer-versus-reference source spelling is not encoded; existing callers consume void*.
    virtual void* getVehicleIcon(int);
    virtual void  addVehicleIcon(const std::string&);
    virtual int   getVehicleIconNumber() const;
    virtual void  setAbilityIcon(const std::string&);
    virtual void* getAbilityIcon(int);
    virtual void  addAbilityIcon(const std::string&);
    virtual int   getAbilityIconNumber() const;
    virtual void  setShowCrossHair(bool);            virtual bool getShowCrossHair();
    virtual void  setUsePlayerIcon(bool);            virtual bool getUsePlayerIcon();
    virtual void  setDisableOnSprint(bool);          virtual bool getDisableOnSprint();
    virtual void  setUseChildHud(bool);              virtual bool getUseChildHud();
    virtual void  setStandardHelpEnabled(bool);      virtual bool getStandardHelpEnabled();
    virtual void  setVehicleIconPos(const Vec2&);
    virtual const Vec2& getVehicleIconPos();
    virtual void  setHasTurretIcon(bool);            virtual bool getHasTurretIcon();
    virtual void  setShowInfo(bool);                 virtual bool getShowInfo();
    virtual void  setIsCoDriver(bool);               virtual bool getIsCoDriver();
    virtual void  setShowOnCamMode(int);
    virtual void  addShowOnCamMode(int);
    virtual bool  getShowOnCamMode(int);
    virtual void  setMiniMapIcon(std::string);       virtual std::string getMiniMapIcon();
    virtual void  setTypeIcon(std::string);          virtual std::string getTypeIcon();
    virtual void  setMiniMapIconSize(int);           virtual int  getMiniMapIconSize();
    virtual void  setMiniMapIconLeaderSize(std::string);
    virtual std::string getMiniMapIconLeaderSize();
    virtual void  setSpottedIcon(const std::string&);virtual std::string getSpottedIcon();
    virtual void  setSpottedIconSize(int);           virtual int  getSpottedIconSize();
    virtual void  setMaxClimbSpeed(int);             virtual int  getMaxClimbSpeed();
    virtual void  setEnablePostProcessingOnGuiIndex(int);
    virtual int   getEnablePostProcessingOnGuiIndex();
    virtual void  setVehicleType(int);               virtual int  getVehicleType();
    virtual void  setUseSelectionIcons(bool);        virtual bool getUseSelectionIcons();
    virtual void  setDisplaySelectionOnEnter(bool);  virtual bool getDisplaySelectionOnEnter();
    virtual void  setUseVehicleCommRose(bool);       virtual bool getUseVehicleCommRose();
    virtual void  setPantingSound(std::string);      virtual std::string getPantingSound();
    virtual void  setInjurySound(std::string);       virtual std::string getInjurySound();
    virtual ~VehicleHudCompTemplate();
    virtual bool  makeScript(io::IStream*);

protected:
    friend class VehicleHudComp;

    int m_refCount;
    bool m_needsBuild;
    std::string m_unknownString10; // [UNRESOLVED name], ctor/dtor-owned
    std::string m_unknownString14; // [UNRESOLVED name], ctor/dtor-owned
    VehicleHudInfo m_vehicleHudInfo;
    VehicleHudIcons m_vehicleHudIcons;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_VEHICLEHUDCOMPTEMPLATE_H
