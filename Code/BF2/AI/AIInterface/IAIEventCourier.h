// Shared engine/DLL contract. The complete declaration is owned by the recovered courier header;
// do not redeclare a shorter class here. Destructor-first order puts eventMissileLock at ELF
// +0x08 / PE +0x04, commanderOrderAccepted at +0x74 / +0x70 and newSquadLeader at +0x88 / +0x84.
// This forwarding include is [CHOSEN, not recovered]; it changes no virtual order or signature.

#ifndef AI_AIINTERFACE_IAIEVENTCOURIER_H
#define AI_AIINTERFACE_IAIEVENTCOURIER_H

#include "../AIDLL/AI/Main/AIEventCourier.h"

#endif // AI_AIINTERFACE_IAIEVENTCOURIER_H
