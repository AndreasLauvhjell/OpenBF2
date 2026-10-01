// [INFERRED] path and non-IBase method names. Factory410382, table8A2DD8,
// complete lifecycle/leaf instructions and signatures read. No owner in server ELF.
#include "ProfileManager.h"
#if !DEDICATED
#include "../External/GameSpy/gp/gp.h"
#include "../External/GameSpy/common/gsAvailable.h"     // login's blocking availability check
#include "../External/GameSpy/common/gsPlatformUtil.h"  // msleep
#include "../Core/Ids.h"
#include "../Core/ClassManager.h"
#include "../Menu/ISwiffHost.h"
#include "LocalProfileManager.h"                        // the GP_ERROR callback's +0x44 probe
#include "Persistence/AntiHacks.h"                      // the identity setCurrentProfile stamps
#include "Persistence/HTTPRequest.h"                    // createStatsPlayer's createplayer.aspx post
#include "../Core/StringUtil.h"                         // formatString, PE 4310D0
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

namespace dice { namespace hfe {
extern ClassManager* classManager;

OnlineProfile::OnlineProfile() : profileId(0) {}

ProfileManager::ProfileManager()
    : m_refCount(1), m_currentProfile(nullptr), m_connection(nullptr),
      m_pendingTransition(false)
{
    // PE4A5A70: native vector/string default construction. No GP initialization.
}

ProfileManager::~ProfileManager()
{
    // PE4A71A0: record first, then string24 and vector10. Deliberately does not
    // call logout, gpDisconnect or gpDestroy; those belong to explicit logout.
    if (m_currentProfile) {
        delete m_currentProfile; // record4A5F00, entries4A5480: native reverse member destruction
        m_currentProfile = nullptr;
    }
}

IBase* ProfileManager::create(unsigned int iid, IBase* outer)
{
    if (outer) return nullptr;
    void* allocation = ::operator new(sizeof(ProfileManager));
    ProfileManager* manager = allocation ? new (allocation) ProfileManager : nullptr;
    // PE4103A7 has no allocation-null guard before the virtual query.
    IBase* result = static_cast<IBase*>(manager->queryInterface(iid));
    if (!result) manager->release();
    return result;
}

void ProfileManager::addRef() { ++m_refCount; }
int ProfileManager::getRef() { return m_refCount; }
int ProfileManager::release()
{
    const int references = --m_refCount;
    if (!references) delete this; // PE4A5AE0 dispatches deleting destructor+60
    return references;
}
void* ProfileManager::queryInterface(unsigned int iid) const
{
    // PE4A5AB0 reads initialized constants90D358=1 and892C1C=121016.
    if (iid == IID_IBase || iid == IID_IProfileManager)
        return const_cast<ProfileManager*>(this);
    return nullptr;
}

bool ProfileManager::selectProfile(const std::string& name)
{
    if (!m_currentProfile) return false; // PE4A4A70
    const std::string password = m_currentProfile->password;
    return login(name, password); // virtual+14; temporary survives the call
}

void ProfileManager::logout()
{
    // PE4A65B0: destroy current record before GP disconnect/destroy, then look
    // up the host and set its selected player id without a host-null guard.
    if (m_currentProfile) {
        delete m_currentProfile;
        m_currentProfile = nullptr;
    }
    if (m_connection) {
        gpDisconnect(&m_connection);
        gpDestroy(&m_connection);
    }
    IBase* host = classManager->getSingleton(std::string("SwiffHost"));
    static_cast<ui::ISwiffHost*>(host)->setSelectedPlayerId(0);
}

OnlineProfile* ProfileManager::getCurrentProfile() { return m_currentProfile; }

bool ProfileManager::getProfileId(int& output)
{
    if (!m_currentProfile) return false; // PE4A4A20 leaves output untouched
    output = m_currentProfile->profileId;
    return true;
}

bool ProfileManager::getUniqueNick(std::string& output)
{
    if (!m_currentProfile) return false; // PE4A4A40 leaves output untouched
    output = m_currentProfile->uniqueNick;
    return true;
}

bool ProfileManager::isLoggedIn()
{
    if (!m_currentProfile) return false; // PE4A4B90 guards the record before GP
    GPEnum connected;
    if (gpIsConnected(&m_connection, &connected) != GP_NO_ERROR) return false;
    return connected != 0;
}

bool ProfileManager::update()
{
    if (!isLoggedIn()) return false; // PE4A4BC0 retains the virtual+44 call
    return gpProcess(&m_connection) == GP_NO_ERROR;
}

bool ProfileManager::getNickSuggestions(std::vector<std::string>& output)
{
    output = m_nickSuggestions; // PE4A5C60, vector<string> assignment47F260
    return true;
}

bool ProfileManager::validateProfileName(const std::string& name, std::string& error)
{
    // PE4A7D50, shared with LocalProfileManager+BC.
    IBase* host = classManager->getSingleton(std::string("SwiffHost"));
    if (!host) return false;
    return static_cast<ui::ISwiffHost*>(host)->validateProfileName(name, error);
}

bool ProfileManager::hasPendingTransition(std::string& output)
{
    if (!m_pendingTransition) return false; // PE4A4C80 leaves output untouched
    output = m_pendingTransitionString;
    return true; // The flag is not cleared by a successful read.
}

void ProfileManager::setPendingTransition(bool pending, const std::string& value)
{
    m_pendingTransition = pending; // PE4A4CB0 stores the flag before string assignment
    m_pendingTransitionString = value;
}


// GameSpy presence configuration. [PLATFORM] The vendored SDK adds a partner-id
// argument; GP_PARTNERID_GAMESPY preserves the original account system.
static const int  kGameSpyProductId  = 10493;
static const int  kGameSpyNamespaceId = 12;
static const char kGameSpyGameName[] = "battlefield2";

namespace {

// Marks a pending transition for a logged-in profile on fatal or disconnected presence errors.
// The SwiffHost lookup remains observable; LocalProfileManager is required.
void profileManagerGPError(GPConnection* /*connection*/, void* arg, void* param)
{
    const GPErrorArg* error = static_cast<const GPErrorArg*>(arg);
    if (error->result == GP_NO_ERROR)
        return;
    (void) dice::hfe::classManager->getSingleton(std::string("SwiffHost"));   // looked up, discarded
    dice::hfe::IBase* local = dice::hfe::classManager->getSingleton(std::string("LocalProfileManager"));
    if (!static_cast<dice::hfe::ILocalProfileManager*>(local)->getCurrentProfile())   // +0x44
        return;
    const bool connectionLost = error->errorCode == GP_NETWORK              // 5
                             || error->errorCode == GP_FORCED_DISCONNECT    // 6
                             || error->errorCode == GP_CONNECTION_CLOSED;   // 7
    if (error->fatal == 1 || connectionLost)
        static_cast<dice::hfe::ProfileManager*>(param)->setPendingTransition(
            true, std::string(error->errorString));                          // virtual +0x5C
}

// the response callback shared by gpConnectUniqueNick and gpCheckUser, whose argument
// records have the same {result, profile} prefix. `param` is an int: the profile id, or zero when
// the request failed.
void profileManagerGPProfileId(GPConnection* /*connection*/, void* arg, void* param)
{
    const GPConnectResponseArg* response = static_cast<const GPConnectResponseArg*>(arg);
    if (response->result != GP_NO_ERROR)
        *static_cast<int*>(param) = 0;
    else
        *static_cast<int*>(param) = response->profile;
}

// the gpGetInfo callback. `param` is a std::string that receives the account's E-mail
// (GPGetInfoResponseArg +0x3C, which is where nick[31] and uniquenick[21] place it), or the empty
// string when the request failed.
void profileManagerGPEmail(GPConnection* /*connection*/, void* arg, void* param)
{
    const GPGetInfoResponseArg* response = static_cast<const GPGetInfoResponseArg*>(arg);
    if (response->result != GP_NO_ERROR)
        *static_cast<std::string*>(param) = "";
    else
        *static_cast<std::string*>(param) = response->email;
}

// the gpGetInfo callback getProfileInformation installs. `param` is the caller's
// ProfileInformation: the profile id, the six strings, the sex and the three birthday parts are
// copied out of the SDK record; nothing is checked, the result included.
void profileManagerGPProfileInfo(GPConnection* /*connection*/, void* arg, void* param)
{
    const GPGetInfoResponseArg* response = static_cast<const GPGetInfoResponseArg*>(arg);
    dice::hfe::ProfileInformation* info = static_cast<dice::hfe::ProfileInformation*>(param);
    info->profileId  = response->profile;
    info->nick       = response->nick;
    info->uniqueNick = response->uniquenick;
    info->email      = response->email;
    info->firstName  = response->firstname;
    info->lastName   = response->lastname;
    info->country    = response->countrycode;
    info->sex        = response->sex;
    info->birthDay   = response->birthday;
    info->birthMonth = response->birthmonth;
    info->birthYear  = response->birthyear;
}

// the gpGetUserNicks callback. `param` is the caller's vector, which is freed rather
// than cleared (the shipped body destroys the elements and releases the buffer), then refilled with
// one entry per returned nick. An entry whose uniquenick is empty is skipped, and every entry's
// profile id starts at zero: the caller resolves them one at a time afterwards.
void profileManagerGPUserNicks(GPConnection* /*connection*/, void* arg, void* param)
{
    const GPGetUserNicksResponseArg* response = static_cast<const GPGetUserNicksResponseArg*>(arg);
    std::vector<dice::hfe::OnlineProfileEntry>* entries =
        static_cast<std::vector<dice::hfe::OnlineProfileEntry>*>(param);
    std::vector<dice::hfe::OnlineProfileEntry>().swap(*entries);
    if (response->result != GP_NO_ERROR || response->numNicks <= 0)
        return;
    for (int i = 0; i < response->numNicks; ++i)
    {
        dice::hfe::OnlineProfileEntry entry;
        entry.profileId  = 0;
        entry.nick       = response->nicks[i];
        entry.uniqueNick = response->uniquenicks[i];
        if (entry.uniqueNick != "")
            entries->push_back(entry);
    }
}

// the gpGetUserNicks callback createOnlineAccount pairs with the one above: the same
// body with the opposite filter. It keeps the nicks whose unique nick is empty - the account's
// pre-unique-nick GameSpy profiles - which is how the shipped body tells "this e-mail already owns
// a registered account" from "this e-mail owns an old profile the requested nick can take over".
void profileManagerGPLegacyNicks(GPConnection* /*connection*/, void* arg, void* param)
{
    const GPGetUserNicksResponseArg* response = static_cast<const GPGetUserNicksResponseArg*>(arg);
    std::vector<dice::hfe::OnlineProfileEntry>* entries =
        static_cast<std::vector<dice::hfe::OnlineProfileEntry>*>(param);
    std::vector<dice::hfe::OnlineProfileEntry>().swap(*entries);
    if (response->result != GP_NO_ERROR || response->numNicks <= 0)
        return;
    for (int i = 0; i < response->numNicks; ++i)
    {
        dice::hfe::OnlineProfileEntry entry;
        entry.profileId  = 0;
        entry.nick       = response->nicks[i];
        entry.uniqueNick = response->uniquenicks[i];
        if (entry.uniqueNick == "")
            entries->push_back(entry);
    }
}

//  (nullsub_2) - the callback createOnlineAccount hands gpRegisterUniqueNick. It does
// nothing; the SDK merely refuses a null one.
void profileManagerGPIgnore(GPConnection* /*connection*/, void* /*arg*/, void* /*param*/)
{
}

} // namespace

// the connection's own error text, or "general error" when it is empty.
// The 256-byte buffer is NOT initialised before the call and gpGetErrorString leaves it untouched
// when the connection is null, so the `buffer[0]` test reads indeterminate bytes on that path. That
// is the shipped body; every caller here reaches it with an initialised connection.
std::string ProfileManager::lastErrorString()
{
    char buffer[GP_ERROR_STRING_LEN];
    gpGetErrorString(&m_connection, buffer);
    if (!buffer[0])
    {
        std::memset(buffer, 0, sizeof(buffer));
        std::strcpy(buffer, "general error");
    }
    return std::string(buffer);
}

// push one message at the front end. `this` is unread by the shipped body.
// With asLocalizationId the message is posted verbatim as a type-3 (localisation id) message. Without
// it the message is an SDK error string, and three of them are translated to localisation ids;
// anything else is posted as a type-1 message, which is the front end's "show this text" kind.
void ProfileManager::reportError(const std::string& message, bool asLocalizationId)
{
    IBase* host = classManager->getSingleton(std::string("SwiffHost"));
    if (!host)
        return;
    ui::ISwiffHost* swiffHost = static_cast<ui::ISwiffHost*>(host);
    if (asLocalizationId)
    {
        swiffHost->postMessage(SwiffHostMessage(3, message, 3));     // +0x1E4
        return;
    }
    if (message == "Could not resolve connection manager host name.")
        swiffHost->postMessage(SwiffHostMessage(3, std::string("connectionfailed"), 3));
    else if (message == "Could not resolve search manager host name.")
        swiffHost->postMessage(SwiffHostMessage(3, std::string("connectionfailed"), 3));
    else if (message == "Invalid e-mail.")
        swiffHost->postMessage(SwiffHostMessage(3, std::string("noemail"), 3));
    else
        swiffHost->postMessage(SwiffHostMessage(1, message, 3));
}

// Replaces the account record and updates the anti-cheat nickname hash and available login ticket.
// Deletes the old record before allocation and does not handle allocation failure.
void ProfileManager::setCurrentProfile(const std::string& email, const std::string& password,
                                       int profileId, const std::string& uniqueNick)
{
    if (m_currentProfile)
        delete m_currentProfile;                 // record 4A5F00, entries 4A5480, free 402D10
    void* allocation = ::operator new(sizeof(OnlineProfile));
    m_currentProfile = allocation ? new (allocation) OnlineProfile : nullptr;
    m_currentProfile->email      = email;
    m_currentProfile->password   = password;
    m_currentProfile->profileId  = profileId;
    m_currentProfile->uniqueNick = uniqueNick;

    AntiHacks::instance()->addPlayerName(uniqueNick);
    char ticket[GP_LOGIN_TICKET_LEN];
    if (gpGetLoginTicket(&m_connection, ticket) == GP_NO_ERROR)
        AntiHacks::instance()->addGSAuth(std::string(ticket));
}

// Fetches account nicknames, then resolves each profile ID synchronously.
// Callbacks may reallocate entries; SDK failure leaves the successfully resolved prefix in place.
void ProfileManager::fetchProfileList(std::vector<OnlineProfileEntry>& entries)
{
    if (!m_currentProfile)
        return;
    if (gpGetUserNicks(&m_connection, m_currentProfile->email.c_str(),
                       m_currentProfile->password.c_str(), GP_BLOCKING,
                       &profileManagerGPUserNicks, &entries) != GP_NO_ERROR)
    {
        reportError(lastErrorString(), false);
        return;
    }
    for (std::size_t index = 0; !entries.empty() && index < entries.size(); ++index)
    {
        int profileId = 0;
        if (gpCheckUser(&m_connection, entries[index].nick.c_str(),
                        m_currentProfile->email.c_str(), m_currentProfile->password.c_str(),
                        GP_BLOCKING, &profileManagerGPProfileId, &profileId) != GP_NO_ERROR)
        {
            reportError(lastErrorString(), false);
            return;
        }
        entries[index].profileId = profileId;
    }
}

// Logs in synchronously by unique nickname after ending the previous session.
// Reports validation/backend failures to the UI. Success replaces account metadata
// and requires the front-end host to publish the selected player id.
bool ProfileManager::login(const std::string& uniqueNick, const std::string& password)
{
    if (password == "")
    {
        IBase* host = classManager->getSingleton(std::string("SwiffHost"));
        if (host)
            static_cast<ui::ISwiffHost*>(host)->postMessage(
                SwiffHostMessage(3, std::string("nopassword"), 3));
        return false;
    }
    if (uniqueNick == "")
    {
        IBase* host = classManager->getSingleton(std::string("SwiffHost"));
        if (host)
            static_cast<ui::ISwiffHost*>(host)->postMessage(
                SwiffHostMessage(3, std::string("noname"), 3));
        return false;
    }

    logout();                                                             // virtual +0x28

    GSIStartAvailableCheck(kGameSpyGameName);
    GSIACResult available;
    for (available = GSIAvailableCheckThink(); available == GSIACWaiting; available = GSIAvailableCheckThink())
        msleep(5);
    if (available != GSIACAvailable)
    {
        reportError(std::string("serveroffline"), true);
        return false;
    }

    if (gpInitialize(&m_connection, kGameSpyProductId, kGameSpyNamespaceId, GP_PARTNERID_GAMESPY) != GP_NO_ERROR)
    {
        reportError(std::string("serveroffline"), true);
        return false;
    }
    gpSetCallback(&m_connection, GP_ERROR, &profileManagerGPError, this);

    int profileId = 0;
    if (gpConnectUniqueNick(&m_connection, uniqueNick.c_str(), password.c_str(),
                            GP_NO_FIREWALL, GP_BLOCKING, &profileManagerGPProfileId,
                            &profileId) != GP_NO_ERROR)
    {
        gpDestroy(&m_connection);
        reportError(lastErrorString(), false);
        return false;
    }

    std::string email;
    gpGetInfo(&m_connection, profileId, GP_DONT_CHECK_CACHE, GP_BLOCKING,
              &profileManagerGPEmail, &email);                            // result deliberately unread
    setCurrentProfile(email, password, profileId, uniqueNick);
    fetchProfileList(m_currentProfile->profiles);
    IBase* host = classManager->getSingleton(std::string("SwiffHost"));
    static_cast<ui::ISwiffHost*>(host)->setSelectedPlayerId(profileId);   // +0x220, unguarded
    return true;
}

// Posts the new player's identity to the stats backend synchronously.
// The response is ignored and this always returns true.
bool ProfileManager::createStatsPlayer(int profileId, const std::string& country, const std::string& nick)
{
    HTTPRequest request;
    std::string url;
    url = request.getGameSpyUrl();
    url += "createplayer.aspx";
    url += "?pid=";
    url += formatString("%d", profileId);
    url += std::string("&cid=") + country;
    url += std::string("&nick=") + nick;
    request.post(url, std::string(""), false);
    return true;
}

// Creates or claims an online account synchronously. Refuses while a profile is
// current and reports validation/backend failures to the UI. Success selects the
// new profile and fills its sibling list. Nick-query failures leave the backend initialized.
bool ProfileManager::createOnlineAccount(const std::string& nick, const std::string& email,
                                         const std::string& password, const std::string& country)
{
    if (m_currentProfile)
        return false;
    std::string error;
    if (nick == "")
    {
        IBase* host = classManager->getSingleton(std::string("SwiffHost"));
        if (host)
            static_cast<ui::ISwiffHost*>(host)->postMessage(
                SwiffHostMessage(3, std::string("noname"), 3));
        return false;
    }
    if (email == "")
    {
        reportError(std::string("noemail"), true);
        return false;
    }
    if (password == "")
    {
        reportError(std::string("nopassword"), true);
        return false;
    }
    if (!validateProfileName(nick, error))                                // virtual +0x54
    {
        reportError(error, true);
        return false;
    }
    if (password.length() < 3)
    {
        reportError(std::string("passwordminimum"), true);
        return false;
    }

    GSIStartAvailableCheck(kGameSpyGameName);
    GSIACResult available;
    for (available = GSIAvailableCheckThink(); available == GSIACWaiting; available = GSIAvailableCheckThink())
        msleep(5);
    if (available != GSIACAvailable
        || gpInitialize(&m_connection, kGameSpyProductId, kGameSpyNamespaceId, GP_PARTNERID_GAMESPY) != GP_NO_ERROR)
    {
        reportError(std::string("serveroffline"), true);
        return false;
    }
    gpSetCallback(&m_connection, GP_ERROR, &profileManagerGPError, this);

    std::vector<OnlineProfileEntry> registered;
    if (gpGetUserNicks(&m_connection, email.c_str(), password.c_str(), GP_BLOCKING,
                       &profileManagerGPUserNicks, &registered) != GP_NO_ERROR)
    {
        reportError(lastErrorString(), false);
        return false;
    }
    if (registered.size() >= 1)
    {
        reportError(std::string("nameexists"), true);
        return false;
    }
    std::vector<OnlineProfileEntry> unregistered;
    if (gpGetUserNicks(&m_connection, email.c_str(), password.c_str(), GP_BLOCKING,
                       &profileManagerGPLegacyNicks, &unregistered) != GP_NO_ERROR)
    {
        reportError(lastErrorString(), false);
        return false;
    }

    int profileId = 0;
    bool existing = false;
    for (std::size_t index = 0; index < unregistered.size(); ++index)   // size re-read each pass
    {
        if (unregistered[index].nick == nick)
        {
            existing = true;
            break;
        }
    }
    if (existing)
    {
        if (gpConnect(&m_connection, nick.c_str(), email.c_str(), password.c_str(),
                      GP_NO_FIREWALL, GP_BLOCKING, &profileManagerGPProfileId, &profileId) != GP_NO_ERROR
            || gpRegisterUniqueNick(&m_connection, nick.c_str(), NULL, GP_BLOCKING,
                                    &profileManagerGPIgnore, NULL) != GP_NO_ERROR)
        {
            reportError(lastErrorString(), false);
            gpDisconnect(&m_connection);
            gpDestroy(&m_connection);
            return false;
        }
    }
    else if (gpConnectNewUser(&m_connection, nick.c_str(), nick.c_str(), email.c_str(),
                              password.c_str(), NULL, GP_NO_FIREWALL, GP_BLOCKING,
                              &profileManagerGPProfileId, &profileId) != GP_NO_ERROR)
    {
        reportError(lastErrorString(), false);
        gpDisconnect(&m_connection);
        gpDestroy(&m_connection);
        return false;
    }

    createStatsPlayer(profileId, country, nick);                          // result unread
    setCurrentProfile(email, password, profileId, nick);
    fetchProfileList(m_currentProfile->profiles);
    IBase* host = classManager->getSingleton(std::string("SwiffHost"));
    static_cast<ui::ISwiffHost*>(host)->setSelectedPlayerId(profileId);   // +0x220, unguarded
    return true;
}

// Updates editable fields for the connected profile, discards SDK results, and pumps the connection once.
// Requires a logged-in session; account identifiers in value are ignored.
bool ProfileManager::setProfileInformation(const ProfileInformation& value)
{
    if (!isLoggedIn())                                                     // virtual +0x44
        return false;
    gpSetInfos(&m_connection, GP_FIRSTNAME, value.firstName.c_str());
    gpSetInfos(&m_connection, GP_LASTNAME, value.lastName.c_str());
    gpSetInfos(&m_connection, GP_COUNTRYCODE, value.country.c_str());
    gpSetInfoi(&m_connection, GP_SEX, value.sex);
    gpSetInfod(&m_connection, GP_BIRTHDAY, value.birthDay, value.birthMonth, value.birthYear);
    update();                                                              // virtual +0x48
    return true;
}

// Logs in by email after resolving its unique nicknames. Uses the requested nickname
// when present, otherwise the first available one. Validation/backend failures are
// reported to the UI; successful lookup logs out before the final nickname login.
bool ProfileManager::loginAccount(const std::string& email, const std::string& password,
                                  const std::string& selectedUniqueNick)
{
    if (password == "")
    {
        IBase* host = classManager->getSingleton(std::string("SwiffHost"));
        if (host)
            static_cast<ui::ISwiffHost*>(host)->postMessage(
                SwiffHostMessage(3, std::string("nopassword"), 3));
        return false;
    }
    if (email == "")
    {
        IBase* host = classManager->getSingleton(std::string("SwiffHost"));
        if (host)
            static_cast<ui::ISwiffHost*>(host)->postMessage(
                SwiffHostMessage(3, std::string("noemail"), 3));
        return false;
    }

    logout();                                                             // virtual +0x28

    GSIStartAvailableCheck(kGameSpyGameName);
    GSIACResult available;
    for (available = GSIAvailableCheckThink(); available == GSIACWaiting; available = GSIAvailableCheckThink())
        msleep(5);
    if (available != GSIACAvailable)
    {
        reportError(std::string("serveroffline"), true);
        return false;
    }
    if (gpInitialize(&m_connection, kGameSpyProductId, kGameSpyNamespaceId, GP_PARTNERID_GAMESPY) != GP_NO_ERROR)
    {
        reportError(std::string("serveroffline"), true);
        logout();
        return false;
    }

    std::vector<OnlineProfileEntry> entries;
    if (gpGetUserNicks(&m_connection, email.c_str(), password.c_str(), GP_BLOCKING,
                       &profileManagerGPUserNicks, &entries) != GP_NO_ERROR)
    {
        reportError(lastErrorString(), false);
        logout();
        return false;
    }
    std::size_t selected = 0;
    for (std::size_t index = 0; !entries.empty() && index < entries.size(); ++index)
    {
        int profileId = 0;
        if (gpCheckUser(&m_connection, entries[index].nick.c_str(), email.c_str(), password.c_str(),
                        GP_BLOCKING, &profileManagerGPProfileId, &profileId) != GP_NO_ERROR)
        {
            reportError(lastErrorString(), false);
            logout();
            return false;
        }
        entries[index].profileId = profileId;
        if (selectedUniqueNick != "" && selectedUniqueNick == entries[index].uniqueNick)
            selected = index;
    }
    if (entries.empty())
    {
        reportError(std::string("invalidaccount"), true);
        logout();
        return false;
    }
    logout();
    return login(entries[selected].uniqueNick, password);                 // virtual +0x14
}

// Logs in by nickname after validating credentials and backend availability.
// Reports missing credentials to the movie host when available.
bool ProfileManager::loginAlternate(const std::string& nick, const std::string& password)
{
    if (password == "")
    {
        IBase* host = classManager->getSingleton(std::string("SwiffHost"));
        if (host)
            static_cast<ui::ISwiffHost*>(host)->postMessage(
                SwiffHostMessage(3, std::string("nopassword"), 3));
        return false;
    }
    if (nick == "")
    {
        IBase* host = classManager->getSingleton(std::string("SwiffHost"));
        if (host)
            static_cast<ui::ISwiffHost*>(host)->postMessage(
                SwiffHostMessage(3, std::string("noname"), 3));
        return false;
    }

    logout();                                                             // virtual +0x28

    GSIStartAvailableCheck(kGameSpyGameName);
    GSIACResult available;
    for (available = GSIAvailableCheckThink(); available == GSIACWaiting; available = GSIAvailableCheckThink())
        msleep(5);
    if (available != GSIACAvailable)
    {
        reportError(std::string("serveroffline"), true);
        return false;
    }
    if (gpInitialize(&m_connection, kGameSpyProductId, kGameSpyNamespaceId, GP_PARTNERID_GAMESPY) != GP_NO_ERROR)
    {
        reportError(std::string("serveroffline"), true);
        logout();
        return false;
    }
    return login(nick, password);                                         // virtual +0x14
}

// Fetches profile information synchronously without the SDK cache. Requires a logged-in session.
// SDK errors are reported and return false.
bool ProfileManager::getProfileInformation(int profileId, ProfileInformation& out)
{
    if (!isLoggedIn())                                                     // virtual +0x44
        return false;
    out.profileId = profileId;
    if (gpGetInfo(&m_connection, profileId, GP_DONT_CHECK_CACHE, GP_BLOCKING,
                  &profileManagerGPProfileInfo, &out) != GP_NO_ERROR)
    {
        reportError(lastErrorString(), false);
        return false;
    }
    return true;
}

// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: profile-manager-account-operations]
// All parameters and returns below are proved by complete PE bodies/callers.
// Their account/GP callback bodies remain visible stops under the client startup
// publication exception. No fabricated account state or successful login is returned.
#define PROFILE_MANAGER_FRONTIER(method) \
    std::fprintf(stderr, "[FRONTIER] dice::hfe::ProfileManager::" #method " reached\n"); \
    std::fflush(stderr); \
    std::abort()

bool ProfileManager::createProfile(const std::string&, const std::string&)
{ PROFILE_MANAGER_FRONTIER(createProfile); }
bool ProfileManager::deleteProfile()
{ PROFILE_MANAGER_FRONTIER(deleteProfile); }
bool ProfileManager::requestNickSuggestions(const std::string&)
{ PROFILE_MANAGER_FRONTIER(requestNickSuggestions); }
#undef PROFILE_MANAGER_FRONTIER

} }
#endif
