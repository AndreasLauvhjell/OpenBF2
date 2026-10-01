// Team scoring interface and configuration. [INFERRED] header placement beside
// the attested implementation. The class is accessed through its typed virtual API.

#ifndef GAME_GAMELOGIC_SCORESYSTEM_SCOREMANAGER_H     // [INFERRED] guard style
#define GAME_GAMELOGIC_SCORESYSTEM_SCOREMANAGER_H

#include "../../../Core/IBase.h"    // IBase (create's return type), IID_IBase
#include "../../../Core/Ids.h"      // the id constants queryInterface tests
#include "../../../IO/Network/NetworkableBase.h"
#include "TeamScore.h"              // the two by-value members at +0xA0 / +0x134

namespace dice {
namespace hfe {

namespace world {

class IPlayer;

// Score interface over IBase. Preserve virtual order and release-based lifetime.
// ScoreManager adds its own destructor and networking/scoring methods after this interface.
// [CHOSEN] Return types without body or caller evidence remain provisional.
class IScoreManager : public IBase
{
public:
    virtual TeamScore* getTeamScore(unsigned int team) = 0;
    virtual void  setWinner(unsigned char w) = 0;
    virtual int   getWinner() = 0;
    virtual void  forceSetWinner(unsigned char w) = 0;
    virtual void  setVictoryType(unsigned char t) = 0;
    virtual int   getVictoryType() = 0;
    virtual void  setHealScoreLimit(float v) = 0;
    virtual float getHealScoreLimit() = 0;
    virtual void  setRepairScoreLimit(float v) = 0;
    virtual float getRepairScoreLimit() = 0;
    virtual void  setAmmoScoreLimit(float v) = 0;
    virtual float getAmmoScoreLimit() = 0;
    virtual void  setTeamDamageLimit(float v) = 0;
    virtual float getTeamDamageLimit() = 0;
    virtual void  setTeamVehicleDamageLimit(float v) = 0;
    virtual float getTeamVehicleDamageLimit() = 0;
    virtual void  reset() = 0;

protected:
    ~IScoreManager() {}
};

class ScoreManager : public IScoreManager, public io::NetworkableBase
{
public:
    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    ScoreManager();

    // Scoring getters retain their non-const signatures.
    virtual TeamScore* getTeamScore(unsigned int team);
    virtual void  setWinner(unsigned char w);
    virtual int   getWinner();
    virtual void  forceSetWinner(unsigned char w);           // +0x9C
    virtual void  setVictoryType(unsigned char t);
    virtual int   getVictoryType();

    // * these five return float - they are the Python-settable limits, not counters. *
    virtual void  setHealScoreLimit(float v);
    virtual float getHealScoreLimit();
    virtual void  setRepairScoreLimit(float v);
    virtual float getRepairScoreLimit();
    virtual void  setAmmoScoreLimit(float v);
    virtual float getAmmoScoreLimit();
    virtual void  setTeamDamageLimit(float v);
    virtual float getTeamDamageLimit();
    virtual void  setTeamVehicleDamageLimit(float v);
    virtual float getTeamVehicleDamageLimit();

    virtual void  reset();
    virtual ~ScoreManager();

    // the INetworkable surface
    // These five are why the first 0x40 bytes of this object are not scoring data: ScoreManager is
    // a replicated object. See the layout note below.
    virtual bool  init(void* arg);
    virtual void  updateStateMask(unsigned int* mask);
    virtual bool  getNetUpdate(io::BitStream& s, io::BaseLineData* baseLine,
                               unsigned int a, int b);
    virtual void  setNetUpdate(io::BitStream& s, float t, bool b,
                               io::BaseLineData* baseLine, int i);
    virtual unsigned int getMinimumBitSize() const;

