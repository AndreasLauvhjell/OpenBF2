// Target-object lifecycle and activation.

#include "TargetObject.h"

#include "../../../Core/Ids.h"
#include "../../../Core/Debug.h"
#include "../../../Core/StringUtil.h"            // formatString - the Debug arm
#include "../../../Core/System.h"                // WorldPref::getTickCount - handleUpdate
#include "../../../IO/Network/INetworkable.h"     // fixed-width networkable id
#include "../../IGameLogic.h"                    // gameLogic - removeTargetObject (+0x1B4)
#include "../../GameLogic/GameLogic.h"           // the m_targetObjectIdCounter poke (+0x0C)
#include "../../../Scene/Object/ObjectManager.h" // objectManager - postMessage (+0xD0)
#include "../../../Scene/Object/ObjectUtil.h"    // setNetworkableComp
#include "../../../Game/IGame.h"                  // game - the IGameServer query (setTargetActive)
#include "../../../Scene/Object/PlayerManager.h"   // playerManager->getLocalHumanPlayer
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"   // getOccupingPlayer
#include "../IPlayer.h"                             // getTeam
#include "../IObjectComponent.h"                    // the armor component word
#include "../GameLogicComp/IArmorGLComp.h"          // getStatus - wreck gate
#include "../ObjectLayoutAccess.h"                 // +0x3C armorComponent
#include "../TargetObjectTemplate.h"
#include <map>

