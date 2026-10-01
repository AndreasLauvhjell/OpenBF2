// Path: [INFERRED] beside AnimatedBundle; no dedicated-image __FILE__ evidence.

// AnimatedBundle owns every interface/refcount slot. MaskObject appends only virtual getType.
#ifndef GAME_OBJECTS_GAME_MASKOBJECT_H
#define GAME_OBJECTS_GAME_MASKOBJECT_H

#include "AnimatedBundle.h"
#include "../MaskObjectTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class Soldier;

class MaskObject : public AnimatedBundle
{
public:
    explicit MaskObject(const MaskObjectTemplate*);
    virtual ~MaskObject();
    virtual void init();
    virtual void handleUpdate(float, unsigned int);
    virtual void handleVisualUpdate(float, float);
    virtual MaskObjectType getType() const; // ELF primary +0x190 / PE primary +0x178

    void updateGeometryPosition(Soldier*);
    Soldier* getSoldier() const;

private:
    const MaskObjectTemplate* m_maskTemplate; // ELF +0x154 / PE +0x16C; non-owning
    bool m_geometryPositionInitialized;      // ELF +0x158 / PE +0x170; ctor false
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif
