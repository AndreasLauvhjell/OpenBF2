// Collision mesh registry and debug-draw interface.
// [INFERRED] header beside the __FILE__-attested Physics/CollisionManager.cpp.
// ICollisionManager and IEventListener are separate bases. Keep virtual order;
// the interface destructor follows the IBase quartet. Layout tables describe retail x86.

#ifndef PHYSICS_COLLISIONMANAGER_H     // [INFERRED] guard style
#define PHYSICS_COLLISIONMANAGER_H

#include <map>
#include <string>
#include <vector>

#include "../Core/IBase.h"
#include "../Core/WeakPtr.h"
#include "../Game/IEventListener.h"        // EventCategory, EventNode, IEventListener
#include "../IO/File/IFileSystem.h"        // dice::hfe::NoCaseStringCompare - the map's comparator
#include "../Math/Vec3.h"                  // Vec3 is a typedef (Math::Vec3), not a class
#include "CollisionMeshTemplate.h"         // world::CollisionMeshTemplate, the map's value type

namespace dice {
namespace hfe {

// Post-frame handles use the shared one-word WeakPtr control-block ownership contract.
namespace world {

// Only pointer declarations are needed here; concrete collision and object interfaces
// are defined in their respective headers.
class ICollisionMesh;
class IObject;

// Collision manager interface with fixed declaration order, including the early virtual destructor.
// Most void return types remain [CHOSEN, not recovered]; getUseDeferredLoading and mesh lookups are observed.
// Return types and parameter names remain [UNRESOLVED] where not constrained by callers.
class ICollisionManager : public IBase
{
public:
    // first. Slots +0x10 / +0x14. Moving this declaration down moves thirty-five slots.
    virtual ~ICollisionManager() {}

    virtual void init()                                             = 0;   // +0x18
    virtual void setUseDeferredLoading(bool on)                     = 0;   // +0x1C
    virtual bool getUseDeferredLoading() const                      = 0;   // +0x20

    // return type void. The 1,189-byte body was read end to end when the
    // deep drive's ObjectTemplate.collisionMesh line hit this slot - no path writes a return
    // value; it registers (or refuses a duplicate of) a CollisionMeshTemplate. The old
    // `ICollisionMesh*` guess is retired.
    virtual void create(const std::string& name)                    = 0;   // +0x24

    virtual CollisionMeshTemplate* getCollisionMeshTemplate(const std::string& name) = 0;  // +0x28
    virtual ICollisionMesh* getCollisionMeshPart(const std::string& name,
                                                 int part)          = 0;   // +0x2C
    virtual void deleteCollisionMesh(const std::string& name)       = 0;   // +0x30

    virtual void addToPostFrameUpdate(ICollisionMesh* mesh)         = 0;   // +0x34
    virtual void postFrameUpdate()                                  = 0;   // +0x38

    virtual void debugHeightmap(std::vector<Vec3>& out, int mode) const = 0;   // +0x3C

    // The debug-draw switches. Twenty-odd of these survive in a headless binary.
    virtual void setGeometry(bool on)                               = 0;   // +0x40
    virtual void setActiveLod(int lod)                              = 0;   // +0x44
    virtual void setDrawDistance(float distance)                    = 0;   // +0x48
    virtual void drawOnlyRoot(bool on)                              = 0;   // +0x4C
    virtual void drawMeshes(bool on)                                = 0;   // +0x50

    // Keep virtual order independent of member order: drawPivots precedes drawBoundingBoxes
    // in the interface even though their stored flags occur in the opposite order.
    virtual void drawPivots(bool on)                                = 0;   // +0x54  -> field +0x37
    virtual void drawBoundingBoxes(bool on)                         = 0;   // +0x58  -> field +0x36

    virtual void drawNormals(bool on)                               = 0;   // +0x5C
    virtual void drawTestedFaces(bool on)                           = 0;   // +0x60
    virtual void drawIntersectFaces(bool on)                        = 0;   // +0x64
    virtual void drawSoldierMesh(bool on)                           = 0;   // +0x68
    virtual void drawPrimitives(bool on)                            = 0;   // +0x6C
    virtual void drawForces(bool on)                                = 0;   // +0x70

    virtual void drawDebugCollisionMeshes()                         = 0;   // +0x74
    virtual void drawLine(const Vec3& a, const Vec3& b, unsigned int color)     = 0;   // +0x78
    virtual void drawTriangle(const Vec3& a, const Vec3& b, const Vec3& c,
                              unsigned int color)                   = 0;   // +0x7C
    virtual void drawForce(const Vec3& origin, const Vec3& force,
                           unsigned int color)                      = 0;   // +0x80

