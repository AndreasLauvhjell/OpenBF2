// [INFERRED] Source placement.

#include "AITemplateArmament.h"

#include <vector>

#include "AIObjectArmament.h"
#include "BFGame.h"                                          // IAIGame::getIsPrecaching (+0x68)

#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Ids.h"
#include "../../../Game/Objects/IObject.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {
namespace ai {

// the vptr, then `mov byte [+4], 0` and `mov dword [+8], 0`.
AITemplateArmament::AITemplateArmament()
    : m_initialized(false), m_isAntiAircraft(0)
{
}

// - the vptr store and, for , the delete. Nothing else.
AITemplateArmament::~AITemplateArmament()
{
}

// createPlugIn - , / : `new AIObjectArmament(object, this)` (0x2C B
// in the ELF, 0x30 in the PE) through checkPointer.
IAIObjectPlugIn* AITemplateArmament::createPlugIn(IAIObject* object, AITemplate* /*aiTemplate*/)
{
    return checkPointer(new AIObjectArmament(object, this));
}

// `xor eax, eax / ret`.
int AITemplateArmament::getId()
{
    return 0;
}

// Once, and never while precaching.
void AITemplateArmament::initFromObject(world::IObject* object)
{
    if (!m_initialized &&
        !static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getIsPrecaching())    // +0x68
    {
        traverseObjects(object);
        m_initialized = true;
    }
}

// For an engine object (IID_IObject): if it is a
// player control object, ask each of its weapons (getWeapons, the flat list) for the IID_IWeapon
// component - and discard it; then recurse into the children along the sibling chain (+0x2C, read
// directly). The control-object query runs twice, as shipped; the weapons are not null-tested.
void AITemplateArmament::traverseObjects(world::IObject* object)
{
    if (!object->queryInterface(world::IID_IObject))
        return;

    if (object->queryInterface(world::IID_IPlayerControlObject))
    {
        world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
            object->queryInterface(world::IID_IPlayerControlObject));
        // getWeapons answers the control object's flat weapon vector (IPlayerControlObject.h
        // [RETIRE-WHEN: pco-getweapons-vector]).
        const std::vector<world::IObject*>& weapons =
            *static_cast<const std::vector<world::IObject*>*>(pco->getWeapons());
        for (std::vector<world::IObject*>::const_iterator it = weapons.begin(); it != weapons.end(); ++it)
            (*it)->queryComponent(IID_IWeapon, IID_IWeapon);                           // result unused
    }

    for (world::IObject* child = object->getChild(); child; child = child->getSibling())
        traverseObjects(child);
}

} // namespace ai
} // namespace hfe
} // namespace dice
