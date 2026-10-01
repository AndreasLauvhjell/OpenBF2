// [INFERRED] path. Full client PE41057A/4B0B00/4B0AC0/6CF0E0/4B08D0/4B0900/4B0920/5A5E20/4B09F0/
// /5D2000/4B0A00/4B0A80 read.
#include "GlobalSettings.h"
#if !DEDICATED
#include "../Core/Ids.h"
#include <new>
#include <sstream>

namespace dice { namespace hfe {

// (55 B): table, refcount 1, both strings constructed, then the default user is assigned the
// empty literal 87FB54 - a store the source made explicitly after the construction.
GlobalSettings::GlobalSettings() : m_refCount(1)
{
    m_defaultUser = "";
}

//: the name prefix, then the default user, then the free.
GlobalSettings::~GlobalSettings() {}

// (63 B): the settings-factory shape - aggregation refused, 64 retail bytes, construction
// only on a non-null allocation, unconditional queryInterface (the shipped null defect is kept).
IBase* GlobalSettings::create(unsigned int iid, IBase* outer)
{
    if (outer) return nullptr;
    void* allocation = ::operator new(sizeof(GlobalSettings));
    GlobalSettings* owner = allocation ? new (allocation) GlobalSettings : nullptr;
    IBase* result = static_cast<IBase*>(owner->queryInterface(iid));
    if (!result) owner->release();
    return result;
}
void GlobalSettings::addRef() { ++m_refCount; }
int GlobalSettings::getRef() { return m_refCount; }
// (23 B): decrement, destroy through the +30 deleting destructor at zero.
int GlobalSettings::release()
{
    const int references = --m_refCount;
    if (!references) delete this;
    return references;
}
// (36 B): IBase (1), then IGlobalSettings (121045).
void* GlobalSettings::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IGlobalSettings)
        return const_cast<GlobalSettings*>(this);
    return nullptr;
}

std::string GlobalSettings::getFileName() { return std::string("Global.con"); }

// (194 B): stringstream(3,1); each row is the literal, the string's c_str, the closing
// quote, endl.
std::string GlobalSettings::getSettingsScript()
{
    std::stringstream script(std::ios::in | std::ios::out);
    script << "GlobalSettings.setDefaultUser \"" << m_defaultUser.c_str() << "\"" << std::endl;
    script << "GlobalSettings.setNamePrefix \"" << m_namePrefix.c_str() << "\"" << std::endl;
    return script.str();
}

// (3 B)
bool GlobalSettings::apply() { return true; }

// (15 B): only the default user is reset; the name prefix survives.
void GlobalSettings::setDefaults() { m_defaultUser = ""; }

// / 5D2000 (the bodies MSVC folded with GameServerSettings' server-name pair)
std::string GlobalSettings::getDefaultUser() { return m_defaultUser; }
void GlobalSettings::setDefaultUser(std::string value) { m_defaultUser = value; }

// (126 B): the stored prefix when it is six characters or fewer, else a fresh string built
// from its first six characters one at a time.
std::string GlobalSettings::getNamePrefix()
{
    if (m_namePrefix.size() <= 6)
        return m_namePrefix;
    std::string prefix;
    for (unsigned int i = 0; i < 6; ++i)
        prefix += m_namePrefix.c_str()[i];
    return prefix;
}

// (61 B): find(8A3510, 0, 1) - a lone 0xA7 byte, the section sign of the Windows-1252
// source - refuses the assignment when the value contains it.
void GlobalSettings::setNamePrefix(std::string value)
{
    if (value.find("\xA7", 0, 1) == std::string::npos)
        m_namePrefix = value;
}

} }
#endif
