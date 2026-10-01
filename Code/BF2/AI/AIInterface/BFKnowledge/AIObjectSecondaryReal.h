// Secondary AI object attached to a primary. It does not enter the information grid directly.
// Cover and physical information plug-ins may be borrowed from the primary.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTSECONDARYREAL_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTSECONDARYREAL_H

#include "AIObjectReal.h"

namespace dice {
namespace hfe {
namespace ai {

class AIObjectSecondaryReal : public AIObjectReal
{
public:
    AIObjectSecondaryReal(AITemplate* aiTemplate, int side, world::IObject* object);
    AIObjectSecondaryReal(AITemplate* aiTemplate, int side, world::IObject* object,
                          IAIObject* primary);
    virtual ~AIObjectSecondaryReal();

    virtual void addRef();                                      // over +0x48
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual void init();                                        // +0x48
    virtual void addInformationToGrid(int side);                // +0xC4, - empty
    virtual void removeFromGrid();                              // +0xC8, - empty
    virtual void reInsertInGrid();                              // +0xCC, - empty
    virtual bool isPrimary() const;                             // +0xF8
    virtual IAIObject* getPrimaryObject();                      // +0xFC
    virtual void setPrimaryObject(IAIObject* primary);          // +0x100
    virtual void setSide(int side);                             // +0x128
    virtual void createInfoPlugins(unsigned int handle, bool real, int side);   // +0x150

protected:
    /* +0x48 */ int m_refCount;
    /* +0x4C */ IAIObject* m_primaryObject;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTSECONDARYREAL_H
