// Object culler interface. [INFERRED] Source placement follows its implementations.
// [PLATFORM] Keep the conditional query order: PE uses frustum/box/sphere/direction,
// while ELF uses direction/sphere/box/frustum.

#ifndef SCENE_OBJECT_ICULLER_H
#define SCENE_OBJECT_ICULLER_H

#include <utility>
#include <vector>

#include "../../Core/IBase.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class Frustum;

namespace world {

class IObject;
class ObjectPredicator;
class CullerObject;

class ICuller : public IBase
{
public:
    virtual unsigned int getClassID() const = 0;                         // +0x10
    virtual CullerObject* createObject(const Vec3& position, float radius,
                                       IObject* object) = 0;             // +0x14

    // the shared base implementation is deliberately zero.  BinTreeCuller inherits it;
    // QuadTreeCuller overrides it with its live object count.
    virtual int getNumberOfObjects() { return 0; }                       // +0x18

    virtual void initQuadTree(const Vec3& centre, float childOffset,
                              float minimumWidth, float width) = 0;      // +0x1C
    virtual void destroyObject(CullerObject* object) = 0;                // +0x20
    virtual CullerObject* updateObject(CullerObject* object,
                                       const Vec3& position,
                                       float radius) = 0;                // +0x24
    virtual void optimize() = 0;                                         // +0x28
    virtual void clear() = 0;                                            // +0x2C
    virtual void getAllObjects(std::vector<IObject*>& out) = 0;          // +0x30

#if defined(_WIN32)
    // PE bf2_w32ded.exe: +0x34 Frustum, +0x38 box, +0x3C sphere, +0x40 direction.
    virtual void getObjects(const Frustum& frustum,
                            std::vector<IObject*>& visible,
                            std::vector<IObject*>& allVisible,
                            ObjectPredicator* predicator) = 0;
    virtual void getObjects(const Vec3& minimum, const Vec3& maximum,
                            std::vector<IObject*>& out,
                            ObjectPredicator* predicator) = 0;
    virtual void getObjects(const Vec3& centre, float radius,
                            std::vector<IObject*>& out,
                            ObjectPredicator* predicator) = 0;
    virtual void getObjects(const Vec3& from, const Vec3& to,
                            std::vector<std::pair<float, IObject*> >& out) = 0;
#else
    // Linux bf2: +0x34 direction, +0x38 sphere, +0x3C box, +0x40 Frustum.
    virtual void getObjects(const Vec3& from, const Vec3& to,
                            std::vector<std::pair<float, IObject*> >& out) = 0;
    virtual void getObjects(const Vec3& centre, float radius,
                            std::vector<IObject*>& out,
                            ObjectPredicator* predicator) = 0;
    virtual void getObjects(const Vec3& minimum, const Vec3& maximum,
                            std::vector<IObject*>& out,
                            ObjectPredicator* predicator) = 0;
    virtual void getObjects(const Frustum& frustum,
                            std::vector<IObject*>& visible,
                            std::vector<IObject*>& allVisible,
                            ObjectPredicator* predicator) = 0;
#endif

    virtual void clearGetObjects() = 0;                                  // +0x44
    virtual void appendGetObjects(const Vec3& centre, float radius,
                                  std::vector<IObject*>& out,
                                  ObjectPredicator* predicator) = 0;     // +0x48

    // Last in both images.  Itanium emits the ordinary/deleting pair at +0x4C/+0x50; MSVC emits
    // its scalar-deleting entry at +0x4C.
    virtual ~ICuller() {}
};

// ELF vtable off_88C8E48 and the PE counterpart are four slots with no destructor:
// getCuller, getCullerObject, destroyObject, updateObject.  Every concrete culler holder begins
// with this exact 8-byte base on x86 (vptr + owning ICuller pointer).
class CullerObject
{
public:
    explicit CullerObject(ICuller* culler) : m_culler(culler) {}

    virtual ICuller* getCuller() { return m_culler; }                    // +0x00
    virtual CullerObject* getCullerObject() { return this; }             // +0x04
    virtual void destroyObject() { m_culler->destroyObject(this); }      // +0x08
    virtual CullerObject* updateObject(const Vec3& position, float radius)
    {
        return m_culler->updateObject(this, position, radius);           // +0x0C
    }

protected:
    // Deliberately non-virtual: no destructor slot exists in either shipped CullerObject table.
    ~CullerObject() {}
    ICuller* m_culler;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // SCENE_OBJECT_ICULLER_H
