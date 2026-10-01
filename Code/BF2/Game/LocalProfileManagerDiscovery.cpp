// [INFERRED] Profile discovery methods; shared file and console operations use native interfaces.
#include "LocalProfileManager.h"
#include "GeneralSettings.h"            // the three settings files validation tests
#include "GameServerSettings.h"
#include "AudioSettings.h"
#include "Main/BF2ClientSeams.h"        // g_generalSettings / g_gameServerSettings / g_audioSettings
#include "../Core/StringUtil.h"
#include "../IO/File/FileManager.h"
#include "../IO/Console/Console.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if !DEDICATED
namespace dice { namespace hfe {

bool LocalProfileManager::getLocalProfiles(std::vector<LocalProfile>& output, bool validOnly)
{
    // PE4ABD4C: the cached path assigns even when the caller passes the cache itself.
    if (!m_profilesDirty && m_filterValidProfiles == validOnly) {
        output = m_profiles;
        return true;
    }

    m_profilesDirty = false;
    m_filterValidProfiles = validOnly;
    // PE4ABD7E..4ABDAC destroys the old elements AND releases the allocation before
    // the next virtual call. Native clear() alone would retain that allocation.
    std::vector<LocalProfile>().swap(m_profiles);

    std::string root = getProfilesPath();
    std::list<io::FileInfo> files;
    io::fileManager->findFiles(root, false, files);
    for (std::list<io::FileInfo>::const_iterator it = files.begin(); it != files.end(); ++it) {
        // PE4ABDF1 copies the complete FileInfo before checking its directory byte.
        io::FileInfo file(*it);
        if (file.isDirectory) {
            LocalProfile profile;
            profile.name = file.name;
            // Validation is unconditional here, including validOnly=false. It
            // precedes changing m_currentProfile or loading any Profile.con.
            const bool valid = validateProfileFiles(profile.name);
            if (validOnly && !valid)
                continue;

            LocalProfile* previousProfile = m_currentProfile;
            m_currentProfile = &profile;
            if (valid) {
                std::string relativePath(profile.name);
                relativePath += "/";
                relativePath += "Profile.con";
                // PE compares the copied FileInfo name, not the record name
                // which validation or console callbacks may have changed.
                if (compareIgnoreCase(file.name.c_str(), "default") == 0) {
                    loadProfileFile(relativePath);
                    m_hasDefaultProfile = true; // even when the file failed to open
                } else if (loadProfileFile(relativePath)) {
                    m_profiles.push_back(profile);
                }
            } else {
                m_profiles.push_back(profile);
            }
            // The relative path dies before restoration, and the stack record
            // dies afterwards. Validated default directories are never cached.
            m_currentProfile = previousProfile;
        }
    }
    output = m_profiles;
    return true; // PE ignores findFiles' result, including its failure return.
}

std::string LocalProfileManager::getProfilesPath()
{
    return std::string("@HOME@/Profiles/");
}

bool LocalProfileManager::findProfileDirectory(const std::string& profileName,
                                             std::string& directory)
{
    std::vector<LocalProfile> profiles;
    getLocalProfiles(profiles, true);
    for (std::size_t index = 0; index < profiles.size(); ++index) {
        // PE431950 uses _stricmp on both c_str values, including NUL truncation.
        if (compareIgnoreCase(profiles[index].profileName, profileName) == 0) {
            directory = profiles[index].name;
            return true;
        }
    }
    return false;
}

bool LocalProfileManager::loadProfileFile(const std::string& relativePath)
{
    std::string path = getProfilesPath();
    path += relativePath;
    std::string commands;
    // The producer is file-like: PE immediately uses INonlinearStream's +38 size
    // slot after open. This is the same typed narrowing used by io::File::open.
    io::INonlinearStream* stream = static_cast<io::INonlinearStream*>(
        io::fileManager->open(path, io::FILEMODE_READ, static_cast<io::FileAccess>(0), 0));
    if (!stream)
        return false;

    const unsigned int size = static_cast<unsigned int>(stream->getSize());
    char* buffer = new char[size + 1u];
    std::memset(buffer, 0, size + 1u);
    stream->read(buffer, size); // retail ignores short/error read counts
    commands = buffer;        // embedded NUL truncates the console input
    delete[] buffer;
    stream->release();

    // PE4A7774..4A77BB reloads the singleton for every call. Callbacks may change it.
    const int previousAccess = io::mainConsole->getAccess();
    io::mainConsole->setAccess(2);
    io::mainConsole->executeLine(commands, io::Console::ignoredString_,
                                 false, false, false, false, false);
    io::mainConsole->setAccess(previousAccess);
    return true;
}

bool LocalProfileManager::validateProfileFiles(const std::string& directory)
{
    // PE4A8596 is native string equality; unlike enumeration's comparison, it
    // is case-sensitive and does not truncate an embedded NUL in the string.
    if (directory == "Default")
        return true;

    {
        std::string fileName = getProfilesPath() + directory + "/Profile.con";
        if (!io::fileManager->fileExists(fileName))
            return false;
    }

    // Validates General, GameServer, and Audio settings files, returning at the first missing file.
    {
        std::string root = getProfilesPath();
        std::string profilePath = root + directory;
        std::string directoryPath = profilePath + "/";
        std::string fileName = directoryPath + static_cast<IGeneralSettings*>(g_generalSettings)->getFileName();   // ds:987390 +10
        if (!io::fileManager->fileExists(fileName))
            return false;
    }
    {
        std::string fileName = getProfilesPath() + directory + "/"
            + static_cast<IGameServerSettings*>(g_gameServerSettings)->getFileName();                                // ds:987354 +10
        if (!io::fileManager->fileExists(fileName))
            return false;
    }
    {
        std::string fileName = getProfilesPath() + directory + "/"
            + static_cast<IAudioSettings*>(g_audioSettings)->getFileName();                                          // ds:987384 +10
        if (!io::fileManager->fileExists(fileName))
            return false;
    }
    return true;
}

} }
#endif
