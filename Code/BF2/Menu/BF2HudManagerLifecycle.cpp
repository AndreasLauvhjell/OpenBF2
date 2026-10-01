// HUD lifecycle. [INFERRED] Source split from Menu/BF2HudManager.cpp.
#include "BF2HudManager.h"
#if !DEDICATED
#include "HudVoip.h"
#include "Ref2System.h"
#include "Bf2HudBuilder.h"
#include "HudBlinker.h"
#include "HudInformationAdapter.h"
#include "HudQuestionManager.h"
#include "HudSquad.h"
#include "HudCommander.h"
#include "HudMessages.h"
#include "HudChat.h"
#include "HudRadio.h"
#include "HudKitSelection.h"
#include "HudScoreboard.h"
#include "HudVoteMap.h"
#include "HudAmmo.h"
#include "HudVehicleInformation.h"
#include "HudLoadingScreen.h"
#include "../Core/EventManager.h"
#include "../IO/Input/Input.h"
#include "../Sound/NullSound.h"

namespace dice { namespace hfe { namespace io { extern IBase* inputManager; } } }
namespace dice { namespace hfe { namespace hud {

BF2HudManager::~BF2HudManager() // 756980, deleting thunk758400
{
    delete m_field2dc;
    if (ref2System) {
        ref2System->close();
        // The close callback may replace the field; retail reloads before delete.
        delete ref2System;
    }
    static_cast<io::IInputManager*>(io::inputManager)->unregisterInputFilter(this);
    eventManager->unregisterEventHandler(static_cast<EventCategory>(2), this);
    eventManager->unregisterEventHandler(static_cast<EventCategory>(1), this);
    eventManager->unregisterEventHandler(static_cast<EventCategory>(0), this);
    eventManager->unregisterEventHandler(static_cast<EventCategory>(10), this);
    delete m_field104;
    delete m_field138;
    delete m_field13c;
    delete m_field0fc;
    delete m_field100;
    delete m_field148;
    delete hudChat;
    delete m_field140;
    delete m_field110;
    delete m_field114;
    delete m_field118;
    delete m_field108;
    delete m_field10c;
    delete m_field11c;
    delete m_field120;
    delete m_field124; // nonvirtual dtor 7A4DD0 then operator delete
    delete m_field128;
    delete hudAmmo;
    delete hudVehicleInformation;
    delete m_field134;
    if (m_field2f0) m_field2f0->release();
    // Native members destruct in their declared reverse order. Retail does not
    // delete +14C, clear the external HUD singleton, or reset these owner fields.
}

} } }
#endif
