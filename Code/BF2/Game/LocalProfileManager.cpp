// [INFERRED] path; class registry name dice.hfe.LocalProfileManager is attested.
// Full PE factory/lifetime reads retained under local_profile_lifetime_20260906.
#include "LocalProfileManager.h"
#if !DEDICATED
#include "../Core/Ids.h"
#include "../Core/ClassManager.h"
#include "../Core/EventManager.h"
#include "ProfileManager.h"
#include "../Menu/ISwiffHost.h"
#include <cstdio>
#include <cstdlib>
#include <new>

namespace dice { namespace hfe {
extern ClassManager* classManager;

LocalProfile::LocalProfile() : totalPlayedTime(0.0f), numTimesLoggedIn(0) {}

LocalProfileManager::LocalProfileManager()
    : m_refCount(1), m_currentProfile(nullptr), m_hasDefaultProfile(false),
      m_profilesDirty(true), m_filterValidProfiles(false), m_field34(-1), m_field54(false)
{
    // Native vectors and string default-construct. Retail allocator words and
    // padding are not explicit stores; the source performs no event registration.
}

IBase* LocalProfileManager::create(unsigned int iid, IBase* outer)
{
    if (outer) return nullptr;
    void* allocation = ::operator new(sizeof(LocalProfileManager));
    LocalProfileManager* manager = allocation ? new (allocation) LocalProfileManager : nullptr;
    // The PE has no null check before queryInterface, including allocation failure.
    IBase* result = static_cast<IBase*>(manager->queryInterface(iid));
    if (!result) manager->release();
    return result;
}

void LocalProfileManager::addRef() { ++m_refCount; }
int LocalProfileManager::getRef() { return m_refCount; }
int LocalProfileManager::release()
{
    const int references = --m_refCount;
    if (!references) delete this; // final concrete owner; PE dispatches deleting dtor+D4
    return references;
}
void* LocalProfileManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_ILocalProfileManager)
        return const_cast<LocalProfileManager*>(this);
    return nullptr;
}

void LocalProfileManager::handleEvent(EventCategory, unsigned int, EventNode*, void*)
{
    // Actual client secondary-table body5B74B0 is RET16.
}

void LocalProfileManager::shutdown()
{
    IBase* online = classManager->getSingleton(std::string("ProfileManager"));
    if (online) {
        // PE4A8226..4A825D: discard logout's result, then perform a fresh host
        // lookup and reset its stats cache only when that host exists.
        static_cast<IProfileManager*>(online)->logout();
        IBase* host = classManager->getSingleton(std::string("SwiffHost"));
        if (host) static_cast<ui::ISwiffHost*>(host)->resetStatsCache();
    }
    m_field34 = -1;
    m_field38 = "";
    if (m_currentProfile) {
        delete m_currentProfile;
        m_currentProfile = nullptr;
    }
    eventManager->unregisterEventHandler(ECGame, static_cast<IEventListener*>(this));
    m_profilesDirty = true;
}

LocalProfileManager::~LocalProfileManager()
{
    shutdown();
    // Actual member destruction follows: string38, vector24, then vector10.
}

} }
#endif
