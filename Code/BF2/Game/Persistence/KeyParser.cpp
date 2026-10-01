// Path: [INFERRED] Game/Persistence/KeyParser.cpp; see KeyParser.h.
#include "KeyParser.h"
#include "SimpleTokenizer.h"
#include "../../Core/StringUtil.h"
#include <cstdint>
#include <cstdlib>

namespace dice { namespace hfe {


KeyMap::KeyMap() = default;
KeyMap::~KeyMap() = default;
KeyMap::Key* KeyMap::findKey(const std::string& name)
{
    for (unsigned int i = 0; i < m_keys.size(); ++i)
        if (compareIgnoreCase(m_keys[i].key, name) == 0)
            return &m_keys[i];
    return 0;
}
KeyMap::Key* KeyMap::getKey(unsigned int index)
{
    return index < m_keys.size() ? &m_keys[index] : 0;
}
void KeyMap::addKey(const Key& key) { m_keys.push_back(key); }
void KeyMap::clear() { m_keys.clear(); }
unsigned int KeyMap::getNumKeys() const { return static_cast<unsigned int>(m_keys.size()); }

KeyParser::KeyParser() = default;
KeyParser::~KeyParser() = default;
void KeyParser::clear() { m_keyMaps.clear(); }
bool KeyParser::addKeyMap(const KeyMap& map) { m_keyMaps.push_back(map); return true; }
KeyMap* KeyParser::getKeyMap(unsigned int index)
{
    return index < m_keyMaps.size() ? &m_keyMaps[index] : 0;
}
unsigned int KeyParser::getNumKeyMaps() const
{
    return static_cast<unsigned int>(m_keyMaps.size());
}

//  (2770 B), PE 0x61DAF0. Only tab and newline delimit tokens.
// Parsing appends to existing state, tolerates exhausted header fields and excess
// values, and clears all maps only for '$' encountered before the first data row.
bool KeyParser::parse(const std::string& text, Type type, void* metadata)
{
    SimpleTokenizer tokenizer(text, "\t\n");
    std::string token;
    tokenizer.next(token);
    if (token != "O")
        return false;
    int* numbers = static_cast<int*>(metadata);
    if (type == Unlocks || type == Type2)
    {
        const int fields = type == Unlocks ? 13 : 5;
        for (int i = 0; i < fields; ++i)
            tokenizer.next(token);
        if (numbers)
            numbers[0] = std::atoi(token.c_str());
        tokenizer.next(token);
        if (numbers)
            numbers[1] = std::atoi(token.c_str());
    }
    else if (type == Search)
    {
        tokenizer.next(token);
        tokenizer.next(token);
        tokenizer.next(token);
        const bool hasRank = token == "rnk";
        if (hasRank)
            tokenizer.next(token);
        tokenizer.next(token);
        tokenizer.next(token);
        if (numbers)
            numbers[0] = std::atoi(token.c_str());
        tokenizer.next(token);
        if (hasRank)
        {
            if (numbers)
                numbers[1] = 1;
            tokenizer.next(token);
        }
    }
    else
    {
        for (int i = 0; i < 4; ++i)
            tokenizer.next(token);
        if (type == Validation)
        {
            std::string* strings = static_cast<std::string*>(metadata);
            tokenizer.next(token);
            tokenizer.next(token);
            tokenizer.next(token);
            strings[0] = token;
            tokenizer.next(token);
            strings[1] = token;
            tokenizer.next(token);
            strings[2] = token;
            for (int i = 0; i < 5; ++i)
                tokenizer.next(token);
            strings[3] = token;
        }
    }

    while (true)
    {
        if (!tokenizer.next(token))
            return false;
        if (token == "H")
            break;
        if (token == "$")
            return true;
    }
    KeyMap map;
    m_keyMaps.push_back(map);
    bool hasData = false;
    while (tokenizer.next(token))
    {
        if (token == "D")
        {
            hasData = true;
            unsigned int index = 0;
            while (tokenizer.next(token))
            {
                if (token == "D")
                {
                    KeyMap nextMap;
                    KeyMap* first = getKeyMap(0);
                    for (unsigned int i = 0; i < first->getNumKeys(); ++i)
                    {
                        KeyMap::Key key;
                        key.key = first->getKey(i)->key;
                        key.value = "";
                        nextMap.addKey(key);
                    }
                    m_keyMaps.push_back(nextMap);
                    index = 0;
                }
                else
                {
                    if (token == "$")
                        return true;
                    KeyMap::Key* key = getKeyMap(getNumKeyMaps() - 1)->getKey(index);
                    if (key)
                    {
                        ++index;
                        key->value = token;
                    }
                }
            }
        }
        else if (token == "$")
        {
            if (!hasData)
                m_keyMaps.clear();
            return true;
        }
        else
        {
            KeyMap::Key key;
            key.key = token.c_str();
            key.value = "";
            getKeyMap(getNumKeyMaps() - 1)->addKey(key);
        }
    }
    return true;
}
} }