    virtual void getNameOfObjectInFocus()                           = 0;   // +0x84
    virtual void updateFlagsOfObjectInFocus(unsigned int mask, unsigned int value) = 0; // +0x88
    virtual void setSleepinessOfObjectInFocus(int sleepiness)       = 0;   // +0x8C
    virtual void identityRotateObjectInFocus()                      = 0;   // +0x90
    virtual void rotateObjectInFocusAroundAxis(unsigned int axis, float radians) = 0;   // +0x94
    virtual void rotateObjectInFocusAroundVector(const Vec3& axis, float radians) = 0;  // +0x98

    virtual void buildDebugCollisionMeshes()                        = 0;   // +0x9C
    virtual void destroyDebugCollisionMeshes()                      = 0;   // +0xA0
};

class CollisionManager : public ICollisionManager, public IEventListener
{
public:
    CollisionManager();

    // Primary vtable, in slot order. Do not reorder.
    virtual void  addRef();                                             // +0x00
    virtual int   getRef();                                             // +0x04
    virtual int   release();                                            // +0x08
    virtual void* queryInterface(unsigned int iid) const;               // +0x0C

    virtual ~CollisionManager();                                        // +0x10 / +0x14


    virtual void init();                                                // +0x18
    virtual void setUseDeferredLoading(bool on);                        // +0x1C
    virtual bool getUseDeferredLoading() const;                         // +0x20
    virtual void create(const std::string& name);                       // +0x24 Creates the named mesh; returns no value.
    virtual CollisionMeshTemplate* getCollisionMeshTemplate(const std::string& name); // +0x28
    virtual ICollisionMesh* getCollisionMeshPart(const std::string& name, int part); // +0x2C
    virtual void deleteCollisionMesh(const std::string& name);          // +0x30
    virtual void addToPostFrameUpdate(ICollisionMesh* mesh);            // +0x34
    virtual void postFrameUpdate();                                     // +0x38
    virtual void debugHeightmap(std::vector<Vec3>& out, int mode) const;// +0x3C
    virtual void setGeometry(bool on);                                  // +0x40
    virtual void setActiveLod(int lod);                                 // +0x44
    virtual void setDrawDistance(float distance);                       // +0x48
    virtual void drawOnlyRoot(bool on);                                 // +0x4C
    virtual void drawMeshes(bool on);                                   // +0x50
    virtual void drawPivots(bool on);                                   // +0x54
    virtual void drawBoundingBoxes(bool on);                            // +0x58
    virtual void drawNormals(bool on);                                  // +0x5C
    virtual void drawTestedFaces(bool on);                              // +0x60
    virtual void drawIntersectFaces(bool on);                           // +0x64
    virtual void drawSoldierMesh(bool on);                              // +0x68
    virtual void drawPrimitives(bool on);                               // +0x6C
    virtual void drawForces(bool on);                                   // +0x70
    virtual void drawDebugCollisionMeshes();                            // +0x74
    virtual void drawLine(const Vec3& a, const Vec3& b, unsigned int color);          // +0x78
    virtual void drawTriangle(const Vec3& a, const Vec3& b, const Vec3& c,
                              unsigned int color);                      // +0x7C
    virtual void drawForce(const Vec3& origin, const Vec3& force, unsigned int color);// +0x80
    virtual void getNameOfObjectInFocus();                              // +0x84
    virtual void updateFlagsOfObjectInFocus(unsigned int mask, unsigned int value);   // +0x88
    virtual void setSleepinessOfObjectInFocus(int sleepiness);          // +0x8C
    virtual void identityRotateObjectInFocus();                         // +0x90
    virtual void rotateObjectInFocusAroundAxis(unsigned int axis, float radians);     // +0x94
    virtual void rotateObjectInFocusAroundVector(const Vec3& axis, float radians);    // +0x98
    virtual void buildDebugCollisionMeshes();                           // +0x9C
    virtual void destroyDebugCollisionMeshes();                         // +0xA0

    // Not on ICollisionManager - the interface's vtable stops at +0xA0. See the header block.
    virtual void drawDebugCollisionMeshRecursive(IObject* root);        // +0xA4

    // The IEventListener half. Secondary vtable slot 0 (thunk 0x839C610); also +0xA8 of the primary
    // under Itanium.
    virtual void handleEvent(EventCategory category, unsigned int id,
                             EventNode* node, void* userData);          // +0xA8

