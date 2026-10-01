// Path:     [INFERRED] - no __FILE__ string names this class. ELF link order places the complete
// runtime immediately before WindAffectorTemplate, so both implementations live in the
// adjacent WindAffector.cpp while this declaration stays beside the component interface.
// Contents: recovered from the symbolised BF2 1.50 ELF and cross-checked against bf2_w32ded.exe.
// Runtime layout is 0x18 bytes in both shipped 32-bit ports and in this source-level model:
// +0x00 vptr, +0x04 refcount, +0x08 template, +0x0C object, +0x10 active, +0x14 blow timer.

#ifndef GAME_OBJECTS_WINDAFFECTOR_H
#define GAME_OBJECTS_WINDAFFECTOR_H

#include "IWindAffector.h"

namespace dice {
namespace hfe {
namespace world {

class WindAffectorTemplate;

class WindAffector : public IWindAffector
{
public:
    explicit WindAffector(const WindAffectorTemplate* objectTemplate);

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual unsigned int getInterfaceID() const;
    virtual void  init(IObject* object);
    virtual void  destroy();
    virtual void  setObject(IObject* object);
    virtual IObject* getObject() const;

    // IObjectComponent's simulation/frame update hooks remain its shipped no-op defaults. Wind
    // affectors instead tick through WindManager::update using this appended operation.
    virtual void  update(float deltaTime);
    virtual bool  getIsActive() const;
    virtual void  setIsActive(bool active);
    virtual float getBoundingRadius() const;
    virtual const Vec3& getAbsolutePosition() const;
    virtual void  getAbsoluteWindDirectionAndSpeed(const Vec3& position,
                                                   Vec3& direction,
                                                   float& speed) const;
    virtual Vec3  getAbsoluteWind(const Vec3& position) const;

    virtual ~WindAffector();
    virtual unsigned int getClassID() const;

private:
    int                         m_refCount;
    const WindAffectorTemplate* m_template;
    IObject*                    m_object;
    bool                        m_active;
    float                       m_remainingBlowTime;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WINDAFFECTOR_H
