// Typed settings singleton and its event registration. Source path [INFERRED].
// Create the singleton before settings reads, register its event after modules initialize and
// unregister that event during shutdown. There is no matching singleton destruction operation.
// The settings global is defined once in Globals.cpp. Preserve the shipped Repostitory spelling.

#include "SettingsRepostitory.h"

#include "ClassManager.h"       // SingletonRegInfo, ClassManager::registerSingleton (vtable +0x44)
#include "EventManager.h"       // eventManager, registerEvent (+0x34) / unregisterEvent (+0x38)
#include "Ids.h"                // IID_IBase, IID_ISettingsRepostitory

namespace dice {
namespace hfe {

// Engine-owned class registry; keep the declaration type consistent with its definition.
extern ClassManager* classManager;

// [CHOSEN] Minimal local declaration matching BF2Engine's singleton factory call.
// Keep the static signature consistent until the shared interface replaces both declarations.
class ISettingsRepostitory { public: static void createSingleton(); };

// Constructs and publishes settings before registering its global with ClassManager.
// Flag 4 denotes an externally created object; registration does not create or own another instance.
void ISettingsRepostitory::createSingleton()
{
    settings = new SettingsRepostitory();                  // operator new(0x78) == 120 bytes

    std::string name("SettingsRepostitory");

    // The cast is mechanical and matches the binary exactly - SingletonRegInfo::instance is an
    // `IBase` and the real SettingsRepostitory is an IBase (through ISettingsRepostitory), so at
    // runtime this is a no-op. It is spelled as a cast only because the header cannot name the base
    // class - see the note above. Core/Module.cpp does the same for `loadStat` and `eventManager`.
    SingletonRegInfo info = { name, reinterpret_cast<IBase**>(&settings), 0, 0, 4 };

    classManager->registerSingleton(info);                 // ClassManager vtable +0x44
}

// Registers the settings-change notification name in core category 5; it adds no handler.
void initSettingsRepostitory()
{
    eventManager->registerEvent(static_cast<EventCategory>(5),   // [UNRESOLVED] enumerator name for
                                                                 //   5; the Core category
                               1,                                // the ChangedSetting event id
                               "ECCoreCEChangedSetting");
}


// Unregisters the setting-change event without releasing or clearing the settings singleton.
void shutdownSettingsRepostitory()
{
    eventManager->unregisterEvent(static_cast<EventCategory>(5), 1);
}

// Constructs empty typed stores with one reference, owned by the registering ClassManager.
SettingsRepostitory::SettingsRepostitory()
    : m_refCount(1)
{
}

// Member stores tear down in reverse declaration order; no global clearing or unregistration occurs here.
SettingsRepostitory::~SettingsRepostitory()
{
}

// Reference-count operations for the settings repository.
void SettingsRepostitory::addRef()
{
    ++m_refCount;
}

int SettingsRepostitory::getRef()
{
    return m_refCount;
}

// the `this` null check is in the binary (`test edx, edx / jz`) and is preserved. It can only
// fire on a call through a null pointer, which is already undefined - the same redundant guard
// appears on EventNode::release (Core/EventManager.h, +0x08). Shipped as written.
int SettingsRepostitory::release()
{
    --m_refCount;
    if (m_refCount == 0 && this)
        delete this;                        // dispatched through vtable +0x84 in the ELF
    return m_refCount;
}

// Returns the repository for IBase or its settings interface, and null for other IDs.
void* SettingsRepostitory::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<SettingsRepostitory*>(this);
    if (iid == IID_ISettingsRepostitory)
        return const_cast<SettingsRepostitory*>(this);
    return 0;
}

// Forwards typed operations to their VariableStorage members. Preserve virtual declaration order.
const int& SettingsRepostitory::intGet(const std::string& k, const int& d) { return m_int.get(k, d); }
void       SettingsRepostitory::intSet(const std::string& k, const int& v) { m_int.set(k, v); }
bool       SettingsRepostitory::intExists(const std::string& k)            { return m_int.exists(k); }
void       SettingsRepostitory::intErase(const std::string& k)             { m_int.erase(k); }

// Float, vtable +0x20..+0x2C, storage +0x18
const float& SettingsRepostitory::floatGet(const std::string& k, const float& d) { return m_float.get(k, d); }
void         SettingsRepostitory::floatSet(const std::string& k, const float& v) { m_float.set(k, v); }
bool         SettingsRepostitory::floatExists(const std::string& k)              { return m_float.exists(k); }
void         SettingsRepostitory::floatErase(const std::string& k)               { m_float.erase(k); }

// Unsigned int, vtable +0x30..+0x3C, storage +0x28
const unsigned int& SettingsRepostitory::U32Get(const std::string& k, const unsigned int& d) { return m_U32.get(k, d); }
void                SettingsRepostitory::U32Set(const std::string& k, const unsigned int& v) { m_U32.set(k, v); }
bool                SettingsRepostitory::U32Exists(const std::string& k)                     { return m_U32.exists(k); }
void                SettingsRepostitory::U32Erase(const std::string& k)                      { m_U32.erase(k); }

// String settings use const references for keys, values and defaults.
const std::string& SettingsRepostitory::stringGet(const std::string& k, const std::string& d) { return m_string.get(k, d); }
void               SettingsRepostitory::stringSet(const std::string& k, const std::string& v) { m_string.set(k, v); }
bool               SettingsRepostitory::stringExists(const std::string& k)                    { return m_string.exists(k); }
void               SettingsRepostitory::stringErase(const std::string& k)                     { m_string.erase(k); }

// Boolean settings return a reference to stored data or the supplied default.
const bool& SettingsRepostitory::boolGet(const std::string& k, const bool& d) { return m_bool.get(k, d); }
void        SettingsRepostitory::boolSet(const std::string& k, const bool& v) { m_bool.set(k, v); }
bool        SettingsRepostitory::boolExists(const std::string& k)             { return m_bool.exists(k); }
void        SettingsRepostitory::boolErase(const std::string& k)              { m_bool.erase(k); }

// Vec3, vtable +0x60..+0x6C, storage +0x58
const Vec3& SettingsRepostitory::Vec3Get(const std::string& k, const Vec3& d) { return m_Vec3.get(k, d); }
void        SettingsRepostitory::Vec3Set(const std::string& k, const Vec3& v) { m_Vec3.set(k, v); }
bool        SettingsRepostitory::Vec3Exists(const std::string& k)             { return m_Vec3.exists(k); }
void        SettingsRepostitory::Vec3Erase(const std::string& k)              { m_Vec3.erase(k); }

// Vec4, vtable +0x70..+0x7C, storage +0x68
const Vec4& SettingsRepostitory::Vec4Get(const std::string& k, const Vec4& d) { return m_Vec4.get(k, d); }
void        SettingsRepostitory::Vec4Set(const std::string& k, const Vec4& v) { m_Vec4.set(k, v); }
bool        SettingsRepostitory::Vec4Exists(const std::string& k)             { return m_Vec4.exists(k); }
void        SettingsRepostitory::Vec4Erase(const std::string& k)              { m_Vec4.erase(k); }

} // namespace hfe
} // namespace dice
