// Client console module lifecycle. [INFERRED] Names and source placement.
#include "ClientConsoleModules.h"
#include "HudConsoleBindings.h"
#include "ProfileConsoleBindings.h"
#include <new>
#if !DEDICATED
#include "../Core/Ids.h"
#include "../Core/ConsoleObjects.h"
#include <cstdio>
#include <cstdlib>
namespace dice { namespace hfe { namespace hud {

void HudConsoleModule::addRef() { ++m_refCount; }
int HudConsoleModule::getRef() { return m_refCount; }
int HudConsoleModule::release()
{
    const int references = --m_refCount;
    if (!references) delete this;
    return references;
}
void* HudConsoleModule::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IClassServer || iid == IID_IModule)
        return const_cast<HudConsoleModule*>(this);
    return nullptr;
}
void HudConsoleModule::getClassList(std::vector<ClassRegInfo>&) {} // actual RET4
void HudConsoleModule::getSingletonList(std::vector<SingletonRegInfo>&) {} // actual RET4

// 75C923 registers the complete A1781C list, then returns true.
bool HudConsoleModule::init()
{
    mainConsoleObjects->registerConsoleObjects(hudConsoleObjects());
    return true;
}
// 75C936 unregisters that same list, then returns true.
bool HudConsoleModule::close()
{
    mainConsoleObjects->unregisterConsoleObjects(hudConsoleObjects());
    return true;
}

void ProfileConsoleModule::addRef() { ++m_refCount; }
int ProfileConsoleModule::getRef() { return m_refCount; }
int ProfileConsoleModule::release()
{
    const int references = --m_refCount;
    if (!references) delete this;
    return references;
}
void* ProfileConsoleModule::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IClassServer || iid == IID_IModule)
        return const_cast<ProfileConsoleModule*>(this);
    return nullptr;
}
void ProfileConsoleModule::getClassList(std::vector<ClassRegInfo>&) {} // actual RET4
void ProfileConsoleModule::getSingletonList(std::vector<SingletonRegInfo>&) {} // actual RET4

// allocation/null test, native owner construction, then list registration.
// Use native sizeof: the retail48-byte owner's STL layout is compiler-specific.
bool ProfileConsoleModule::init()
{
    void* allocation = ::operator new(sizeof(ProfileConsoleOwner));
    if (allocation) new (allocation) ProfileConsoleOwner;
    mainConsoleObjects->registerConsoleObjects(profileConsoleBindingList());
    return true;
}
// 7702A6 unregisters the list before deleting the optional A22158 profile owner.
bool ProfileConsoleModule::close()
{
    mainConsoleObjects->unregisterConsoleObjects(profileConsoleBindingList());
    delete profileConsoleOwner;
    return true;
}

} } }
#endif
