// [INFERRED] Console bindings for LocalProfile and GlobalSettings script properties.
// The method set follows profile scripts and the profile writer; generated binding
// names and active-singleton checks are inferred. Both objects use end-user access.
#include "../Core/BuildConfig.h"

#if !DEDICATED

#include "LocalProfileManager.h"
#include "GlobalSettings.h"
#include "../Core/ConsoleObjectMacro.h"
#include "Main/BF2ClientSeams.h"        // g_localProfileManager, ds:0x987364

#include <string>

namespace dice {
namespace hfe {
namespace {

const char* const kLocalProfileConsoleTag = "game.localprofile";

ILocalProfileManager* localProfiles()
{
    return static_cast<ILocalProfileManager*>(g_localProfileManager);
}

// Each forwarder is the executor body the generator would have emitted: the singleton, then the
// interface call. No guard here - the row's active check is what keeps the executor away from a
// missing singleton.
void consoleProfileSetName(const std::string& value)        { localProfiles()->setName(value); }
void consoleProfileSetNick(const std::string& value)        { localProfiles()->setNick(value); }
void consoleProfileSetGamespyNick(const std::string& value) { localProfiles()->setGamespyNick(value); }
void consoleProfileSetEmail(const std::string& value)       { localProfiles()->setEmail(value); }

// saveCurrentProfile writes the password through PasswordCodec::encode, so the script's single
// argument is always the encoded form and the manager's second parameter is the "this text is
// encoded" flag it decodes on.
void consoleProfileSetPassword(const std::string& value)    { localProfiles()->setPassword(value, true); }

void consoleProfileSetTotalPlayedTime(float value)          { localProfiles()->setTotalPlayedTime(value); }
void consoleProfileSetNumTimesLoggedIn(int value)           { localProfiles()->setNumTimesLoggedIn(value); }

void consoleProfileAddDemoBookmark(const std::string& field00, const std::string& field1c,
                                   const std::string& field38, const std::string& field54)
{
    localProfiles()->addDemoBookmark(field00, field1c, field38, field54);
}

// Global.con holds the default profile selection and name prefix at the profiles root.
IGlobalSettings* globalSettings()
{
    return static_cast<IGlobalSettings*>(g_globalSettings);
}

// The interface takes these by value, as the retail setters do (retn 1Ch).
void consoleGlobalSetDefaultUser(const std::string& value) { globalSettings()->setDefaultUser(value); }
void consoleGlobalSetNamePrefix(const std::string& value)  { globalSettings()->setNamePrefix(value); }

} // namespace

// : SetConsoleClassDefaults("LocalProfile", access 1, category 0). Console dispatch is
// case-insensitive, so the scripts' exact spelling does not matter.
BF2_CONSOLE_OBJECT("LocalProfile", 1, 0)

BF2_CONSOLE_METHOD_1_ACTIVE(setName, consoleProfileSetName,
                            g_localProfileManager != 0, std::string, kLocalProfileConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setNick, consoleProfileSetNick,
                            g_localProfileManager != 0, std::string, kLocalProfileConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setGamespyNick, consoleProfileSetGamespyNick,
                            g_localProfileManager != 0, std::string, kLocalProfileConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setEmail, consoleProfileSetEmail,
                            g_localProfileManager != 0, std::string, kLocalProfileConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setPassword, consoleProfileSetPassword,
                            g_localProfileManager != 0, std::string, kLocalProfileConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setTotalPlayedTime, consoleProfileSetTotalPlayedTime,
                            g_localProfileManager != 0, float, kLocalProfileConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setNumTimesLoggedIn, consoleProfileSetNumTimesLoggedIn,
                            g_localProfileManager != 0, int, kLocalProfileConsoleTag)
BF2_CONSOLE_METHOD_4(addDemoBookmark, consoleProfileAddDemoBookmark,
                     g_localProfileManager != 0,
                     std::string, std::string, std::string, std::string, kLocalProfileConsoleTag)

// : SetConsoleClassDefaults("GlobalSettings", access 1, category 0). Both objects share
// one binding list, as HeightmapConsole.cpp's pair does, so one registration covers them.
BF2_CONSOLE_OBJECT("GlobalSettings", 1, 0)

BF2_CONSOLE_METHOD_1_ACTIVE(setDefaultUser, consoleGlobalSetDefaultUser,
                            g_globalSettings != 0, std::string, kLocalProfileConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setNamePrefix, consoleGlobalSetNamePrefix,
                            g_globalSettings != 0, std::string, kLocalProfileConsoleTag)

// Handed to the console registry by ModuleGame::init (Core/ModuleRegistry.cpp), which is the module
// that registers the LocalProfileManager class and singleton in the first place. The list carries
// both objects.
ConsoleObjectList& localProfileConsoleObjects()
{
    return consoleBindingList(kLocalProfileConsoleTag);
}

} // namespace hfe
} // namespace dice

#endif // !dedicated
