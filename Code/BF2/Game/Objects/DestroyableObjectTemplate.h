// Configures destructible geometry, respawn behavior and AI walkability.
// [INFERRED] Source placement; field names follow console properties.

#ifndef GAME_OBJECTS_DESTROYABLEOBJECTTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_DESTROYABLEOBJECTTEMPLATE_H

#include "Base/BundleTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class DestroyableObjectTemplate : public BundleTemplate
{
public:
    DestroyableObjectTemplate();                 // C1/C2 164 B -

    virtual void  addRef();                      // refcount +0x17C
    virtual int   getRef();
    virtual int   release();                     // delete on 0 via D0
    virtual void* queryInterface(unsigned int) const;   // the Bundle-chain five-way hop
    virtual unsigned int getClassID() const;     // reads the CID global
    virtual ~DestroyableObjectTemplate();        // vptr resets + ~BundleTemplate only
    virtual void* queryImplementation(unsigned int);    // CID hop -> Bundle's chain
    virtual IObject* createObject() const;       // new(0x140) DestroyableObject [FRONTIER]
    virtual bool  makeScript(io::IStream*);

    friend struct GameTemplateConsoleAccess;     // recovered ObjectTemplate.* console executors

protected:
    /* 0x17C */ int   m_refCount;                // read directly from addRef / getRef

    // GameLogic reads these directly when configuring the respawn spawner.
public:
    /* 0x180 */ float respawnDelay;              // float - the spawner's delay
    /* 0x184 */ int   nrOfRespawns;              // int   - the spawner's nrOfObjectToSpawn

    // AI plug-ins read these adjacent Boolean fields and the pathfinding bounds directly.
    /* 0x188 */ bool  usedByAi;
    /* 0x189 */ bool  initiallyWalkable;
    /* 0x18A */ char  _pad18A[2];

    // Three-component bounding-box corners. [UNRESOLVED] Original scalar types; currently represented as floats.
    /* 0x18C */ float minCorner[3];              // the object's bounding box, 0x18C..0x197
    /* 0x198 */ float maxCorner[3];              // 0x198..0x1A3

    // `initiallyWalkable` is worth a note for a rebuild: it is a bool on a destroyable object, so
    // AI walkability is authored per-object and presumably re-evaluated when the object breaks.
    // [UNRESOLVED] what updates it at runtime; AIObjectPhysical::init is its one reader.
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_DESTROYABLEOBJECTTEMPLATE_H
