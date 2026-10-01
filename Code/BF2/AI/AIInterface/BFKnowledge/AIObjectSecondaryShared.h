// Secondary AI object using its primary's shared information.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTSECONDARYSHARED_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTSECONDARYSHARED_H

#include "AIObjectShared.h"

namespace dice {
namespace hfe {
namespace ai {

class AIObjectSecondaryShared : public AIObjectShared
{
public:
    AIObjectSecondaryShared(AITemplate* aiTemplate, int side, world::IObject* object);
    AIObjectSecondaryShared(AITemplate* aiTemplate, int side, world::IObject* object,
                            IAIObject* primary);
    virtual ~AIObjectSecondaryShared();

    virtual void addRef();                                      // over +0x40
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
    /* +0x40 */ int m_refCount;
    /* +0x44 */ IAIObject* m_primaryObject;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTSECONDARYSHARED_H
