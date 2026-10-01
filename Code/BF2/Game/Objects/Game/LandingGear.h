// Path:     [INFERRED] - the symbolised ELF does not expose a __FILE__ string for this class;
// placed beside the other vehicle object leaves (Wing and FloatingBundle).
// Contents: recovered from the symbolised BF2 1.50 ELF and cross-checked against bf2_w32ded.exe.
// LandingGear is a RotationalBundle leaf. The ELF factory allocates 0x1CC bytes; the Win32 retail
// factory allocates 0x1E4 because its base hierarchy has a different ABI layout. Both reference
// binaries add the same 28-byte source-level tail below. This tree's already-recovered bases have
// their own layout, so the leaf must stay typed and unpadded rather than copying either total size.

#ifndef GAME_OBJECTS_GAME_LANDINGGEAR_H
#define GAME_OBJECTS_GAME_LANDINGGEAR_H

#include "RotationalBundle.h"

namespace dice {
namespace hfe {
namespace world {

class LandingGearTemplate;

class LandingGear : public RotationalBundle
{
public:
    explicit LandingGear(const LandingGearTemplate* objectTemplate);
    virtual ~LandingGear();

    virtual void handleUpdate(float deltaTime, unsigned int updateFlags);
    virtual void handleMessage(TemplateMessage message, IPlayer* player,
                               bool flag);

protected:
    /* ELF +0x1B0 */ float        m_gearUpDelayRemaining;
    /* ELF +0x1B4 */ float        m_gearDownDelayRemaining;
    /* ELF +0x1B8 */ bool         m_gearUp;
    // Copied from m_currentRotation by the constructor. No recovered LandingGear method reads it,
    // so its exact role remains [UNRESOLVED].
    /* ELF +0x1BC */ Vec3         m_savedRotation;
    /* ELF +0x1C8 */ unsigned int m_lastFullUpdateTick;                            // [CHOSEN name]
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_LANDINGGEAR_H
