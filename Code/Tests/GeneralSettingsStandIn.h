// [TEMP]
// Native test scaffolding for BF2_PLUGIN_ABI_NATIVE=1; every IGeneralSettings slot aborts unless overridden.
#ifndef STUBS_GENERALSETTINGSSTANDIN_H
#define STUBS_GENERALSETTINGSSTANDIN_H

#include "../BF2/Game/GeneralSettings.h"
#include <cstdlib>
#include <string>

struct GeneralSettingsStandIn : dice::hfe::IGeneralSettings {
    typedef dice::hfe::GeneralSettingsRgba Rgba;
    typedef dice::hfe::GeneralSettingsRgb Rgb;
    typedef dice::hfe::GeneralServerEntry Entry;
    void addRef() override { std::abort(); }
    int getRef() override { std::abort(); }
    int release() override { std::abort(); }
    void* queryInterface(unsigned int) const override { std::abort(); }
    std::string getFileName() override { std::abort(); }
    std::string getSettingsScript() override { std::abort(); }
    bool apply() override { std::abort(); }
    void setDefaults() override { std::abort(); }
    void setDefaultValues() override { std::abort(); }
    void cancel() override { std::abort(); }
    std::string getSortKey() const override { std::abort(); }
    void setSortKey(std::string) override { std::abort(); }
    int getSortOrder() const override { std::abort(); }
    void setSortOrder(int) override { std::abort(); }
    int getNumRoundsPlayed() const override { std::abort(); }
    void setNumRoundsPlayed(int) override { std::abort(); }
    std::string getServerFilter() const override { std::abort(); }
    void setServerFilter(std::string) override { std::abort(); }
    float getHUDTransparency() const override { std::abort(); }
    void setHUDTransparency(float) override { std::abort(); }
    Rgba getCrosshairColor() const override { std::abort(); }
    void setCrosshairColor(int, int, int, int) override { std::abort(); }
    Rgb getBuddytagColor() const override { std::abort(); }
    void setBuddytagColor(int, int, int) override { std::abort(); }
    Rgb getSquadtagColor() const override { std::abort(); }
    void setSquadtagColor(int, int, int) override { std::abort(); }
    bool getMinimapRotate() const override { std::abort(); }
    void setMinimapRotate(bool) override { std::abort(); }
    float getMinimapTransparency() const override { std::abort(); }
    void setMinimapTransparency(float) override { std::abort(); }
    bool getViewIntroMovie() const override { std::abort(); }
    void setViewIntroMovie(bool) override { std::abort(); }
    bool getOutOfVoting() const override { std::abort(); }
    void setOutOfVoting(bool) override { std::abort(); }
    std::string getBFTVSaveDirectory() const override { std::abort(); }
    void setBFTVSaveDirectory(std::string) override { std::abort(); }
    bool getConfirmQuit() const override { std::abort(); }
    void setConfirmQuit(bool) override { std::abort(); }
    float getMapIconAlphaTransparency() const override { std::abort(); }
    void setMapIconAlphaTransparency(float) override { std::abort(); }
    bool getUseAdvancedServerBrowser() const override { std::abort(); }
    void setUseAdvancedServerBrowser(bool) override { std::abort(); }
    bool getUseBots() const override { std::abort(); }
    void setUseBots(bool) override { std::abort(); }
    int getMaxBots() const override { std::abort(); }
    void setMaxBots(int) override { std::abort(); }
    bool getMaxBotsIncludeHumans() const override { std::abort(); }
    void setMaxBotsIncludeHumans(bool) override { std::abort(); }
    float getBotSkill() const override { std::abort(); }
    void setBotSkill(float) override { std::abort(); }
    int getLastAwardsCheckDate() const override { std::abort(); }
    void setLastAwardsCheckDate(int) override { std::abort(); }
    bool getAllowPunkBuster() const override { std::abort(); }
    void setAllowPunkBuster(bool) override { std::abort(); }
    bool getItemSelectionReverseItems() const override { std::abort(); }
    void setItemSelectionReverseItems(bool) override { std::abort(); }
    bool getAutoReload() const override { std::abort(); }
    void setAutoReload(bool) override { std::abort(); }
    float getCoopBotRatio() const override { std::abort(); }
    void setCoopBotRatio(float) override { std::abort(); }
    int getCoopBotCount() const override { std::abort(); }
    void setCoopBotCount(int) override { std::abort(); }
    bool getAutoReady() const override { std::abort(); }
    void setAutoReady(bool) override { std::abort(); }
    int getConnectionType() const override { std::abort(); }
    void setConnectionType(int) override { std::abort(); }
    void setToggleFilters(int) override { std::abort(); }
    bool getToggleFilter(int) const override { std::abort(); }
    void setToggleFilter(int, bool) override { std::abort(); }
    void setPlayedVOHelp(const std::string&) override { std::abort(); }
    bool checkPlayedVOHelp(const std::string&) const override { std::abort(); }
    void addFavouriteServer(const std::string&, int, const std::string&) override { std::abort(); }
    int getNumFavouriteServers() const override { std::abort(); }
    Entry* getFavouriteServer(unsigned int) override { std::abort(); }
    bool isFavouriteServer(const std::string&, int) override { std::abort(); }
    bool removeFavouriteServerEntry(unsigned int) override { std::abort(); }
    bool removeFavouriteServer(const std::string&, int) override { std::abort(); }
    Entry* findFavouriteServer(const std::string&, int) override { std::abort(); }
    void addServerHistory(const std::string&, int, const std::string&, int) override { std::abort(); }
    int getServerHistoryCount() const override { std::abort(); }
    Entry* getServerHistory(unsigned int) override { std::abort(); }
    void sortServerHistory() override { std::abort(); }
    bool removeServerHistoryEntry(unsigned int) override { std::abort(); }
    bool removeServerHistory(const std::string&, int) override { std::abort(); }
    Entry* findServerHistory(const std::string&, int) override { std::abort(); }
    void clearServerHistory() override { std::abort(); }
    void clearFavouriteServers() override { std::abort(); }
    bool getLCDDisplayModes() const override { std::abort(); }
    void setLCDDisplayModes(bool) override { std::abort(); }
};

#endif
