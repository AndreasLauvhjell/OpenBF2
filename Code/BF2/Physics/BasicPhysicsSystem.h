// Global gravity, wind and air-density settings.
// [INFERRED] placement from ELF link order in the Physics translation-unit cluster.
// Preserve the shipped wind mismatch: setWind writes m_wind, while getWind reads
// m_reportedWind. update() does not copy between them.

#ifndef PHYSICS_BASICPHYSICSSYSTEM_H     // [INFERRED] guard style
#define PHYSICS_BASICPHYSICSSYSTEM_H

#include <string>

#include "../Core/IBase.h"
#include "../Math/Vec3.h"                  // Vec3 is a typedef (Math::Vec3), not a class

namespace dice {
namespace hfe {
namespace world {

// Global physics settings interface derived from IBase. Keep the virtual destructor
// before the settings methods. [UNRESOLVED] Original parameter names are unavailable.
class IPhysicsSystem : public IBase
{
public:
    // first. Slots +0x10 / +0x14. Moving this declaration down moves eight slots.
    virtual ~IPhysicsSystem() {}

    virtual void        update(float deltaTime)                     = 0;   // +0x18
    virtual void        setGravity(float gravity)                   = 0;   // +0x1C
    virtual float       getGravity()                                = 0;   // +0x20
    virtual void        setWind(Vec3 wind)                          = 0;   // +0x24  by value
    virtual Vec3        getWind()                                   = 0;   // +0x28
    virtual void        setAirDensityZeroAtHeight(float height)     = 0;   // +0x2C
    virtual float       getAirDensityZeroAtHeight()                 = 0;   // +0x30
    virtual std::string listProperties()                            = 0;   // +0x34
};

// The one implementation. `dice.hfe.world.PhysicsSystem.Basic`, CID 50131.
class BasicPhysicsSystem : public IPhysicsSystem
{
public:
    BasicPhysicsSystem();

    // Vtable, in slot order. Do not reorder.
    virtual void  addRef();                                             // +0x00
    virtual int   getRef();                                             // +0x04
    virtual int   release();                                            // +0x08
    virtual void* queryInterface(unsigned int iid) const;               // +0x0C

    virtual ~BasicPhysicsSystem();                                      // +0x10 / +0x14


    virtual void        update(float deltaTime);                        // +0x18
    virtual void        setGravity(float gravity);                      // +0x1C
    virtual float       getGravity();                                   // +0x20
    virtual void        setWind(Vec3 wind);                             // +0x24
    virtual Vec3        getWind();                                      // +0x28
    virtual void        setAirDensityZeroAtHeight(float height);        // +0x2C
    virtual float       getAirDensityZeroAtHeight();                    // +0x30
    virtual std::string listProperties();                               // +0x34

private:
    // the 52 bytes. Offsets, types and initial values are all read from the constructor and the
    // accessors; the vptr at +0x00 is implicit (the base list above places it).

    // field names are [CHOSEN]; not one of them carries a symbol. m_wind and m_reportedWind are
    // named for the accessor that touches each, because the two accessors disagree - see the
    // shipped-bug note in the header block.
    /* +0x04 */ int   m_refCount;                   // = 1 at construction (0x835C25F). The
                                                    // one-reference rule holds; ClassFactory<T>
                                                    // ::create does not addRef.
    /* +0x08 */ float m_gravity;                    // = -14.73f  (0xC16BAE14 at 0x835C241)
    /* +0x0C */ Vec3  m_wind;                       // = (0,0,0). written by setWind. Read by nobody.
    /* +0x18 */ Vec3  m_reportedWind;               // = (0,0,0). read by getWind. Written by nobody.
    /* +0x24 */ Vec3  m_unusedVec3;                 // Initialized to zero; role unresolved.
    // [UNRESOLVED] This vector is initialized but has no identified reader or writer afterward.
    /* +0x30 */ float m_airDensityZeroAtHeight;     // = 1000.0f  (0x447A0000 at 0x835C284)
};

// IBase storage used by the module's singleton registration; defined in Core/Globals.cpp.
extern IBase* basicPhysicsSystem;

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_BASICPHYSICSSYSTEM_H
