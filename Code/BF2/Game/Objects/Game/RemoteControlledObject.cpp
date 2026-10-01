// Remote-controlled object lifecycle and team state.

#include "RemoteControlledObject.h"

#include "../../../Core/Ids.h"
#include "../../IGameLogic.h"                                // gameLogic - add / removeRemoteControlledObject
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"   // getTeam (+0x98)
#include "../IWeaponObject.h"
#include "../ObjectLayoutAccess.h"                          // the +0x2C sibling, +0x14 template
#include "../RemoteControlledObjectTemplate.h"

namespace dice {
namespace hfe {
namespace world {

// findWeapons - 0x81A4E40, 457 B. The root itself when it has a template (+0x14) and answers
// IID_IWeaponObject - and then NO walk; otherwise a breadth-first walk from the root: a child
// with a template answering IID_IWeaponObject is collected (and not descended into), any other
// child is queued unless it answers IID_IPlayerControlObject (a nested vehicle position is a
// boundary). Note the asymmetry, as read: a matching root ends the search at once.
void findWeapons(IObject* root, std::list<IObject*>& out)
{
    if (!root)
        return;
    if (object_detail::LayoutAccess::objectTemplate(root) && root->queryInterface(IID_IWeaponObject))
    {
        out.push_back(root);
        return;
    }
    std::list<IObject*> work;
    work.push_back(root);
    while (!work.empty())
    {
        IObject* node = work.front();
        IObject* child = node->getChild();                                           // +0xF0
        work.pop_front();
        for (; child; child = object_detail::LayoutAccess::sibling(child))           // +0x2C
        {
            if (object_detail::LayoutAccess::objectTemplate(child) && child->queryInterface(IID_IWeaponObject))
            {
                out.push_back(child);
                continue;
            }
            if (!child->queryInterface(IID_IPlayerControlObject))
                work.push_back(child);
        }
    }
}

// Bundle(template); the vptrs; count 1; the template; id -1; owner and weapon 0.
RemoteControlledObject::RemoteControlledObject(const RemoteControlledObjectTemplate* objectTemplate)
    : Bundle(objectTemplate)
    , m_refCount(1)                                                                  // +0x13C
    , m_template(objectTemplate)                                                     // +0x140
    , m_objectId(-1)                                                                 // +0x144
    , m_owner(0)                                                                     // +0x148
    , m_weapon(0)                                                                    // +0x14C
{
}

RemoteControlledObject::~RemoteControlledObject() {}                                 // ~Bundle

// The quartet - 0x81A4BA0 / 0x81A4BD0 / 0x81A4BF0 / 0x81A4C40.
void RemoteControlledObject::addRef()  { ++m_refCount; }
int  RemoteControlledObject::getRef()  { return m_refCount; }
int  RemoteControlledObject::release()
{
    const int left = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = left;
    if (last)
        delete this;                                                                 // the D0 slot
    return left;
}

void* RemoteControlledObject::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObject)
        return const_cast<RemoteControlledObject*>(this);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<RemoteControlledObject*>(this));   // this + 0x80
    return 0;
}

void RemoteControlledObject::init()
{
    Bundle::init();
    m_objectId = static_cast<int>(getObjectId());                                    // +0x10
    m_owner = m_parent;                                                              // +0x28
    std::list<IObject*> weapons;
    findWeapons(m_owner, weapons);
    if (!weapons.empty())
        m_weapon = static_cast<IWeaponObject*>(weapons.front()->queryInterface(IID_IWeaponObject)); // +0x14C
    addToGameLogic();
}


void RemoteControlledObject::destroy(bool force)
{
    removeFromGameLogic();
    SimpleObject::destroy(force);                                                    // Bundle's is skipped
}

void RemoteControlledObject::enableCachedObject()
{
    addToGameLogic();
    Bundle::enableCachedObject();
}

void RemoteControlledObject::disableCachedObject()
{
    removeFromGameLogic();
    SimpleObject::disableCachedObject();                                             // Bundle's is skipped
}


void RemoteControlledObject::addToGameLogic()
{
    if (m_objectId != -1)
        gameLogic->addRemoteControlledObject(this);                                  // +0x1C0
}

void RemoteControlledObject::removeFromGameLogic()
{
    gameLogic->removeRemoteControlledObject(this);                                   // +0x1C8
}

void RemoteControlledObject::setTeam(int)
{
    addToGameLogic();
}

// the root's IPlayerControlObject team, 0 without one.
int RemoteControlledObject::getTeam() const
{
    IObject* root = getRootParent(this);
    IPlayerControlObject* pco = static_cast<IPlayerControlObject*>(root->queryInterface(IID_IPlayerControlObject));
    return pco ? pco->getTeam() : 0;                                                 // ipco +0x98
}

// Bundle's.
void RemoteControlledObject::handleMessage(TemplateMessage message, IPlayer* player, bool flag)
{
    Bundle::handleMessage(message, player, flag);
}

// The three own slots' defaults.
void RemoteControlledObject::initMission(Vec3&)      {}
int  RemoteControlledObject::updateMission(float)    { return 0; }
int  RemoteControlledObject::getStatus() const       { return 0; }

} // namespace world
} // namespace hfe
} // namespace dice
