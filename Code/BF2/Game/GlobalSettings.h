// Default profile selection and the clan tag prepended to player names.
#ifndef GAME_GLOBALSETTINGS_H
#define GAME_GLOBALSETTINGS_H
#include "../Core/IBase.h"
#include <string>
#if !DEDICATED
namespace dice { namespace hfe {

// String getters return copies and setters take values.
// [INFERRED] apply and setDefaults names follow their callers. Final release deletes through the concrete owner.
class IGlobalSettings : public IBase {
public:
    virtual std::string getFileName() = 0;                  //+10 4B0900 "Global.con"
    virtual std::string getSettingsScript() = 0;            //+14 4B0920: the two rows
    virtual bool apply() = 0;                               //+18 5A5E20: true; LocalProfileManager::applySettings 4A7810
    virtual void setDefaults() = 0;                         //+1C 4B09F0: default user ""; loadCurrentProfile 4AB430
    virtual std::string getDefaultUser() = 0;               //+20 4B71E0 (ICF-shared with GameServerSettings' server-name getter)
    virtual void setDefaultUser(std::string value) = 0;     //+24 5D2000 (ICF-shared with its setter)
    virtual std::string getNamePrefix() = 0;                //+28 4B0A00: at most the first six characters
    virtual void setNamePrefix(std::string value) = 0;      //+2C 4B0A80: refused when the value contains '\xA7'
};

class GlobalSettings final : public IGlobalSettings {
public:
    GlobalSettings();                                       // retail 64 bytes
    ~GlobalSettings();                                      // body; the +30 slot itself is withheld
    static IBase* create(unsigned int iid, IBase* outer);
    void addRef() override;                                 // shared
    int getRef() override;                                  // shared
    int release() override;                                 // shared
    void* queryInterface(unsigned int iid) const override;
    std::string getFileName() override;
    std::string getSettingsScript() override;
    bool apply() override;
    void setDefaults() override;
    std::string getDefaultUser() override;
    void setDefaultUser(std::string value) override;
    std::string getNamePrefix() override;
    void setNamePrefix(std::string value) override;

private:
    friend struct GlobalSettingsFixture;
    int m_refCount;                                         // retail +04
    std::string m_defaultUser;                              // retail +08
    std::string m_namePrefix;                               // retail +24
};

} }
#endif
#endif
