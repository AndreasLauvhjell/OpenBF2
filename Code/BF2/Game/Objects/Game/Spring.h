// Wheel suspension object linking PhysicsSpring contact, wheel effects and WheelSound.
// Caches its engine through a weak reference. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_SPRING_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_SPRING_H

#include "../Base/Bundle.h"
#include "../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {
class WheelSound;
namespace world {

class SpringTemplate;
class IObjectTemplate;

// (the Spring TU's free function): a template whose class is EffectBundle or
// WheelEffectBundle - Spring::init tells its effect children apart with it.
bool isEffectBundle(const IObjectTemplate* objectTemplate);

class Spring : public Bundle
{
public:
    explicit Spring(const SpringTemplate* objectTemplate);
    virtual ~Spring();

    virtual void init();
    virtual void handleUpdate(float, unsigned int);
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*, CollisionFaceInfo*);
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);
    virtual void initNetworkableComp();
    virtual IObject* getEngine() const;                                     // (+0x180)

    friend class hfe::WheelSound;                                           // reads +0x164
    friend class PhysicsSpring;                                             // reads +0x13E m_occupied
                                                                            // in its updatePhysics

protected:
    /* 0x13C */ bool  m_hasContact;
    /* 0x13D */ bool  m_byte13D;
    /* 0x13E */ bool  m_occupied;
    /* 0x140 */ int   m_dword140;
    /* 0x144 */ int   m_material;
    /* 0x148 */ int   m_prevMaterial;
    /* 0x14C */ Vec3  m_contactPosition;
    /* 0x158 */ Vec3  m_contactNormal;
    /* 0x164 */ float m_rotationSpeed;
    /* 0x168 */ WheelSound* m_wheelSound;
    /* 0x16C */ mutable WeakPtr<IObject> m_engine;                          // getEngine is const and fills it
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_SPRING_H
