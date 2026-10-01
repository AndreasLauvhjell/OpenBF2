// [INFERRED] path; field method names follow the LocalProfile console metadata.
// Complete PE bodies and callers: local_profile_suffix_continue_20260906;
// fresh literal/append reads: local_profile_validation_20260906/fields.
#include "LocalProfileManager.h"
#include "PasswordCodec.h"
#if !DEDICATED
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace dice { namespace hfe {

void LocalProfileManager::setName(const std::string& value) // PE4A7860
{
    if (m_currentProfile)
        m_currentProfile->profileName = value;
}

void LocalProfileManager::setGamespyNick(const std::string& value) // PE4A7880
{
    if (m_currentProfile)
        m_currentProfile->gamespyNick = value;
}

void LocalProfileManager::setEmail(const std::string& value) // PE4A78A0
{
    if (m_currentProfile)
        m_currentProfile->email = value;
}

void LocalProfileManager::setPassword(const std::string& value, bool encoded) // PE4A78C0
{
    if (!m_currentProfile)
        return;
    if (!encoded) {
        m_currentProfile->password = value;
        return;
    }
    // Decodes a nonempty copy in place and stores it even when decoding fails.
    PasswordCodec codec;
    std::string text(value);
    if (!text.empty())
        codec.decode(text);
    m_currentProfile->password = text;
}

void LocalProfileManager::setNick(const std::string& value) // PE4A7A40
{
    if (m_currentProfile)
        m_currentProfile->nick = value;
}

void LocalProfileManager::setNumTimesLoggedIn(int value) // PE4A7AA0
{
    if (m_currentProfile)
        m_currentProfile->numTimesLoggedIn = value;
}

std::string LocalProfileManager::getName() // PE4A7940
{
    if (!m_currentProfile)
        return std::string("");
    return m_currentProfile->profileName;
}

std::string LocalProfileManager::getGamespyNick() // PE4A7980
{
    if (!m_currentProfile)
        return std::string("");
    return m_currentProfile->gamespyNick;
}

std::string LocalProfileManager::getEmail() // PE4A79C0
{
    if (!m_currentProfile)
        return std::string("");
    return m_currentProfile->email;
}

std::string LocalProfileManager::getPassword() // PE4A7A00
{
    if (!m_currentProfile)
        return std::string("");
    return m_currentProfile->password;
}

std::string LocalProfileManager::getNick() // PE4A7A60
{
    if (!m_currentProfile)
        return std::string("");
    return m_currentProfile->nick;
}

int LocalProfileManager::getNumTimesLoggedIn() // PE4A7AC0
{
    if (!m_currentProfile)
        return 0;
    return m_currentProfile->numTimesLoggedIn;
}

void LocalProfileManager::addDemoBookmark(const std::string& field00,
    const std::string& field1c, const std::string& field38,
    const std::string& field54) // PE4AC030
{
    // Retail default-constructs all four strings before their ordered assignments.
    // The local copy also preserves inputs aliased into m_demoBookmarks on growth.
    LocalDemoBookmark bookmark;
    bookmark.field00 = field00;
    bookmark.field1c = field1c;
    bookmark.field38 = field38;
    bookmark.field54 = field54;
    m_demoBookmarks.push_back(bookmark); // complete4ABA60: append one native record
}

float LocalProfileManager::getTotalPlayedTime() // PE4A7D10
{
    if (!m_currentProfile)
        return 0.0f; // actual FLD constant93C850, bits00000000
    return m_currentProfile->totalPlayedTime;
}

void LocalProfileManager::setTotalPlayedTime(float value) // PE4A7D30
{
    if (m_currentProfile) {
        // PE4A7D3A copies the float's dword directly, preserving its payload bits.
        std::memcpy(&m_currentProfile->totalPlayedTime, &value, sizeof(value));
    }
}

} }
#endif