namespace dice {
namespace hfe {
namespace world {

// Initializes identically on client and server, leaving the word at +0x150 untouched.
TargetObject::TargetObject(const TargetObjectTemplate* objectTemplate)
    : Bundle(objectTemplate)
{
    m_template = objectTemplate;                 // +0x13C
    m_targetObjectId = -1;                       // +0x140
    m_registeredWithGameLogic = false;           // +0x144 (byte store)
    m_targetActive = false;                      // +0x145 (byte store)
    m_notifyOnDeactivate = false;                // +0x146 (byte store)
    m_activatedTick = 0;                         // +0x148
    m_targetPlayerId = -1;                       // +0x14C
    // +0x150: not written, as shipped
    m_emptySinceTick = 0;                        // +0x154
    m_occupantTeam = -1;                         // +0x158
}

// /0x81C0270/0x81C02A0 - vptr restores + Bundle::~Bundle only.
TargetObject::~TargetObject() {}

// init - 0x81C0330, 66 B. Bundle::init, then message 29 to self at delay 0.0 - the target wants
// its id resolved and its status machine started on the very next pump, not after a grace second.
void TargetObject::init()
{
    Bundle::init();
    objectManager->postMessage(this, static_cast<TemplateMessage>(29), 0.0f);
}

// Unregisters from GameLogic before base destruction; pending message 29 is not cancelled.
void TargetObject::destroy(bool force)
{
    if (m_registeredWithGameLogic)
    {
        m_registeredWithGameLogic = false;
        gameLogic->removeTargetObject(this);     // IGameLogic slot +0x1B4, recovered registry body
    }
    SimpleObject::destroy(force);
}

// handleUpdate - 0x81C05E0, 132 B. replaces SimpleObject::handleUpdate outright - no base
// call, both parameters unused. The whole body is the time-to-live expiry check: an active target
// whose template sets a finite positive timeToLive deactivates (with notify) once
// activation-tick + timeToLiveTicks falls behind the world tick counter.
// The tick comparison is unsigned in the ELF (`jnb`); getTickCount returns the tree's committed
// `int`, so the cast reproduces the wraparound behaviour exactly.
void TargetObject::handleUpdate(float, unsigned int)
{
    if (m_template->timeToLive != -1.0f && m_targetActive && m_template->timeToLive > 0.0f)
    {
        if (m_activatedTick + m_template->timeToLiveTicks <
            static_cast<unsigned int>(WorldPref::getTickCount()))
        {
            setTargetActive(false, true);
        }
    }
}

// Message 29 resolves the target ID; every message updates activation status.
void TargetObject::handleMessage(TemplateMessage message, IPlayer* player, bool)
{
    if (message == static_cast<TemplateMessage>(29))
        initTargetId();
    updateActiveStatus(message, player);
}

// initNetworkableComp - 0x81C0380, 373 B, the sibling shape with this family's constants:
// CID_TargetObjectNetworkable (39018 in Ids.h; its complete factory/contract is registered in
// ModuleNetworkables). Stock UniqueTarget leaves NetworkableInfo empty and therefore stays local;
// LaserPainterTarget configures LaserTargetInfo and takes this live component arm. Gate spelled
// with the out-of-line compare the codegen shows (SupplyObject's inlined the length read instead).
void TargetObject::initNetworkableComp()
{
    if (m_template && m_template->getNetworkableInfo().compare("") != 0
        && !setNetworkableComp(this, CID_TargetObjectNetworkable))
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Game/TargetObject.cpp", 80, __FUNCTION__)
            << formatString("Failed to create TargetObjectNetworkable");
    }
}

// Initializes the target id from this object's network component, its root's component, or a fallback counter.
// Returns false only when the fallback produces the -1 sentinel.
bool TargetObject::initTargetId()
{
    if (m_targetObjectId != -1)
        return true;

    if (getNetworkable())
    {
        m_targetObjectId = getNetworkable()->m_networkableId;
    }
    else
    {
        io::INetworkable* rootNetworkable = getRootParent(this)->getNetworkable();
        if (rootNetworkable)
            m_targetObjectId = rootNetworkable->m_networkableId;
    }

    if (m_targetObjectId != -1)
        return true;

    const int c = static_cast<GameLogic*>(gameLogic)->m_targetObjectIdCounter++;
    m_targetObjectId = c - 10000;
    return c != 9999;
}

// active AND the root parent not flag-bit-0 (hidden). The root read goes
// through the public getFlags accessor; the ELF reads the +0x04 dword's low byte directly.
bool TargetObject::getTargetActive()
{
    if (!m_targetActive)
        return false;
    return ((getRootParent(this)->getFlags() ^ 1u) & 1u) != 0;
}

int TargetObject::getTargetType() const { return m_template->targetType; }
int TargetObject::getTargetObjectId()   { return m_targetObjectId; }
int TargetObject::getTargetPlayerId() const { return m_targetPlayerId; }
void TargetObject::setTargetPlayerId(int playerId) { m_targetPlayerId = playerId; }

// Collects player-control objects in the hierarchy by seat id for target occupancy checks.
void findPlayerControlObjects(IObject* root, std::map<int, IPlayerControlObject*>& out);

void TargetObject::updateActiveStatus(TemplateMessage message, IPlayer* player)
{
    if (!gameLogic->isServer())
    {
        if (!m_template->getNetworkableInfo().empty())                                // template +0x94, the string's length
            return;
    }
    const int m = static_cast<int>(message);
    if (m == 30)
        setTargetActive(false, true);
    bool active = true;
    IObject* root = getRootParent(this);
    bool decided = false;
    if (m_template->disableWhenWreck)                                                // +0x188
    {
        IObjectComponent* armor = 0;
        for (IObject* walk = this; walk; walk = walk->getParent())
        {
            armor = object_detail::LayoutAccess::armorComponent(walk);                // +0x3C
            if (armor)
                break;
        }
        if (armor && static_cast<IArmorGLComp*>(armor)->getStatus() == 1)
        {
            active = false;
            decided = true;
        }
    }
    if (!decided && m_template->disableWhenEmptyVehicle)                             // +0x189
    {
        std::map<int, IPlayerControlObject*> pcos;
        findPlayerControlObjects(root, pcos);
        bool occupied = false;
        std::map<int, IPlayerControlObject*>::iterator first = pcos.begin();
        if (first != pcos.end() && first->second->getOccupingPlayer()                // ipco +0x10
            && (m != 3 || first->second->getOccupingPlayer() != player))
        {
            occupied = true;
            if (player)
                m_occupantTeam = player->getTeam();                                  // +0x158 <- IPlayer +0xE8
        }
        if (pcos.empty() || occupied)
        {
            m_emptySinceTick = 0;                                                    // +0x154
        }
        else if (!m_targetActive)
        {
            active = false;
        }
        else
        {
            if (!m_emptySinceTick)
                m_emptySinceTick = WorldPref::getTickCount();
            float delay = m_template->disableWhenEmptyVehicleDelay;                  // +0x18C
            if (IPlayer* local = playerManager->getLocalHumanPlayer())              // +0x30
                if (local->getTeam() == m_occupantTeam)
                    delay = -1.0f;
            const float deadline = delay * 30.0f + static_cast<float>(m_emptySinceTick);
            if (static_cast<float>(WorldPref::getTickCount()) <= deadline)
            {
                active = m_targetActive;
                objectManager->postMessage(this, static_cast<TemplateMessage>(31), 2.0f);   // +0xD0
            }
            else
                active = false;
        }
    }
    if (m_targetActive != active && (m_targetActive || !m_notifyOnDeactivate))
        setTargetActive(active, false);
}

void TargetObject::setTargetActive(bool active, bool notify)
{
    if (m_targetObjectId == -1)
        initTargetId();
    m_notifyOnDeactivate = notify;                                                   // +0x146
    if (active)
    {
        if (!m_registeredWithGameLogic)
        {
            m_registeredWithGameLogic = true;
            gameLogic->addTargetObject(this);                                        // +0x1AC
        }
        if (game->queryInterface(IID_IGameServer))
        {
            m_activatedTick = WorldPref::getTickCount();                            // +0x148
            if (m_template->timeToLive != -1.0f)                                    // +0x180
                updateFlags(0x800u, 0u);
        }
    }
    else
    {
        if (m_registeredWithGameLogic)
        {
            m_registeredWithGameLogic = false;
            gameLogic->removeTargetObject(this);                                     // +0x1B4
        }
        if (m_template->disableWhenEmptyVehicle || m_template->disableWhenWreck)
            updateFlags(0u, 0x800u);
    }
    m_emptySinceTick = 0;
    m_targetActive = active;
}

// Still unwritten, sizes in the header: addToNametagManager (329 B), removeFromNametagManager
// (238 B) - client-side nametags.

} // namespace world
} // namespace hfe
} // namespace dice
