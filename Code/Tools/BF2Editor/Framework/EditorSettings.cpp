// Code/Tools/BF2Editor/Framework/EditorSettings.cpp - see EditorSettings.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/EditorSettings.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace bf2editor
{

namespace
{

std::string trimmed(const std::string& text)
{
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])))
        ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
        --end;
    return text.substr(begin, end - begin);
}

} // namespace

bool EditorSettings::isValidKey(const std::string& key)
{
    if (key.empty())
        return false;
    for (size_t i = 0; i < key.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(key[i]);
        if (!std::isalnum(c) && c != '.' && c != '_' && c != '-')
            return false;
    }
    return true;
}

void EditorSettings::parse(const std::string& text, std::vector<std::string>& problems)
{
    std::istringstream lines(text);
    std::string line;
    int number = 0;
    while (std::getline(lines, line))
    {
        ++number;
        const std::string content = trimmed(line);
        if (content.empty() || content[0] == '#' || content[0] == ';')
            continue;
        const size_t equals = content.find('=');
        const std::string key = equals == std::string::npos ? std::string() : trimmed(content.substr(0, equals));
        if (!isValidKey(key))
        {
            char where[32];
            std::snprintf(where, sizeof(where), "line %d", number);
            problems.push_back(std::string(where) + " is not a setting and was skipped: " + content);
            continue;
        }
        m_values[key] = trimmed(content.substr(equals + 1));
    }
}

std::string EditorSettings::serialize() const
{
    std::string text = "# BF2Editor settings - written by the editor on exit; `key = value` per line.\n";
    for (std::map<std::string, std::string>::const_iterator it = m_values.begin(); it != m_values.end(); ++it)
        text += it->first + " = " + it->second + "\n";
    return text;
}

bool EditorSettings::load(const std::string& path, std::vector<std::string>& problems)
{
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in)
        return true;                                // no file yet: the defaults
    std::ostringstream text;
    text << in.rdbuf();
    parse(text.str(), problems);
    return true;
}

bool EditorSettings::save(const std::string& path, std::string& error) const
{
    // Written whole to a temporary file first, so a failed write never leaves half a file behind.
    const std::string temporary = path + ".tmp";
    {
        std::ofstream out(temporary.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out)
        {
            error = "cannot write " + temporary;
            return false;
        }
        const std::string text = serialize();
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!out)
        {
            error = "cannot write " + temporary;
            return false;
        }
    }
    std::remove(path.c_str());
    if (std::rename(temporary.c_str(), path.c_str()) != 0)
    {
        error = "cannot replace " + path;
        return false;
    }
    return true;
}

bool EditorSettings::has(const std::string& key) const
{
    return m_values.find(key) != m_values.end();
}

std::string EditorSettings::get(const std::string& key, const std::string& fallback) const
{
    std::map<std::string, std::string>::const_iterator found = m_values.find(key);
    return found == m_values.end() ? fallback : found->second;
}

float EditorSettings::getFloat(const std::string& key, float fallback) const
{
    const std::string text = get(key, std::string());
    if (text.empty())
        return fallback;
    char* end = 0;
    const double value = std::strtod(text.c_str(), &end);
    return (end && *end == 0) ? static_cast<float>(value) : fallback;
}

int EditorSettings::getInt(const std::string& key, int fallback) const
{
    const std::string text = get(key, std::string());
    if (text.empty())
        return fallback;
    char* end = 0;
    const long value = std::strtol(text.c_str(), &end, 10);
    return (end && *end == 0) ? static_cast<int>(value) : fallback;
}

bool EditorSettings::getBool(const std::string& key, bool fallback) const
{
    const std::string text = get(key, std::string());
    if (text == "1" || text == "true")
        return true;
    if (text == "0" || text == "false")
        return false;
    return fallback;
}

bool EditorSettings::set(const std::string& key, const std::string& value)
{
    if (!isValidKey(key) || value.find('\n') != std::string::npos || value.find('\r') != std::string::npos)
        return false;
    m_values[key] = trimmed(value);
    return true;
}

bool EditorSettings::setFloat(const std::string& key, float value)
{
    char text[32];
    std::snprintf(text, sizeof(text), "%.9g", value);
    return set(key, text);
}

bool EditorSettings::setInt(const std::string& key, int value)
{
    char text[32];
    std::snprintf(text, sizeof(text), "%d", value);
    return set(key, text);
}

bool EditorSettings::setBool(const std::string& key, bool value)
{
    return set(key, value ? "1" : "0");
}

} // namespace bf2editor
