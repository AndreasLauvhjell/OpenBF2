// Path: [INFERRED] Game/Persistence/KeyParser.h; no __FILE__ attestation.
// ELF KeyMap 0x82BE8C0..0x82BEBB0, KeyParser 0x82BF0C0..0x82BFE10.
// PE parser 0x61DAF0, map helpers 0x61ECC0..0x61EE10.
// Containers use the compiling STL; neither port's raw STL layout is imposed.
#ifndef GAME_PERSISTENCE_KEYPARSER_H
#define GAME_PERSISTENCE_KEYPARSER_H
#include <string>
#include <vector>

namespace dice { namespace hfe {

class KeyMap
{
public:
    struct Key { std::string key; std::string value; };
    KeyMap();
    ~KeyMap();
    Key* findKey(const std::string&);
    Key* getKey(unsigned int);
    void addKey(const Key&);
    void clear();
    unsigned int getNumKeys() const;
private:
    std::vector<Key> m_keys;
};

class KeyParser
{
public:
    // Numeric values attested; enumerator spellings are [INFERRED].
    enum Type { Stats = 0, Unlocks = 1, Type2 = 2, Search = 3, Validation = 4 };
    KeyParser();
    ~KeyParser();
    void clear();
    bool parse(const std::string&, Type, void*);
    bool addKeyMap(const KeyMap&);
    KeyMap* getKeyMap(unsigned int);
    unsigned int getNumKeyMaps() const;
private:
    std::vector<KeyMap> m_keyMaps;
};

} }
#endif
