// Falcon force-feedback configuration: force caps, vehicle controls, sensitivity,
// physics, recoil, and shake. Preserve the 510-slot interface; its deleting
// destructor remains withheld. [INFERRED] path and names not supplied by scripts.
#ifndef GAME_HAPTICSETTINGS_H
#define GAME_HAPTICSETTINGS_H
#include "../Core/IBase.h"
#include "../Math/Vec3.h"
#include <string>
#include <vector>
#if !DEDICATED
namespace dice { namespace hfe {

// One weapon-class row (retail 32 bytes: a 28-byte string and the class id). addWeaponClassMap
// lowercases the name before it is matched or stored; getWeaponClass matches the name as given.
struct WeaponClassEntry {
    std::string name;
    int weaponClass;
};

// Transition levels use 0=medium and 1=high; smoothing levels use 0=low, 1=medium, 2=high.
// Unknown levels read zero or ignore writes. Weapon classes 5 and 10 write the pistol row; unknown IDs read it.
class IHapticSettings : public IBase {
public:
    virtual std::string getFileName() = 0;          //+10 4B7E90 "Haptic.con"
    virtual std::string getSettingsScript() = 0;    //+14 4B9C30 (13,511 B): the annotated Haptic.con text
    virtual bool apply() = 0;                       //+18 4B7EB0: hudManager != 0
    virtual void setDefaults() = 0;                 //+1C 4B7EC0: jumps to setDefaultValues; createDefaultProfile 4A8129
    virtual void setDefaultValues() = 0;            //+20 4BF990 (8,158 B): every value and the shipped weapon-class table
    virtual void loadCurrentProfileFile() = 0;      //+24 4B99F0: <profile name>/Haptic.con through LocalProfileManager::loadProfileFile
    virtual bool reloadCurrentProfileFile() = 0;    //+28 4B9930: the same body reporting whether it loaded
    virtual float getForceCapX() = 0; //+2C 6FEAB0 +8
    virtual void setForceCapX(float) = 0;
    virtual float getForceCapY() = 0; //+34 7E38A0 +C
    virtual void setForceCapY(float) = 0;
    virtual float getForceCapZ() = 0; //+3C 6893E0 +10
    virtual void setForceCapZ(float) = 0;
    virtual Vec3 getControlBoxLength() = 0; //+44 4BD490 +14 [INFERRED] name
    virtual void setControlBoxLength(float x, float y, float z) = 0;
    virtual Vec3 getControlBoxStiffness() = 0; //+4C 4BD4C0 +20 [INFERRED] name
    virtual void setControlBoxStiffness(float x, float y, float z) = 0;
    virtual Vec3 getControlBoxLengthSOLDIER() = 0; //+54 4BD4F0 +2C
    virtual void setControlBoxLengthSOLDIER(float x, float y, float z) = 0;
    virtual Vec3 getControlBoxStiffnessSOLDIER() = 0; //+5C 4BD520 +38
    virtual void setControlBoxStiffnessSOLDIER(float x, float y, float z) = 0;
    virtual Vec3 getControlBoxLengthAIR() = 0;
    virtual void setControlBoxLengthAIR(float x, float y, float z) = 0;
    virtual Vec3 getControlBoxStiffnessAIR() = 0; //+6C 4BD580 +50
    virtual void setControlBoxStiffnessAIR(float x, float y, float z) = 0;
    virtual Vec3 getControlBoxLengthHELI() = 0; //+74 4BD5B0 +5C
    virtual void setControlBoxLengthHELI(float x, float y, float z) = 0;
    virtual Vec3 getControlBoxStiffnessHELI() = 0; //+7C 4BD5E0 +68
    virtual void setControlBoxStiffnessHELI(float x, float y, float z) = 0;
    virtual Vec3 getControlBoxLengthLAND() = 0;
    virtual void setControlBoxLengthLAND(float x, float y, float z) = 0;
    virtual Vec3 getControlBoxStiffnessLAND() = 0; //+8C 4BD640 +80
    virtual void setControlBoxStiffnessLAND(float x, float y, float z) = 0;
    virtual Vec3 getControlBoxLengthSEA() = 0; //+94 4BD670 +8C
    virtual void setControlBoxLengthSEA(float x, float y, float z) = 0;
    virtual Vec3 getControlBoxStiffnessSEA() = 0; //+9C 4BD6A0 +98
    virtual void setControlBoxStiffnessSEA(float x, float y, float z) = 0; //+A0 4BF390
    virtual Vec3 getAimSensitivity() = 0; //+A4 4BD6D0 +A4 [INFERRED] name
    virtual void setAimSensitivity(float x, float y, float z) = 0; //+A8 4BF3D0
    virtual Vec3 getAimSensitivityLength() = 0; //+AC 4BD700 +B0 [INFERRED] name
    virtual void setAimSensitivityLength(float x, float y, float z) = 0; //+B0 4BF410
    virtual Vec3 getTurnSensitivity() = 0; //+B4 4BD730 +BC [INFERRED] name
    virtual void setTurnSensitivity(float x, float y, float z) = 0; //+B8 4BF450
    virtual Vec3 getAimSensitivitySOLDIER() = 0; //+BC 4BD760 +C8
    virtual void setAimSensitivitySOLDIER(float x, float y, float z) = 0; //+C0 4BF490
    virtual Vec3 getAimSensitivityLengthSOLDIER() = 0; //+C4 4BD790 +D4
    virtual void setAimSensitivityLengthSOLDIER(float x, float y, float z) = 0; //+C8 4BF4D0
    virtual Vec3 getTurnSensitivitySOLDIER() = 0; //+CC 4BD7C0 +E0
    virtual void setTurnSensitivitySOLDIER(float x, float y, float z) = 0;
    virtual Vec3 getAimSensitivityAIR() = 0; //+D4 4BD7F0 +EC
    virtual void setAimSensitivityAIR(float x, float y, float z) = 0; //+D8 4BF550
    virtual Vec3 getAimSensitivityLengthAIR() = 0; //+DC 4BD820 +F8
    virtual void setAimSensitivityLengthAIR(float x, float y, float z) = 0; //+E0 4BF590
    virtual Vec3 getTurnSensitivityAIR() = 0; //+E4 4BD850 +104
    virtual void setTurnSensitivityAIR(float x, float y, float z) = 0; //+E8 4BF5D0
    virtual Vec3 getAimSensitivityHELI() = 0; //+EC 4BD880 +110
    virtual void setAimSensitivityHELI(float x, float y, float z) = 0; //+F0 4BF610
    virtual Vec3 getAimSensitivityLengthHELI() = 0; //+F4 4BD8B0 +11C
    virtual void setAimSensitivityLengthHELI(float x, float y, float z) = 0; //+F8 4BF650
    virtual Vec3 getTurnSensitivityHELI() = 0; //+FC 4BD8E0 +128
    virtual void setTurnSensitivityHELI(float x, float y, float z) = 0;
    virtual Vec3 getAimSensitivityLAND() = 0;
    virtual void setAimSensitivityLAND(float x, float y, float z) = 0;
    virtual Vec3 getAimSensitivityLengthLAND() = 0; //+10C 4BD940 +140
    virtual void setAimSensitivityLengthLAND(float x, float y, float z) = 0;
    virtual Vec3 getTurnSensitivityLAND() = 0; //+114 4BD970 +14C
    virtual void setTurnSensitivityLAND(float x, float y, float z) = 0;
    virtual Vec3 getAimSensitivitySEA() = 0; //+11C 4BD9A0 +158
    virtual void setAimSensitivitySEA(float x, float y, float z) = 0;
    virtual Vec3 getAimSensitivityLengthSEA() = 0;
    virtual void setAimSensitivityLengthSEA(float x, float y, float z) = 0;
    virtual Vec3 getTurnSensitivitySEA() = 0; //+12C 4BDA00 +170
    virtual void setTurnSensitivitySEA(float x, float y, float z) = 0;
    virtual Vec3 getPhysicsSOLDIERScale() = 0; //+134 4BDA30 +17C
    virtual void setPhysicsSOLDIERScale(float x, float y, float z) = 0;
    virtual float getPhysicsSOLDIERScaleXLow() = 0; //+13C 4BDA60 +188
    virtual void setPhysicsSOLDIERScaleXLow(float) = 0;
    virtual float getPhysicsSOLDIERScaleYLow() = 0; //+144 4BDA80 +18C
    virtual void setPhysicsSOLDIERScaleYLow(float) = 0;
    virtual float getPhysicsSOLDIERScaleZLow() = 0; //+14C 4BDA90 +190
    virtual void setPhysicsSOLDIERScaleZLow(float) = 0;
    virtual float getPhysicsSOLDIERScaleXMed() = 0;
    virtual void setPhysicsSOLDIERScaleXMed(float) = 0;
    virtual float getPhysicsSOLDIERScaleYMed() = 0; //+15C 4BDAD0 +198
    virtual void setPhysicsSOLDIERScaleYMed(float) = 0;
    virtual float getPhysicsSOLDIERScaleZMed() = 0; //+164 4BDAF0 +19C
    virtual void setPhysicsSOLDIERScaleZMed(float) = 0;
    virtual float getPhysicsSOLDIERScaleXHigh() = 0; //+16C 4BDB10 +1A0
    virtual void setPhysicsSOLDIERScaleXHigh(float) = 0;
    virtual float getPhysicsSOLDIERScaleYHigh() = 0; //+174 4BDB20 +1A4
    virtual void setPhysicsSOLDIERScaleYHigh(float) = 0;
    virtual float getPhysicsSOLDIERScaleZHigh() = 0; //+17C 4BDB30 +1A8
    virtual void setPhysicsSOLDIERScaleZHigh(float) = 0;
    virtual float getPhysicsSOLDIERTransitionValue(int level) = 0; //+184 4B7ED0: 0 getPhysicsSOLDIERTransitionValueMed, 1 getPhysicsSOLDIERTransitionValueHigh
    virtual void setPhysicsSOLDIERTransitionValue(int level, float value) = 0; //+188 4B7F10: 0 setPhysicsSOLDIERTransitionValueMed, 1 setPhysicsSOLDIERTransitionValueHigh
    virtual float getPhysicsSOLDIERTransitionValueMed() = 0; //+18C 4BDB50 +1AC
    virtual void setPhysicsSOLDIERTransitionValueMed(float) = 0;
    virtual float getPhysicsSOLDIERTransitionValueHigh() = 0; //+194 4BDB70 +1B0
    virtual void setPhysicsSOLDIERTransitionValueHigh(float) = 0;
    virtual int getPhysicsSOLDIERSmoothTimeX(int level) = 0; //+19C 4B7F40: 0 getPhysicsSOLDIERSmoothTimeXLow, 1 getPhysicsSOLDIERSmoothTimeXMed, 2 getPhysicsSOLDIERSmoothTimeXHigh
    virtual void setPhysicsSOLDIERSmoothTimeX(int level, int value) = 0; //+1A0 4B7F80: 0 setPhysicsSOLDIERSmoothTimeXLow, 1 setPhysicsSOLDIERSmoothTimeXMed, 2 setPhysicsSOLDIERSmoothTimeXHigh
    virtual int getPhysicsSOLDIERSmoothTimeY(int level) = 0; //+1A4 4B7FD0: 0 getPhysicsSOLDIERSmoothTimeYLow, 1 getPhysicsSOLDIERSmoothTimeYMed, 2 getPhysicsSOLDIERSmoothTimeYHigh
    virtual void setPhysicsSOLDIERSmoothTimeY(int level, int value) = 0; //+1A8 4B8010: 0 setPhysicsSOLDIERSmoothTimeYLow, 1 setPhysicsSOLDIERSmoothTimeYMed, 2 setPhysicsSOLDIERSmoothTimeYHigh
    virtual int getPhysicsSOLDIERSmoothTimeZ(int level) = 0; //+1AC 4B8060: 0 getPhysicsSOLDIERSmoothTimeZLow, 1 getPhysicsSOLDIERSmoothTimeZMed, 2 getPhysicsSOLDIERSmoothTimeZHigh
    virtual void setPhysicsSOLDIERSmoothTimeZ(int level, int value) = 0; //+1B0 4B80A0: 0 setPhysicsSOLDIERSmoothTimeZLow, 1 setPhysicsSOLDIERSmoothTimeZMed, 2 setPhysicsSOLDIERSmoothTimeZHigh
    virtual int getPhysicsSOLDIERSmoothTimeXLow() = 0; //+1B4 4BDB90 +1B4
    virtual void setPhysicsSOLDIERSmoothTimeXLow(int) = 0; //+1B8 4BDBA0
    virtual int getPhysicsSOLDIERSmoothTimeYLow() = 0; //+1BC 4BDBB0 +1B8
    virtual void setPhysicsSOLDIERSmoothTimeYLow(int) = 0; //+1C0 4BDBC0
    virtual int getPhysicsSOLDIERSmoothTimeZLow() = 0; //+1C4 4BDBD0 +1BC
    virtual void setPhysicsSOLDIERSmoothTimeZLow(int) = 0; //+1C8 4BDBE0
    virtual int getPhysicsSOLDIERSmoothTimeXMed() = 0; //+1CC 4BDBF0 +1C0
    virtual void setPhysicsSOLDIERSmoothTimeXMed(int) = 0; //+1D0 4BDC00
    virtual int getPhysicsSOLDIERSmoothTimeYMed() = 0; //+1D4 4BDC10 +1C4
    virtual void setPhysicsSOLDIERSmoothTimeYMed(int) = 0; //+1D8 5AB340
    virtual int getPhysicsSOLDIERSmoothTimeZMed() = 0; //+1DC 4BDC20 +1C8
    virtual void setPhysicsSOLDIERSmoothTimeZMed(int) = 0; //+1E0 5AB350
    virtual int getPhysicsSOLDIERSmoothTimeXHigh() = 0; //+1E4 4BDC30 +1CC
    virtual void setPhysicsSOLDIERSmoothTimeXHigh(int) = 0; //+1E8 5AB360
    virtual int getPhysicsSOLDIERSmoothTimeYHigh() = 0; //+1EC 4BDC40 +1D0
    virtual void setPhysicsSOLDIERSmoothTimeYHigh(int) = 0; //+1F0 4B3DC0
    virtual int getPhysicsSOLDIERSmoothTimeZHigh() = 0; //+1F4 4BDC50 +1D4
    virtual void setPhysicsSOLDIERSmoothTimeZHigh(int) = 0; //+1F8 4BDC60
    virtual Vec3 getPhysicsLANDScale() = 0; //+1FC 4BDC70 +1D8
    virtual void setPhysicsLANDScale(float x, float y, float z) = 0;
    virtual float getPhysicsLANDScaleXLow() = 0; //+204 4BDCA0 +1E4
    virtual void setPhysicsLANDScaleXLow(float) = 0;
    virtual float getPhysicsLANDScaleYLow() = 0; //+20C 4BDCC0 +1E8
    virtual void setPhysicsLANDScaleYLow(float) = 0;
    virtual float getPhysicsLANDScaleZLow() = 0; //+214 4BDCE0 +1EC
    virtual void setPhysicsLANDScaleZLow(float) = 0;
    virtual float getPhysicsLANDScaleXMed() = 0; //+21C 4BDCF0 +1F0
    virtual void setPhysicsLANDScaleXMed(float) = 0;
    virtual float getPhysicsLANDScaleYMed() = 0; //+224 4BDD10 +1F4
    virtual void setPhysicsLANDScaleYMed(float) = 0;
    virtual float getPhysicsLANDScaleZMed() = 0; //+22C 4BDD20 +1F8
    virtual void setPhysicsLANDScaleZMed(float) = 0;
    virtual float getPhysicsLANDScaleXHigh() = 0; //+234 4BDD30 +1FC
    virtual void setPhysicsLANDScaleXHigh(float) = 0;
    virtual float getPhysicsLANDScaleYHigh() = 0; //+23C 4BDD40 +200
    virtual void setPhysicsLANDScaleYHigh(float) = 0;
    virtual float getPhysicsLANDScaleZHigh() = 0;
    virtual void setPhysicsLANDScaleZHigh(float) = 0;
    virtual float getPhysicsLANDTransitionValue(int level) = 0; //+24C 4B80F0: 0 getPhysicsLANDTransitionValueMed, 1 getPhysicsLANDTransitionValueHigh
    virtual void setPhysicsLANDTransitionValue(int level, float value) = 0; //+250 4B8130: 0 setPhysicsLANDTransitionValueMed, 1 setPhysicsLANDTransitionValueHigh
    virtual float getPhysicsLANDTransitionValueMed() = 0;
    virtual void setPhysicsLANDTransitionValueMed(float) = 0;
    virtual float getPhysicsLANDTransitionValueHigh() = 0; //+25C 4BDD70 +20C
    virtual void setPhysicsLANDTransitionValueHigh(float) = 0;
    virtual int getPhysicsLANDSmoothTimeX(int level) = 0; //+264 4B8160: 0 getPhysicsLANDSmoothTimeXLow, 1 getPhysicsLANDSmoothTimeXMed, 2 getPhysicsLANDSmoothTimeXHigh
    virtual void setPhysicsLANDSmoothTimeX(int level, int value) = 0; //+268 4B81A0: 0 setPhysicsLANDSmoothTimeXLow, 1 setPhysicsLANDSmoothTimeXMed, 2 setPhysicsLANDSmoothTimeXHigh
    virtual int getPhysicsLANDSmoothTimeY(int level) = 0; //+26C 4B81F0: 0 getPhysicsLANDSmoothTimeYLow, 1 getPhysicsLANDSmoothTimeYMed, 2 getPhysicsLANDSmoothTimeYHigh
    virtual void setPhysicsLANDSmoothTimeY(int level, int value) = 0; //+270 4B8230: 0 setPhysicsLANDSmoothTimeYLow, 1 setPhysicsLANDSmoothTimeYMed, 2 setPhysicsLANDSmoothTimeYHigh
    virtual int getPhysicsLANDSmoothTimeZ(int level) = 0; //+274 4B8280: 0 getPhysicsLANDSmoothTimeZLow, 1 getPhysicsLANDSmoothTimeZMed, 2 getPhysicsLANDSmoothTimeZHigh
    virtual void setPhysicsLANDSmoothTimeZ(int level, int value) = 0; //+278 4B82C0: 0 setPhysicsLANDSmoothTimeZLow, 1 setPhysicsLANDSmoothTimeZMed, 2 setPhysicsLANDSmoothTimeZHigh
    virtual int getPhysicsLANDSmoothTimeXLow() = 0; //+27C 4BDD90 +210
    virtual void setPhysicsLANDSmoothTimeXLow(int) = 0;
    virtual int getPhysicsLANDSmoothTimeYLow() = 0;
    virtual void setPhysicsLANDSmoothTimeYLow(int) = 0;
    virtual int getPhysicsLANDSmoothTimeZLow() = 0; //+28C 4BDDD0 +218
    virtual void setPhysicsLANDSmoothTimeZLow(int) = 0;
    virtual int getPhysicsLANDSmoothTimeXMed() = 0; //+294 4BDDE0 +21C
    virtual void setPhysicsLANDSmoothTimeXMed(int) = 0;
    virtual int getPhysicsLANDSmoothTimeYMed() = 0; //+29C 4BDE00 +220
    virtual void setPhysicsLANDSmoothTimeYMed(int) = 0; //+2A0 4BDE10
    virtual int getPhysicsLANDSmoothTimeZMed() = 0; //+2A4 4BDE20 +224
    virtual void setPhysicsLANDSmoothTimeZMed(int) = 0; //+2A8 4BDE30
    virtual int getPhysicsLANDSmoothTimeXHigh() = 0; //+2AC 4BDE40 +228
    virtual void setPhysicsLANDSmoothTimeXHigh(int) = 0; //+2B0 4BDE50
    virtual int getPhysicsLANDSmoothTimeYHigh() = 0; //+2B4 4BDE60 +22C
    virtual void setPhysicsLANDSmoothTimeYHigh(int) = 0; //+2B8 4BDE70
    virtual int getPhysicsLANDSmoothTimeZHigh() = 0; //+2BC 4BDE80 +230
    virtual void setPhysicsLANDSmoothTimeZHigh(int) = 0; //+2C0 4BDE90
    virtual Vec3 getPhysicsAIRScale() = 0; //+2C4 4BDEA0 +234
    virtual void setPhysicsAIRScale(float x, float y, float z) = 0; //+2C8 4BF8D0
    virtual float getPhysicsAIRScaleXLow() = 0; //+2CC 4BDED0 +240
    virtual void setPhysicsAIRScaleXLow(float) = 0; //+2D0 4BDEE0
    virtual float getPhysicsAIRScaleYLow() = 0; //+2D4 4BDEF0 +244
    virtual void setPhysicsAIRScaleYLow(float) = 0; //+2D8 4BDF00
    virtual float getPhysicsAIRScaleZLow() = 0; //+2DC 4BDF10 +248
    virtual void setPhysicsAIRScaleZLow(float) = 0; //+2E0 4BDF20
    virtual float getPhysicsAIRScaleXMed() = 0; //+2E4 4BDF30 +24C
    virtual void setPhysicsAIRScaleXMed(float) = 0; //+2E8 4BDF40
    virtual float getPhysicsAIRScaleYMed() = 0; //+2EC 4BDF50 +250
    virtual void setPhysicsAIRScaleYMed(float) = 0; //+2F0 594620
    virtual float getPhysicsAIRScaleZMed() = 0; //+2F4 4BDF60 +254
    virtual void setPhysicsAIRScaleZMed(float) = 0; //+2F8 4BDF70
    virtual float getPhysicsAIRScaleXHigh() = 0; //+2FC 4BDF80 +258
    virtual void setPhysicsAIRScaleXHigh(float) = 0;
    virtual float getPhysicsAIRScaleYHigh() = 0; //+304 4BDFA0 +25C
    virtual void setPhysicsAIRScaleYHigh(float) = 0;
    virtual float getPhysicsAIRScaleZHigh() = 0; //+30C 4BDFB0 +260
    virtual void setPhysicsAIRScaleZHigh(float) = 0;
    virtual float getPhysicsAIRTransitionValue(int level) = 0; //+314 4B8310: 0 getPhysicsAIRTransitionValueMed, 1 getPhysicsAIRTransitionValueHigh
    virtual void setPhysicsAIRTransitionValue(int level, float value) = 0; //+318 4B8350: 0 setPhysicsAIRTransitionValueMed, 1 setPhysicsAIRTransitionValueHigh
    virtual float getPhysicsAIRTransitionValueMed() = 0; //+31C 4BDFC0 +264
    virtual void setPhysicsAIRTransitionValueMed(float) = 0;
    virtual float getPhysicsAIRTransitionValueHigh() = 0;
    virtual void setPhysicsAIRTransitionValueHigh(float) = 0;
    virtual int getPhysicsAIRSmoothTimeX(int level) = 0; //+32C 4B8380: 0 getPhysicsAIRSmoothTimeXLow, 1 getPhysicsAIRSmoothTimeXMed, 2 getPhysicsAIRSmoothTimeXHigh
    virtual void setPhysicsAIRSmoothTimeX(int level, int value) = 0; //+330 4B83C0: 0 setPhysicsAIRSmoothTimeXLow, 1 setPhysicsAIRSmoothTimeXMed, 2 setPhysicsAIRSmoothTimeXHigh
    virtual int getPhysicsAIRSmoothTimeY(int level) = 0; //+334 4B8410: 0 getPhysicsAIRSmoothTimeYLow, 1 getPhysicsAIRSmoothTimeYMed, 2 getPhysicsAIRSmoothTimeYHigh
    virtual void setPhysicsAIRSmoothTimeY(int level, int value) = 0; //+338 4B8450: 0 setPhysicsAIRSmoothTimeYLow, 1 setPhysicsAIRSmoothTimeYMed, 2 setPhysicsAIRSmoothTimeYHigh
    virtual int getPhysicsAIRSmoothTimeZ(int level) = 0; //+33C 4B84A0: 0 getPhysicsAIRSmoothTimeZLow, 1 getPhysicsAIRSmoothTimeZMed, 2 getPhysicsAIRSmoothTimeZHigh
    virtual void setPhysicsAIRSmoothTimeZ(int level, int value) = 0; //+340 4B84E0: 0 setPhysicsAIRSmoothTimeZLow, 1 setPhysicsAIRSmoothTimeZMed, 2 setPhysicsAIRSmoothTimeZHigh
    virtual int getPhysicsAIRSmoothTimeXLow() = 0; //+344 4BDFF0 +26C
    virtual void setPhysicsAIRSmoothTimeXLow(int) = 0;
    virtual int getPhysicsAIRSmoothTimeYLow() = 0; //+34C 4BE000 +270
    virtual void setPhysicsAIRSmoothTimeYLow(int) = 0;
    virtual int getPhysicsAIRSmoothTimeZLow() = 0;
    virtual void setPhysicsAIRSmoothTimeZLow(int) = 0;
    virtual int getPhysicsAIRSmoothTimeXMed() = 0; //+35C 4BE030 +278
    virtual void setPhysicsAIRSmoothTimeXMed(int) = 0;
    virtual int getPhysicsAIRSmoothTimeYMed() = 0; //+364 4BE050 +27C
    virtual void setPhysicsAIRSmoothTimeYMed(int) = 0;
    virtual int getPhysicsAIRSmoothTimeZMed() = 0; //+36C 4BE070 +280
    virtual void setPhysicsAIRSmoothTimeZMed(int) = 0;
    virtual int getPhysicsAIRSmoothTimeXHigh() = 0;
    virtual void setPhysicsAIRSmoothTimeXHigh(int) = 0;
    virtual int getPhysicsAIRSmoothTimeYHigh() = 0; //+37C 4BE0B0 +288
    virtual void setPhysicsAIRSmoothTimeYHigh(int) = 0;
    virtual int getPhysicsAIRSmoothTimeZHigh() = 0; //+384 4BE0D0 +28C
    virtual void setPhysicsAIRSmoothTimeZHigh(int) = 0;
    virtual Vec3 getPhysicsHELIScale() = 0; //+38C 4BE0F0 +290
    virtual void setPhysicsHELIScale(float x, float y, float z) = 0;
    virtual float getPhysicsHELIScaleXLow() = 0; //+394 4BE120 +29C
    virtual void setPhysicsHELIScaleXLow(float) = 0;
    virtual float getPhysicsHELIScaleYLow() = 0; //+39C 4BE140 +2A0
    virtual void setPhysicsHELIScaleYLow(float) = 0; //+3A0 4BE150
    virtual float getPhysicsHELIScaleZLow() = 0; //+3A4 4BE160 +2A4
    virtual void setPhysicsHELIScaleZLow(float) = 0; //+3A8 4BE170
    virtual float getPhysicsHELIScaleXMed() = 0; //+3AC 4BE180 +2A8
    virtual void setPhysicsHELIScaleXMed(float) = 0; //+3B0 4BE190
    virtual float getPhysicsHELIScaleYMed() = 0; //+3B4 4BE1A0 +2AC
    virtual void setPhysicsHELIScaleYMed(float) = 0; //+3B8 4BE1B0
    virtual float getPhysicsHELIScaleZMed() = 0; //+3BC 4BE1C0 +2B0
    virtual void setPhysicsHELIScaleZMed(float) = 0; //+3C0 4BE1D0
    virtual float getPhysicsHELIScaleXHigh() = 0; //+3C4 4BE1E0 +2B4
    virtual void setPhysicsHELIScaleXHigh(float) = 0; //+3C8 4BE1F0
    virtual float getPhysicsHELIScaleYHigh() = 0; //+3CC 4BE200 +2B8
    virtual void setPhysicsHELIScaleYHigh(float) = 0; //+3D0 4BE210
    virtual float getPhysicsHELIScaleZHigh() = 0; //+3D4 4BE220 +2BC
    virtual void setPhysicsHELIScaleZHigh(float) = 0; //+3D8 4BE230
    virtual float getPhysicsHELITransitionValue(int level) = 0; //+3DC 4B8530: 0 getPhysicsHELITransitionValueMed, 1 getPhysicsHELITransitionValueHigh
    virtual void setPhysicsHELITransitionValue(int level, float value) = 0; //+3E0 4B8570: 0 setPhysicsHELITransitionValueMed, 1 setPhysicsHELITransitionValueHigh
    virtual float getPhysicsHELITransitionValueMed() = 0; //+3E4 4BE240 +2C0
    virtual void setPhysicsHELITransitionValueMed(float) = 0; //+3E8 4BE250
    virtual float getPhysicsHELITransitionValueHigh() = 0; //+3EC 4BE260 +2C4
    virtual void setPhysicsHELITransitionValueHigh(float) = 0; //+3F0 4BE270
    virtual int getPhysicsHELISmoothTimeX(int level) = 0; //+3F4 4B85A0: 0 getPhysicsHELISmoothTimeXLow, 1 getPhysicsHELISmoothTimeXMed, 2 getPhysicsHELISmoothTimeXHigh
    virtual void setPhysicsHELISmoothTimeX(int level, int value) = 0; //+3F8 4B85E0: 0 setPhysicsHELISmoothTimeXLow, 1 setPhysicsHELISmoothTimeXMed, 2 setPhysicsHELISmoothTimeXHigh
    virtual int getPhysicsHELISmoothTimeY(int level) = 0; //+3FC 4B8630: 0 getPhysicsHELISmoothTimeYLow, 1 getPhysicsHELISmoothTimeYMed, 2 getPhysicsHELISmoothTimeYHigh
    virtual void setPhysicsHELISmoothTimeY(int level, int value) = 0; //+400 4B8670: 0 setPhysicsHELISmoothTimeYLow, 1 setPhysicsHELISmoothTimeYMed, 2 setPhysicsHELISmoothTimeYHigh
    virtual int getPhysicsHELISmoothTimeZ(int level) = 0; //+404 4B86C0: 0 getPhysicsHELISmoothTimeZLow, 1 getPhysicsHELISmoothTimeZMed, 2 getPhysicsHELISmoothTimeZHigh
    virtual void setPhysicsHELISmoothTimeZ(int level, int value) = 0; //+408 4B8700: 0 setPhysicsHELISmoothTimeZLow, 1 setPhysicsHELISmoothTimeZMed, 2 setPhysicsHELISmoothTimeZHigh
    virtual int getPhysicsHELISmoothTimeXLow() = 0; //+40C 4BE280 +2C8
    virtual void setPhysicsHELISmoothTimeXLow(int) = 0;
    virtual int getPhysicsHELISmoothTimeYLow() = 0; //+414 4BE2A0 +2CC
    virtual void setPhysicsHELISmoothTimeYLow(int) = 0;
    virtual int getPhysicsHELISmoothTimeZLow() = 0; //+41C 4BE2C0 +2D0
    virtual void setPhysicsHELISmoothTimeZLow(int) = 0;
    virtual int getPhysicsHELISmoothTimeXMed() = 0; //+424 4BE2E0 +2D4
    virtual void setPhysicsHELISmoothTimeXMed(int) = 0;
    virtual int getPhysicsHELISmoothTimeYMed() = 0; //+42C 4BE300 +2D8
    virtual void setPhysicsHELISmoothTimeYMed(int) = 0;
    virtual int getPhysicsHELISmoothTimeZMed() = 0; //+434 4BE320 +2DC
    virtual void setPhysicsHELISmoothTimeZMed(int) = 0;
    virtual int getPhysicsHELISmoothTimeXHigh() = 0; //+43C 4BE340 +2E0
    virtual void setPhysicsHELISmoothTimeXHigh(int) = 0;
    virtual int getPhysicsHELISmoothTimeYHigh() = 0; //+444 4BE360 +2E4
    virtual void setPhysicsHELISmoothTimeYHigh(int) = 0;
    virtual int getPhysicsHELISmoothTimeZHigh() = 0; //+44C 4BE380 +2E8
    virtual void setPhysicsHELISmoothTimeZHigh(int) = 0;
    virtual Vec3 getPhysicsSEAScale() = 0; //+454 4BE3A0 +2EC
    virtual void setPhysicsSEAScale(float x, float y, float z) = 0;
    virtual float getPhysicsSEAScaleXLow() = 0; //+45C 4BE3D0 +2F8
    virtual void setPhysicsSEAScaleXLow(float) = 0;
    virtual float getPhysicsSEAScaleYLow() = 0; //+464 4BE3F0 +2FC
    virtual void setPhysicsSEAScaleYLow(float) = 0;
    virtual float getPhysicsSEAScaleZLow() = 0; //+46C 4BE410 +300
    virtual void setPhysicsSEAScaleZLow(float) = 0;
    virtual float getPhysicsSEAScaleXMed() = 0;
    virtual void setPhysicsSEAScaleXMed(float) = 0;
    virtual float getPhysicsSEAScaleYMed() = 0; //+47C 4BE450 +308
    virtual void setPhysicsSEAScaleYMed(float) = 0;
    virtual float getPhysicsSEAScaleZMed() = 0; //+484 4BE470 +30C
    virtual void setPhysicsSEAScaleZMed(float) = 0;
    virtual float getPhysicsSEAScaleXHigh() = 0; //+48C 4BE490 +310
    virtual void setPhysicsSEAScaleXHigh(float) = 0;
    virtual float getPhysicsSEAScaleYHigh() = 0;
    virtual void setPhysicsSEAScaleYHigh(float) = 0;
    virtual float getPhysicsSEAScaleZHigh() = 0; //+49C 4BE4D0 +318
    virtual void setPhysicsSEAScaleZHigh(float) = 0; //+4A0 4BE4E0
    virtual float getPhysicsSEATransitionValue(int level) = 0; //+4A4 4B8750: 0 getPhysicsSEATransitionValueMed, 1 getPhysicsSEATransitionValueHigh
    virtual void setPhysicsSEATransitionValue(int level, float value) = 0; //+4A8 4B8790: 0 setPhysicsSEATransitionValueMed, 1 setPhysicsSEATransitionValueHigh
    virtual float getPhysicsSEATransitionValueMed() = 0; //+4AC 4BE4F0 +31C
    virtual void setPhysicsSEATransitionValueMed(float) = 0; //+4B0 4E1860
    virtual float getPhysicsSEATransitionValueHigh() = 0; //+4B4 4BE500 +320
    virtual void setPhysicsSEATransitionValueHigh(float) = 0; //+4B8 4BE510
    virtual int getPhysicsSEASmoothTimeX(int level) = 0; //+4BC 4B87C0: 0 getPhysicsSEASmoothTimeXLow, 1 getPhysicsSEASmoothTimeXMed, 2 getPhysicsSEASmoothTimeXHigh
    virtual void setPhysicsSEASmoothTimeX(int level, int value) = 0; //+4C0 4B8800: 0 setPhysicsSEASmoothTimeXLow, 1 setPhysicsSEASmoothTimeXMed, 2 setPhysicsSEASmoothTimeXHigh
    virtual int getPhysicsSEASmoothTimeY(int level) = 0; //+4C4 4B8850: 0 getPhysicsSEASmoothTimeYLow, 1 getPhysicsSEASmoothTimeYMed, 2 getPhysicsSEASmoothTimeYHigh
    virtual void setPhysicsSEASmoothTimeY(int level, int value) = 0; //+4C8 4B8890: 0 setPhysicsSEASmoothTimeYLow, 1 setPhysicsSEASmoothTimeYMed, 2 setPhysicsSEASmoothTimeYHigh
    virtual int getPhysicsSEASmoothTimeZ(int level) = 0; //+4CC 4B88E0: 0 getPhysicsSEASmoothTimeZLow, 1 getPhysicsSEASmoothTimeZMed, 2 getPhysicsSEASmoothTimeZHigh
    virtual void setPhysicsSEASmoothTimeZ(int level, int value) = 0; //+4D0 4B8920: 0 setPhysicsSEASmoothTimeZLow, 1 setPhysicsSEASmoothTimeZMed, 2 setPhysicsSEASmoothTimeZHigh
    virtual int getPhysicsSEASmoothTimeXLow() = 0; //+4D4 4BE520 +324
    virtual void setPhysicsSEASmoothTimeXLow(int) = 0; //+4D8 4BE530
    virtual int getPhysicsSEASmoothTimeYLow() = 0; //+4DC 4BE540 +328
    virtual void setPhysicsSEASmoothTimeYLow(int) = 0; //+4E0 4BE550
    virtual int getPhysicsSEASmoothTimeZLow() = 0; //+4E4 4BE560 +32C
    virtual void setPhysicsSEASmoothTimeZLow(int) = 0; //+4E8 4BE570
    virtual int getPhysicsSEASmoothTimeXMed() = 0; //+4EC 4BE580 +330
    virtual void setPhysicsSEASmoothTimeXMed(int) = 0; //+4F0 4BE590
    virtual int getPhysicsSEASmoothTimeYMed() = 0; //+4F4 4BE5A0 +334
    virtual void setPhysicsSEASmoothTimeYMed(int) = 0; //+4F8 4BE5B0
    virtual int getPhysicsSEASmoothTimeZMed() = 0; //+4FC 4BE5C0 +338
    virtual void setPhysicsSEASmoothTimeZMed(int) = 0;
    virtual int getPhysicsSEASmoothTimeXHigh() = 0; //+504 4BE5E0 +33C
    virtual void setPhysicsSEASmoothTimeXHigh(int) = 0;
    virtual int getPhysicsSEASmoothTimeYHigh() = 0; //+50C 4BE600 +340
    virtual void setPhysicsSEASmoothTimeYHigh(int) = 0;
    virtual int getPhysicsSEASmoothTimeZHigh() = 0;
    virtual void setPhysicsSEASmoothTimeZHigh(int) = 0;
    virtual float getDamageScale() = 0; //+51C 4BE640 +348
    virtual void setDamageScale(float) = 0;
    virtual float getRecoilPunchScale(int weaponClass) = 0; //+524 4B8970 (269 B): by class id, any other id reads the pistol row
    virtual void setRecoilPunchScale(int weaponClass, float value) = 0; //+528 4B8B40 (333 B): by class id; 5 and 10 share the pistol row, other ids are ignored
    virtual float getRecoilPunchTime(int weaponClass) = 0; //+52C 4B8D50 (269 B): by class id, any other id reads the pistol row
    virtual void setRecoilPunchTime(int weaponClass, float value) = 0; //+530 4B8F20 (333 B): by class id; 5 and 10 share the pistol row, other ids are ignored
    virtual float getRecoilYawScale(int weaponClass) = 0; //+534 4B9130 (269 B): by class id, any other id reads the pistol row
    virtual void setRecoilYawScale(int weaponClass, float value) = 0; //+538 4B9300 (333 B): by class id; 5 and 10 share the pistol row, other ids are ignored
    virtual float getRecoilPitchScale(int weaponClass) = 0; //+53C 4B9510 (269 B): by class id, any other id reads the pistol row
    virtual void setRecoilPitchScale(int weaponClass, float value) = 0; //+540 4B96E0 (333 B): by class id; 5 and 10 share the pistol row, other ids are ignored
    virtual float getRecoilPunchScaleRIFLE() = 0; //+544 4BE660 +34C
    virtual void setRecoilPunchScaleRIFLE(float) = 0;
    virtual float getRecoilPunchTimeRIFLE() = 0; //+54C 4BE670 +350
    virtual void setRecoilPunchTimeRIFLE(float) = 0;
    virtual float getRecoilYawScaleRIFLE() = 0;
    virtual void setRecoilYawScaleRIFLE(float) = 0;
    virtual float getRecoilPitchScaleRIFLE() = 0; //+55C 4BE6B0 +358
    virtual void setRecoilPitchScaleRIFLE(float) = 0;
    virtual float getRecoilPunchScaleRIFLELAUNCHER() = 0; //+564 4BE6D0 +35C
    virtual void setRecoilPunchScaleRIFLELAUNCHER(float) = 0;
    virtual float getRecoilPunchTimeRIFLELAUNCHER() = 0; //+56C 4BE6F0 +360
    virtual void setRecoilPunchTimeRIFLELAUNCHER(float) = 0;
    virtual float getRecoilYawScaleRIFLELAUNCHER() = 0;
    virtual void setRecoilYawScaleRIFLELAUNCHER(float) = 0;
    virtual float getRecoilPitchScaleRIFLELAUNCHER() = 0; //+57C 4BE720 +368
    virtual void setRecoilPitchScaleRIFLELAUNCHER(float) = 0;
    virtual float getRecoilPunchScaleCARBINE() = 0; //+584 4BE740 +36C
    virtual void setRecoilPunchScaleCARBINE(float) = 0;
    virtual float getRecoilPunchTimeCARBINE() = 0; //+58C 4BE760 +370
    virtual void setRecoilPunchTimeCARBINE(float) = 0;
    virtual float getRecoilYawScaleCARBINE() = 0;
    virtual void setRecoilYawScaleCARBINE(float) = 0;
    virtual float getRecoilPitchScaleCARBINE() = 0; //+59C 4BE7A0 +378
    virtual void setRecoilPitchScaleCARBINE(float) = 0; //+5A0 4BE7B0
    virtual float getRecoilPunchScaleLMG() = 0; //+5A4 4BE7C0 +37C
    virtual void setRecoilPunchScaleLMG(float) = 0; //+5A8 4BE7D0
    virtual float getRecoilPunchTimeLMG() = 0; //+5AC 4BE7E0 +380
    virtual void setRecoilPunchTimeLMG(float) = 0; //+5B0 4BE7F0
    virtual float getRecoilYawScaleLMG() = 0; //+5B4 4BE800 +384
    virtual void setRecoilYawScaleLMG(float) = 0; //+5B8 4BE810
    virtual float getRecoilPitchScaleLMG() = 0; //+5BC 4BE820 +388
    virtual void setRecoilPitchScaleLMG(float) = 0; //+5C0 4BE830
    virtual float getRecoilPunchScaleSNIPER() = 0; //+5C4 4BE840 +38C
    virtual void setRecoilPunchScaleSNIPER(float) = 0; //+5C8 4BE850
    virtual float getRecoilPunchTimeSNIPER() = 0; //+5CC 4BE860 +390
    virtual void setRecoilPunchTimeSNIPER(float) = 0; //+5D0 4BE870
    virtual float getRecoilYawScaleSNIPER() = 0; //+5D4 4BE880 +394
    virtual void setRecoilYawScaleSNIPER(float) = 0; //+5D8 4BE890
    virtual float getRecoilPitchScaleSNIPER() = 0; //+5DC 4BE8A0 +398
    virtual void setRecoilPitchScaleSNIPER(float) = 0; //+5E0 4BE8B0
    virtual float getRecoilPunchScalePISTOL() = 0; //+5E4 4BE8C0 +39C
    virtual void setRecoilPunchScalePISTOL(float) = 0; //+5E8 4BE8D0
    virtual float getRecoilPunchTimePISTOL() = 0; //+5EC 4BE8E0 +3A0
    virtual void setRecoilPunchTimePISTOL(float) = 0; //+5F0 4BE8F0
    virtual float getRecoilYawScalePISTOL() = 0; //+5F4 4BE900 +3A4
    virtual void setRecoilYawScalePISTOL(float) = 0; //+5F8 4BE910
    virtual float getRecoilPitchScalePISTOL() = 0; //+5FC 4BE920 +3A8
    virtual void setRecoilPitchScalePISTOL(float) = 0;
    virtual float getRecoilPunchScaleLAUNCHER() = 0; //+604 4BE940 +3AC
    virtual void setRecoilPunchScaleLAUNCHER(float) = 0;
    virtual float getRecoilPunchTimeLAUNCHER() = 0; //+60C 4BE960 +3B0
    virtual void setRecoilPunchTimeLAUNCHER(float) = 0;
    virtual float getRecoilYawScaleLAUNCHER() = 0; //+614 4BE980 +3B4
    virtual void setRecoilYawScaleLAUNCHER(float) = 0;
    virtual float getRecoilPitchScaleLAUNCHER() = 0; //+61C 4BE9A0 +3B8
    virtual void setRecoilPitchScaleLAUNCHER(float) = 0;
    virtual float getRecoilPunchScaleSMG() = 0; //+624 4BE9C0 +3BC
    virtual void setRecoilPunchScaleSMG(float) = 0;
    virtual float getRecoilPunchTimeSMG() = 0; //+62C 4BE9D0 +3C0
    virtual void setRecoilPunchTimeSMG(float) = 0;
    virtual float getRecoilYawScaleSMG() = 0; //+634 4BE9F0 +3C4
    virtual void setRecoilYawScaleSMG(float) = 0;
    virtual float getRecoilPitchScaleSMG() = 0; //+63C 4BEA10 +3C8
    virtual void setRecoilPitchScaleSMG(float) = 0;
    virtual float getRecoilPunchScaleSHOTGUN() = 0; //+644 4BEA30 +3CC
    virtual void setRecoilPunchScaleSHOTGUN(float) = 0;
    virtual float getRecoilPunchTimeSHOTGUN() = 0; //+64C 4BEA50 +3D0
    virtual void setRecoilPunchTimeSHOTGUN(float) = 0;
    virtual float getRecoilYawScaleSHOTGUN() = 0; //+654 4BEA70 +3D4
    virtual void setRecoilYawScaleSHOTGUN(float) = 0;
    virtual float getRecoilPitchScaleSHOTGUN() = 0; //+65C 4BEA90 +3D8
    virtual void setRecoilPitchScaleSHOTGUN(float) = 0;
    virtual float getRecoilPunchScaleKNIFE() = 0; //+664 4BEAB0 +3DC
    virtual void setRecoilPunchScaleKNIFE(float) = 0;
    virtual float getRecoilPunchTimeKNIFE() = 0; //+66C 4BEAD0 +3E0
    virtual void setRecoilPunchTimeKNIFE(float) = 0;
    virtual float getRecoilYawScaleKNIFE() = 0; //+674 4BEAF0 +3E4
    virtual void setRecoilYawScaleKNIFE(float) = 0;
    virtual float getRecoilPitchScaleKNIFE() = 0; //+67C 4BEB10 +3E8
    virtual void setRecoilPitchScaleKNIFE(float) = 0;
    virtual float getRecoilPunchScaleDEFIB() = 0; //+684 4BEB30 +3EC
    virtual void setRecoilPunchScaleDEFIB(float) = 0;
    virtual float getRecoilPunchTimeDEFIB() = 0; //+68C 4BEB50 +3F0
    virtual void setRecoilPunchTimeDEFIB(float) = 0;
    virtual float getRecoilYawScaleDEFIB() = 0; //+694 4BEB70 +3F4
    virtual void setRecoilYawScaleDEFIB(float) = 0;
    virtual float getRecoilPitchScaleDEFIB() = 0; //+69C 4BEB90 +3F8
    virtual void setRecoilPitchScaleDEFIB(float) = 0; //+6A0 4BEBA0
    virtual float getRecoilPunchScaleDROP() = 0; //+6A4 4BEBB0 +3FC
    virtual void setRecoilPunchScaleDROP(float) = 0; //+6A8 4BEBC0
    virtual float getRecoilPunchTimeDROP() = 0; //+6AC 4BEBD0 +400
    virtual void setRecoilPunchTimeDROP(float) = 0; //+6B0 4BEBE0
    virtual float getRecoilYawScaleDROP() = 0; //+6B4 4BEBF0 +404
    virtual void setRecoilYawScaleDROP(float) = 0; //+6B8 4BEC00
    virtual float getRecoilPitchScaleDROP() = 0; //+6BC 4BEC10 +408
    virtual void setRecoilPitchScaleDROP(float) = 0; //+6C0 4BEC20
    virtual float getRecoilPunchScaleTHROWN() = 0; //+6C4 4BEC30 +40C
    virtual void setRecoilPunchScaleTHROWN(float) = 0; //+6C8 4BEC40
    virtual float getRecoilPunchTimeTHROWN() = 0; //+6CC 4BEC50 +410
    virtual void setRecoilPunchTimeTHROWN(float) = 0; //+6D0 4BEC60
    virtual float getRecoilYawScaleTHROWN() = 0; //+6D4 4BEC70 +414
    virtual void setRecoilYawScaleTHROWN(float) = 0; //+6D8 4BEC80
    virtual float getRecoilPitchScaleTHROWN() = 0; //+6DC 4BEC90 +418
    virtual void setRecoilPitchScaleTHROWN(float) = 0; //+6E0 4BECA0
    virtual float getRecoilPunchScaleV_LMG() = 0; //+6E4 4BECB0 +41C
    virtual void setRecoilPunchScaleV_LMG(float) = 0; //+6E8 4BECC0
    virtual float getRecoilPunchTimeV_LMG() = 0; //+6EC 4BECD0 +420
    virtual void setRecoilPunchTimeV_LMG(float) = 0; //+6F0 4BECE0
    virtual float getRecoilYawScaleV_LMG() = 0; //+6F4 4BECF0 +424
    virtual void setRecoilYawScaleV_LMG(float) = 0; //+6F8 4BED00
    virtual float getRecoilPitchScaleV_LMG() = 0; //+6FC 4BED10 +428
    virtual void setRecoilPitchScaleV_LMG(float) = 0;
    virtual float getRecoilPunchScaleV_HMG() = 0; //+704 4BED30 +42C
    virtual void setRecoilPunchScaleV_HMG(float) = 0;
    virtual float getRecoilPunchTimeV_HMG() = 0; //+70C 4BED50 +430
    virtual void setRecoilPunchTimeV_HMG(float) = 0;
    virtual float getRecoilYawScaleV_HMG() = 0;
    virtual void setRecoilYawScaleV_HMG(float) = 0;
    virtual float getRecoilPitchScaleV_HMG() = 0; //+71C 4BED90 +438
    virtual void setRecoilPitchScaleV_HMG(float) = 0;
    virtual float getRecoilPunchScaleV_HELIGUN() = 0; //+724 4BEDB0 +43C
    virtual void setRecoilPunchScaleV_HELIGUN(float) = 0;
    virtual float getRecoilPunchTimeV_HELIGUN() = 0; //+72C 4BEDD0 +440
    virtual void setRecoilPunchTimeV_HELIGUN(float) = 0;
    virtual float getRecoilYawScaleV_HELIGUN() = 0;
    virtual void setRecoilYawScaleV_HELIGUN(float) = 0;
    virtual float getRecoilPitchScaleV_HELIGUN() = 0; //+73C 4BEE10 +448
    virtual void setRecoilPitchScaleV_HELIGUN(float) = 0;
    virtual float getRecoilPunchScaleV_AIRGUN() = 0; //+744 4BEE30 +44C
    virtual void setRecoilPunchScaleV_AIRGUN(float) = 0;
    virtual float getRecoilPunchTimeV_AIRGUN() = 0; //+74C 4BEE50 +450
    virtual void setRecoilPunchTimeV_AIRGUN(float) = 0;
    virtual float getRecoilYawScaleV_AIRGUN() = 0;
    virtual void setRecoilYawScaleV_AIRGUN(float) = 0;
    virtual float getRecoilPitchScaleV_AIRGUN() = 0; //+75C 4BEE90 +458
    virtual void setRecoilPitchScaleV_AIRGUN(float) = 0;
    virtual float getRecoilPunchScaleV_MISSILE() = 0; //+764 4BEEB0 +45C
    virtual void setRecoilPunchScaleV_MISSILE(float) = 0;
    virtual float getRecoilPunchTimeV_MISSILE() = 0; //+76C 4BEED0 +460
    virtual void setRecoilPunchTimeV_MISSILE(float) = 0;
    virtual float getRecoilYawScaleV_MISSILE() = 0;
    virtual void setRecoilYawScaleV_MISSILE(float) = 0;
    virtual float getRecoilPitchScaleV_MISSILE() = 0; //+77C 4BEF10 +468
    virtual void setRecoilPitchScaleV_MISSILE(float) = 0;
    virtual float getRecoilPunchScaleV_ARMORCANNON() = 0; //+784 4BEF30 +46C
    virtual void setRecoilPunchScaleV_ARMORCANNON(float) = 0;
    virtual float getRecoilPunchTimeV_ARMORCANNON() = 0; //+78C 4BEF50 +470
    virtual void setRecoilPunchTimeV_ARMORCANNON(float) = 0;
    virtual float getRecoilYawScaleV_ARMORCANNON() = 0;
    virtual void setRecoilYawScaleV_ARMORCANNON(float) = 0;
    virtual float getRecoilPitchScaleV_ARMORCANNON() = 0; //+79C 4BEF90 +478
    virtual void setRecoilPitchScaleV_ARMORCANNON(float) = 0; //+7A0 4BEFA0
    virtual float getRecoilPunchScaleV_COAXIALGUN() = 0; //+7A4 4BEFB0 +47C
    virtual void setRecoilPunchScaleV_COAXIALGUN(float) = 0; //+7A8 4BEFC0
    virtual float getRecoilPunchTimeV_COAXIALGUN() = 0; //+7AC 4BEFD0 +480
    virtual void setRecoilPunchTimeV_COAXIALGUN(float) = 0; //+7B0 4BEFE0
    virtual float getRecoilYawScaleV_COAXIALGUN() = 0; //+7B4 4BEFF0 +484
    virtual void setRecoilYawScaleV_COAXIALGUN(float) = 0; //+7B8 4BF000
    virtual float getRecoilPitchScaleV_COAXIALGUN() = 0; //+7BC 4BF010 +488
    virtual void setRecoilPitchScaleV_COAXIALGUN(float) = 0; //+7C0 6B7620
    virtual float getUnresolved48C() = 0; //+7C4 4BF020 +48C [INFERRED] name
    virtual void setUnresolved48C(float) = 0; //+7C8 6B75E0
    virtual float getUnresolved490() = 0; //+7CC 4BF030 +490 [INFERRED] name
    virtual void setUnresolved490(float) = 0; //+7D0 4BF040
    virtual float getGeneralShakeScale() = 0; //+7D4 4BF050 +494
    virtual void setGeneralShakeScale(float) = 0; //+7D8 4BF060
    virtual float getExplosionShakeScale() = 0; //+7DC 4BF070 +498
    virtual void setExplosionShakeScale(float) = 0; //+7E0 4BF080
    virtual float getSpeedShakeScale() = 0; //+7E4 4BF090 +49C
    virtual void setSpeedShakeScale(float) = 0; //+7E8 4BF0A0
    // //+7EC 4BD100 other (96 B)
    // //+7F0 4C1970 other (330 B)
    virtual int getWeaponClass(const std::string& name) = 0;                    //+7EC 4BD100: the row's class id, -1 when absent
    virtual void addWeaponClassMap(const std::string& name, int weaponClass) = 0; //+7F0 4C1970: lowercased; updates or appends
    // The deleting destructor at +7F4 (4C1AC0) is the only withheld slot.
};

struct HapticSettingsFixture;
class HapticSettings final : public IHapticSettings {
public:
    HapticSettings();                                       // retail 1200 bytes; ends in setDefaultValues through the table
    // [CHOSEN, not recovered] [RETIRE-WHEN: haptic-settings-prefix-publication]
    // Direct final-owner deletion; the deleting destructor lives at the withheld +7F4 (4C1AC0).
    ~HapticSettings();                                      // body
    static IBase* create(unsigned int iid, IBase* outer);
    void addRef() override;                                 // shared
    int getRef() override;                                  // shared
    int release() override;
    void* queryInterface(unsigned int iid) const override;
    std::string getFileName() override;
    std::string getSettingsScript() override;
    bool apply() override;
    void setDefaults() override;
    void setDefaultValues() override;
    void loadCurrentProfileFile() override;
    bool reloadCurrentProfileFile() override;
    float getForceCapX() override;
    void setForceCapX(float value) override;
    float getForceCapY() override;
    void setForceCapY(float value) override;
    float getForceCapZ() override;
    void setForceCapZ(float value) override;
    Vec3 getControlBoxLength() override;
    void setControlBoxLength(float x, float y, float z) override;
    Vec3 getControlBoxStiffness() override;
    void setControlBoxStiffness(float x, float y, float z) override;
    Vec3 getControlBoxLengthSOLDIER() override;
    void setControlBoxLengthSOLDIER(float x, float y, float z) override;
    Vec3 getControlBoxStiffnessSOLDIER() override;
    void setControlBoxStiffnessSOLDIER(float x, float y, float z) override;
    Vec3 getControlBoxLengthAIR() override;
    void setControlBoxLengthAIR(float x, float y, float z) override;
    Vec3 getControlBoxStiffnessAIR() override;
    void setControlBoxStiffnessAIR(float x, float y, float z) override;
    Vec3 getControlBoxLengthHELI() override;
    void setControlBoxLengthHELI(float x, float y, float z) override;
    Vec3 getControlBoxStiffnessHELI() override;
    void setControlBoxStiffnessHELI(float x, float y, float z) override;
    Vec3 getControlBoxLengthLAND() override;
    void setControlBoxLengthLAND(float x, float y, float z) override;
    Vec3 getControlBoxStiffnessLAND() override;
    void setControlBoxStiffnessLAND(float x, float y, float z) override;
    Vec3 getControlBoxLengthSEA() override;
    void setControlBoxLengthSEA(float x, float y, float z) override;
    Vec3 getControlBoxStiffnessSEA() override;
    void setControlBoxStiffnessSEA(float x, float y, float z) override;
    Vec3 getAimSensitivity() override;
    void setAimSensitivity(float x, float y, float z) override;
    Vec3 getAimSensitivityLength() override;
    void setAimSensitivityLength(float x, float y, float z) override;
    Vec3 getTurnSensitivity() override;
    void setTurnSensitivity(float x, float y, float z) override;
    Vec3 getAimSensitivitySOLDIER() override;
    void setAimSensitivitySOLDIER(float x, float y, float z) override;
    Vec3 getAimSensitivityLengthSOLDIER() override;
    void setAimSensitivityLengthSOLDIER(float x, float y, float z) override;
    Vec3 getTurnSensitivitySOLDIER() override;
    void setTurnSensitivitySOLDIER(float x, float y, float z) override;
    Vec3 getAimSensitivityAIR() override;
    void setAimSensitivityAIR(float x, float y, float z) override;
    Vec3 getAimSensitivityLengthAIR() override;
    void setAimSensitivityLengthAIR(float x, float y, float z) override;
    Vec3 getTurnSensitivityAIR() override;
    void setTurnSensitivityAIR(float x, float y, float z) override;
    Vec3 getAimSensitivityHELI() override;
    void setAimSensitivityHELI(float x, float y, float z) override;
    Vec3 getAimSensitivityLengthHELI() override;
    void setAimSensitivityLengthHELI(float x, float y, float z) override;
    Vec3 getTurnSensitivityHELI() override;
    void setTurnSensitivityHELI(float x, float y, float z) override;
    Vec3 getAimSensitivityLAND() override;
    void setAimSensitivityLAND(float x, float y, float z) override;
    Vec3 getAimSensitivityLengthLAND() override;
    void setAimSensitivityLengthLAND(float x, float y, float z) override;
    Vec3 getTurnSensitivityLAND() override;
    void setTurnSensitivityLAND(float x, float y, float z) override;
    Vec3 getAimSensitivitySEA() override;
    void setAimSensitivitySEA(float x, float y, float z) override;
    Vec3 getAimSensitivityLengthSEA() override;
    void setAimSensitivityLengthSEA(float x, float y, float z) override;
    Vec3 getTurnSensitivitySEA() override;
    void setTurnSensitivitySEA(float x, float y, float z) override;
    Vec3 getPhysicsSOLDIERScale() override;
    void setPhysicsSOLDIERScale(float x, float y, float z) override;
    float getPhysicsSOLDIERScaleXLow() override;
    void setPhysicsSOLDIERScaleXLow(float value) override;
    float getPhysicsSOLDIERScaleYLow() override;
    void setPhysicsSOLDIERScaleYLow(float value) override;
    float getPhysicsSOLDIERScaleZLow() override;
    void setPhysicsSOLDIERScaleZLow(float value) override;
    float getPhysicsSOLDIERScaleXMed() override;
    void setPhysicsSOLDIERScaleXMed(float value) override;
    float getPhysicsSOLDIERScaleYMed() override;
    void setPhysicsSOLDIERScaleYMed(float value) override;
    float getPhysicsSOLDIERScaleZMed() override;
    void setPhysicsSOLDIERScaleZMed(float value) override;
    float getPhysicsSOLDIERScaleXHigh() override;
    void setPhysicsSOLDIERScaleXHigh(float value) override;
    float getPhysicsSOLDIERScaleYHigh() override;
    void setPhysicsSOLDIERScaleYHigh(float value) override;
    float getPhysicsSOLDIERScaleZHigh() override;
    void setPhysicsSOLDIERScaleZHigh(float value) override;
    float getPhysicsSOLDIERTransitionValue(int level) override;
    void setPhysicsSOLDIERTransitionValue(int level, float value) override;
    float getPhysicsSOLDIERTransitionValueMed() override;
    void setPhysicsSOLDIERTransitionValueMed(float value) override;
    float getPhysicsSOLDIERTransitionValueHigh() override;
    void setPhysicsSOLDIERTransitionValueHigh(float value) override;
    int getPhysicsSOLDIERSmoothTimeX(int level) override;
    void setPhysicsSOLDIERSmoothTimeX(int level, int value) override;
    int getPhysicsSOLDIERSmoothTimeY(int level) override;
    void setPhysicsSOLDIERSmoothTimeY(int level, int value) override;
    int getPhysicsSOLDIERSmoothTimeZ(int level) override;
    void setPhysicsSOLDIERSmoothTimeZ(int level, int value) override;
    int getPhysicsSOLDIERSmoothTimeXLow() override;
    void setPhysicsSOLDIERSmoothTimeXLow(int value) override;
    int getPhysicsSOLDIERSmoothTimeYLow() override;
    void setPhysicsSOLDIERSmoothTimeYLow(int value) override;
    int getPhysicsSOLDIERSmoothTimeZLow() override;
    void setPhysicsSOLDIERSmoothTimeZLow(int value) override;
    int getPhysicsSOLDIERSmoothTimeXMed() override;
    void setPhysicsSOLDIERSmoothTimeXMed(int value) override;
    int getPhysicsSOLDIERSmoothTimeYMed() override;
    void setPhysicsSOLDIERSmoothTimeYMed(int value) override;
    int getPhysicsSOLDIERSmoothTimeZMed() override;
    void setPhysicsSOLDIERSmoothTimeZMed(int value) override;
    int getPhysicsSOLDIERSmoothTimeXHigh() override;
    void setPhysicsSOLDIERSmoothTimeXHigh(int value) override;
    int getPhysicsSOLDIERSmoothTimeYHigh() override;
    void setPhysicsSOLDIERSmoothTimeYHigh(int value) override;
    int getPhysicsSOLDIERSmoothTimeZHigh() override;
    void setPhysicsSOLDIERSmoothTimeZHigh(int value) override;
    Vec3 getPhysicsLANDScale() override;
    void setPhysicsLANDScale(float x, float y, float z) override;
    float getPhysicsLANDScaleXLow() override;
    void setPhysicsLANDScaleXLow(float value) override;
    float getPhysicsLANDScaleYLow() override;
    void setPhysicsLANDScaleYLow(float value) override;
    float getPhysicsLANDScaleZLow() override;
    void setPhysicsLANDScaleZLow(float value) override;
    float getPhysicsLANDScaleXMed() override;
    void setPhysicsLANDScaleXMed(float value) override;
    float getPhysicsLANDScaleYMed() override;
    void setPhysicsLANDScaleYMed(float value) override;
    float getPhysicsLANDScaleZMed() override;
    void setPhysicsLANDScaleZMed(float value) override;
    float getPhysicsLANDScaleXHigh() override;
    void setPhysicsLANDScaleXHigh(float value) override;
    float getPhysicsLANDScaleYHigh() override;
    void setPhysicsLANDScaleYHigh(float value) override;
    float getPhysicsLANDScaleZHigh() override;
    void setPhysicsLANDScaleZHigh(float value) override;
    float getPhysicsLANDTransitionValue(int level) override;
    void setPhysicsLANDTransitionValue(int level, float value) override;
    float getPhysicsLANDTransitionValueMed() override;
    void setPhysicsLANDTransitionValueMed(float value) override;
    float getPhysicsLANDTransitionValueHigh() override;
    void setPhysicsLANDTransitionValueHigh(float value) override;
    int getPhysicsLANDSmoothTimeX(int level) override;
    void setPhysicsLANDSmoothTimeX(int level, int value) override;
    int getPhysicsLANDSmoothTimeY(int level) override;
    void setPhysicsLANDSmoothTimeY(int level, int value) override;
    int getPhysicsLANDSmoothTimeZ(int level) override;
    void setPhysicsLANDSmoothTimeZ(int level, int value) override;
    int getPhysicsLANDSmoothTimeXLow() override;
    void setPhysicsLANDSmoothTimeXLow(int value) override;
    int getPhysicsLANDSmoothTimeYLow() override;
    void setPhysicsLANDSmoothTimeYLow(int value) override;
    int getPhysicsLANDSmoothTimeZLow() override;
    void setPhysicsLANDSmoothTimeZLow(int value) override;
    int getPhysicsLANDSmoothTimeXMed() override;
    void setPhysicsLANDSmoothTimeXMed(int value) override;
    int getPhysicsLANDSmoothTimeYMed() override;
    void setPhysicsLANDSmoothTimeYMed(int value) override;
    int getPhysicsLANDSmoothTimeZMed() override;
    void setPhysicsLANDSmoothTimeZMed(int value) override;
    int getPhysicsLANDSmoothTimeXHigh() override;
    void setPhysicsLANDSmoothTimeXHigh(int value) override;
    int getPhysicsLANDSmoothTimeYHigh() override;
    void setPhysicsLANDSmoothTimeYHigh(int value) override;
    int getPhysicsLANDSmoothTimeZHigh() override;
    void setPhysicsLANDSmoothTimeZHigh(int value) override;
    Vec3 getPhysicsAIRScale() override;
    void setPhysicsAIRScale(float x, float y, float z) override;
    float getPhysicsAIRScaleXLow() override;
    void setPhysicsAIRScaleXLow(float value) override;
    float getPhysicsAIRScaleYLow() override;
    void setPhysicsAIRScaleYLow(float value) override;
    float getPhysicsAIRScaleZLow() override;
    void setPhysicsAIRScaleZLow(float value) override;
    float getPhysicsAIRScaleXMed() override;
    void setPhysicsAIRScaleXMed(float value) override;
    float getPhysicsAIRScaleYMed() override;
    void setPhysicsAIRScaleYMed(float value) override;
    float getPhysicsAIRScaleZMed() override;
    void setPhysicsAIRScaleZMed(float value) override;
    float getPhysicsAIRScaleXHigh() override;
    void setPhysicsAIRScaleXHigh(float value) override;
    float getPhysicsAIRScaleYHigh() override;
    void setPhysicsAIRScaleYHigh(float value) override;
    float getPhysicsAIRScaleZHigh() override;
    void setPhysicsAIRScaleZHigh(float value) override;
    float getPhysicsAIRTransitionValue(int level) override;
    void setPhysicsAIRTransitionValue(int level, float value) override;
    float getPhysicsAIRTransitionValueMed() override;
    void setPhysicsAIRTransitionValueMed(float value) override;
    float getPhysicsAIRTransitionValueHigh() override;
    void setPhysicsAIRTransitionValueHigh(float value) override;
    int getPhysicsAIRSmoothTimeX(int level) override;
    void setPhysicsAIRSmoothTimeX(int level, int value) override;
    int getPhysicsAIRSmoothTimeY(int level) override;
    void setPhysicsAIRSmoothTimeY(int level, int value) override;
    int getPhysicsAIRSmoothTimeZ(int level) override;
    void setPhysicsAIRSmoothTimeZ(int level, int value) override;
    int getPhysicsAIRSmoothTimeXLow() override;
    void setPhysicsAIRSmoothTimeXLow(int value) override;
    int getPhysicsAIRSmoothTimeYLow() override;
    void setPhysicsAIRSmoothTimeYLow(int value) override;
    int getPhysicsAIRSmoothTimeZLow() override;
    void setPhysicsAIRSmoothTimeZLow(int value) override;
    int getPhysicsAIRSmoothTimeXMed() override;
    void setPhysicsAIRSmoothTimeXMed(int value) override;
    int getPhysicsAIRSmoothTimeYMed() override;
    void setPhysicsAIRSmoothTimeYMed(int value) override;
    int getPhysicsAIRSmoothTimeZMed() override;
    void setPhysicsAIRSmoothTimeZMed(int value) override;
    int getPhysicsAIRSmoothTimeXHigh() override;
    void setPhysicsAIRSmoothTimeXHigh(int value) override;
    int getPhysicsAIRSmoothTimeYHigh() override;
    void setPhysicsAIRSmoothTimeYHigh(int value) override;
    int getPhysicsAIRSmoothTimeZHigh() override;
    void setPhysicsAIRSmoothTimeZHigh(int value) override;
    Vec3 getPhysicsHELIScale() override;
    void setPhysicsHELIScale(float x, float y, float z) override;
    float getPhysicsHELIScaleXLow() override;
    void setPhysicsHELIScaleXLow(float value) override;
    float getPhysicsHELIScaleYLow() override;
    void setPhysicsHELIScaleYLow(float value) override;
    float getPhysicsHELIScaleZLow() override;
    void setPhysicsHELIScaleZLow(float value) override;
    float getPhysicsHELIScaleXMed() override;
    void setPhysicsHELIScaleXMed(float value) override;
    float getPhysicsHELIScaleYMed() override;
    void setPhysicsHELIScaleYMed(float value) override;
    float getPhysicsHELIScaleZMed() override;
    void setPhysicsHELIScaleZMed(float value) override;
    float getPhysicsHELIScaleXHigh() override;
    void setPhysicsHELIScaleXHigh(float value) override;
    float getPhysicsHELIScaleYHigh() override;
    void setPhysicsHELIScaleYHigh(float value) override;
    float getPhysicsHELIScaleZHigh() override;
    void setPhysicsHELIScaleZHigh(float value) override;
    float getPhysicsHELITransitionValue(int level) override;
    void setPhysicsHELITransitionValue(int level, float value) override;
    float getPhysicsHELITransitionValueMed() override;
    void setPhysicsHELITransitionValueMed(float value) override;
    float getPhysicsHELITransitionValueHigh() override;
    void setPhysicsHELITransitionValueHigh(float value) override;
    int getPhysicsHELISmoothTimeX(int level) override;
    void setPhysicsHELISmoothTimeX(int level, int value) override;
    int getPhysicsHELISmoothTimeY(int level) override;
    void setPhysicsHELISmoothTimeY(int level, int value) override;
    int getPhysicsHELISmoothTimeZ(int level) override;
    void setPhysicsHELISmoothTimeZ(int level, int value) override;
    int getPhysicsHELISmoothTimeXLow() override;
    void setPhysicsHELISmoothTimeXLow(int value) override;
    int getPhysicsHELISmoothTimeYLow() override;
    void setPhysicsHELISmoothTimeYLow(int value) override;
    int getPhysicsHELISmoothTimeZLow() override;
    void setPhysicsHELISmoothTimeZLow(int value) override;
    int getPhysicsHELISmoothTimeXMed() override;
    void setPhysicsHELISmoothTimeXMed(int value) override;
    int getPhysicsHELISmoothTimeYMed() override;
    void setPhysicsHELISmoothTimeYMed(int value) override;
    int getPhysicsHELISmoothTimeZMed() override;
    void setPhysicsHELISmoothTimeZMed(int value) override;
    int getPhysicsHELISmoothTimeXHigh() override;
    void setPhysicsHELISmoothTimeXHigh(int value) override;
    int getPhysicsHELISmoothTimeYHigh() override;
    void setPhysicsHELISmoothTimeYHigh(int value) override;
    int getPhysicsHELISmoothTimeZHigh() override;
    void setPhysicsHELISmoothTimeZHigh(int value) override;
    Vec3 getPhysicsSEAScale() override;
    void setPhysicsSEAScale(float x, float y, float z) override;
    float getPhysicsSEAScaleXLow() override;
    void setPhysicsSEAScaleXLow(float value) override;
    float getPhysicsSEAScaleYLow() override;
    void setPhysicsSEAScaleYLow(float value) override;
    float getPhysicsSEAScaleZLow() override;
    void setPhysicsSEAScaleZLow(float value) override;
    float getPhysicsSEAScaleXMed() override;
    void setPhysicsSEAScaleXMed(float value) override;
    float getPhysicsSEAScaleYMed() override;
    void setPhysicsSEAScaleYMed(float value) override;
    float getPhysicsSEAScaleZMed() override;
    void setPhysicsSEAScaleZMed(float value) override;
    float getPhysicsSEAScaleXHigh() override;
    void setPhysicsSEAScaleXHigh(float value) override;
    float getPhysicsSEAScaleYHigh() override;
    void setPhysicsSEAScaleYHigh(float value) override;
    float getPhysicsSEAScaleZHigh() override;
    void setPhysicsSEAScaleZHigh(float value) override;
    float getPhysicsSEATransitionValue(int level) override;
    void setPhysicsSEATransitionValue(int level, float value) override;
    float getPhysicsSEATransitionValueMed() override;
    void setPhysicsSEATransitionValueMed(float value) override;
    float getPhysicsSEATransitionValueHigh() override;
    void setPhysicsSEATransitionValueHigh(float value) override;
    int getPhysicsSEASmoothTimeX(int level) override;
    void setPhysicsSEASmoothTimeX(int level, int value) override;
    int getPhysicsSEASmoothTimeY(int level) override;
    void setPhysicsSEASmoothTimeY(int level, int value) override;
    int getPhysicsSEASmoothTimeZ(int level) override;
    void setPhysicsSEASmoothTimeZ(int level, int value) override;
    int getPhysicsSEASmoothTimeXLow() override;
    void setPhysicsSEASmoothTimeXLow(int value) override;
    int getPhysicsSEASmoothTimeYLow() override;
    void setPhysicsSEASmoothTimeYLow(int value) override;
    int getPhysicsSEASmoothTimeZLow() override;
    void setPhysicsSEASmoothTimeZLow(int value) override;
    int getPhysicsSEASmoothTimeXMed() override;
    void setPhysicsSEASmoothTimeXMed(int value) override;
    int getPhysicsSEASmoothTimeYMed() override;
    void setPhysicsSEASmoothTimeYMed(int value) override;
    int getPhysicsSEASmoothTimeZMed() override;
    void setPhysicsSEASmoothTimeZMed(int value) override;
    int getPhysicsSEASmoothTimeXHigh() override;
    void setPhysicsSEASmoothTimeXHigh(int value) override;
    int getPhysicsSEASmoothTimeYHigh() override;
    void setPhysicsSEASmoothTimeYHigh(int value) override;
    int getPhysicsSEASmoothTimeZHigh() override;
    void setPhysicsSEASmoothTimeZHigh(int value) override;
    float getDamageScale() override;
    void setDamageScale(float value) override;
    float getRecoilPunchScale(int weaponClass) override;
    void setRecoilPunchScale(int weaponClass, float value) override;
    float getRecoilPunchTime(int weaponClass) override;
    void setRecoilPunchTime(int weaponClass, float value) override;
    float getRecoilYawScale(int weaponClass) override;
    void setRecoilYawScale(int weaponClass, float value) override;
    float getRecoilPitchScale(int weaponClass) override;
    void setRecoilPitchScale(int weaponClass, float value) override;
    float getRecoilPunchScaleRIFLE() override;
    void setRecoilPunchScaleRIFLE(float value) override;
    float getRecoilPunchTimeRIFLE() override;
    void setRecoilPunchTimeRIFLE(float value) override;
    float getRecoilYawScaleRIFLE() override;
    void setRecoilYawScaleRIFLE(float value) override;
    float getRecoilPitchScaleRIFLE() override;
    void setRecoilPitchScaleRIFLE(float value) override;
    float getRecoilPunchScaleRIFLELAUNCHER() override;
    void setRecoilPunchScaleRIFLELAUNCHER(float value) override;
    float getRecoilPunchTimeRIFLELAUNCHER() override;
    void setRecoilPunchTimeRIFLELAUNCHER(float value) override;
    float getRecoilYawScaleRIFLELAUNCHER() override;
    void setRecoilYawScaleRIFLELAUNCHER(float value) override;
    float getRecoilPitchScaleRIFLELAUNCHER() override;
    void setRecoilPitchScaleRIFLELAUNCHER(float value) override;
    float getRecoilPunchScaleCARBINE() override;
    void setRecoilPunchScaleCARBINE(float value) override;
    float getRecoilPunchTimeCARBINE() override;
    void setRecoilPunchTimeCARBINE(float value) override;
    float getRecoilYawScaleCARBINE() override;
    void setRecoilYawScaleCARBINE(float value) override;
    float getRecoilPitchScaleCARBINE() override;
    void setRecoilPitchScaleCARBINE(float value) override;
    float getRecoilPunchScaleLMG() override;
    void setRecoilPunchScaleLMG(float value) override;
    float getRecoilPunchTimeLMG() override;
    void setRecoilPunchTimeLMG(float value) override;
    float getRecoilYawScaleLMG() override;
    void setRecoilYawScaleLMG(float value) override;
    float getRecoilPitchScaleLMG() override;
    void setRecoilPitchScaleLMG(float value) override;
    float getRecoilPunchScaleSNIPER() override;
    void setRecoilPunchScaleSNIPER(float value) override;
    float getRecoilPunchTimeSNIPER() override;
    void setRecoilPunchTimeSNIPER(float value) override;
    float getRecoilYawScaleSNIPER() override;
    void setRecoilYawScaleSNIPER(float value) override;
    float getRecoilPitchScaleSNIPER() override;
    void setRecoilPitchScaleSNIPER(float value) override;
    float getRecoilPunchScalePISTOL() override;
    void setRecoilPunchScalePISTOL(float value) override;
    float getRecoilPunchTimePISTOL() override;
    void setRecoilPunchTimePISTOL(float value) override;
    float getRecoilYawScalePISTOL() override;
    void setRecoilYawScalePISTOL(float value) override;
    float getRecoilPitchScalePISTOL() override;
    void setRecoilPitchScalePISTOL(float value) override;
    float getRecoilPunchScaleLAUNCHER() override;
    void setRecoilPunchScaleLAUNCHER(float value) override;
    float getRecoilPunchTimeLAUNCHER() override;
    void setRecoilPunchTimeLAUNCHER(float value) override;
    float getRecoilYawScaleLAUNCHER() override;
    void setRecoilYawScaleLAUNCHER(float value) override;
    float getRecoilPitchScaleLAUNCHER() override;
    void setRecoilPitchScaleLAUNCHER(float value) override;
    float getRecoilPunchScaleSMG() override;
    void setRecoilPunchScaleSMG(float value) override;
    float getRecoilPunchTimeSMG() override;
    void setRecoilPunchTimeSMG(float value) override;
    float getRecoilYawScaleSMG() override;
    void setRecoilYawScaleSMG(float value) override;
    float getRecoilPitchScaleSMG() override;
    void setRecoilPitchScaleSMG(float value) override;
    float getRecoilPunchScaleSHOTGUN() override;
    void setRecoilPunchScaleSHOTGUN(float value) override;
    float getRecoilPunchTimeSHOTGUN() override;
    void setRecoilPunchTimeSHOTGUN(float value) override;
    float getRecoilYawScaleSHOTGUN() override;
    void setRecoilYawScaleSHOTGUN(float value) override;
    float getRecoilPitchScaleSHOTGUN() override;
    void setRecoilPitchScaleSHOTGUN(float value) override;
    float getRecoilPunchScaleKNIFE() override;
    void setRecoilPunchScaleKNIFE(float value) override;
    float getRecoilPunchTimeKNIFE() override;
    void setRecoilPunchTimeKNIFE(float value) override;
    float getRecoilYawScaleKNIFE() override;
    void setRecoilYawScaleKNIFE(float value) override;
    float getRecoilPitchScaleKNIFE() override;
    void setRecoilPitchScaleKNIFE(float value) override;
    float getRecoilPunchScaleDEFIB() override;
    void setRecoilPunchScaleDEFIB(float value) override;
    float getRecoilPunchTimeDEFIB() override;
    void setRecoilPunchTimeDEFIB(float value) override;
    float getRecoilYawScaleDEFIB() override;
    void setRecoilYawScaleDEFIB(float value) override;
    float getRecoilPitchScaleDEFIB() override;
    void setRecoilPitchScaleDEFIB(float value) override;
    float getRecoilPunchScaleDROP() override;
    void setRecoilPunchScaleDROP(float value) override;
    float getRecoilPunchTimeDROP() override;
    void setRecoilPunchTimeDROP(float value) override;
    float getRecoilYawScaleDROP() override;
    void setRecoilYawScaleDROP(float value) override;
    float getRecoilPitchScaleDROP() override;
    void setRecoilPitchScaleDROP(float value) override;
    float getRecoilPunchScaleTHROWN() override;
    void setRecoilPunchScaleTHROWN(float value) override;
    float getRecoilPunchTimeTHROWN() override;
    void setRecoilPunchTimeTHROWN(float value) override;
    float getRecoilYawScaleTHROWN() override;
    void setRecoilYawScaleTHROWN(float value) override;
    float getRecoilPitchScaleTHROWN() override;
    void setRecoilPitchScaleTHROWN(float value) override;
    float getRecoilPunchScaleV_LMG() override;
    void setRecoilPunchScaleV_LMG(float value) override;
    float getRecoilPunchTimeV_LMG() override;
    void setRecoilPunchTimeV_LMG(float value) override;
    float getRecoilYawScaleV_LMG() override;
    void setRecoilYawScaleV_LMG(float value) override;
    float getRecoilPitchScaleV_LMG() override;
    void setRecoilPitchScaleV_LMG(float value) override;
    float getRecoilPunchScaleV_HMG() override;
    void setRecoilPunchScaleV_HMG(float value) override;
    float getRecoilPunchTimeV_HMG() override;
    void setRecoilPunchTimeV_HMG(float value) override;
    float getRecoilYawScaleV_HMG() override;
    void setRecoilYawScaleV_HMG(float value) override;
    float getRecoilPitchScaleV_HMG() override;
    void setRecoilPitchScaleV_HMG(float value) override;
    float getRecoilPunchScaleV_HELIGUN() override;
    void setRecoilPunchScaleV_HELIGUN(float value) override;
    float getRecoilPunchTimeV_HELIGUN() override;
    void setRecoilPunchTimeV_HELIGUN(float value) override;
    float getRecoilYawScaleV_HELIGUN() override;
    void setRecoilYawScaleV_HELIGUN(float value) override;
    float getRecoilPitchScaleV_HELIGUN() override;
    void setRecoilPitchScaleV_HELIGUN(float value) override;
    float getRecoilPunchScaleV_AIRGUN() override;
    void setRecoilPunchScaleV_AIRGUN(float value) override;
    float getRecoilPunchTimeV_AIRGUN() override;
    void setRecoilPunchTimeV_AIRGUN(float value) override;
    float getRecoilYawScaleV_AIRGUN() override;
    void setRecoilYawScaleV_AIRGUN(float value) override;
    float getRecoilPitchScaleV_AIRGUN() override;
    void setRecoilPitchScaleV_AIRGUN(float value) override;
    float getRecoilPunchScaleV_MISSILE() override;
    void setRecoilPunchScaleV_MISSILE(float value) override;
    float getRecoilPunchTimeV_MISSILE() override;
    void setRecoilPunchTimeV_MISSILE(float value) override;
    float getRecoilYawScaleV_MISSILE() override;
    void setRecoilYawScaleV_MISSILE(float value) override;
    float getRecoilPitchScaleV_MISSILE() override;
    void setRecoilPitchScaleV_MISSILE(float value) override;
    float getRecoilPunchScaleV_ARMORCANNON() override;
    void setRecoilPunchScaleV_ARMORCANNON(float value) override;
    float getRecoilPunchTimeV_ARMORCANNON() override;
    void setRecoilPunchTimeV_ARMORCANNON(float value) override;
    float getRecoilYawScaleV_ARMORCANNON() override;
    void setRecoilYawScaleV_ARMORCANNON(float value) override;
    float getRecoilPitchScaleV_ARMORCANNON() override;
    void setRecoilPitchScaleV_ARMORCANNON(float value) override;
    float getRecoilPunchScaleV_COAXIALGUN() override;
    void setRecoilPunchScaleV_COAXIALGUN(float value) override;
    float getRecoilPunchTimeV_COAXIALGUN() override;
    void setRecoilPunchTimeV_COAXIALGUN(float value) override;
    float getRecoilYawScaleV_COAXIALGUN() override;
    void setRecoilYawScaleV_COAXIALGUN(float value) override;
    float getRecoilPitchScaleV_COAXIALGUN() override;
    void setRecoilPitchScaleV_COAXIALGUN(float value) override;
    float getUnresolved48C() override;
    void setUnresolved48C(float value) override;
    float getUnresolved490() override;
    void setUnresolved490(float value) override;
    float getGeneralShakeScale() override;
    void setGeneralShakeScale(float value) override;
    float getExplosionShakeScale() override;
    void setExplosionShakeScale(float value) override;
    float getSpeedShakeScale() override;
    void setSpeedShakeScale(float value) override;
    int getWeaponClass(const std::string& name) override;
    void addWeaponClassMap(const std::string& name, int weaponClass) override;
private:
    friend struct HapticSettingsFixture;
    int m_refCount;                          // retail +04
    float m_forceCapX;                     // retail +8
    float m_forceCapY;                     // retail +C
    float m_forceCapZ;                     // retail +10
    Vec3 m_controlBoxLength;               // retail +14 [INFERRED] name, not in the script
    Vec3 m_controlBoxStiffness;            // retail +20 [INFERRED] name, not in the script
    Vec3 m_controlBoxLengthSOLDIER;        // retail +2C
    Vec3 m_controlBoxStiffnessSOLDIER;     // retail +38
    Vec3 m_controlBoxLengthAIR;            // retail +44
    Vec3 m_controlBoxStiffnessAIR;         // retail +50
    Vec3 m_controlBoxLengthHELI;           // retail +5C
    Vec3 m_controlBoxStiffnessHELI;        // retail +68
    Vec3 m_controlBoxLengthLAND;           // retail +74
    Vec3 m_controlBoxStiffnessLAND;        // retail +80
    Vec3 m_controlBoxLengthSEA;            // retail +8C
    Vec3 m_controlBoxStiffnessSEA;         // retail +98
    Vec3 m_aimSensitivity;                 // retail +A4 [INFERRED] name, not in the script
    Vec3 m_aimSensitivityLength;           // retail +B0 [INFERRED] name, not in the script
    Vec3 m_turnSensitivity;                // retail +BC [INFERRED] name, not in the script
    Vec3 m_aimSensitivitySOLDIER;          // retail +C8
    Vec3 m_aimSensitivityLengthSOLDIER;    // retail +D4
    Vec3 m_turnSensitivitySOLDIER;         // retail +E0
    Vec3 m_aimSensitivityAIR;              // retail +EC
    Vec3 m_aimSensitivityLengthAIR;        // retail +F8
    Vec3 m_turnSensitivityAIR;             // retail +104
    Vec3 m_aimSensitivityHELI;             // retail +110
    Vec3 m_aimSensitivityLengthHELI;       // retail +11C
    Vec3 m_turnSensitivityHELI;            // retail +128
    Vec3 m_aimSensitivityLAND;             // retail +134
    Vec3 m_aimSensitivityLengthLAND;       // retail +140
    Vec3 m_turnSensitivityLAND;            // retail +14C
    Vec3 m_aimSensitivitySEA;              // retail +158
    Vec3 m_aimSensitivityLengthSEA;        // retail +164
    Vec3 m_turnSensitivitySEA;             // retail +170
    Vec3 m_physicsSOLDIERScale;            // retail +17C
    float m_physicsSOLDIERScaleXLow;       // retail +188
    float m_physicsSOLDIERScaleYLow;       // retail +18C
    float m_physicsSOLDIERScaleZLow;       // retail +190
    float m_physicsSOLDIERScaleXMed;       // retail +194
    float m_physicsSOLDIERScaleYMed;       // retail +198
    float m_physicsSOLDIERScaleZMed;       // retail +19C
    float m_physicsSOLDIERScaleXHigh;      // retail +1A0
    float m_physicsSOLDIERScaleYHigh;      // retail +1A4
    float m_physicsSOLDIERScaleZHigh;      // retail +1A8
    float m_physicsSOLDIERTransitionValueMed;  // retail +1AC
    float m_physicsSOLDIERTransitionValueHigh;  // retail +1B0
    int m_physicsSOLDIERSmoothTimeXLow;    // retail +1B4
    int m_physicsSOLDIERSmoothTimeYLow;    // retail +1B8
    int m_physicsSOLDIERSmoothTimeZLow;    // retail +1BC
    int m_physicsSOLDIERSmoothTimeXMed;    // retail +1C0
    int m_physicsSOLDIERSmoothTimeYMed;    // retail +1C4
    int m_physicsSOLDIERSmoothTimeZMed;    // retail +1C8
    int m_physicsSOLDIERSmoothTimeXHigh;   // retail +1CC
    int m_physicsSOLDIERSmoothTimeYHigh;   // retail +1D0
    int m_physicsSOLDIERSmoothTimeZHigh;   // retail +1D4
    Vec3 m_physicsLANDScale;               // retail +1D8
    float m_physicsLANDScaleXLow;          // retail +1E4
    float m_physicsLANDScaleYLow;          // retail +1E8
    float m_physicsLANDScaleZLow;          // retail +1EC
    float m_physicsLANDScaleXMed;          // retail +1F0
    float m_physicsLANDScaleYMed;          // retail +1F4
    float m_physicsLANDScaleZMed;          // retail +1F8
    float m_physicsLANDScaleXHigh;         // retail +1FC
    float m_physicsLANDScaleYHigh;         // retail +200
    float m_physicsLANDScaleZHigh;         // retail +204
    float m_physicsLANDTransitionValueMed;  // retail +208
    float m_physicsLANDTransitionValueHigh;  // retail +20C
    int m_physicsLANDSmoothTimeXLow;       // retail +210
    int m_physicsLANDSmoothTimeYLow;       // retail +214
    int m_physicsLANDSmoothTimeZLow;       // retail +218
    int m_physicsLANDSmoothTimeXMed;       // retail +21C
    int m_physicsLANDSmoothTimeYMed;       // retail +220
    int m_physicsLANDSmoothTimeZMed;       // retail +224
    int m_physicsLANDSmoothTimeXHigh;      // retail +228
    int m_physicsLANDSmoothTimeYHigh;      // retail +22C
    int m_physicsLANDSmoothTimeZHigh;      // retail +230
    Vec3 m_physicsAIRScale;                // retail +234
    float m_physicsAIRScaleXLow;           // retail +240
    float m_physicsAIRScaleYLow;           // retail +244
    float m_physicsAIRScaleZLow;           // retail +248
    float m_physicsAIRScaleXMed;           // retail +24C
    float m_physicsAIRScaleYMed;           // retail +250
    float m_physicsAIRScaleZMed;           // retail +254
    float m_physicsAIRScaleXHigh;          // retail +258
    float m_physicsAIRScaleYHigh;          // retail +25C
    float m_physicsAIRScaleZHigh;          // retail +260
    float m_physicsAIRTransitionValueMed;  // retail +264
    float m_physicsAIRTransitionValueHigh;  // retail +268
    int m_physicsAIRSmoothTimeXLow;        // retail +26C
    int m_physicsAIRSmoothTimeYLow;        // retail +270
    int m_physicsAIRSmoothTimeZLow;        // retail +274
    int m_physicsAIRSmoothTimeXMed;        // retail +278
    int m_physicsAIRSmoothTimeYMed;        // retail +27C
    int m_physicsAIRSmoothTimeZMed;        // retail +280
    int m_physicsAIRSmoothTimeXHigh;       // retail +284
    int m_physicsAIRSmoothTimeYHigh;       // retail +288
    int m_physicsAIRSmoothTimeZHigh;       // retail +28C
    Vec3 m_physicsHELIScale;               // retail +290
    float m_physicsHELIScaleXLow;          // retail +29C
    float m_physicsHELIScaleYLow;          // retail +2A0
    float m_physicsHELIScaleZLow;          // retail +2A4
    float m_physicsHELIScaleXMed;          // retail +2A8
    float m_physicsHELIScaleYMed;          // retail +2AC
    float m_physicsHELIScaleZMed;          // retail +2B0
    float m_physicsHELIScaleXHigh;         // retail +2B4
    float m_physicsHELIScaleYHigh;         // retail +2B8
    float m_physicsHELIScaleZHigh;         // retail +2BC
    float m_physicsHELITransitionValueMed;  // retail +2C0
    float m_physicsHELITransitionValueHigh;  // retail +2C4
    int m_physicsHELISmoothTimeXLow;       // retail +2C8
    int m_physicsHELISmoothTimeYLow;       // retail +2CC
    int m_physicsHELISmoothTimeZLow;       // retail +2D0
    int m_physicsHELISmoothTimeXMed;       // retail +2D4
    int m_physicsHELISmoothTimeYMed;       // retail +2D8
    int m_physicsHELISmoothTimeZMed;       // retail +2DC
    int m_physicsHELISmoothTimeXHigh;      // retail +2E0
    int m_physicsHELISmoothTimeYHigh;      // retail +2E4
    int m_physicsHELISmoothTimeZHigh;      // retail +2E8
    Vec3 m_physicsSEAScale;                // retail +2EC
    float m_physicsSEAScaleXLow;           // retail +2F8
    float m_physicsSEAScaleYLow;           // retail +2FC
    float m_physicsSEAScaleZLow;           // retail +300
    float m_physicsSEAScaleXMed;           // retail +304
    float m_physicsSEAScaleYMed;           // retail +308
    float m_physicsSEAScaleZMed;           // retail +30C
    float m_physicsSEAScaleXHigh;          // retail +310
    float m_physicsSEAScaleYHigh;          // retail +314
    float m_physicsSEAScaleZHigh;          // retail +318
    float m_physicsSEATransitionValueMed;  // retail +31C
    float m_physicsSEATransitionValueHigh;  // retail +320
    int m_physicsSEASmoothTimeXLow;        // retail +324
    int m_physicsSEASmoothTimeYLow;        // retail +328
    int m_physicsSEASmoothTimeZLow;        // retail +32C
    int m_physicsSEASmoothTimeXMed;        // retail +330
    int m_physicsSEASmoothTimeYMed;        // retail +334
    int m_physicsSEASmoothTimeZMed;        // retail +338
    int m_physicsSEASmoothTimeXHigh;       // retail +33C
    int m_physicsSEASmoothTimeYHigh;       // retail +340
    int m_physicsSEASmoothTimeZHigh;       // retail +344
    float m_damageScale;                   // retail +348
    float m_recoilPunchScaleRIFLE;         // retail +34C
    float m_recoilPunchTimeRIFLE;          // retail +350
    float m_recoilYawScaleRIFLE;           // retail +354
    float m_recoilPitchScaleRIFLE;         // retail +358
    float m_recoilPunchScaleRIFLELAUNCHER;  // retail +35C
    float m_recoilPunchTimeRIFLELAUNCHER;  // retail +360
    float m_recoilYawScaleRIFLELAUNCHER;   // retail +364
    float m_recoilPitchScaleRIFLELAUNCHER;  // retail +368
    float m_recoilPunchScaleCARBINE;       // retail +36C
    float m_recoilPunchTimeCARBINE;        // retail +370
    float m_recoilYawScaleCARBINE;         // retail +374
    float m_recoilPitchScaleCARBINE;       // retail +378
    float m_recoilPunchScaleLMG;           // retail +37C
    float m_recoilPunchTimeLMG;            // retail +380
    float m_recoilYawScaleLMG;             // retail +384
    float m_recoilPitchScaleLMG;           // retail +388
    float m_recoilPunchScaleSNIPER;        // retail +38C
    float m_recoilPunchTimeSNIPER;         // retail +390
    float m_recoilYawScaleSNIPER;          // retail +394
    float m_recoilPitchScaleSNIPER;        // retail +398
    float m_recoilPunchScalePISTOL;        // retail +39C
    float m_recoilPunchTimePISTOL;         // retail +3A0
    float m_recoilYawScalePISTOL;          // retail +3A4
    float m_recoilPitchScalePISTOL;        // retail +3A8
    float m_recoilPunchScaleLAUNCHER;      // retail +3AC
    float m_recoilPunchTimeLAUNCHER;       // retail +3B0
    float m_recoilYawScaleLAUNCHER;        // retail +3B4
    float m_recoilPitchScaleLAUNCHER;      // retail +3B8
    float m_recoilPunchScaleSMG;           // retail +3BC
    float m_recoilPunchTimeSMG;            // retail +3C0
    float m_recoilYawScaleSMG;             // retail +3C4
    float m_recoilPitchScaleSMG;           // retail +3C8
    float m_recoilPunchScaleSHOTGUN;       // retail +3CC
    float m_recoilPunchTimeSHOTGUN;        // retail +3D0
    float m_recoilYawScaleSHOTGUN;         // retail +3D4
    float m_recoilPitchScaleSHOTGUN;       // retail +3D8
    float m_recoilPunchScaleKNIFE;         // retail +3DC
    float m_recoilPunchTimeKNIFE;          // retail +3E0
    float m_recoilYawScaleKNIFE;           // retail +3E4
    float m_recoilPitchScaleKNIFE;         // retail +3E8
    float m_recoilPunchScaleDEFIB;         // retail +3EC
    float m_recoilPunchTimeDEFIB;          // retail +3F0
    float m_recoilYawScaleDEFIB;           // retail +3F4
    float m_recoilPitchScaleDEFIB;         // retail +3F8
    float m_recoilPunchScaleDROP;          // retail +3FC
    float m_recoilPunchTimeDROP;           // retail +400
    float m_recoilYawScaleDROP;            // retail +404
    float m_recoilPitchScaleDROP;          // retail +408
    float m_recoilPunchScaleTHROWN;        // retail +40C
    float m_recoilPunchTimeTHROWN;         // retail +410
    float m_recoilYawScaleTHROWN;          // retail +414
    float m_recoilPitchScaleTHROWN;        // retail +418
    float m_recoilPunchScaleV_LMG;         // retail +41C
    float m_recoilPunchTimeV_LMG;          // retail +420
    float m_recoilYawScaleV_LMG;           // retail +424
    float m_recoilPitchScaleV_LMG;         // retail +428
    float m_recoilPunchScaleV_HMG;         // retail +42C
    float m_recoilPunchTimeV_HMG;          // retail +430
    float m_recoilYawScaleV_HMG;           // retail +434
    float m_recoilPitchScaleV_HMG;         // retail +438
    float m_recoilPunchScaleV_HELIGUN;     // retail +43C
    float m_recoilPunchTimeV_HELIGUN;      // retail +440
    float m_recoilYawScaleV_HELIGUN;       // retail +444
    float m_recoilPitchScaleV_HELIGUN;     // retail +448
    float m_recoilPunchScaleV_AIRGUN;      // retail +44C
    float m_recoilPunchTimeV_AIRGUN;       // retail +450
    float m_recoilYawScaleV_AIRGUN;        // retail +454
    float m_recoilPitchScaleV_AIRGUN;      // retail +458
    float m_recoilPunchScaleV_MISSILE;     // retail +45C
    float m_recoilPunchTimeV_MISSILE;      // retail +460
    float m_recoilYawScaleV_MISSILE;       // retail +464
    float m_recoilPitchScaleV_MISSILE;     // retail +468
    float m_recoilPunchScaleV_ARMORCANNON;  // retail +46C
    float m_recoilPunchTimeV_ARMORCANNON;  // retail +470
    float m_recoilYawScaleV_ARMORCANNON;   // retail +474
    float m_recoilPitchScaleV_ARMORCANNON;  // retail +478
    float m_recoilPunchScaleV_COAXIALGUN;  // retail +47C
    float m_recoilPunchTimeV_COAXIALGUN;   // retail +480
    float m_recoilYawScaleV_COAXIALGUN;    // retail +484
    float m_recoilPitchScaleV_COAXIALGUN;  // retail +488
    float m_unresolved48C;                 // retail +48C [INFERRED] name, not in the script
    float m_unresolved490;                 // retail +490 [INFERRED] name, not in the script
    float m_generalShakeScale;             // retail +494
    float m_explosionShakeScale;           // retail +498
    float m_speedShakeScale;               // retail +49C
    std::vector<WeaponClassEntry> m_weaponClassMap;   // retail +4A0, a 16-byte VC7.1 vector of 32-byte rows
};

} }
#endif
#endif
