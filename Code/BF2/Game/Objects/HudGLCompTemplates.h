// HUD and game-logic component templates. IConsoleSaveable must remain the secondary base.
// Its makeScript slot follows each primary interface's own virtuals. IK rotations are stored as matrices.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_HUDGLCOMPTEMPLATES_H     // [INFERRED] guard style
#define GAME_OBJECTS_HUDGLCOMPTEMPLATES_H

// Aggregate includes for HUD and gameplay component templates.
#include "AbilityGLCompTemplate.h"
#include "AbilityHudCompTemplate.h"
#include "HelpHudCompTemplate.h"
#include "PostProcessCompTemplate.h"
#include "RadioGLCompTemplate.h"
#include "StrategicObjectCompTemplate.h"
#include "VehicleHudCompTemplate.h"
#include "WarningHudCompTemplate.h"
#include "WeaponHudCompTemplate.h"
#include "WindAffectorTemplate.h"

#include <string>

#include "CompTemplateInterfaces.h"
#include "IObjectTemplateComponent.h"
#include "../../IO/Console/IConsoleSaveable.h"
#include "../../Math/Mat4.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace io { class IStream; }
namespace world {


// Inverse-kinematics attachment points; rotations are stored as matrices and converted for Euler getters.
class IkTemplateComponent : public IIkTemplateComponent, public io::IConsoleSaveable
{
public:
    IkTemplateComponent(); // ELF 81E6B90 / dedicated PE 5F8B90, both 188 bytes
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();
    virtual void  objectCreated(IObject*);
    virtual void  objectDestroyed(IObject*);
    virtual void  setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual void  setPositionLeft(const Vec3&);
    virtual const Vec3& getPositionLeft() const;
    virtual void  setRotationLeft(const Vec3&);
    virtual Vec3  getRotationLeft() const; // Converts the stored rotation and returns it by value.
    virtual const Mat4& getRotationLeftMat() const;                  // — the stored form
    virtual void  setPoleVectorLeft(const Vec3&);
    virtual const Vec3& getPoleVectorLeft() const;
    virtual void  setPositionRight(const Vec3&);
    virtual const Vec3& getPositionRight() const;
    virtual void  setRotationRight(const Vec3&);
    virtual Vec3  getRotationRight() const; // Converts the stored rotation and returns it by value.
    virtual const Mat4& getRotationRightMat() const;                 // — the stored form
    virtual void  setPoleVectorRight(const Vec3&);
    virtual const Vec3& getPoleVectorRight() const;
    virtual ~IkTemplateComponent();
    virtual bool  makeScript(io::IStream*);                          // +0x6C — last

private:
    // Offsets agree in both x86 images; this class contains no STL members.
    int m_refCount;             // +08
    Vec3 m_positionLeft;        // +0C
    Mat4 m_rotationLeft;        // +18
    Vec3 m_poleVectorLeft;      // +58
    Vec3 m_positionRight;       // +64
    Mat4 m_rotationRight;       // +70
    Vec3 m_poleVectorRight;     // +B0
};


} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_HUDGLCOMPTEMPLATES_H
