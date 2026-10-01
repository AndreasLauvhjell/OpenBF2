// [INFERRED] path and member spellings. Client class identity is attested by
// factory410382 and vtable8A2DD8. No matching owner exists in the server ELF.
// Complete PE primary table through deleting destructor+60; no opaque slots.
#ifndef GAME_PROFILEMANAGER_H
#define GAME_PROFILEMANAGER_H
#include "../Core/IBase.h"
#include <string>
#include <vector>
#if !DEDICATED
namespace dice { namespace hfe {

struct OnlineProfileEntry {
    int profileId;               // retail+00
    std::string nick;            // retail+04
    std::string uniqueNick;      // retail+20
};

struct OnlineProfile {
    OnlineProfile(); // PE4A64C0 inlined construction
    // Native STL members, not a reproduction of the retail104-byte object.
    std::string email;           // retail+00
    std::string password;        // retail+1C
    std::vector<OnlineProfileEntry> profiles; // retail+38
    int profileId;               // retail+48
    std::string uniqueNick;      // retail+4C; populated by4A64C0, not the GP session ticket
};

struct ProfileInformation {
    // PE4A4920 maps GPGetInfoResponseArg into this native record.
    int profileId;               // retail+00
    std::string nick;            // retail+04
    std::string uniqueNick;      // retail+20
    std::string email;           // retail+3C
    std::string firstName;       // retail+58
    std::string lastName;        // retail+74
    std::string country;         // retail+90
    int sex;                    // retail+AC
    int birthYear;               // retail+B0
    int birthMonth;              // retail+B4
    int birthDay;                // retail+B8
};

class IProfileManager : public IBase {
public:
    virtual bool createOnlineAccount(const std::string& nick, const std::string& email,
        const std::string& password, const std::string& country) = 0; // +10 4A6630
    virtual bool login(const std::string& uniqueNick, const std::string& password) = 0; // +14 4A6BD0
    virtual bool loginAccount(const std::string& email, const std::string& password,
        const std::string& selectedUniqueNick) = 0; // +18 4A5F60
    virtual bool loginAlternate(const std::string& nick, const std::string& password) = 0; // +1C 4A50B0
    virtual bool createProfile(const std::string& uniqueNick, const std::string& country) = 0; // +20 4A6EB0
    virtual bool selectProfile(const std::string& name) = 0; // +24 4A4A70
    virtual void logout() = 0; // +28 4A65B0; caller-discard void spelling
    virtual OnlineProfile* getCurrentProfile() = 0; // +2C 68E7B0
    virtual bool getProfileId(int& output) = 0; // +30 4A4A20
    virtual bool getUniqueNick(std::string& output) = 0; // +34 4A4A40
    virtual bool deleteProfile() = 0; // +38 4A54B0
    virtual bool getProfileInformation(int profileId, ProfileInformation& output) = 0; // +3C 4A52C0
    virtual bool setProfileInformation(const ProfileInformation& value) = 0; // +40 4A4AD0
    virtual bool isLoggedIn() = 0; // +44 4A4B90
    virtual bool update() = 0; // +48 4A4BC0
    virtual bool requestNickSuggestions(const std::string& nick) = 0; // +4C 4A5BD0
    virtual bool getNickSuggestions(std::vector<std::string>& output) = 0; // +50 4A5C60
    virtual bool validateProfileName(const std::string& name, std::string& error) = 0; // +54 4A7D50
    virtual bool hasPendingTransition(std::string& output) = 0; // +58 4A4C80
    virtual void setPendingTransition(bool pending, const std::string& value) = 0; // +5C 4A4CB0
    virtual ~IProfileManager() = default; // +60 4A71A0
    // [INFERRED] Non-IBase method names, pointer/reference source spellings and
    // record field names; PE instructions prove argument widths and return forms.
};

struct ProfileManagerFixture;
class ProfileManager : public IProfileManager {
public:
    ProfileManager();
    ~ProfileManager() override;
    static IBase* create(unsigned int iid, IBase* outer);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int iid) const override;
    bool createOnlineAccount(const std::string&, const std::string&, const std::string&, const std::string&) override;
    bool login(const std::string&, const std::string&) override;
    bool loginAccount(const std::string&, const std::string&, const std::string&) override;
    bool loginAlternate(const std::string&, const std::string&) override;
    bool createProfile(const std::string&, const std::string&) override;
    bool selectProfile(const std::string&) override;
    void logout() override;
    OnlineProfile* getCurrentProfile() override;
    bool getProfileId(int&) override;
    bool getUniqueNick(std::string&) override;
    bool deleteProfile() override;
    bool getProfileInformation(int, ProfileInformation&) override;
    bool setProfileInformation(const ProfileInformation&) override;
    bool isLoggedIn() override;
    bool update() override;
    bool requestNickSuggestions(const std::string&) override;
    bool getNickSuggestions(std::vector<std::string>&) override;
    bool validateProfileName(const std::string&, std::string&) override;
    bool hasPendingTransition(std::string&) override;
    void setPendingTransition(bool, const std::string&) override;
private:
    friend struct ProfileManagerFixture;
    // The five helpers the login family shares; all [INFERRED] names, all non-virtual so the
    // published table is unchanged. PE bodies 4A4EF0, 4A4BE0, 4A64C0, 4A63A0 and 4A4DA0.
    void        reportError(const std::string& message, bool asLocalizationId);
    std::string lastErrorString();
    void        setCurrentProfile(const std::string& email, const std::string& password,
                                  int profileId, const std::string& uniqueNick);
    void        fetchProfileList(std::vector<OnlineProfileEntry>& entries);
    bool        createStatsPlayer(int profileId, const std::string& country, const std::string& nick);

    int m_refCount;                           // retail+04
    OnlineProfile* m_currentProfile;          // retail+08
    void* m_connection;                       // retail+0C, upstream GPConnection
    std::vector<std::string> m_nickSuggestions; // retail+10
    bool m_pendingTransition;                 // retail+20
    std::string m_pendingTransitionString;     // retail+24
};

} }
#endif
#endif