    // NON-virtual. Absent from both vtables; reached only by name.
    void preLevelLoad();
    void postLevelLoad();
    // Prunes a registry entry and returns the iterator from which traversal continues.
    std::map<std::string, std::vector<CollisionMeshTemplate*>, NoCaseStringCompare>::iterator
    erase(std::map<std::string, std::vector<CollisionMeshTemplate*>, NoCaseStringCompare>::iterator it);
    void load(std::vector<CollisionMeshTemplate*>& templates);
    void drawDebugPrimitives();
    void rotateObjectAroundVector(IObject* object, const Vec3& axis, float radians);
    // Returns the focused object, or null. Static entry point; no manager instance is required.
    static IObject* getObjectInFocus();

    // Module factory used by ClassManager to create the collision-manager singleton.
    static IBase* create(unsigned int iid, IBase* outer);

private:
    // Offsets describe the original x86 layout, including the two interface vptrs.
    // [CHOSEN] Field names are descriptive; native STL member sizes may differ.
    /* +0x08 */ int m_refCount;                     // Initially one reference.
    // Construction owns one reference; create does not add another.
    // Case-insensitive mesh registry; original ELF map offsets do not apply to current STL layout.
    /* +0x0C */ std::map<std::string, std::vector<CollisionMeshTemplate*>,
                         NoCaseStringCompare> m_collisionMeshTemplates;

    // +0x18 .. +0x23. addToPostFrameUpdate appends, postFrameUpdate walks it and then resize(0)s it.
    // 4-byte elements (`>> 2` at 0x839AE2C and in init).
    /* +0x18 */ std::vector<WeakPtr<ICollisionMesh> > m_postFrameUpdate;

    /* +0x24 */ bool  m_useDeferredLoading;         // = true (`mov byte ptr [ebx+24h], 1`). init()
                                                    //   does not reset this - it survives a level
                                                    //   change.
    /* +0x25 */ bool  m_unknown25;                  // = false. [UNRESOLVED] - no accessor writes it
                                                    //   and init() does not touch it. The only
                                                    //   statement about it in the whole binary is
                                                    //   this initialiser.
    /* +0x26 */ // two bytes of padding
    /* +0x28 */ int   m_activeLod;                  // = 0
    /* +0x2C */ bool  m_geometry;                   // = false   (a byte - setGeometry writes `dl`)
    /* +0x2D */ // three bytes of padding
    /* +0x30 */ float m_drawDistance;               // = 200.0f  (0x43480000 at 0x83974F4)

    // Ten one-byte switches, +0x34 .. +0x3D, all false. the struct order is not the vtable order -
    // see the warning on drawPivots / drawBoundingBoxes above.
    /* +0x34 */ bool m_drawOnlyRoot;
    /* +0x35 */ bool m_drawMeshes;
    /* +0x36 */ bool m_drawBoundingBoxes;
    /* +0x37 */ bool m_drawPivots;
    /* +0x38 */ bool m_drawNormals;
    /* +0x39 */ bool m_drawTestedFaces;
    /* +0x3A */ bool m_drawIntersectFaces;
    /* +0x3B */ bool m_drawSoldierMesh;
    /* +0x3C */ bool m_drawPrimitives;
    /* +0x3D */ bool m_drawForces;
    /* +0x3E */ // two bytes of padding

    // four vectors, +0x40 .. +0x6F, and they are two parallel pairs: geometry and colour kept apart.
    // drawLine pushes two Vec3 then one colour; drawTriangle pushes three Vec3 then one colour. So
    // the line list is read two-at-a-time against one colour and the triangle list three-at-a-time
    // against one colour - the colour vectors are per-primitive, not per-vertex.
    /* +0x40 */ std::vector<Vec3>         m_lineVertices;      // 0x40 begin / 0x44 end / 0x48 cap
    /* +0x4C */ std::vector<unsigned int> m_lineColors;        // 0x4C / 0x50 / 0x54
    /* +0x58 */ std::vector<Vec3>         m_triangleVertices;  // 0x58 / 0x5C / 0x60
    /* +0x64 */ std::vector<unsigned int> m_triangleColors;    // 0x64 / 0x68 / 0x6C
};

// Shared collision-manager interface pointer; registry storage casts to IBase only at registration.
extern ICollisionManager* collisionManager;

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_COLLISIONMANAGER_H
