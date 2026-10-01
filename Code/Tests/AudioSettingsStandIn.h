// [TEMP]
// Native test scaffolding: all IAudioSettings slots abort unless the fixture overrides them.
#ifndef STUBS_AUDIOSETTINGSSTANDIN_H
#define STUBS_AUDIOSETTINGSSTANDIN_H

#include "../BF2/Game/AudioSettings.h"
#include <cstdlib>
#include <string>
#include <vector>

struct AudioSettingsStandIn : dice::hfe::IAudioSettings {
    void addRef() override { std::abort(); }
    int getRef() override { std::abort(); }
    int release() override { std::abort(); }
    void* queryInterface(unsigned int) const override { std::abort(); }
    std::string getFileName() override { std::abort(); }
    std::string getSettingsScript() override { std::abort(); }
    bool apply(bool) override { std::abort(); }
    bool hasChanges() override { std::abort(); }
    void revertChanges() override { std::abort(); }
    void setDefaults() override { std::abort(); }
    bool needsRestart() override { std::abort(); }
    bool hasEax() override { std::abort(); }
    bool hasZenithBoard() override { std::abort(); }
    int getStatus() override { std::abort(); }
    bool getVoipEnabled() const override { std::abort(); }
    void setVoipEnabled(bool) override { std::abort(); }
    float getVoipPlaybackVolume() const override { std::abort(); }
    void setVoipPlaybackVolume(float) override { std::abort(); }
    float getVoipCaptureVolume() const override { std::abort(); }
    void setVoipCaptureVolume(float) override { std::abort(); }
    float getVoipCaptureThreshold() const override { std::abort(); }
    void setVoipCaptureThreshold(float) override { std::abort(); }
    bool getVoipBoostEnabled() const override { std::abort(); }
    void setVoipBoostEnabled(bool) override { std::abort(); }
    bool getVoipUsePushToTalk() const override { std::abort(); }
    void setVoipUsePushToTalk(bool) override { std::abort(); }
    std::string getProvider() const override { std::abort(); }
    void setProvider(std::string) override { std::abort(); }
    std::string getSoundQuality() const override { std::abort(); }
    void setSoundQuality(std::string) override { std::abort(); }
    float getEffectsVolume() const override { std::abort(); }
    void setEffectsVolume(float) override { std::abort(); }
    float getMusicVolume() const override { std::abort(); }
    void setMusicVolume(float) override { std::abort(); }
    float getHelpVoiceVolume() const override { std::abort(); }
    void setHelpVoiceVolume(float) override { std::abort(); }
    bool getEnglishOnlyVoices() const override { std::abort(); }
    void setEnglishOnlyVoices(bool) override { std::abort(); }
    bool getEnableEAX() const override { std::abort(); }
    void setEnableEAX(bool) override { std::abort(); }
    std::vector<std::string> getProviderNames(int*) override { std::abort(); }
    std::vector<std::string> getSoundQualityNames(int*) override { std::abort(); }
    bool getAlwaysAllowSystemOpenAL() const override { std::abort(); }
    void setAlwaysAllowSystemOpenAL(bool) override { std::abort(); }
    int getForceNumberOfVoices() const override { std::abort(); }
    void setForceNumberOfVoices(int) override { std::abort(); }
};

#endif
