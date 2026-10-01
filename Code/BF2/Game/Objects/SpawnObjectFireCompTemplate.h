// Deploy/parachute specialization of SingleFireCompTemplate.
// Adds a leaf-owned reference count; inherited vector layout differs by platform.

#ifndef GAME_OBJECTS_SPAWNOBJECTFIRECOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_SPAWNOBJECTFIRECOMPTEMPLATE_H

#include "SingleFireCompTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class SpawnObjectFireCompTemplate : public SingleFireCompTemplate
{
public:
    SpawnObjectFireCompTemplate();

    // This leaf owns a second reference count, exactly like SingleFireCompTemplate below it.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int);
    virtual ~SpawnObjectFireCompTemplate();
    virtual bool  makeScript(io::IStream*);

protected:
    // Documentary ELF offset only.  Do not assert it on the PE/current STL layout.
    /* +0x90 */ int m_refCount;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_SPAWNOBJECTFIRECOMPTEMPLATE_H
