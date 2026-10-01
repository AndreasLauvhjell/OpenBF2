// Vehicle-specific action-plan primitives registered by statement type.
// Keep registry spellings such as Infantery and TriggerContinously unchanged.
// [CHOSEN, not recovered] These entries are grouped into one translation unit.
// [FRONTIER] Remaining execute methods stop by name.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate execution, then split entries into their own files.

#ifndef AI_AIDLL_AI_AIPLANINTERPRETER_BAPENTRIES_H
#define AI_AIDLL_AI_AIPLANINTERPRETER_BAPENTRIES_H

#include "BAPEntry.h"

namespace dice {
namespace hfe {
namespace ai {


// EntryTankMoveTo - statement type 7.
class EntryTankMoveTo : public BAPEntry
{
public:
    EntryTankMoveTo();
    virtual ~EntryTankMoveTo();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryTankMoveToObject - statement type 9.
class EntryTankMoveToObject : public BAPEntry
{
public:
    EntryTankMoveToObject();
    virtual ~EntryTankMoveToObject();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryTankMoveToDirection - statement type 8.
class EntryTankMoveToDirection : public BAPEntry
{
public:
    EntryTankMoveToDirection();
    virtual ~EntryTankMoveToDirection();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryTankResetControls - statement type 14.
class EntryTankResetControls : public BAPEntry
{
public:
    EntryTankResetControls();
    virtual ~EntryTankResetControls();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryTankTurnTo - statement type 24.
class EntryTankTurnTo : public BAPEntry
{
public:
    EntryTankTurnTo();
    virtual ~EntryTankTurnTo();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
    // Everything else on these
    // forty-two classes is the four slots below.
    void getCentreOfFacingSide(const Vec2&, const Vec2&, const Vec2&, const Vec2&, Vec2&);
};

// EntryCarMoveTo - statement type 7.
class EntryCarMoveTo : public BAPEntry
{
public:
    EntryCarMoveTo();
    virtual ~EntryCarMoveTo();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryCarMoveToDirection - statement type 8.
class EntryCarMoveToDirection : public BAPEntry
{
public:
    EntryCarMoveToDirection();
    virtual ~EntryCarMoveToDirection();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryCarMoveToObject - statement type 9.
class EntryCarMoveToObject : public BAPEntry
{
public:
    EntryCarMoveToObject();
    virtual ~EntryCarMoveToObject();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryCarResetControls - statement type 14.
class EntryCarResetControls : public BAPEntry
{
public:
    EntryCarResetControls();
    virtual ~EntryCarResetControls();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryPlaneMoveTo - statement type 7.
class EntryPlaneMoveTo : public BAPEntry
{
public:
    EntryPlaneMoveTo();
    virtual ~EntryPlaneMoveTo();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryPlaneMoveToDirection - statement type 8.
class EntryPlaneMoveToDirection : public BAPEntry
{
public:
    EntryPlaneMoveToDirection();
    virtual ~EntryPlaneMoveToDirection();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryPlaneMoveToObject - statement type 9.
class EntryPlaneMoveToObject : public BAPEntry
{
public:
    EntryPlaneMoveToObject();
    virtual ~EntryPlaneMoveToObject();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryPlaneAimAt - statement type 6.
class EntryPlaneAimAt : public BAPEntry
{
public:
    EntryPlaneAimAt();
    virtual ~EntryPlaneAimAt();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryPlaneResetControls - statement type 14.
class EntryPlaneResetControls : public BAPEntry
{
public:
    EntryPlaneResetControls();
    virtual ~EntryPlaneResetControls();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryBoatMoveTo - statement type 7.
class EntryBoatMoveTo : public BAPEntry
{
public:
    EntryBoatMoveTo();
    virtual ~EntryBoatMoveTo();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryBoatMoveToObject - statement type 9.
class EntryBoatMoveToObject : public BAPEntry
{
public:
    EntryBoatMoveToObject();
    virtual ~EntryBoatMoveToObject();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryBoatMoveToDirection - statement type 8.
class EntryBoatMoveToDirection : public BAPEntry
{
public:
    EntryBoatMoveToDirection();
    virtual ~EntryBoatMoveToDirection();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryBoatResetControls - statement type 14.
class EntryBoatResetControls : public BAPEntry
{
public:
    EntryBoatResetControls();
    virtual ~EntryBoatResetControls();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryInfanteryMoveTo - statement type 7.
class EntryInfanteryMoveTo : public BAPEntry
{
public:
    EntryInfanteryMoveTo();
    virtual ~EntryInfanteryMoveTo();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryInfanteryMoveToObject - statement type 9.
class EntryInfanteryMoveToObject : public BAPEntry
{
public:
    EntryInfanteryMoveToObject();
    virtual ~EntryInfanteryMoveToObject();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryInfanteryMoveToDirection - statement type 8.
class EntryInfanteryMoveToDirection : public BAPEntry
{
public:
    EntryInfanteryMoveToDirection();
    virtual ~EntryInfanteryMoveToDirection();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryInfanteryResetControls - statement type 14.
class EntryInfanteryResetControls : public BAPEntry
{
public:
    EntryInfanteryResetControls();
    virtual ~EntryInfanteryResetControls();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryTrigger - statement type 11.
class EntryTrigger : public BAPEntry
{
public:
    EntryTrigger();
    virtual ~EntryTrigger();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryTriggerContinously - statement type 12.
class EntryTriggerContinously : public BAPEntry
{
public:
    EntryTriggerContinously();
    virtual ~EntryTriggerContinously();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryKeyTurretAimAt - statement type 6.
class EntryKeyTurretAimAt : public BAPEntry
{
public:
    EntryKeyTurretAimAt();
    virtual ~EntryKeyTurretAimAt();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryMouseTurretAimAt - statement type 6.
class EntryMouseTurretAimAt : public BAPEntry
{
public:
    EntryMouseTurretAimAt();
    virtual ~EntryMouseTurretAimAt();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryMouseTurretLookAt - statement type 23.
class EntryMouseTurretLookAt : public BAPEntry
{
public:
    EntryMouseTurretLookAt();
    virtual ~EntryMouseTurretLookAt();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntrySense - statement type 20.
class EntrySense : public BAPEntry
{
public:
    EntrySense();
    virtual ~EntrySense();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntrySoldierPose - statement type 26.
class EntrySoldierPose : public BAPEntry
{
public:
    EntrySoldierPose();
    virtual ~EntrySoldierPose();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryMoveToMedium - statement type 7.
class EntryMoveToMedium : public BAPEntry
{
public:
    EntryMoveToMedium();
    virtual ~EntryMoveToMedium();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryMoveToMediumBoat - statement type 7.
class EntryMoveToMediumBoat : public BAPEntry
{
public:
    EntryMoveToMediumBoat();
    virtual ~EntryMoveToMediumBoat();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryMoveToMediumSoldier - statement type 7.
class EntryMoveToMediumSoldier : public BAPEntry
{
public:
    EntryMoveToMediumSoldier();
    virtual ~EntryMoveToMediumSoldier();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryMoveToObjectMedium - statement type 9.
class EntryMoveToObjectMedium : public BAPEntry
{
public:
    EntryMoveToObjectMedium();
    virtual ~EntryMoveToObjectMedium();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryMoveToObjectMediumBoat - statement type 9.
class EntryMoveToObjectMediumBoat : public BAPEntry
{
public:
    EntryMoveToObjectMediumBoat();
    virtual ~EntryMoveToObjectMediumBoat();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryMoveToObjectMediumSoldier - statement type 9.
class EntryMoveToObjectMediumSoldier : public BAPEntry
{
public:
    EntryMoveToObjectMediumSoldier();
    virtual ~EntryMoveToObjectMediumSoldier();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryInfoWrapper - statement type 15.
class EntryInfoWrapper : public BAPEntry
{
public:
    EntryInfoWrapper();
    virtual ~EntryInfoWrapper();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryHeliMoveTo - statement type 7.
class EntryHeliMoveTo : public BAPEntry
{
public:
    EntryHeliMoveTo();
    virtual ~EntryHeliMoveTo();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryHeliMoveToDirection - statement type 8.
class EntryHeliMoveToDirection : public BAPEntry
{
public:
    EntryHeliMoveToDirection();
    virtual ~EntryHeliMoveToDirection();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryHeliMoveToObject - statement type 9.
class EntryHeliMoveToObject : public BAPEntry
{
public:
    EntryHeliMoveToObject();
    virtual ~EntryHeliMoveToObject();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryHeliAimAt - statement type 6.
class EntryHeliAimAt : public BAPEntry
{
public:
    EntryHeliAimAt();
    virtual ~EntryHeliAimAt();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryHeliResetControls - statement type 14.
class EntryHeliResetControls : public BAPEntry
{
public:
    EntryHeliResetControls();
    virtual ~EntryHeliResetControls();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};

// EntryHeliMoveAwayFromObject - statement type 10.
class EntryHeliMoveAwayFromObject : public BAPEntry
{
public:
    EntryHeliMoveAwayFromObject();
    virtual ~EntryHeliMoveAwayFromObject();

    virtual bool execute(BotScript *, Bot *, bool&, bool&);
    virtual int getTargetId();
};


} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPLANINTERPRETER_BAPENTRIES_H
