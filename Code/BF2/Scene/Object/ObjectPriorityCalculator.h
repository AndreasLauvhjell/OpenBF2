// Path: [INFERRED] beside ObjectManager; no __FILE__ witness in either dedicated image.
// RTTI: world::ObjectPriorityCalculator derives IObjectPriorityCalculator; 8 bytes, five slots.
#ifndef SCENE_OBJECT_OBJECTPRIORITYCALCULATOR_H
#define SCENE_OBJECT_OBJECTPRIORITYCALCULATOR_H
#include "../../Core/IBase.h"
namespace dice { namespace hfe { namespace world {
class IObjectPriorityCalculator : public IBase
{
public:
    virtual void update() = 0;
};
class ObjectPriorityCalculator : public IObjectPriorityCalculator
{
public:
    ObjectPriorityCalculator();
    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual void update();
    static IBase* create(unsigned int iid, IBase* outer);
private:
    int m_refCount;
};
} } }
#endif
