// [INFERRED] path and source names; ModuleGame attests dice.hfe.AudioSettings.
// Native storage and the complete retail interface.
#ifndef GAME_AUDIOSETTINGS_H
#define GAME_AUDIOSETTINGS_H
#include "../Core/IBase.h"
#include <iosfwd>
#include <string>
#include <vector>
#if !DEDICATED
namespace dice { namespace hfe {

// Names follow the actual AudioSettings console metadata and script literals.
struct AudioSettingsData {
    AudioSettingsData();
    void reset();
    void copyFrom(const AudioSettingsData&);    // observed member-copy order
    bool differsFrom(const AudioSettingsData&) const;
    // [INFERRED] Returns true if any restart-relevant field is equal, and false only
    // when all five differ. Preserve this original comparison behavior.
    bool anyRestartFieldEqual(const AudioSettingsData&) const;
    void write(std::ostream&) const;
    bool voipEnabled;                          // retail00
    float voipPlaybackVolume;                  //04
    float voipCaptureVolume;                   //08
    float voipCaptureThreshold;                //0C
    bool voipBoostEnabled;                     //10
    bool voipUsePushToTalk;                     //11
    std::string provider;                      //14
    std::string soundQuality;                  //30
    float effectsVolume;                       //4C
    float musicVolume;                         //50
    float helpVoiceVolume;                     //54
    bool englishOnlyVoices;                    //58
    bool enableEAX;                            //59
    bool alwaysAllowSystemOpenAL;              //5A
    int forceNumberOfVoices;                   //5C
};

struct AudioSettingsEnum {
    void add(const char* name, int value);
    int getValue(const char* name) const;        // 1 when the name is absent
    const char* getName(int value) const;        // null when the value is absent
    std::vector<std::string> names;
    std::vector<int> values;
};

// Audio-settings virtual interface used by the movie bindings. Preserve slot
// order; the trailing deleting-destructor slot remains withheld.
// [INFERRED] Names and constness.
class IAudioSettings : public IBase {
public:
    virtual std::string getFileName() = 0;
    virtual std::string getSettingsScript() = 0;
    virtual bool apply(bool reportErrors) = 0;  //+18 4B1750; helper tests the low byte
    virtual bool hasChanges() = 0;              //+1C 4B1140
    virtual void revertChanges() = 0;           //+20 4B1220: pending takes current ([INFERRED] name)
    virtual void setDefaults() = 0;             //+24 4B1180: status 0, pending reset; LocalProfileManager
                                                //   ::createDefaultProfile 4A8108 ([INFERRED] name)
    virtual bool needsRestart() = 0;            //+28 4B1150: in a game, with changes, 4B1090
    virtual bool hasEax() = 0;                  //+2C 4B1360: provider code 2 or 4
    virtual bool hasZenithBoard() = 0;          //+30 4B13C0: provider code 3 or 4
    virtual int  getStatus() = 0;               //+34 68E7B0 (ICF): the +0x08 status, 1 after a failed apply
    virtual bool  getVoipEnabled() const = 0;
    virtual void  setVoipEnabled(bool value) = 0;           //+3C 4B18F0
    virtual float getVoipPlaybackVolume() const = 0;
    virtual void  setVoipPlaybackVolume(float value) = 0;
    virtual float getVoipCaptureVolume() const = 0;
    virtual void  setVoipCaptureVolume(float value) = 0;    //+4C 4B1920
    virtual float getVoipCaptureThreshold() const = 0;
    virtual void  setVoipCaptureThreshold(float value) = 0;
    virtual bool  getVoipBoostEnabled() const = 0;
    virtual void  setVoipBoostEnabled(bool value) = 0;      //+5C 4B1940
    virtual bool  getVoipUsePushToTalk() const = 0;
    virtual void  setVoipUsePushToTalk(bool value) = 0;
    virtual std::string getProvider() const = 0;
    virtual void  setProvider(std::string value) = 0;       //+6C 4B1980, by value
    virtual std::string getSoundQuality() const = 0;
    virtual void  setSoundQuality(std::string value) = 0;   //+74 4B0B60, by value
    virtual float getEffectsVolume() const = 0;
    virtual void  setEffectsVolume(float value) = 0;        //+7C 4B19C0
    virtual float getMusicVolume() const = 0;
    virtual void  setMusicVolume(float value) = 0;
    virtual float getHelpVoiceVolume() const = 0;
    virtual void  setHelpVoiceVolume(float value) = 0;      //+8C 4B19E0
    virtual bool  getEnglishOnlyVoices() const = 0;
    virtual void  setEnglishOnlyVoices(bool value) = 0;
    virtual bool  getEnableEAX() const = 0;
    virtual void  setEnableEAX(bool value) = 0;             //+9C 4B1A00
    virtual std::vector<std::string> getProviderNames(int* count) = 0;      //+A0 4B1A60
    virtual std::vector<std::string> getSoundQualityNames(int* count) = 0;  //+A4 4B1AB0
    virtual bool  getAlwaysAllowSystemOpenAL() const = 0;   //+A8 4B1A10
    virtual void  setAlwaysAllowSystemOpenAL(bool value) = 0; //+AC 4B1A20
    virtual int   getForceNumberOfVoices() const = 0;       //+B0 504BE0
    virtual void  setForceNumberOfVoices(int value) = 0;    //+B4 4B1A30
    // +B8 is the deleting destructor (4B1C20), the one slot still withheld: see ~AudioSettings.
};

struct AudioSettingsFixture;
class AudioSettings final : public IAudioSettings {
public:
    AudioSettings();                            // retail268 bytes
    // [CHOSEN, not recovered] [RETIRE-WHEN: audio-settings-prefix-publication]
    // Direct final-owner deletion avoids inventing the absent suffix/destructor slot.
    ~AudioSettings();
    static IBase* create(unsigned int iid, IBase* outer);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int iid) const override;
    std::string getFileName() override;
    std::string getSettingsScript() override;
    bool apply(bool reportErrors) override;
    bool hasChanges() override;
    void revertChanges() override;
    void setDefaults() override;
    bool needsRestart() override;               //+28, out of line
    bool hasEax() override;                     //+2C, out of line
    bool hasZenithBoard() override;             //+30, out of line
    int  getStatus() override { return m_status; }   //+34: `mov eax, [ecx+8]`
    // Console/UI accessors edit the pending record; apply promotes it to current state.
    // [INFERRED] Names follow console properties. String setters take their value by copy.
    bool  getVoipEnabled() const override                { return m_pending.voipEnabled; }             // +0x38
    void  setVoipEnabled(bool value) override            { m_pending.voipEnabled = value; }            // +0x3C
    float getVoipPlaybackVolume() const override         { return m_pending.voipPlaybackVolume; }      // +0x40
    void  setVoipPlaybackVolume(float value) override    { m_pending.voipPlaybackVolume = value; }     // +0x44
    float getVoipCaptureVolume() const override          { return m_pending.voipCaptureVolume; }       // +0x48
    void  setVoipCaptureVolume(float value) override     { m_pending.voipCaptureVolume = value; }      // +0x4C
    float getVoipCaptureThreshold() const override       { return m_pending.voipCaptureThreshold; }    // +0x50
    void  setVoipCaptureThreshold(float value) override  { m_pending.voipCaptureThreshold = value; }   // +0x54
    bool  getVoipBoostEnabled() const override           { return m_pending.voipBoostEnabled; }        // +0x58
    void  setVoipBoostEnabled(bool value) override       { m_pending.voipBoostEnabled = value; }       // +0x5C
    bool  getVoipUsePushToTalk() const override          { return m_pending.voipUsePushToTalk; }       // +0x60
    void  setVoipUsePushToTalk(bool value) override      { m_pending.voipUsePushToTalk = value; }      // +0x64
    std::string getProvider() const override             { return m_pending.provider; }                // +0x68
    void  setProvider(std::string value) override        { m_pending.provider = value; }               // +0x6C
    std::string getSoundQuality() const override         { return m_pending.soundQuality; }            // +0x70
    void  setSoundQuality(std::string value) override    { m_pending.soundQuality = value; }           // +0x74
    float getEffectsVolume() const override              { return m_pending.effectsVolume; }           // +0x78
    void  setEffectsVolume(float value) override         { m_pending.effectsVolume = value; }          // +0x7C
    float getMusicVolume() const override                { return m_pending.musicVolume; }             // +0x80
    void  setMusicVolume(float value) override           { m_pending.musicVolume = value; }            // +0x84
    float getHelpVoiceVolume() const override            { return m_pending.helpVoiceVolume; }         // +0x88
    void  setHelpVoiceVolume(float value) override       { m_pending.helpVoiceVolume = value; }        // +0x8C
    bool  getEnglishOnlyVoices() const override          { return m_pending.englishOnlyVoices; }       // +0x90
    void  setEnglishOnlyVoices(bool value) override      { m_pending.englishOnlyVoices = value; }      // +0x94
    bool  getEnableEAX() const override                  { return m_pending.enableEAX; }               // +0x98
    void  setEnableEAX(bool value) override              { m_pending.enableEAX = value; }              // +0x9C
    std::vector<std::string> getProviderNames(int* count) override;       //+A0, out of line
    std::vector<std::string> getSoundQualityNames(int* count) override;   //+A4, out of line
    bool  getAlwaysAllowSystemOpenAL() const override    { return m_pending.alwaysAllowSystemOpenAL; } // +0xA8
    void  setAlwaysAllowSystemOpenAL(bool value) override{ m_pending.alwaysAllowSystemOpenAL = value; }// +0xAC
    int   getForceNumberOfVoices() const override        { return m_pending.forceNumberOfVoices; }     // +0xB0
    void  setForceNumberOfVoices(int value) override     { m_pending.forceNumberOfVoices = value; }    // +0xB4
private:
    friend struct AudioSettingsFixture;
    bool applyRecord(AudioSettingsData& data, bool reportErrors); // recovered
    void reportError(const std::string& message); // the ICF-shared SwiffHost message body
    int m_refCount;                             //retail04
    int m_status;                               //08
    AudioSettingsEnum m_providers;              //0C
    AudioSettingsEnum m_qualities;              //2C
    AudioSettingsData m_current;                //4C
    AudioSettingsData m_pending;                //AC
};

} }
#endif
#endif
