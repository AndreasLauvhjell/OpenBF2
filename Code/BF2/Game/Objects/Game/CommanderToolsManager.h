// Owns per-team commander-tool state and registers the process singleton and game-logic listener.
// [INFERRED] source placement.

#ifndef GAME_OBJECTS_GAME_COMMANDERTOOLSMANAGER_H
#define GAME_OBJECTS_GAME_COMMANDERTOOLSMANAGER_H

#include "../../../Math/Vec3.h"
#include "../../IEventListener.h"

namespace dice {
namespace hfe {
namespace world {

// ECGameLogic ids 17, 18 and 21 all carry this 16-byte notification shape. `id` is a team for
// UAV/drop requests and the requesting player/connection id for a satellite scan.
struct CommanderToolNotification
{
    Vec3 position;
    int id;
};

class CommanderToolState
{
public:
    CommanderToolState();
    ~CommanderToolState();                                     // empty

    void reset();
    void resetToysStats();
    void update(float deltaTime);
    bool launchUAV(const Vec3& position);
    bool startScan();

    Vec3 CalculateRandomDropVehicleDeviation(int maximum, int minimum);
    bool CheckForCollisionWithStrategicObjects(const Vec3& position);
    int  dropVehicle(const Vec3& position, int team);
    bool IsVehiclePermittedtoLand(Vec3& position);             // dice spelling/case
    bool CheckVehicleDropGrid(const Vec3& position, float& height);
    void findValidLandingPositionForDropVehicle(Vec3& position);
    bool isXZIntersectionForVehicleVsDecalCloserThan(
        const Vec3& position, float distance, float& height);

    bool getUAVIsReady() const;
    bool getScanIsReady() const;
    bool getUAVActive(Vec3* position) const;
    bool getScanActive() const;

    // [CHOSEN] typed views for the two retail consumers that directly read this state in the ELF.
    // They expose recovered fields without exporting compiler-specific byte offsets.
    float getArtilleryArmor() const { return m_artilleryArmor; }
    float getUAVArmor() const { return m_uavArmor; }
    float getSatelliteArmor() const { return m_satelliteArmor; }
    int getArtilleryCount() const { return m_artilleryCount; }
    int getUAVCount() const { return m_uavCount; }
    int getSatelliteCount() const { return m_satelliteCount; }
    int getDropVehicleCount() const { return m_dropVehicleCount; }

private:
    friend class CommanderToolsManager;

    // ELF layout is exactly 0x38 bytes. MSVC's layout is the same here: only floats/ints/Vec3.
    /* +0x00 */ float m_uavActiveTime;
    /* +0x04 */ float m_uavReloadTime;
    /* +0x08 */ float m_scanActiveTime;
    /* +0x0C */ float m_scanReloadTime;
    /* +0x10 */ Vec3  m_uavPosition;
    /* +0x1C */ float m_artilleryArmor;
    /* +0x20 */ float m_uavArmor;
    /* +0x24 */ float m_satelliteArmor;
    /* +0x28 */ int   m_artilleryCount;
    /* +0x2C */ int   m_uavCount;
    /* +0x30 */ int   m_satelliteCount;
    /* +0x34 */ int   m_dropVehicleCount;
};

static_assert(sizeof(CommanderToolState) == 0x38,
              "CommanderToolState must retain the recovered 56-byte layout");

class CommanderToolsManager : public IEventListener
{
public:
    CommanderToolsManager();

    // IEventListener has no virtual destructor. Declaration order therefore fixes the recovered
    // [handleEvent, D1, D0] Itanium table; keep handleEvent before this destructor.
    virtual void handleEvent(EventCategory category, unsigned int id,
                             EventNode* node, void* userData);
    virtual ~CommanderToolsManager();

    void update(float deltaTime);
    void reset();
    void handleUAVEvent(const Vec3& position, int team);
    void handleSatelliteEvent(int playerId);
    void handleDropVehicleEvent(const Vec3& position, int team);
    bool getUAVActive(int team, Vec3* position);
    bool getUAVArmor(int team);
    bool getSatelliteScanActive(int team);
    float getPercentOfDropVehicles(int team);
    void addDropVehicleToTeam(int team);
    void updateCommanderToys();

    // [CHOSEN] source-level replacement for retail's direct `this + 56*team - 52` arithmetic.
    // The guard is only for new callers; recovered methods that index without a guard continue to
    // do so internally and preserve retail behavior.
    CommanderToolState* getToolStateForTeam(int team)
    {
        return static_cast<unsigned int>(team - 1) <= 1u ? &m_teamStates[team - 1] : 0;
    }
    const CommanderToolState* getToolStateForTeam(int team) const
    {
        return static_cast<unsigned int>(team - 1) <= 1u ? &m_teamStates[team - 1] : 0;
    }

private:
    /* +0x04 */ CommanderToolState m_teamStates[2];
    // [PLATFORM] ELF aligns double to four bytes here (+0x74, sizeof 0x8C); Win32 MSVC inserts
    // four bytes of padding (+0x78, sizeof 0x90). PE ctor 0x565B50 writes this[15..17] as doubles.
    double m_lastDropVehicleTimes[3];                           // indexed directly by team
};

#if defined(_M_IX86)
static_assert(sizeof(CommanderToolsManager) == 0x90,
              "CommanderToolsManager must retain its recovered Win32 x86 layout");
#elif defined(__i386__)
static_assert(sizeof(CommanderToolsManager) == 0x8C,
              "CommanderToolsManager must retain its recovered ELF x86 layout");
#endif

extern CommanderToolsManager* commanderToolsManager;           // ds:0x8AAC270, Core/Globals.cpp

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_COMMANDERTOOLSMANAGER_H