    // score values: getter + setter for every one, +112..+244
    virtual void setDeath(int v);               virtual int getDeath();               // +112 / +116
    virtual void setKill(int v);                virtual int getKill();                // +120 / +124
    virtual void setAssistKill(int v);          virtual int getAssistKill();          // +128 / +132
    virtual void setInDirectKill(int v);        virtual int getInDirectKill();        // +136 / +140
    virtual void setTK(int v);                  virtual int getTK();                  // +144 / +148
    virtual void setHeal(int v);                virtual int getHeal();                // +152 / +156
    virtual void setSoldierRevive(int v);       virtual int getSoldierRevive();       // +160 / +164
    virtual void setRepair(int v);              virtual int getRepair();              // +168 / +172
    virtual void setAmmo(int v);                virtual int getAmmo();                // +176 / +180
    virtual void setVehicleRevive(int v);       virtual int getVehicleRevive();       // +184 / +188
    virtual void setFlagCapture(int v);         virtual int getFlagCapture();         // +192 / +196
    virtual void setFlagReturn(int v);          virtual int getFlagReturn();          // +200 / +204
    virtual void setControlPointCapture(int v); virtual int getControlPointCapture(); // +208 / +212
    virtual void setObjective(int v);           virtual int getObjective();           // +216 / +220
    virtual void setDriverPassenger(int v);     virtual int getDriverPassenger();     // +224 / +228
    virtual void setTeamDamage(int v);          virtual int getTeamDamage();          // +232 / +236
    virtual void setTeamVehicleDamage(int v);   virtual int getTeamVehicleDamage();   // +240 / +244

    static IBase* create(unsigned int iid, IBase* outer);

    void removeAll();

protected:
    // NetworkableBase is a separate base subobject preceding the reference count.
    // Recorded offsets belong to the original x86 layout.
    /*  64 */ int   m_refCount;                 // +0x40  addRef/getRef/release

    // Per-action scoring values.
    /*  68 */ int   m_death;                    // +0x44
    /*  72 */ int   m_kill;                     // +0x48
    /*  76 */ int   m_assistKill;               // +0x4C
    /*  80 */ int   m_inDirectKill;             // +0x50   distinct from m_kill - explosions/roadkill
    /*  84 */ int   m_TK;                       // +0x54   team kill
    /*  88 */ int   m_heal;                     // +0x58
    /*  92 */ int   m_soldierRevive;            // +0x5C   distinct from m_vehicleRevive
    /*  96 */ int   m_repair;                   // +0x60
    /* 100 */ int   m_vehicleRevive;            // +0x64
    /* 104 */ int   m_ammo;                     // +0x68
    /* 108 */ int   m_flagCapture;              // +0x6C
    /* 112 */ int   m_flagReturn;               // +0x70
    /* 116 */ int   m_controlPointCapture;      // +0x74
    /* 120 */ int   m_objective;                // +0x78
    /* 124 */ int   m_driverPassenger;          // +0x7C   the driver-assist credit
    /* 128 */ int   m_teamDamage;               // +0x80
    /* 132 */ int   m_teamVehicleDamage;        // +0x84

    // Script-configured thresholds for healing, repairs, ammunition, and team damage.
    /* 136 */ float m_healScoreLimit;           // +0x88
    /* 140 */ float m_repairScoreLimit;         // +0x8C
    /* 144 */ float m_ammoScoreLimit;           // +0x90
    /* 148 */ float m_teamDamageLimit;          // +0x94
    /* 152 */ float m_teamVehicleDamageLimit;   // +0x98

    /* 156 */ unsigned char m_winner;           // +0x9C  written by forceSetWinner
    /* 157 */ unsigned char m_victoryType;      // +0x9D  getVictoryType - byte, adjacent to m_winner

    // Per-team counters for teams 1 and 2. Recorded ELF STL offsets do not transfer
    // to MSVC or native-width layouts.
    TeamScore m_teamScore1;                     // +0xA0   teamId 1
    TeamScore m_teamScore2;                     // +0x134  teamId 2

    // Owned ring of four 164-byte score snapshots, containing both teams and two header bytes.
    /* +0x1C8 */ unsigned char* m_scoreBuffer;  // update[4] - init news it, the dtor delete[]s it
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_SCORESYSTEM_SCOREMANAGER_H
