// Mission object registered with GameLogic and associated with a parent weapon.
// Team lookup follows the root player-control object; setTeam refreshes registration.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_REMOTECONTROLLEDOBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_REMOTECONTROLLEDOBJECT_H

#include <list>

#include "../Base/Bundle.h"

namespace dice {
namespace hfe {
class GameLogic;
namespace world {

class RemoteControlledObjectTemplate;
class IWeaponObject;
class ArtilleryHandler;

// (the RemoteControlledObject TU's free function): every object in `root`'s
// subtree that HAS a template and answers IID_IWeaponObject, root first, then breadth-first;
// a subtree under an object answering IID_IPlayerControlObject is not entered.
void findWeapons(IObject* root, std::list<IObject*>& out);

class RemoteControlledObject : public Bundle
{
public:
    explicit RemoteControlledObject(const RemoteControlledObjectTemplate* objectTemplate);
    virtual ~RemoteControlledObject();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual void  init();
    virtual void  disableCachedObject();
    virtual void  enableCachedObject();
    virtual void  destroy(bool force);
    virtual void  handleMessage(TemplateMessage, IPlayer*, bool);

    // the three own slots, +0x180..+0x188
    virtual void  initMission(Vec3&);
    virtual int   updateMission(float);                                      // 0
    virtual int   getStatus() const;                                        // 0

    // non-virtual
    void addToGameLogic();
    void removeFromGameLogic();
    void setTeam(int team);
    int  getTeam() const;

protected:
    friend class ArtilleryHandler;
    friend class ::dice::hfe::GameLogic;
    /* 0x13C */ int   m_refCount;
    /* 0x140 */ const RemoteControlledObjectTemplate* m_template;
    /* 0x144 */ int   m_objectId;
    /* 0x148 */ IObject* m_owner;
    /* 0x14C */ IWeaponObject* m_weapon;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_REMOTECONTROLLEDOBJECT_H
