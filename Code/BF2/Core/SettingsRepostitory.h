// Typed named settings; preserve the shipped Repostitory spelling. Header path [INFERRED].
// Getters return a reference to stored data or the supplied default, whose lifetime remains the
// caller's responsibility. Each type's virtual group is Get, Set, Exists, Erase; preserve that order.

// Stores seven typed variable families in interface order: int, float, unsigned, string, bool,
// Vec3 and Vec4. Accessors delegate to the matching VariableStorage without additional policy.
// Preserve family order; container widths follow the native STL. VariableStorage's original
// header placement and field names remain [INFERRED].

#ifndef CORE_SETTINGSREPOSTITORY_H     // [INFERRED] guard style
#define CORE_SETTINGSREPOSTITORY_H

#include <map>
#include <string>
#include "../Math/Vec3.h"      // Vec3 is a typedef (Math::Vec3), not a class - forward-declaring it
                               // as `class Vec3;` is a hard error (C2371, different basic types) and
                               // cascades into every other header that uses Vec3 by value.

namespace dice {
namespace hfe {

// Map-backed settings storage with an iterator cache updated by get, set and exists.
// erase does not explicitly invalidate the cache. Native STL layouts differ from the 16-byte x86 reference.
template <class K, class V>
class VariableStorage
{
public:
    typedef std::map<K, V>              Map;
    typedef typename Map::iterator      iterator;

    VariableStorage() : m_map(), m_last() {}

    // Returns a reference to the stored value, or defaultValue when key is absent.
    // The caller must keep the referenced value alive while using the result.
    const V& get(const K& key, const V& defaultValue)
    {
        m_last = m_map.find(key);
        if (m_last == m_map.end())
            return defaultValue;
        return m_last->second;
    }

    void set(const K& key, const V& value)
    {
        m_last = m_map.find(key);
        if (m_last == m_map.end())
            m_map.insert(std::pair<K, V>(key, value));    // _Rb_tree::insert_unique, 0x83CCB1E
        else
            m_last->second = value;                       // `*(v3 + 20) = *a3` - overwrite in place
    }

    bool exists(const K& key)
    {
        m_last = m_map.find(key);
        return m_last != m_map.end();
    }

    // Does not touch m_last, so an erase leaves the cache pointing at a freed node. Harmless as
    //   shipped (nothing reads the cache), and preserved because it is what the binary does: the
    //   body is a single tail-jump into _Rb_tree::erase(const key_type&).
    void erase(const K& key) { m_map.erase(key); }

private:
    Map      m_map;      // +0x00
    iterator m_last;     // +0x0C - zeroed by the constructor, i.e. not initialised to end()
};

// Flattens the original ISettingsRepostitory/IBase chain while preserving its vtable.
// Restore that base atomically with removal of the minimal interface declaration used by engine startup.
class SettingsRepostitory      // [sic] - dice's spelling
{
public:
    SettingsRepostitory();
    // IBase quartet, +0x00..+0x0C
    /* +0x00 */ virtual void addRef();
    /* +0x04 */ virtual int  getRef();
    /* +0x08 */ virtual int  release();
    /* +0x0C */ virtual void* queryInterface(unsigned int) const;


    // Get returns a const reference to the stored value or caller-supplied default; preserve its lifetime.
    // Exists returns bool. Set/Erase remain void because recovered callers do not establish a result.
    /* +0x10 */ virtual const int& intGet(const std::string&, const int&);
    /* +0x14 */ virtual void intSet(const std::string&, const int&);
    /* +0x18 */ virtual bool intExists(const std::string&);
    /* +0x1C */ virtual void intErase(const std::string&);

    // Float, +0x20..+0x2C
    /* +0x20 */ virtual const float& floatGet(const std::string&, const float&);
    /* +0x24 */ virtual void floatSet(const std::string&, const float&);
    /* +0x28 */ virtual bool floatExists(const std::string&);
    /* +0x2C */ virtual void floatErase(const std::string&);

    // Unsigned int, +0x30..+0x3C
    /* +0x30 */ virtual const unsigned int& U32Get(const std::string&, const unsigned int&);
    /* +0x34 */ virtual void U32Set(const std::string&, const unsigned int&);
    /* +0x38 */ virtual bool U32Exists(const std::string&);
    /* +0x3C */ virtual void U32Erase(const std::string&);

    // String, +0x40..+0x4C
    /* +0x40 */ virtual const std::string& stringGet(const std::string&, const std::string&);
    /* +0x44 */ virtual void stringSet(const std::string&, const std::string&);
    /* +0x48 */ virtual bool stringExists(const std::string&);
    /* +0x4C */ virtual void stringErase(const std::string&);

    // Bool, +0x50..+0x5C
    /* +0x50 */ virtual const bool& boolGet(const std::string&, const bool&);
    /* +0x54 */ virtual void boolSet(const std::string&, const bool&);
    /* +0x58 */ virtual bool boolExists(const std::string&);
    /* +0x5C */ virtual void boolErase(const std::string&);

    // Vec3, +0x60..+0x6C
    /* +0x60 */ virtual const Vec3& Vec3Get(const std::string&, const Vec3&);
    /* +0x64 */ virtual void Vec3Set(const std::string&, const Vec3&);
    /* +0x68 */ virtual bool Vec3Exists(const std::string&);
    /* +0x6C */ virtual void Vec3Erase(const std::string&);

    // Vec4, +0x70..+0x7C
    /* +0x70 */ virtual const Vec4& Vec4Get(const std::string&, const Vec4&);
    /* +0x74 */ virtual void Vec4Set(const std::string&, const Vec4&);
    /* +0x78 */ virtual bool Vec4Exists(const std::string&);
    /* +0x7C */ virtual void Vec4Erase(const std::string&);

    // Keep the destructor after the settings interface methods; ELF uses two slots and MSVC one.
    /* +0x80 */ virtual ~SettingsRepostitory();

    // createSingleton belongs to ISettingsRepostitory, not this concrete class.

private:
    // Storage, +0x04..+0x77. See the layout block at the top of this file.
    int m_refCount;                                          // +0x04, constructed to 1

    VariableStorage<std::string, int>          m_int;        // +0x08
    VariableStorage<std::string, float>        m_float;      // +0x18
    VariableStorage<std::string, unsigned int> m_U32;        // +0x28
    VariableStorage<std::string, std::string>  m_string;     // +0x38
    VariableStorage<std::string, bool>         m_bool;       // +0x48
    VariableStorage<std::string, Vec3>         m_Vec3;       // +0x58
    VariableStorage<std::string, Vec4>         m_Vec4;       // +0x68
};

// Registers and unregisters the repository's setting-change event.
void initSettingsRepostitory();
void shutdownSettingsRepostitory();

// The singleton. Read as `mov ecx, ds:_ZN4dice3hfe8settingsE` at every call site.
extern SettingsRepostitory* settings;

// IID_ISettingsRepostitory = 40011, CID_SettingsRepostitory = 40054  -- see Core/Ids.h.
// Both carry the same misspelling.

} // namespace hfe
} // namespace dice

#endif
