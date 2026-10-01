// Path: [INFERRED] beside the other world runtime products; no __FILE__ string found.
#ifndef GAME_OBJECTS_GAME_AREAOBJECT_H
#define GAME_OBJECTS_GAME_AREAOBJECT_H

#include "../Base/Bundle.h"

namespace dice { namespace hfe { namespace world {
class AreaObjectTemplate;
class AreaObject : public Bundle
{
public:
    explicit AreaObject(const AreaObjectTemplate*);
    virtual ~AreaObject();
    virtual void init();
    virtual void handleFrameUpdate(float);
    void addAbsolutePoint();

private:
    const AreaObjectTemplate* m_areaTemplate; // ELF +0x13C / PE +0x154
    Mat4 m_areaTransformation;                // ELF +0x140 / PE +0x158 [INFERRED name]
};
} } }
#endif
