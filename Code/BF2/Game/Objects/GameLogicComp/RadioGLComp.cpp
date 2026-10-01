// Radio component playback and command-channel dispatch.

#include "RadioGLComp.h"

#include <cstring>
#include <string>

#include "../RadioGLCompTemplate.h"
#include "../IObject.h"
#include "../IPlayer.h"
#include "../Weapons/GenericFireArm.h"
#include "../../IGame.h"                       // game, TargetChannel, radioMessage/localRadioMessage
#include "../../IGameLogic.h"                  // gameLogic->isServer()
#include "../../../Scene/Object/PlayerManager.h"   // playerManager->getLocalHumanPlayer()
#include "../../../Core/Ids.h"

namespace dice {
namespace hfe {
namespace world {

// The shipped call is deliberately blind: primary-object vslot +0x1C0, which is
// GenericFireArm::getPlayer for the only dynamic type allowed to own this component. The real
// firearm class now lets us preserve that assumption as a typed virtual call.
IPlayer* radioCompOwnerPlayer(IObject* object)
{
    return static_cast<GenericFireArm*>(object)->getPlayer();
}

// (C1) / 0x82237B0 (C2), 44 B each: vptr, refCount = 1, both enabled flags true, the
// template, repaired-latch false, destroyed-latch true (destroyed fires first, then they
// alternate). m_object is NOT initialised - as shipped; init()/setObject() store it before any
// reader runs on the retail component path.
RadioGLComp::RadioGLComp(RadioGLCompTemplate* objectTemplate)
    : m_refCount(1)
    , m_template(objectTemplate)
    , m_failMessageEnabled(true)
    , m_useMessageEnabled(true)
    , m_playRepairedLatch(false)
    , m_playDestroyedLatch(true)
{
}

// vptr store only; no members need teardown.
RadioGLComp::~RadioGLComp()
{
}

// IBase - the standard component quartet (0x8223CD0 / 0x8223CE0 / 0x8223CF0 / 0x8223D30).
// release() dispatches the deleting destructor through the vtable (+0x58, the Itanium D0) and
// tests `this` after decrementing through it, as shipped; `delete this` is the typed spelling.
void RadioGLComp::addRef()  { ++m_refCount; }
int  RadioGLComp::getRef()  { return m_refCount; }

int RadioGLComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

// Three ids answer with `this` (single inheritance - no second base here).
void* RadioGLComp::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectComponent || iid == IID_IRadioGLComp)
        return const_cast<RadioGLComp*>(this);
    return 0;
}

unsigned int RadioGLComp::getInterfaceID() const  { return IID_IRadioGLComp; }

// the base default is empty; this override stores the object, same as setObject.
void RadioGLComp::init(IObject* object)       { m_object = object; }
void RadioGLComp::setObject(IObject* object)  { m_object = object; }
IObject* RadioGLComp::getObject() const       { return m_object; }

void RadioGLComp::handleUpdate(float)         {}                                     // empty

// hands out THE template'S payload block: the address of m_failMessage
// (template+0x10; the five message strings in declaration order). The template header makes this
// class a friend for exactly this field-address take.
void* RadioGLComp::getGLInfo()
{
    return &m_template->m_failMessage;
}

// the appended NON-const slot (+0x5C), constant false. The +0x24 hasUpdate() const
// slot stays IObjectComponent's default (also false).
bool RadioGLComp::hasUpdate()
{
    return false;
}

// is1P - 0x8223BD0, 114 B, NON-virtual. "Does this component's owner warrant a callout": false
// without an object or an owner player; otherwise true for a first-person-view player, or - on
// the server - for an AI player. (The shipped shape is `hasFPV || (isServer && isAI)`; a human
// player on a dedicated server answers false, which is why fail/use callouts are client/AI-only.)
bool RadioGLComp::is1P()
{
    IObject* object = m_object;
    if (!object)
        return false;

    IPlayer* player = radioCompOwnerPlayer(object);      // ELF: object vtable +0x1C0
    if (!player)
        return false;

    return player->hasFirstPersonView()                                  // IPlayer +0x188
        || (gameLogic->isServer() && player->getIsAIPlayer());           // IPlayer +0x64
}

// playFailMessage - 0x8223840, 286 B. One-shot: fires only while enabled, then disarms itself
// (through the virtual setter, +0x3C on self - a plain call reaches the same overrider). The
// disarm happens whenever the message text is non-empty, even IF the player gate declined the
// send - as shipped. Channel 10 = TCLocalTeam; the two trailing bools are (spatial? true,
// bypassSpamCheck false) - the same six-argument radioMessage GameServer implements.
void RadioGLComp::playFailMessage(bool warn)
{
    if (is1P() && warn && getFailMessageEnabled())                       // virtual +0x60 on self
    {
        std::string message = m_template->getFailMessage();              // template +0x30, by value
        if (message.length())
        {
            IObject* object = m_object;
            if (object)
            {
                IPlayer* player = radioCompOwnerPlayer(object);          // +0x1C0
                if (player)
                {
                    if ((gameLogic->isServer() && player->getIsAIPlayer())
                        || playerManager->getLocalHumanPlayer() == player)
                    {
                        game->radioMessage(TCLocalTeam, message, player, player->getId(), true, false);
                    }
                }
            }
            setFailMessageEnabled(false);                                // virtual +0x3C on self
        }
    }
}

// playUseMessage - 0x8223960, 357 B. Same shape as playFailMessage with ONE quirk: the
// bypassSpamCheck argument is true exactly when the message text contains "AUTO_" (the stock
// auto-callout prefix - `std::string::find("AUTO_", 0, strlen("AUTO_")) != npos`, the strlen
// spelled out in the shipped code rather than folded).
void RadioGLComp::playUseMessage(bool warn)
{
    if (is1P() && warn && getUseMessageEnabled())                        // virtual +0x64 on self
    {
        std::string message = m_template->getUseMessage();               // template +0x38, by value
        if (message.length())
        {
            IObject* object = m_object;
            if (object)
            {
                IPlayer* player = radioCompOwnerPlayer(object);          // +0x1C0
                if (player)
                {
                    if ((gameLogic->isServer() && player->getIsAIPlayer())
                        || playerManager->getLocalHumanPlayer() == player)
                    {
                        const bool bypassSpamCheck =
                            message.find("AUTO_", 0, std::strlen("AUTO_")) != std::string::npos;
                        game->radioMessage(TCLocalTeam, message, player, player->getId(), true,
                                           bypassSpamCheck);
                    }
                }
            }
            setUseMessageEnabled(false);                                 // virtual +0x44 on self
        }
    }
}

// The destroyed/repaired latch pair - 0x8223AD0 / 0x8223B50, 126 B each. Each fires only while
// its own latch is armed, then arms the other: destroyed -> repaired -> destroyed, strictly
// alternating, starting with destroyed (ctor). Channel 2 = TCCommander, no player, no first-
// person gate - these are the only server-reachable callouts. The message temporary dies at the
// end of the call statement, before the latch stores - the shipped instruction order.
void RadioGLComp::playDestroyedMessage()
{
    if (m_playDestroyedLatch)
    {
        game->localRadioMessage(TCCommander, m_template->getDestroyedMessage());   // template +0x40
        m_playDestroyedLatch = false;
        m_playRepairedLatch = true;
    }
}

void RadioGLComp::playRepairedMessage()
{
    if (m_playRepairedLatch)
    {
        game->localRadioMessage(TCCommander, m_template->getRepairedMessage());    // template +0x48
        m_playRepairedLatch = false;
        m_playDestroyedLatch = true;
    }
}

// The enabled pair - 0x8223DB0 / 0x8223DF0, 43 B each: the write itself is is1P()-gated, so only
// a first-person/AI owner can re-arm (or pre-disarm) the one-shot callouts. (The ELF leaves
// is1P's result in eax at return - register residue of this exact shape, not a return value; the
// slot is void.) Getters 0x8223DE0 / 0x8223E20.
void RadioGLComp::setFailMessageEnabled(bool enabled)
{
    if (is1P())
        m_failMessageEnabled = enabled;
}

void RadioGLComp::setUseMessageEnabled(bool enabled)
{
    if (is1P())
        m_useMessageEnabled = enabled;
}

bool RadioGLComp::getFailMessageEnabled()  { return m_failMessageEnabled; }
bool RadioGLComp::getUseMessageEnabled()   { return m_useMessageEnabled; }

} // namespace world
} // namespace hfe
} // namespace dice
