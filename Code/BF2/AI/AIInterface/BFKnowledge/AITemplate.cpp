// [INFERRED] Source placement.

#include "AITemplate.h"

#include "AIObjectPrimaryReal.h"
#include "AIObjectPrimaryShared.h"
#include "AIObjectSecondaryReal.h"
#include "AIObjectSecondaryShared.h"
#include "AITemplatePhysical.h"
#include "AITemplatePlugIn.h"
#include "IAIObjectPlugIn.h"
#include "BFGame.h"                                   // IAIGame::getIsPrecaching
#include "InformationType.h"

#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/Ids.h"                        // IID_ISoldier
#include "../../../Game/Objects/IObject.h"            // queryInterface

namespace dice {
namespace hfe {
namespace ai {

// Everything zero, the name copied, no wreck.
AITemplate::AITemplate(const std::string& name)
    : m_plugIns(0), m_plugInMask(0), m_types(0), m_degeneration(0.0f), m_allowedTimeDiff(0.0f),
      m_basicTemp(0.0f), m_commonKnowledge(false), m_secondary(false), m_initialized(false),
      m_name(name), m_wreckTemplate(0)
{
}

// Releases the plug-in array; the manager owns its normal plug-ins. The wreck owns its
// physical plug-in (slot 4), which is deleted before the wreck itself.
AITemplate::~AITemplate()
{
    if (m_wreckTemplate)
    {
        delete m_wreckTemplate->m_plugIns[4];
        delete m_wreckTemplate;
    }
    if (m_plugIns)
        delete[] m_plugIns;
}

// Initializes once outside precaching, setting or clearing the soldier flag from the object.
void AITemplate::initFromObject(world::IObject* object)
{
    if (!m_initialized &&
        !static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getIsPrecaching())    // +0x68
    {
        if (object->queryInterface(world::IID_ISoldier))
            m_types |= 1 << ITSoldier;
        else
            m_types &= ~(1 << ITSoldier);
        m_initialized = true;
    }
}

// Creates an AI object for the supplied side and engine object.
// The secondary and commonKnowledge flags select its concrete type.
IAIObject* AITemplate::createObject(int side, world::IObject* object)
{
    IAIObject* aiObject;
    if (m_secondary)
    {
        if (m_commonKnowledge)
            aiObject = checkPointer(new AIObjectSecondaryShared(this, side, object));
        else
            aiObject = checkPointer(new AIObjectSecondaryReal(this, side, object));
    }
    else if (m_commonKnowledge)
    {
        aiObject = checkPointer(new AIObjectPrimaryShared(this, side, object));
    }
    else
    {
        aiObject = checkPointer(new AIObjectPrimaryReal(this, side, object));
    }

    if (m_plugIns)
    {
        for (int i = 0; i < 6; ++i)
        {
            if (m_plugIns[i])
                aiObject->addPlugIn(m_plugIns[i]->createPlugIn(aiObject, this));
        }
    }
    return aiObject;
}

// Lazily allocates six plug-in slots and updates their mask. Replacing a slot deletes the
// old plug-in without removing its manager-map entry. Preserves repeated getId calls.
void AITemplate::addPlugIn(AITemplatePlugIn* plugIn)
{
    if (!m_plugIns)
    {
        m_plugIns = checkPointer(new AITemplatePlugIn*[6]);
        for (int i = 0; i <= 5; ++i)
            m_plugIns[i] = 0;
    }

    if (m_plugIns[plugIn->getId()])
        delete m_plugIns[plugIn->getId()];

    m_plugIns[plugIn->getId()] = plugIn;
    m_plugInMask |= 1 << plugIn->getId();
}

// The five tests read the array without a null test - a template with no plug-in at all faults on
// any of them. Reproduced (, 20- each).
bool AITemplate::isArmament() const { return m_plugIns[0] != 0; }
bool AITemplate::isCover() const    { return m_plugIns[2] != 0; }
bool AITemplate::isPhysical() const { return m_plugIns[4] != 0; }
bool AITemplate::isMobile() const   { return m_plugIns[3] != 0; }
bool AITemplate::isUnit() const     { return m_plugIns[5] != 0; }

// Creates this template's wreck template when applicable.
bool AITemplate::createWreckTemplate()
{
    if (!(m_types & (1 << ITHasWreck)))
        return false;

    m_wreckTemplate = checkPointer(new AITemplate(m_name + "_wreck"));
    m_wreckTemplate->m_types |= 1 << ITObstacle;
    m_wreckTemplate->m_degeneration = 20.0f;
    m_wreckTemplate->m_basicTemp = 1.0f;
    m_wreckTemplate->m_commonKnowledge = true;
    m_wreckTemplate->m_allowedTimeDiff = 10.0f;

    AITemplatePhysical* const physical = checkPointer(new AITemplatePhysical);
    physical->m_strType = static_cast<AITemplatePhysical*>(m_plugIns[4])->m_strType;
    m_wreckTemplate->addPlugIn(physical);
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
